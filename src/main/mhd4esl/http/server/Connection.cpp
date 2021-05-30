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

#include <mhd4esl/http/server/Connection.h>
#include <mhd4esl/Logger.h>

#include <esl/Stacktrace.h>

#include <microhttpd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mhd4esl {
namespace http {
namespace server {

namespace {
Logger logger("mhd4esl::Connection");
}

Connection::Connection(MHD_Connection& mhdConnection)
: mhdConnection(mhdConnection)
{ }

Connection::~Connection() {
	for(auto& response : responseQueue) {
		MHD_destroy_response(std::get<1>(response));
	}
}

bool Connection::sendQueue() noexcept {
	bool rv = true;

	for(auto& response : responseQueue) {
		rv &= std::get<0>(response)();
		if(rv) {
			responseSent = true;
		}
	}

	return rv;
}

bool Connection::isResponseQueueEmpty() noexcept {
	return responseQueue.empty();
}

bool Connection::hasResponseSent() noexcept {
	return responseSent;
}

bool Connection::sendResponse(const esl::http::server::Response& response, const void* data, std::size_t size) noexcept {
    if(!response.isValid()) {
    	return false;
    }

    MHD_Response* mhdResponse = MHD_create_response_from_buffer(size, const_cast<void*>(data), MHD_RESPMEM_PERSISTENT);

    return sendResponse(response, mhdResponse);
}

bool Connection::sendResponse(const esl::http::server::Response& response, esl::io::Output output) noexcept {
	if(!response.isValid()) {
		logger.error << "MHD: invalid response object\n";
		return false;
	}

	if(output) {
logger.error << "MHD: output object is valid\n";
	}
	else {
logger.error << "MHD: empty output object\n";
	}
	esl::io::Output* outputPtr = new esl::io::Output(std::move(output));
	MHD_Response* mhdResponse = MHD_create_response_from_callback(-1, 8192, contentReaderCallback, outputPtr, contentReaderFreeCallback);

	return sendResponse(response, mhdResponse);
}

bool Connection::sendResponse(const esl::http::server::Response& response, boost::filesystem::path path) noexcept {
    if(!response.isValid()) {
    	return false;
    }

    int fd = open(path.generic_string().c_str(), O_RDONLY);
    if(fd < 0) {
        return false;
    }
    size_t size = static_cast<size_t>(lseek(fd, 0, SEEK_END));
    lseek(fd, 0, SEEK_SET);

    MHD_Response* mhdResponse = MHD_create_response_from_fd(size, fd);

    return sendResponse(response, mhdResponse);
}

bool Connection::sendResponse(const esl::http::server::Response& response, MHD_Response* mhdResponse) noexcept {
	if(mhdResponse == nullptr) {
		logger.warn << "- mhdResponse == nullptr\n";
		return false;
	}

	for(const auto& header : response.getHeaders()) {
		MHD_add_response_header(mhdResponse, header.first.c_str(), header.second.c_str());
	}

	std::function<bool()> sendFunc;
	if(response.getStatusCode() == 401) {
	    std::string realmId = response.getRealmId();

		sendFunc = [this, mhdResponse, realmId]() {
		    return MHD_queue_basic_auth_fail_response(&mhdConnection, realmId.c_str(), mhdResponse) == MHD_YES;
		};
	}
	else {
		unsigned short httpStatusCode = response.getStatusCode();

		sendFunc = [this, httpStatusCode, mhdResponse]() {
		    return MHD_queue_response(&mhdConnection, httpStatusCode, mhdResponse) == MHD_YES;
		};
	}

	responseQueue.push_back(std::make_tuple(sendFunc, mhdResponse));

	return true;
}

long int Connection::contentReaderCallback(void* cls, uint64_t bytesTransmitted, char* buffer, size_t bufferSize) {
    esl::io::Output* outputPtr = static_cast<esl::io::Output*>(cls);
    if(outputPtr == nullptr) {
        return MHD_CONTENT_READER_END_OF_STREAM;
    }

    std::unique_ptr<esl::Stacktrace> stacktrace = nullptr;
    try {
        std::size_t size = outputPtr->getReader().read(buffer, bufferSize);
    	if(size == esl::io::Reader::npos) {
logger.error << "MHD: output object reader returned npos\n";
            return MHD_CONTENT_READER_END_OF_STREAM;
        }

logger.error << "MHD: output object reader returned " << size << "\n";
        return size;
    }
    catch (std::exception& e) {
    	logger.error << e.what() << std::endl;

    	const esl::Stacktrace* stacktracePtr = esl::getStacktrace(e);
    	if(stacktracePtr) {
            stacktrace.reset(new esl::Stacktrace(*stacktracePtr));
    	}
    }
    catch (...) {
    	logger.error << "unknown exception" << std::endl;
    }

    if(stacktrace) {
        stacktrace->dump(logger.error);
    }
    return MHD_CONTENT_READER_END_WITH_ERROR;
}

void Connection::contentReaderFreeCallback(void* cls) {
    esl::io::Output* outputPtr = static_cast<esl::io::Output*>(cls);

    if(outputPtr) {
        delete outputPtr;
    }
}

} /* namespace server */
} /* namespace http */
} /* namespace mhd4esl */
