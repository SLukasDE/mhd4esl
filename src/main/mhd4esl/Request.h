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

#ifndef MHD4ESL_REQUEST_H_
#define MHD4ESL_REQUEST_H_

#include <esl/http/server/Request.h>

#include <string>
#include <map>
#include <memory>
#include <cstdint>

struct MHD_Connection;

namespace mhd4esl {

class Request : public esl::http::server::Request {
public:
	Request(MHD_Connection& mhdConnection, const char* httpVersion, const char* method, const char* url, bool isHttps, uint16_t hostPort);
	~Request() = default;

	bool isHTTPS() const noexcept override;
	const std::string& getHTTPVersion() const noexcept override;
	const std::string& getUsername() const noexcept override;
	const std::string& getPassword() const noexcept override;

	const std::string& getHost() const noexcept override;
	const std::string& getHostAddress() const noexcept override;
	uint16_t getHostPort() const noexcept override;

	const std::string& getRemoteAddress() const noexcept override;
	uint16_t getRemotePort() const noexcept override;

	const std::string& getPath() const noexcept override;
	const std::string& getMethod() const noexcept override;
	bool hasArgument(const std::string& key) const noexcept override;
	const std::string& getArgument(const std::string& key) const override;


private:
	MHD_Connection& mhdConnection;

	bool isHttps;
	const std::string httpVersion;
	std::string username;
	std::string password;

	std::string host;
	const uint16_t hostPort;
	std::string hostAddress;

	std::string remoteAddress;
	uint16_t remotePort = 0;

	const std::string method;
	const std::string url;


    std::string acceptHeader;
    std::string contentTypeHeader;
    std::string contentEncodingHeader;

	mutable std::map<std::string, std::string> arguments;
};

} /* namespace mhd4esl */

#endif /* MHD4ESL_REQUEST_H_ */
