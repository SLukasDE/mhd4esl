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

#include <mhd4esl/com/http/server/RequestContext.h>
#include <mhd4esl/com/http/server/Socket.h>

namespace mhd4esl {
namespace com {
namespace http {
namespace server {

RequestContext::RequestContext(MHD_Connection& mhdConnection, const char* version, const char* method, const char* url, bool isHTTPS, uint16_t port)
: esl::com::http::server::RequestContext(),
  connection(mhdConnection),
  request(mhdConnection, version, method, url, isHTTPS, port)
{ }

esl::com::http::server::Connection& RequestContext::getConnection() const {
	return connection;
}

const esl::com::http::server::Request& RequestContext::getRequest() const {
	return request;
}

const std::string& RequestContext::getPath() const {
	return request.getPath();
}

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */
