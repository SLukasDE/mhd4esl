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

#include <mhd4esl/Connection.h>
#include <esl/http/server/ResponseBasicAuth.h>
#include <esl/http/server/ResponseDynamic.h>
#include <esl/http/server/ResponseStatic.h>
#include <microhttpd.h>

#include <esl/Stacktrace.h>
#include <esl/logging/Logger.h>

namespace mhd4esl {
esl::logging::Logger Connection::logger("mhd4esl::Connection");

Connection::Connection(MHD_Connection& mhdConnection, const char* version, const char* method, const char* url)
: esl::http::server::Connection(),
  mhdConnection(mhdConnection),
  request(mhdConnection, version, method, url)
{
}

Connection::~Connection() {
	for(auto mhdResponse : mhdResponses) {
		MHD_destroy_response(mhdResponse);
	}
}

const esl::http::server::Request& Connection::getRequest() const noexcept {
	return request;
}


bool Connection::sendResponse(std::unique_ptr<esl::http::server::ResponseBasicAuth> response) noexcept {
    if(!response->isValid()) {
    	return false;
    }

    MHD_Response* mhdResponse = MHD_create_response_from_buffer(response->getContentDataSize(), const_cast<void*>(static_cast<const void*>(response->getContentData())), MHD_RESPMEM_PERSISTENT);
	if(mhdResponse == nullptr) {
		return false;
	}
	mhdResponses.push_back(mhdResponse);

	for(const auto& header : response->getHeaders()) {
		MHD_add_response_header(mhdResponse, header.first.c_str(), header.second.c_str());
	}

    std::string realmId = response->getRealmId();
	std::function<bool()> sendFunc = [this, mhdResponse, realmId]() {
	    return MHD_queue_basic_auth_fail_response(&mhdConnection, realmId.c_str(), mhdResponse) == MHD_YES;
	};
	queueToSend.push_back(sendFunc);

//	MHD_destroy_response(mhdResponse);
	return true;
}

bool Connection::sendResponse(std::unique_ptr<esl::http::server::ResponseDynamic> response) noexcept {
    esl::http::server::ResponseDynamic* resposePtr = response.release();

    if(!resposePtr->isValid()) {
    	return false;
    }

    MHD_Response* mhdResponse = MHD_create_response_from_callback (-1, 8192, contentReaderCallback, resposePtr, contentReaderFreeCallback);
//    MHD_Response* mhdResponse = MHD_create_response_from_buffer(response->getContentDataSize(), const_cast<void*>(static_cast<const void*>(response->getContentData())), MHD_RESPMEM_PERSISTENT);
	if(mhdResponse == nullptr) {
		return false;
	}
	mhdResponses.push_back(mhdResponse);

	for(const auto& header : resposePtr->getHeaders()) {
		MHD_add_response_header(mhdResponse, header.first.c_str(), header.second.c_str());
	}

	unsigned short httpStatus = resposePtr->getHttpStatus();
	std::function<bool()> sendFunc = [this, httpStatus, mhdResponse]() {
	    return MHD_queue_response(&mhdConnection, httpStatus, mhdResponse) == MHD_YES;
	};
	queueToSend.push_back(sendFunc);

//	MHD_destroy_response(mhdResponse);
	return true;
}

bool Connection::sendResponse(std::unique_ptr<esl::http::server::ResponseStatic> response) noexcept {
    if(!response->isValid()) {
    	return false;
    }

    MHD_Response* mhdResponse = MHD_create_response_from_buffer(response->getContentDataSize(), const_cast<void*>(static_cast<const void*>(response->getContentData())), MHD_RESPMEM_PERSISTENT);
	if(mhdResponse == nullptr) {
		return false;
	}
	mhdResponses.push_back(mhdResponse);

	for(const auto& header : response->getHeaders()) {
		MHD_add_response_header(mhdResponse, header.first.c_str(), header.second.c_str());
	}

	unsigned short httpStatus = response->getHttpStatus();
	std::function<bool()> sendFunc = [this, httpStatus, mhdResponse]() {
	    return MHD_queue_response(&mhdConnection, httpStatus, mhdResponse) == MHD_YES;
	};
	queueToSend.push_back(sendFunc);

//	MHD_destroy_response(mhdResponse);
	return true;
}

bool Connection::sendQueue() noexcept {
	bool rv = true;

	for(auto& sendFunc : queueToSend) {
		rv &= sendFunc();
		if(rv) {
			responseSent = true;
		}
	}
	queueToSend.clear();

	return rv;
}

bool Connection::hasResponseSent() noexcept {
	return responseSent;
}

long int Connection::contentReaderCallback(void* cls, uint64_t bytesTransmitted, char* buffer, size_t bufferSize) {
	esl::http::server::ResponseDynamic* responseDynamic = static_cast<esl::http::server::ResponseDynamic*>(cls);

    std::unique_ptr<esl::Stacktrace> stacktrace = nullptr;
    try {
        int size;
        size = responseDynamic->getData(buffer, bufferSize);
        if(size <= 0) {
            return MHD_CONTENT_READER_END_OF_STREAM;
        }
        return size;
    }
#if 0
    catch (esl::Exception& e) {
    	logger.error << e.what() << std::endl;
        stacktrace.reset(new esl::Stacktrace);
    }
#endif
    catch (std::exception& e) {
    	logger.error << e.what() << std::endl;
        stacktrace.reset(new esl::Stacktrace);
    }
    catch (...) {
    	logger.error << "unknown exception" << std::endl;
        stacktrace.reset(new esl::Stacktrace);
    }
    if(stacktrace) {
        logger.error << "  *** Fehler beim Bearbeiten der Anfrage: START *** \n";
        stacktrace->dump(logger);
        logger.error << "  *** Fehler beim Bearbeiten der Anfrage: ENDE *** \n";
    }
    return MHD_CONTENT_READER_END_WITH_ERROR;
}

void Connection::contentReaderFreeCallback(void* cls) {
	esl::http::server::ResponseDynamic* responseDynamic = static_cast<esl::http::server::ResponseDynamic*>(cls);

    if(responseDynamic == nullptr) {
        return;
    }
    delete responseDynamic;
}

} /* namespace mhd4esl */
