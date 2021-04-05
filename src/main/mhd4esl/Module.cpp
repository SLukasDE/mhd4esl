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

#include <mhd4esl/Module.h>
#include <mhd4esl/http/server/Socket.h>

#include <esl/http/server/Interface.h>
#include <esl/http/server/requesthandler/Interface.h>
#include <esl/module/Interface.h>
#include <esl/Stacktrace.h>

#include <stdexcept>
#include <memory>
#include <new>         // placement new
#include <type_traits> // aligned_storage

namespace mhd4esl {

namespace {

class Module : public esl::module::Module {
public:
	Module();
};

typename std::aligned_storage<sizeof(Module), alignof(Module)>::type moduleBuffer; // memory for the object;
Module* modulePtr = nullptr;

Module::Module()
: esl::module::Module()
{
	esl::module::Module::initialize(*this);

	addInterface(std::unique_ptr<const esl::module::Interface>(new esl::http::server::Interface(
			getId(), http::server::Socket::getImplementation(), &http::server::Socket::create)));
}

} /* anonymous namespace */

esl::module::Module& getModule() {
	if(modulePtr == nullptr) {
		/* ***************** *
		 * initialize module *
		 * ***************** */

		modulePtr = reinterpret_cast<Module*> (&moduleBuffer);
		new (modulePtr) Module; // placement new
	}

	return *modulePtr;
}

} /* namespace mhd4esl */
