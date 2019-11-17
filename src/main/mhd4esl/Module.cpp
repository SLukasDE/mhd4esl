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

#include <mhd4esl/Module.h>
#include <mhd4esl/Socket.h>
#include <esl/http/server/Interface.h>
#include <esl/http/server/RequestHandlerFactory.h>
#include <esl/bootstrap/Interface.h>
#include <new>         // placement new
#include <type_traits> // aligned_storage

namespace mhd4esl {

namespace {
class Module : public esl::bootstrap::Module {
public:
	Module() = default;
	~Module() = default;

	static void initialize();

private:
	esl::http::server::Interface interfaceHttpServer;
};

typename std::aligned_storage<sizeof(Module), alignof(Module)>::type moduleBuffer; // memory for the object;
Module& module = reinterpret_cast<Module&> (moduleBuffer);

esl::http::server::Interface::Socket* createSocket(uint16_t port, uint16_t numThreads, esl::http::server::RequestHandlerFactory requestHandlerFactory) {
	return new Socket(port, numThreads, requestHandlerFactory);
}

void Module::initialize() {
	static bool isInitialized = false;

	if(isInitialized == false) {
		isInitialized = true;

		/* ***************** *
		 * initialize module *
		 * ***************** */
		new (&module) Module(); // placement new
		esl::bootstrap::Module::initialize(module);
		esl::http::server::Interface::initialize(module.interfaceHttpServer, &createSocket);
		module.interfacesProvided.next = &module.interfaceHttpServer;
	}
}
}

const esl::bootstrap::Module& getModule() {
	Module::initialize();
	return module;
}

} /* namespace mhd4esl */
