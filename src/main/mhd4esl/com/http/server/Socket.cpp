/*
 * This file is part of mhd4esl.
 * Copyright (C) 2019-2021 Sven Lukas
 *
 * Mhd4esl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mhd4esl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with mhd4esl.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <mhd4esl/com/http/server/Socket.h>
#include <mhd4esl/com/http/server/RequestContext.h>
#include <mhd4esl/com/http/server/Connection.h>
#include <mhd4esl/Module.h>
#include <mhd4esl/Logger.h>

#include <esl/module/Module.h>
#include <esl/io/Writer.h>
#include <esl/Stacktrace.h>

#include <microhttpd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <mutex>
#include <map>


namespace mhd4esl {
namespace com {
namespace http {
namespace server {

namespace {
Logger logger("mhd4esl::com::http::server::Socket");

const std::string PAGE_404(
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<title>404</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>404</h1>\n"
		"</body>\n"
		"</html>\n");
const std::string PAGE_500(
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<title>500</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>500</h1>\n"
		"</body>\n"
		"</html>\n");

void mhdRequestCompletedHandler(void* cls,
        MHD_Connection* mhdConnection,
        void** connectionSpecificDataPtr,
        enum MHD_RequestTerminationCode toe) noexcept
{
	RequestContext** requestContext = reinterpret_cast<RequestContext**>(connectionSpecificDataPtr);

    if(*requestContext == nullptr) {
        logger.debug << "Complete with NULL\n";
        return;
    }

    logger.debug << "Complete with Request\n";
    delete *requestContext;
    *requestContext = nullptr;
}

struct Cert {
	gnutls_pcert_st pcrt;
	gnutls_privkey_t key;
};

using Certs = std::map<std::string, Cert>;

std::mutex socketCertsMutex;
std::map<Socket*, Certs> socketCerts; // SocketPtr -> (Hostname -> cert)

bool hasMatchingHostname(const std::string& hostname, const std::string& hostnamePattern) {
	logger.debug << "Check if hostname = \"" << hostname << "\" matches to hostnamePatter = \"" << hostnamePattern << "\".\n";

	if(hostnamePattern.empty()) {
		return true;
	}
	else if(hostnamePattern.at(0) == '*') {
		std::string::size_type patternSize = hostnamePattern.size()-1;

		return (hostname.size() >= patternSize && hostname.substr(hostname.size() - patternSize) == hostnamePattern.substr(1));
	}

	return hostname == hostnamePattern;
}

int
mhdSniCallback(gnutls_session_t session,
              const gnutls_datum_t* req_ca_dn,
              int nreqs,
              const gnutls_pk_algorithm_t* pk_algos,
              int pk_algos_length,
              gnutls_pcert_st** pcert,
              unsigned int *pcert_length,
              gnutls_privkey_t * pkey)
{
	std::string hostname;
	logger.info << "Certificate requested.\n";

	{
		char name[256];
		size_t name_len = sizeof (name);
		unsigned int type;

		if(GNUTLS_E_SUCCESS == gnutls_server_name_get(session, name, &name_len, &type, 0 /* index */)) {
			hostname = name;
		}
		else {
			logger.warn << "Failed to get hostname.\n";
//			return -1;
		}
	}

	logger.info << "Certificate for hostname \"" << hostname << "\" requested.\n";

	Cert* foundCert = nullptr;
	std::string foundHostname;
	bool done = false;

    std::lock_guard<std::mutex> socketCertsLock(socketCertsMutex);

	for(auto& certs : socketCerts) {
		for(auto& cert : certs.second) {
			logger.debug << "Check certificate for hostname \"" << cert.first << "\".\n";
			if(!hasMatchingHostname(hostname, cert.first)) {
				logger.debug << "Wrong certificate.\n";
				continue;
			}
			logger.debug << "Certificate found.\n";

			if(!cert.first.empty() && cert.first.at(0) != '*') {
				done = true;
			}
			else if(cert.first.size() < foundHostname.size()) {
				continue;
			}

			foundCert = &(cert.second);
			foundHostname = cert.first;

			if(done) {
				break;
			}
		}

		if(done) {
			break;
		}
	}

	if(foundCert == nullptr) {
		logger.info << "No certificate found for hostname=\"" << hostname << "\"\n";
		return -1;
	}

	logger.info << "Certificate found for hostname \"" << foundHostname << "\".\n";

	*pkey = foundCert->key;
	*pcert_length = 1;
	*pcert = &foundCert->pcrt;

	return 0;
}

} /* anonymour namespace */


std::unique_ptr<esl::com::http::server::Interface::Socket> Socket::create(const esl::com::http::server::Interface::Settings& settings) {
	return std::unique_ptr<esl::com::http::server::Interface::Socket>(new Socket(settings));
}

