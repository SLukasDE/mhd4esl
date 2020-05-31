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

#ifndef MHD4ESL_REQUESTCONTEXT_H_
#define MHD4ESL_REQUESTCONTEXT_H_

#include <esl/http/server/RequestContext.h>
#include <esl/http/server/requesthandler/Interface.h>
#include <esl/http/server/Connection.h>
#include <esl/http/server/Request.h>
#include <esl/object/Interface.h>

#include <mhd4esl/Connection.h>
#include <mhd4esl/Request.h>

#include <string>
#include <memory>
#include <cstdint>

struct MHD_Connection;

namespace mhd4esl {

class Socket;

class RequestContext : public esl::http::server::RequestContext {
	friend class Socket;
public:
	RequestContext(const Socket& socket, MHD_Connection& mhdConnection, const char* version, const char* method, const char* url, bool isHTTPS, uint16_t port);

	esl::http::server::Connection& getConnection() const override;
	const esl::http::server::Request& getRequest() const override;
	const std::string& getPath() const override;

	esl::object::Interface::Object* getObject(const std::string& id) const override;

private:
	const Socket& socket;
	mutable Connection connection;
	Request request;
	std::unique_ptr<esl::http::server::requesthandler::Interface::RequestHandler> requestHandler;
};

} /* namespace mhd4esl */

#endif /* MHD4ESL_REQUESTCONTEXT_H_ */
