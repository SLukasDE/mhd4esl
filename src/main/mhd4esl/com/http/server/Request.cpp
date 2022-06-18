/*
 * This file is part of mhd4esl.
 * Copyright (C) 2019-2022 Sven Lukas
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

#include <mhd4esl/com/http/server/Request.h>

#include <esl/stacktrace/Stacktrace.h>
#include <esl/utility/String.h>

#include <microhttpd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstring>

namespace mhd4esl {
namespace com {
namespace http {
namespace server {

Request::Request(MHD_Connection& aMhdConnection, const char* aHttpVersion, const char* aMethod, const char* aUrl, bool aIsHttps, uint16_t aHostPort)
: mhdConnection(aMhdConnection),
  isHttps(aIsHttps),
  httpVersion(aHttpVersion),
  hostPort(aHostPort),
  method(aMethod),
  url(aUrl)
{
	const MHD_ConnectionInfo* connectionInfo = MHD_get_connection_info(&mhdConnection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

	MHD_get_connection_values(&mhdConnection, MHD_HEADER_KIND, readHeaders, this);

	if(connectionInfo != nullptr) {
		char strBuffer[INET6_ADDRSTRLEN];

		switch(connectionInfo->client_addr->sa_family) {
		case AF_INET:
			if(inet_ntop(connectionInfo->client_addr->sa_family,
					&reinterpret_cast<sockaddr_in const*>(connectionInfo->client_addr)->sin_addr,
					strBuffer, INET6_ADDRSTRLEN) != nullptr) {
				remoteAddress = std::string(strBuffer);
			}
			break;
		case AF_INET6:
			if(inet_ntop(connectionInfo->client_addr->sa_family,
					&reinterpret_cast<sockaddr_in6 const*>(connectionInfo->client_addr)->sin6_addr,
					strBuffer, INET6_ADDRSTRLEN) != nullptr) {
				remoteAddress = std::string(strBuffer);
			}
			break;
		}

		remotePort = static_cast<uint16_t>(reinterpret_cast<sockaddr_in const*>(connectionInfo->client_addr)->sin_port);
	}
}

bool Request::isHTTPS() const noexcept {
	return isHttps;
}

const std::string& Request::getHTTPVersion() const noexcept {
	return httpVersion;
}

const std::string& Request::getHostName() const noexcept {
	return hostName;
}

const std::string& Request::getHostAddress() const noexcept {
	return hostAddress;
}

uint16_t Request::getHostPort() const noexcept {
	return hostPort;
}

const std::string& Request::getPath() const noexcept {
	return url;
}

const esl::utility::HttpMethod& Request::getMethod() const noexcept {
	return method;
}

const std::map<std::string, std::string>& Request::getHeaders() const noexcept {
	return headers;
}

const esl::utility::MIME& Request::getContentType() const noexcept {
	return contentType;
}

bool Request::hasArgument(const std::string& key) const noexcept {
	auto iter = arguments.find(key);
	if(iter == arguments.end()) {
		const char* value = MHD_lookup_connection_value(&mhdConnection, MHD_GET_ARGUMENT_KIND, key.c_str());
		if(value != nullptr) {
			iter = arguments.emplace(std::make_pair(key, std::string(value))).first;
		}
	}

	return iter != arguments.end();
}

const std::string& Request::getArgument(const std::string& key) const {
	if(!hasArgument(key)) {
		throw esl::stacktrace::Stacktrace::add(std::runtime_error("argument \"" + key + "\" no found"));
	}
	return arguments[key];
}

const std::string& Request::getRemoteAddress() const noexcept {
	return remoteAddress;
}

uint16_t Request::getRemotePort() const noexcept {
	return remotePort;
}

int Request::readHeaders(void* requestPtr, MHD_ValueKind, const char* key, const char* valuePtr) {
	Request& request = *reinterpret_cast<Request*>(requestPtr);

	std::string& value = request.headers[key];
	if(valuePtr) {
		value = valuePtr;

		if(std::strncmp(key, "Host", 4) == 0) {
			std::string::size_type pos = value.find_first_of(':');
			if(pos == std::string::npos) {
				request.hostName = value;
			}
			else {
				request.hostName = value.substr(0, pos);
			}
		}
		else if(std::strncmp(key, "Content-Type", 12) == 0) {
			// Value could be "text/html; charset=UTF-8", so we have to split for ';' character and we take first element
			std::vector<std::string> contentTypes = esl::utility::String::split(esl::utility::String::trim(value), ';');
			if(!contentTypes.empty()) {
				request.contentType = esl::utility::MIME(esl::utility::String::trim(contentTypes.front()));
			}
		}
		/*
		else if(key == "Content-Encoding") {
			request.contentEncodingHeader = value;
		}
		else if(key == "Accept") {
			request.acceptHeader = value;
		}
		*/
	}

	return MHD_YES;
}

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */
