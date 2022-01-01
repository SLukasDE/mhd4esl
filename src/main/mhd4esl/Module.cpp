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

#include <mhd4esl/Module.h>
#include <mhd4esl/com/http/server/Socket.h>

#include <esl/com/http/server/Interface.h>
#include <esl/Module.h>

namespace mhd4esl {

void Module::install(esl::module::Module& module) {
	esl::setModule(module);

	module.addInterface(esl::com::http::server::Interface::createInterface(
			com::http::server::Socket::getImplementation(),
			&com::http::server::Socket::create));
}

} /* namespace mhd4esl */
