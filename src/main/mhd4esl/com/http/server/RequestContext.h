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

#ifndef MHD4ESL_COM_HTTP_SERVER_REQUESTCONTEXT_H_
#define MHD4ESL_COM_HTTP_SERVER_REQUESTCONTEXT_H_

#include <mhd4esl/com/http/server/Connection.h>
#include <mhd4esl/com/http/server/Request.h>

#include <common4esl/object/Context.h>

#include <esl/com/http/server/RequestContext.h>
#include <esl/com/http/server/Connection.h>
#include <esl/com/http/server/Request.h>
#include <esl/io/Input.h>
//#include <esl/object/Object.h>
#include <esl/object/Context.h>

#include <string>
#include <memory>
#include <cstdint>

struct MHD_Connection;

namespace mhd4esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

class RequestContext : public esl::com::http::server::RequestContext {
	friend class Socket;
public:
	RequestContext(MHD_Connection& mhdConnection, const char* version, const char* method, const char* url, bool isHTTPS, uint16_t port);

	esl::com::http::server::Connection& getConnection() const override;
	const esl::com::http::server::Request& getRequest() const override;
	const std::string& getPath() const override;
	esl::object::Context& getObjectContext() override;

private:
	mutable Connection connection;
	Request request;
	esl::io::Input input;
	common4esl::object::Context context;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* inline namespace v1_6 */
} /* namespace mhd4esl */

#endif /* MHD4ESL_COM_HTTP_SERVER_REQUESTCONTEXT_H_ */
