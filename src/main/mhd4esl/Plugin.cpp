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

#include <mhd4esl/Plugin.h>
#include <mhd4esl/com/http/server/Socket.h>

#include <esl/com/http/server/Socket.h>

namespace mhd4esl {

void Plugin::install(esl::plugin::Registry& registry, const char* data) {
	esl::plugin::Registry::set(registry);

	registry.addPlugin<esl::com::http::server::Socket>(
			"mhd4esl/com/http/server/Socket",
			&com::http::server::Socket::create);
}

} /* namespace mhd4esl */
