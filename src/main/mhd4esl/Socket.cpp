/*
 * This file is part of mhd4esl.
 * Copyright (C) 2019 Sven Lukas
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

#include <mhd4esl/Socket.h>
#include <mhd4esl/Connection.h>
#include <mhd4esl/Module.h>
#include <esl/bootstrap/Module.h>
#include <esl/http/server/RequestHandler.h>
#include <esl/http/server/ResponseStatic.h>
#include <esl/Stacktrace.h>
#include <esl/logging/Logger.h>
#include <microhttpd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <map>

namespace mhd4esl {
esl::logging::Logger logger("mhd4esl::Socket");

namespace {
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

//const std::string REQUEST_ENTITY_TOO_LARGE_PAGE("Request too large");
//const std::string ACCESS_DENIED_PAGE("<html><head><title>Zugriff verboten</title></head><body>Sie sind nicht berechtigt auf die angeforderte Seite zuzugreifen</body></html>");
//const std::string UNAUTHORIZED_PAGE("<html><head><title>Zugriff Verweigert</title></head><body>Der Zugriff wurde verweigert</body></html>");

class InternalRequestHandler : public esl::http::server::RequestHandler {
public:
	InternalRequestHandler(Connection& connection, unsigned short httpStatus, const std::string& content);

	bool addContent(esl::http::server::Connection& connection, const esl::http::server::Request& request, const char* contentData, std::size_t contentDataSize) override;
};

InternalRequestHandler::InternalRequestHandler(Connection& connection, unsigned short httpStatus, const std::string& content)
: esl::http::server::RequestHandler()
{
	std::unique_ptr<esl::http::server::ResponseStatic> response(new esl::http::server::ResponseStatic(httpStatus, "text/html", content.data(), content.size()));
	connection.sendResponse(std::move(response));
	connection.sendQueue();
}

bool InternalRequestHandler::addContent(esl::http::server::Connection& connection, const esl::http::server::Request& request, const char* contentData, std::size_t contentDataSize) {
	return false;
}

void mhdRequestCompletedHandler(void* cls,
        MHD_Connection* mhdConnection,
        void** connectionSpecificDataPtr,
        enum MHD_RequestTerminationCode toe) noexcept
{
	Connection** connection = reinterpret_cast<Connection**>(connectionSpecificDataPtr);

    if(*connection == nullptr) {
        logger.debug << "Complete with NULL\n";
        return;
    }

    logger.debug << "Complete with Request " << &(*connection)->getRequest() << "\n";
    delete *connection;
    *connection = nullptr;
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
		logger.error << "No certificate found for hostname=\"" << hostname << "\"\n";
		return -1;
	}

	logger.info << "Certificate found for hostname \"" << foundHostname << "\".\n";

	*pkey = foundCert->key;
	*pcert_length = 1;
	*pcert = &foundCert->pcrt;

	return 0;
}

} /* anonymour namespace */

Socket::Socket(uint16_t aPort, uint16_t aNumThreads, esl::http::server::RequestHandlerFactory aRequestHandlerFactory)
: esl::http::server::Interface::Socket(),
  port(aPort),
  numThreads(aNumThreads),
  requestHandlerFactory(aRequestHandlerFactory)
{
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

bool Socket::listen() {
    if (daemonPtr != nullptr) {
    	logger.debug << "HTTP socket already listening " << port << std::endl;
    	return true;
    }

    if(socketCerts[this].empty()) {
        daemonPtr = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG, // | MHD_USE_SUSPEND_RESUME,
//        daemonPtr = MHD_start_daemon(MHD_USE_POLL_INTERNALLY | MHD_USE_DEBUG , // | MHD_USE_SUSPEND_RESUME,
                port,
                0, 0, mhdAcceptHandler, this,
                MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,
//                MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
                MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) numThreads,
                MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 1000,
                MHD_OPTION_END);
    }
    else {
        daemonPtr = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG | MHD_USE_SSL, // | MHD_USE_SUSPEND_RESUME,
//        daemonPtr = MHD_start_daemon(MHD_USE_POLL_INTERNALLY | MHD_USE_DEBUG , // | MHD_USE_SUSPEND_RESUME,
                port,
                0, 0, mhdAcceptHandler, this,
                MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,
//                MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
                MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) numThreads,
                MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 1000,
