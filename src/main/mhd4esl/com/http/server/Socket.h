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

#ifndef MHD4ESL_COM_HTTP_SERVER_SOCKET_H_
#define MHD4ESL_COM_HTTP_SERVER_SOCKET_H_

#include <esl/com/http/server/MHDSocket.h>

#include <esl/com/http/server/RequestHandler.h>
#include <esl/com/http/server/Request.h>
#include <esl/object/Object.h>

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string.h> // size_t
#include <utility>

#include <microhttpd.h>
//struct MHD_Connection;
//forward declaration of "enum MHD_Result" is not possible

namespace mhd4esl {
inline namespace v1_6 {
namespace com {
namespace http {
namespace server {

class RequestContext;

class Socket : public esl::com::http::server::Socket {
public:
	Socket(const esl::com::http::server::MHDSocket::Settings& settings);
	~Socket();

	void listen(const esl::com::http::server::RequestHandler& requestHandler) override;
	void listen(const esl::com::http::server::RequestHandler& requestHandler, std::function<void()> onReleasedHandler) override;
	void release() override;

	bool wait(std::uint32_t ms);

private:
	static MHD_Result mhdAcceptHandler(void* cls,
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

	esl::com::http::server::MHDSocket::Settings settings;
	const esl::com::http::server::RequestHandler* requestHandler = nullptr;
	void* daemonPtr = nullptr; // MHD_Daemon*
	bool usingTLS = false;
	std::function<void()> onReleasedHandler;


	/* ****************** *
	 * wait method *
	 * ****************** */
	std::mutex waitNotifyMutex;
	std::condition_variable waitCondVar;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* inline namespace v1_6 */
} /* namespace mhd4esl */

#endif /* MHD4ESL_COM_HTTP_SERVER_SOCKET_H_ */
