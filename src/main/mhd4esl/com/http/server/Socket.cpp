/*
 * This file is part of mhd4esl.
 * Copyright (C) 2019-2023 Sven Lukas
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

#include <gtx4esl/crypto/Entries.h>
#include <gtx4esl/crypto/Entry.h>

#include <esl/Logger.h>
#include <esl/plugin/Registry.h>
#include <esl/utility/String.h>

#include <esl/io/Writer.h>
#include <esl/system/Stacktrace.h>

#include <microhttpd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>

#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace mhd4esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

namespace {
esl::Logger logger("mhd4esl::com::http::server::Socket");

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
        logger.error << "Request completed, but there is no RequestContext to delete\n";
        return;
    }

    delete *requestContext;
    *requestContext = nullptr;
}

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

int mhdSniCallback(gnutls_session_t session,
		const gnutls_datum_t* req_ca_dn, int nreqs,
		const gnutls_pk_algorithm_t* pk_algos, int pk_algos_length,
		gnutls_pcert_st** pcert, unsigned int *pcertLength, gnutls_privkey_t * pkey)
{
	std::string hostname;
	logger.info << "Certificate requested.\n";

	{
		char name[256];
		size_t nameLength = 255;
		unsigned int type;

		switch(gnutls_server_name_get(session, name, &nameLength, &type, 0)) {
		case GNUTLS_E_SHORT_MEMORY_BUFFER:
			logger.warn << "Length to retrieve SNI server name is too big. " << nameLength << " bytes are required.\n";
			return -1;
		case GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE:
			logger.warn << "Cannot get SNI server name at index 0.\n";
			return -1;
		case GNUTLS_E_SUCCESS:
			hostname = name;
			break;
		default:
			logger.warn << "Failed to get SNI server name.\n";
			return -1;
		}
	}

	logger.trace << "Search certificate for hostname \"" << hostname << "\".\n";
	const std::string* foundHostname = nullptr;

	gtx4esl::crypto::Entries* keyStoreEntriesPtr = esl::plugin::Registry::get().findObject<gtx4esl::crypto::Entries>();

	if(keyStoreEntriesPtr) {
		for(auto& entry : keyStoreEntriesPtr->entryByHostname) {
			logger.trace << "Check certificate for hostname \"" << entry.first << "\".\n";
			if(!hasMatchingHostname(hostname, entry.first)) {
				logger.trace << "Certificate for hostname \"" << entry.first << "\" does not match\n";
				continue;
			}

			if(!entry.first.empty() && entry.first.at(0) != '*') {
				logger.trace << "Certificate for hostname \"" << entry.first << "\" is a perfect match\n";
				foundHostname = &entry.first;
				*pkey = entry.second.key;
				*pcertLength = 1;
				*pcert = &entry.second.pcrt;
				return 0;
			}
			else if(foundHostname != nullptr && entry.first.size() < foundHostname->size()) {
				logger.trace << "Skip certificate for hostname \"" << entry.first << "\" because better match has been found already.\n";
				continue;
			}

			logger.trace << "Use certificate for hostname \"" << entry.first << "\"\n";
			foundHostname = &entry.first;
			*pkey = entry.second.key;
			*pcertLength = 1;
			*pcert = &entry.second.pcrt;
		}
	}

	if(foundHostname == nullptr) {
		logger.warn << "No certificate found for hostname=\"" << hostname << "\"\n";
		return -1;
	}

	logger.trace << "Certificate found for hostname \"" << *foundHostname << "\".\n";
	return 0;
}

} /* anonymour namespace */


Socket::Socket(const esl::com::http::server::MHDSocket::Settings& aSettings)
: settings(aSettings)
{ }

Socket::~Socket() {
	if (daemonPtr != nullptr) {
		logger.debug << "Stopping HTTP socket at port " << settings.port << std::endl;
		MHD_Daemon* d = static_cast<MHD_Daemon*>(daemonPtr);
		MHD_stop_daemon (d);
		daemonPtr = nullptr;
	}
}

void Socket::listen(const esl::com::http::server::RequestHandler& requestHandler) {
}