//				MHD_OPTION_HTTPS_MEM_KEY, &keyVectBuffer[0],
//				MHD_OPTION_HTTPS_MEM_CERT, &certVectBuffer[0],
				MHD_OPTION_HTTPS_CERT_CALLBACK, &mhdSniCallback,
                MHD_OPTION_END);
    }

    if(daemonPtr == nullptr) {
    	logger.debug << "Couldn't start HTTP socket at port " << port << std::endl;
    	return false;
    }

	logger.debug << "HTTP socket started at port " << port << std::endl;
	return true;
}

void Socket::release() {
    if (daemonPtr != nullptr) {
    	logger.debug << "HTTP socket already released " << port << std::endl;
    	return;
    }

    MHD_stop_daemon(static_cast<MHD_Daemon *>(daemonPtr));
    daemonPtr = nullptr;
	logger.debug << "HTTP socket released at port " << port << std::endl;
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

	Connection** connection = reinterpret_cast<Connection**>(connectionSpecificDataPtr);
    if(*connection == nullptr) {
    	*connection = new Connection(*mhdConnection, version, method, url);
    }

    socket->accessThreadInc();
    bool rv = socket->accept(**connection, uploadData, uploadDataSize);
    socket->accessThreadDec();
    return rv ? MHD_YES : MHD_NO;
}

bool Socket::accept(Connection& connection, const char* uploadData, std::size_t* uploadDataSize) noexcept {
    std::unique_ptr<esl::Stacktrace> stacktrace = nullptr;
    try {
        if(!connection.requestHandler) {
//            connection.requestHandler = requestHandlerFactory.createRequestHandler(connection, connection.getRequest());
            connection.requestHandler = requestHandlerFactory(connection, connection.getRequest());
            if(connection.requestHandler && *uploadDataSize == 0) {
                return true;
            }
        }

        if(connection.requestHandler) {
            bool lastCall = (*uploadDataSize == 0);

            if(connection.hasResponseSent()) {
                logger.debug << "accessHandler: Send PRE-YES" << std::endl;
            	*uploadDataSize = 0;
                return true;
            }

        	if(connection.requestHandler->addContent(connection, connection.getRequest(), uploadData, *uploadDataSize) == false) {
        		// markieren, dass nicht mehr request handler aufgerufen wird;
            	connection.sendQueue();
        	}
        	*uploadDataSize = 0;

            if(connection.hasResponseSent()) {
            	return true;
            }

            logger.debug << "(2) lastCall = " << (lastCall ? "true" : "false") << std::endl;
            return !lastCall;
        }

        connection.requestHandler.reset(new InternalRequestHandler(connection, 404, PAGE_404));
        return true;

    }
    catch (const std::exception& e) {
//        error = new esl::Exception(__func__, __FILE__, __LINE__, e.what());
    	logger.error << e.what() << std::endl;
//    	esl::Stacktrace* st = boost::get_error_info<esl::StacktraceType>(e);
    	esl::Stacktrace* st = esl::getStacktrace(e);
    	if(st) {
            st->dump(logger);
    	}
    	else {
            stacktrace.reset(new esl::Stacktrace);
    	}
    }
    catch (...) {
//    	error = new esl::Exception(__func__, __FILE__, __LINE__, "unknown exception");
    	logger.error << "unknown exception" << std::endl;
        stacktrace.reset(new esl::Stacktrace);
    }
//    if(error) {
    if(stacktrace) {
        logger.error << "  *** BEGIN *** -> Error on handling request\n";
        stacktrace->dump(logger);
        logger.error << "  ***  END  *** -> Error on handling request\n";
//        delete error;
    }

    // wenn wir hier landen, hat es einen internen Fehler gegeben
    connection.requestHandler.reset(new InternalRequestHandler(connection, 500, PAGE_500));
    return true;
}

} /* namespace mhd4esl */