Socket::Socket(const esl::com::http::server::Interface::Settings& settings) {
	bool hasPort = false;

	for(const auto& setting : settings) {
		if(setting.first == "threads") {
			numThreads = static_cast<uint16_t>(std::stoi(setting.second));
		    if(numThreads <= 0) {
		    	throw esl::addStacktrace(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
		else if(setting.first == "port") {
			hasPort = true;
			port = static_cast<uint16_t>(std::stoi(setting.second));
		    if(port <= 0) {
		    	throw esl::addStacktrace(std::runtime_error("Invalid value for \"" + setting.first + "\"=\"" + setting.second + "\""));
		    }
		}
	}

	if(!hasPort) {
    	throw esl::addStacktrace(std::runtime_error("Parameter \"port\" is missing"));
	}

	std::lock_guard<std::mutex> socketCertsLock(socketCertsMutex);
	socketCerts.insert(std::make_pair(this, Certs()));
}

Socket::~Socket() {
	if (daemonPtr != nullptr) {
		logger.debug << "Stopping HTTP socket at port " << port << std::endl;
		MHD_Daemon* d = static_cast<MHD_Daemon*>(daemonPtr);
		MHD_stop_daemon (d);
		daemonPtr = nullptr;

		std::lock_guard<std::mutex> socketCertsLock(socketCertsMutex);
		socketCerts.erase(this);
	}
}

void Socket::addTLSHost(const std::string& hostname, std::vector<unsigned char> certificate, std::vector<unsigned char> key) {
	if (daemonPtr != nullptr) {
		throw esl::addStacktrace(std::runtime_error("Calling Socket::addTLSHost not allowed, because HTTP socket is already listening"));
	}

	std::lock_guard<std::mutex> socketCertsLock(socketCertsMutex);

	Cert& cert = socketCerts[this][hostname];
	gnutls_datum_t gnutls_datum;
	int rc;

	gnutls_datum.data = &certificate[0];
	gnutls_datum.size = static_cast<unsigned int>(certificate.size());
	rc = gnutls_pcert_import_x509_raw(&cert.pcrt, &gnutls_datum, GNUTLS_X509_FMT_PEM, 0);
	if(rc < 0) {
		logger.error << "Error installing certificate: " << gnutls_strerror (rc) << "\n";
		throw esl::addStacktrace(std::runtime_error("Error installing certificate: " + std::string(gnutls_strerror (rc))));
	}

	gnutls_datum.data = &key[0];
	gnutls_datum.size = static_cast<unsigned int>(key.size());
	gnutls_privkey_init(&cert.key);
	rc = gnutls_privkey_import_x509_raw(cert.key, &gnutls_datum, GNUTLS_X509_FMT_PEM, nullptr, 0);
	if(rc < 0) {
		logger.error << "Error installing key: " << gnutls_strerror (rc) << "\n";
		throw esl::addStacktrace(std::runtime_error("Error installing key"));
	}
	logger.info << "Successfully installed certificate and key for hostname \"" << hostname << "\"\n";
}

void Socket::addObjectFactory(const std::string& id, Socket::ObjectFactory objectFactory) {
	if (daemonPtr != nullptr) {
		throw esl::addStacktrace(std::runtime_error("Calling Socket::addObjectFactory not allowed, because HTTP socket is already listening"));
	}

	objectFactories[id] = objectFactory;
}

Socket::ObjectFactory Socket::getObjectFactory(const std::string& id) const {
	auto iter = objectFactories.find(id);
	if(iter != std::end(objectFactories)) {
		return iter->second;
	}
	return nullptr;
}

void Socket::listen(esl::com::http::server::requesthandler::Interface::CreateInput aCreateInput) {
	createInput = aCreateInput;
	if (daemonPtr != nullptr) {
		logger.warn << "HTTP socket (port=" << port << ") is already listening." << std::endl;
		return;
	}

	usingTLS = !socketCerts[this].empty();
	unsigned int flags = MHD_USE_SELECT_INTERNALLY;

	flags |= MHD_USE_THREAD_PER_CONNECTION;
	// flags |= MHD_USE_POLL_INTERNALLY;

	// flags |= MHD_USE_SUSPEND_RESUME;

#ifdef MHD4ESL_LOGGING_LEVEL_DEBUG
    flags |= MHD_USE_DEBUG;
#endif

	if(usingTLS) {
		daemonPtr = MHD_start_daemon(flags | MHD_USE_SSL,
				port,
				0, 0, mhdAcceptHandler, this,
				MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,
				// MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
				MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
				MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) numThreads,
				MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 1000,
				MHD_OPTION_HTTPS_CERT_CALLBACK, &mhdSniCallback,
				MHD_OPTION_END);
	}
	else {
		daemonPtr = MHD_start_daemon(flags,
				port,
				0, 0, mhdAcceptHandler, this,
				MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,
				// MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
				MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
				MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) numThreads,
				MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 1000,
				MHD_OPTION_END);
	}

	if(daemonPtr == nullptr) {
		throw esl::addStacktrace(std::runtime_error("Couldn't start HTTP socket at port " + std::to_string(port) + ". Maybe there is already a socket listening on this port."));
	}

	logger.debug << "HTTP socket started at port " << port << std::endl;
}

void Socket::release() {
	if (daemonPtr == nullptr) {
		logger.debug << "HTTP socket already released " << port << std::endl;
		return;
	}

	MHD_stop_daemon(static_cast<MHD_Daemon *>(daemonPtr));
	daemonPtr = nullptr;
	logger.debug << "HTTP socket released at port " << port << std::endl;
}

bool Socket::wait(std::uint32_t ms) {
	return true;
}

int Socket::mhdAcceptHandler(void* cls,
		MHD_Connection* mhdConnection,
		const char* url,
		const char* method,
		const char* version,
		const char* uploadData,
		size_t* uploadDataSize,
		void** connectionSpecificDataPtr) noexcept
{
	Socket* socket = static_cast<Socket*>(cls);
	if(socket == nullptr) {
		logger.error << "  *** socket == nullptr *** \n";
		return MHD_NO;
	}

	if(mhdConnection == nullptr) {
		logger.error << "  *** mhdConnection == nullptr *** \n";
		return MHD_NO;
	}

	RequestContext** requestContext = reinterpret_cast<RequestContext**>(connectionSpecificDataPtr);
	if(*requestContext == nullptr) {
		try {
			*requestContext = new RequestContext(*socket, *mhdConnection, version, method, url, socket->usingTLS, socket->port);
			(*requestContext)->input = socket->createInput(**requestContext);
			if((*requestContext)->input) {
				if(*uploadDataSize == 0) {
					return MHD_YES;
				}
			}
			else {
				logger.debug << "Failed to create input\n";
			}
		}
		catch (const std::exception& e) {
			logger.error << e.what() << std::endl;

			const esl::Stacktrace* stacktrace = esl::getStacktrace(e);
			if(stacktrace) {
				stacktrace->dump(logger.error);
			}
			return MHD_NO;
		}
		catch (...) {
			logger.error << "unknown exception" << std::endl;
			return MHD_NO;
		}
	}

	socket->accessThreadInc();
	bool rv = socket->accept(**requestContext, uploadData, uploadDataSize);
	socket->accessThreadDec();
	return rv ? MHD_YES : MHD_NO;
}

bool Socket::accept(RequestContext& requestContext, const char* uploadData, std::size_t* uploadDataSize) noexcept {
	try {
		if(!requestContext.input) {
			logger.debug << "No input\n";
			*uploadDataSize = 0;

			if(requestContext.connection.isResponseQueueEmpty()) {
				logger.debug << "Nothing in response queue -> push 404 page into respone queue\n";
				esl::com::http::server::Response response(404, esl::utility::MIME::textHtml);
				requestContext.connection.send(response, PAGE_404.data(), PAGE_404.size());
			}

			// send response queue, so this method will not be called again
			if(!requestContext.connection.hasResponseSent()) {
				requestContext.connection.sendQueue();
			}

			return true;
		}

		bool lastCall = (*uploadDataSize == 0);
		std::size_t size = requestContext.input.getWriter().write(uploadData, *uploadDataSize);

		if(lastCall || size == esl::io::Writer::npos) {
			*uploadDataSize = 0;

			//logger.debug << "Reset input object\n";
			//requestContext.input = esl::utility::io::Input();

			// drop connection
			if(requestContext.connection.isResponseQueueEmpty()) {
				logger.debug << "There was no response sent -> drop connection\n";
				return false;
			}

			// send response queue, so this method will not be called again
			if(!requestContext.connection.hasResponseSent()) {
				requestContext.connection.sendQueue();
			}
			return true;
		}

		*uploadDataSize -= size;

		return true;
	}
	catch (const std::exception& e) {
		logger.error << e.what() << std::endl;

		const esl::Stacktrace* stacktrace = esl::getStacktrace(e);
		if(stacktrace) {
			stacktrace->dump(logger.error);
		}
	}
	catch (...) {
		logger.error << "unknown exception" << std::endl;
	}

	// wenn wir hier landen, hat es einen internen Fehler gegeben
	if(requestContext.connection.isResponseQueueEmpty()) {
		esl::com::http::server::Response response(500, esl::utility::MIME::textHtml);
		requestContext.connection.send(response, PAGE_500.data(), PAGE_500.size());
	}

	// send response queue, so this method will not be called again
	if(!requestContext.connection.hasResponseSent()) {
		requestContext.connection.sendQueue();
	}

	return true;
}

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */
