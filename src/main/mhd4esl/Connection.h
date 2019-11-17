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

#ifndef MHD4ESL_CONNECTION_H_
#define MHD4ESL_CONNECTION_H_

#include <mhd4esl/Request.h>
#include <esl/http/server/Connection.h>
#include <esl/http/server/RequestHandler.h>
#include <esl/logging/Logger.h>
#include <string>
#include <memory>
#include <vector>

struct MHD_Connection;
struct MHD_Response;

namespace mhd4esl {

class Socket;

class Connection : public esl::http::server::Connection {
friend class Socket;
public:
	Connection(MHD_Connection& mhdConnection, const char* version, const char* method, const char* url);
	~Connection();

	const esl::http::server::Request& getRequest() const noexcept override;
	bool sendResponse(std::unique_ptr<esl::http::server::ResponseBasicAuth> response) noexcept override;
	bool sendResponse(std::unique_ptr<esl::http::server::ResponseDynamic> response) noexcept override;
	bool sendResponse(std::unique_ptr<esl::http::server::ResponseStatic> response) noexcept override;

	bool sendQueue() noexcept;
	bool hasResponseSent() noexcept;

private:
    static long int contentReaderCallback(void* cls, uint64_t bytesTransmitted, char* buffer, size_t bufferSize);
    static void contentReaderFreeCallback(void* cls);

	std::unique_ptr<esl::http::server::RequestHandler> requestHandler;
	MHD_Connection& mhdConnection;
	std::vector<MHD_Response*> mhdResponses;
	std::vector<std::function<bool()>> queueToSend;
	bool responseSent = false;
	Request request;

	static esl::logging::Logger logger;
};

} /* namespace mhd4esl */

#endif /* MHD4ESL_CONNECTION_H_ */
