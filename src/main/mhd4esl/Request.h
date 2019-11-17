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

#ifndef MHD4ESL_REQUEST_H_
#define MHD4ESL_REQUEST_H_

#include <esl/http/server/Request.h>
#include <string>
#include <map>

struct MHD_Connection;

namespace mhd4esl {

class Request : public esl::http::server::Request {
public:
	Request(MHD_Connection& mhdConnection, const char* version, const char* method, const char* url);
	~Request() = default;

	const std::string& getVersion() const noexcept override;
	const std::string& getMethod() const noexcept override;
	const std::string& getUrl() const noexcept override;
	bool hasArgument(const std::string& key) const noexcept override;
	const std::string& getArgument(const std::string& key) const override;

	const std::string& getClientAddress() const noexcept override;

	const std::string& getUsername() const noexcept override;
	const std::string& getPassword() const noexcept override;

private:
	MHD_Connection& mhdConnection;

	const std::string version;
	const std::string method;
	const std::string url;

	std::string username;
	std::string password;

    std::string acceptHeader;
    std::string contentTypeHeader;
    std::string contentEncodingHeader;

	std::string clientAddress;
	mutable std::map<std::string, std::string> arguments;
};

} /* namespace mhd4esl */

#endif /* MHD4ESL_REQUEST_H_ */