void Socket::listen(const esl::com::http::server::RequestHandler& aRequestHandler, std::function<void()> aOnReleasedHandler) {
	if (daemonPtr != nullptr) {
		throw esl::system::Stacktrace::add(std::runtime_error("HTTP socket (port=" + std::to_string(settings.port) + ") is already listening."));
	}

	requestHandler = &aRequestHandler;

	unsigned int flags = MHD_USE_SELECT_INTERNALLY;

	flags |= MHD_USE_THREAD_PER_CONNECTION;
	// flags |= MHD_USE_POLL_INTERNALLY;

	// flags |= MHD_USE_SUSPEND_RESUME;

#ifdef MHD4ESL_LOGGING_LEVEL_DEBUG
    flags |= MHD_USE_DEBUG;
#endif

	if(settings.https) {
		std::lock_guard<std::mutex> lock(waitNotifyMutex);

	    flags |= MHD_USE_SSL;
		daemonPtr = MHD_start_daemon(flags, settings.port, 0, 0, mhdAcceptHandler, this,
				MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,
				MHD_OPTION_HTTPS_CERT_CALLBACK, &mhdSniCallback,

				MHD_OPTION_PER_IP_CONNECTION_LIMIT, (unsigned int) settings.perIpConnectionLimit,
				MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) settings.connectionTimeout,
				MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) settings.numThreads,
				MHD_OPTION_CONNECTION_LIMIT, (unsigned int) settings.connectionLimit,
				MHD_OPTION_END);
	}
	else {
		std::lock_guard<std::mutex> lock(waitNotifyMutex);

		daemonPtr = MHD_start_daemon(flags, settings.port, 0, 0, mhdAcceptHandler, this,
				MHD_OPTION_NOTIFY_COMPLETED, &mhdRequestCompletedHandler, nullptr,

				MHD_OPTION_PER_IP_CONNECTION_LIMIT, (unsigned int) settings.perIpConnectionLimit,
				MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) settings.connectionTimeout,
				MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) settings.numThreads,
				MHD_OPTION_CONNECTION_LIMIT, (unsigned int) settings.connectionLimit,
				MHD_OPTION_END);
	}

	waitCondVar.notify_all();

	if(daemonPtr == nullptr) {
		throw esl::system::Stacktrace::add(std::runtime_error("Couldn't start HTTP socket at port " + std::to_string(settings.port) + ". Maybe there is already a socket listening on this port."));
	}

	onReleasedHandler = aOnReleasedHandler;
	logger.debug << "HTTP socket started at port " << settings.port << std::endl;
}

void Socket::release() {
	if (daemonPtr == nullptr) {
		logger.debug << "HTTP socket already released " << settings.port << std::endl;
		return;
	}

	logger.debug << "Releasing HTTP socket at port " << settings.port << " ..." << std::endl;
	MHD_stop_daemon(static_cast<MHD_Daemon *>(daemonPtr));
	{
		std::lock_guard<std::mutex> lock(waitNotifyMutex);
		daemonPtr = nullptr;
    }
	logger.debug << "HTTP socket released at port " << settings.port << std::endl;
	if(onReleasedHandler) {
		onReleasedHandler();
	}
	waitCondVar.notify_all();
}

bool Socket::wait(std::uint32_t ms) {
	std::unique_lock<std::mutex> waitNotifyLock(waitNotifyMutex);

	if(ms == 0) {
		waitCondVar.wait(waitNotifyLock, [this] {
				return daemonPtr == nullptr;
		});
		return true;
	}
	else {
		return waitCondVar.wait_for(waitNotifyLock, std::chrono::milliseconds(ms), [this] {
				return daemonPtr == nullptr;
		});
	}
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
			*requestContext = new RequestContext(*mhdConnection, version, method, url, socket->usingTLS, socket->settings.port);
			(*requestContext)->input = socket->requestHandler->accept(**requestContext);

			if((*requestContext)->input &&*uploadDataSize == 0) {
				return MHD_YES;
			}
		}
		catch (const std::exception& e) {
			logger.error << "std::exception::what(): " << e.what() << std::endl;

			const esl::system::Stacktrace* stacktrace = esl::system::Stacktrace::get(e);
			if(stacktrace) {
				logger.error << "Stacktrace:\n";
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
	bool rv = accept(**requestContext, uploadData, uploadDataSize);
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
				esl::com::http::server::Response response(404, esl::utility::MIME::Type::textHtml);
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

		const esl::system::Stacktrace* stacktrace = esl::system::Stacktrace::get(e);
		if(stacktrace) {
			stacktrace->dump(logger.error);
		}
	}
	catch (...) {
		logger.error << "unknown exception" << std::endl;
	}

	// wenn wir hier landen, hat es einen internen Fehler gegeben
	if(requestContext.connection.isResponseQueueEmpty()) {
		esl::com::http::server::Response response(500, esl::utility::MIME::Type::textHtml);
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
} /* inline namespace v1_6 */
} /* namespace mhd4esl */
