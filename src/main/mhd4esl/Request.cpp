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

#include <mhd4esl/Request.h>
#include <esl/Stacktrace.h>
#include <microhttpd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace mhd4esl {

Request::Request(MHD_Connection& aMhdConnection, const char* aHttpVersion, const char* aMethod, const char* aUrl, bool aIsHttps, unsigned int aPort)
: esl::http::server::Request(),
  mhdConnection(aMhdConnection),
  isHttps(aIsHttps),
  httpVersion(aHttpVersion),
  port(aPort),
  method(aMethod),
  url(aUrl)
{
	const MHD_ConnectionInfo* connectionInfo = MHD_get_connection_info(&mhdConnection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

	if(connectionInfo != nullptr) {
		char str[INET6_ADDRSTRLEN];
		bool conversionSuccessfull = false;

		switch(connectionInfo->client_addr->sa_family) {
		case AF_INET:
			conversionSuccessfull = inet_ntop(connectionInfo->client_addr->sa_family,
					&reinterpret_cast<sockaddr_in const*>(connectionInfo->client_addr)->sin_addr,
					str, INET6_ADDRSTRLEN) != nullptr;
			break;
		case AF_INET6:
			conversionSuccessfull = inet_ntop(connectionInfo->client_addr->sa_family,
					&reinterpret_cast<sockaddr_in6 const*>(connectionInfo->client_addr)->sin6_addr,
					str, INET6_ADDRSTRLEN) != nullptr;
			break;
		}
		if(conversionSuccessfull) {
			clientAddress = ::std::string(str);
		}
	}

	char* tmpPassword = nullptr;
	char* tmpUsername = nullptr;

	tmpUsername = MHD_basic_auth_get_username_password(&mhdConnection, &tmpPassword);

	if(tmpUsername != nullptr) {
		username = tmpUsername;
		::free(tmpUsername);
	}

	if(tmpPassword != nullptr) {
		password = tmpPassword;
		::free(tmpPassword);
    }

    const char* acceptHeaderStr = MHD_lookup_connection_value(&mhdConnection, MHD_HEADER_KIND, "Accept");
    if(acceptHeaderStr) {
        acceptHeader = std::string(acceptHeaderStr);
    }

    const char* contentTypeHeaderStr = MHD_lookup_connection_value(&mhdConnection, MHD_HEADER_KIND, "Content-Type");
    if(contentTypeHeaderStr) {
        contentTypeHeader = std::string(contentTypeHeaderStr);
    }
    // e.g. Content-Encoding: gzip
    ::std::string contentEncodingHeader;
    const char* contentEncodingHeaderStr = MHD_lookup_connection_value(&mhdConnection, MHD_HEADER_KIND, "Content-Encoding");
    if(contentEncodingHeaderStr) {
        contentEncodingHeader = std::string(contentEncodingHeaderStr);
    }

}

bool Request::isHTTPS() const noexcept {
	return isHttps;
}

const std::string& Request::getHTTPVersion() const noexcept {
	return httpVersion;
}

const std::string& Request::getUsername() const noexcept {
	return username;
}

const std::string& Request::getPassword() const noexcept {
	return password;
}

const std::string& Request::getDomain() const noexcept {
	if(!host) {
	    const char* hostStr = MHD_lookup_connection_value(&mhdConnection, MHD_HEADER_KIND, "Host");
        if(hostStr) {
        	std::string tmpStr(hostStr);

            host.reset(new std::string(tmpStr.substr(0, tmpStr.find_first_of(':'))));
        }
        else {
            host.reset(new std::string);
        }
	}
	return *host.get();
}

unsigned int Request::getPort() const noexcept {
	return port;
}

const std::string& Request::getPath() const noexcept {
	return url;
}

const std::string& Request::getMethod() const noexcept {
	return method;
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
		throw esl::addStacktrace(std::runtime_error("argument \"" + key + "\" no found"));
	}
	return arguments[key];
}

const std::string& Request::getClientAddress() const noexcept {
	return clientAddress;
}

} /* namespace mhd4esl */
