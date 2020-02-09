/*
 * This file is part of mhd4esl.
 * Copyright (C) 2019, 2020 Sven Lukas
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

#include <mhd4esl/RequestContext.h>
#include <mhd4esl/Socket.h>

namespace mhd4esl {

RequestContext::RequestContext(const Socket& aSocket, MHD_Connection& mhdConnection, const char* version, const char* method, const char* url, bool isHTTPS, unsigned int port)
: esl::http::server::RequestContext(),
  socket(aSocket),
  connection(mhdConnection),
  request(mhdConnection, version, method, url, isHTTPS, port)
{ }

esl::http::server::Connection& RequestContext::getConnection() const {
	return connection;
}
const esl::http::server::Request& RequestContext::getRequest() const {
	return request;
}

const std::string& RequestContext::getPath() const {
	return request.getPath();
}

esl::Object* RequestContext::getObject(const std::string& id) const {
	esl::http::server::Interface::Socket::GetObject getObject = socket.getObject(id);
	if(getObject) {
		return getObject(*this);
	}
	return nullptr;
}

} /* namespace mhd4esl */
