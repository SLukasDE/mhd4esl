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

#ifndef MHD4ESL_SOCKET_H_
#define MHD4ESL_SOCKET_H_

#include <esl/http/server/requesthandler/Interface.h>
#include <esl/http/server/Interface.h>
#include <esl/http/server/Request.h>
#include <esl/object/Values.h>

#include <cstdint>
#include <string.h> // size_t
#include <string>
#include <vector>
#include <memory>
#include <map>

struct MHD_Connection;

namespace mhd4esl {

class RequestContext;

class Socket : public esl::http::server::Interface::Socket {
public:
	static inline const char* getImplementation() {
		return "mhd4esl";
	}

	static std::unique_ptr<esl::http::server::Interface::Socket> create(uint16_t port, esl::http::server::requesthandler::Interface::CreateRequestHandler createRequestHandler, const esl::object::Values<std::string>& settings);

	Socket(std::uint16_t port, esl::http::server::requesthandler::Interface::CreateRequestHandler createRequestHandler, const esl::object::Values<std::string>& settings);
	~Socket();

	void addTLSHost(const std::string& hostname, std::vector<unsigned char> certificate, std::vector<unsigned char> key) override;

	void addObjectFactory(const std::string& id, ObjectFactory objectFactory) override;
	ObjectFactory getObjectFactory(const std::string& id) const;

	bool listen() override;
	void release() override;

private:
	static int mhdAcceptHandler(void* cls,
	        MHD_Connection* connection,
	        const char* url,
	        const char* method,
	        const char* version,
	        const char* uploadData,
	        size_t* uploadDataSize,
	        void** connectionSpecificDataPtr) noexcept;
	bool accept(RequestContext& requestContext, const char* uploadData, size_t* uploadDataSize) noexcept;

	void accessThreadInc() noexcept {}
	void accessThreadDec() noexcept {}

	uint16_t port;
	uint16_t numThreads = 4;
	esl::http::server::requesthandler::Interface::CreateRequestHandler createRequestHandler;

	void* daemonPtr = nullptr; // MHD_Daemon*
	bool usingTLS = false;
	std::map<std::string, ObjectFactory> objectFactories;
};

} /* namespace mhd4esl */

#endif /* MHD4ESL_SOCKET_H_ */
