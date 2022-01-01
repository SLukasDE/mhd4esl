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

#ifndef MHD4ESL_COM_HTTP_SERVER_SOCKET_H_
#define MHD4ESL_COM_HTTP_SERVER_SOCKET_H_

#include <esl/com/http/server/requesthandler/Interface.h>
#include <esl/com/http/server/Interface.h>
#include <esl/com/http/server/Request.h>
#include <esl/object/Values.h>
#include <esl/object/Interface.h>

#include <cstdint>
#include <string.h> // size_t
#include <string>
#include <vector>
#include <memory>
#include <condition_variable>
#include <mutex>

struct MHD_Connection;

namespace mhd4esl {
namespace com {
namespace http {
namespace server {

class RequestContext;

class Socket : public esl::com::http::server::Interface::Socket {
public:
	static inline const char* getImplementation() {
		return "mhd4esl";
	}

	static std::unique_ptr<esl::com::http::server::Interface::Socket> create(const esl::com::http::server::Interface::Settings& settings);

	Socket(const esl::com::http::server::Interface::Settings& settings);
	~Socket();

	void addTLSHost(const std::string& hostname, std::vector<unsigned char> certificate, std::vector<unsigned char> key) override;

	void listen(const esl::com::http::server::requesthandler::Interface::RequestHandler& requestHandler, std::function<void()> onReleasedHandler) override;
	void release() override;
	bool wait(std::uint32_t ms) override;

private:
	static int mhdAcceptHandler(void* cls,
	        MHD_Connection* connection,
	        const char* url,
	        const char* method,
	        const char* version,
	        const char* uploadData,
	        size_t* uploadDataSize,
	        void** connectionSpecificDataPtr) noexcept;
	static bool accept(RequestContext& requestContext, const char* uploadData, size_t* uploadDataSize) noexcept;

	void accessThreadInc() noexcept {}
	void accessThreadDec() noexcept {}

	struct Settings {
		uint16_t port = 0;
		uint16_t numThreads = 4;
		unsigned int connectionTimeout = 120;
		unsigned int connectionLimit = 1000;
		unsigned int perIpConnectionLimit = 0;
	} settings;
	const esl::com::http::server::requesthandler::Interface::RequestHandler* requestHandler = nullptr;

	void* daemonPtr = nullptr; // MHD_Daemon*
	bool usingTLS = false;
	std::function<void()> onReleasedHandler = nullptr;


	/* ****************** *
	 * wait method *
	 * ****************** */
	std::mutex waitNotifyMutex;
	std::condition_variable waitCondVar;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */

#endif /* MHD4ESL_COM_HTTP_SERVER_SOCKET_H_ */
