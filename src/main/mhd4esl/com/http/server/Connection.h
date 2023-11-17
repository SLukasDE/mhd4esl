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

#ifndef MHD4ESL_COM_HTTP_SERVER_CONNECTION_H_
#define MHD4ESL_COM_HTTP_SERVER_CONNECTION_H_

#include <esl/com/http/server/Connection.h>
#include <esl/com/http/server/Response.h>
#include <esl/io/Output.h>

#include <boost/filesystem.hpp>

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

struct MHD_Connection;
struct MHD_Response;

namespace mhd4esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

class Connection : public esl::com::http::server::Connection {
friend class Socket;
public:
	Connection(MHD_Connection& mhdConnection);
	~Connection();

	bool sendQueue() noexcept;
	bool isResponseQueueEmpty() noexcept;
	bool hasResponseSent() noexcept;

	bool send(const esl::com::http::server::Response& response, const void* data, std::size_t size) noexcept;
	bool send(const esl::com::http::server::Response& response, esl::io::Output output) override;
	bool send(const esl::com::http::server::Response& response, boost::filesystem::path path) override;

private:
	bool sendResponse(const esl::com::http::server::Response& response, MHD_Response* mhdResponse) noexcept;

    static long int contentReaderCallback(void* cls, uint64_t bytesTransmitted, char* buffer, size_t bufferSize);
    static void contentReaderFreeCallback(void* cls);

	MHD_Connection& mhdConnection;
	std::vector<std::tuple<std::function<bool()>, MHD_Response*>> responseQueue;
	bool responseSent = false;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* inline namespace v1_6 */
} /* namespace mhd4esl */

#endif /* MHD4ESL_COM_HTTP_SERVER_CONNECTION_H_ */
