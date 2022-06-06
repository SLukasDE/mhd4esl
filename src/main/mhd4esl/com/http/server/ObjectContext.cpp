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

#include <mhd4esl/com/http/server/ObjectContext.h>

namespace mhd4esl {
namespace com {
namespace http {
namespace server {

std::set<std::string> ObjectContext::getObjectIds() const {
	std::set<std::string> rv;

	for(const auto& object : objects) {
		rv.insert(object.first);
	}

	return rv;
}

esl::object::Interface::Object* ObjectContext::findRawObject(const std::string& id) {
	auto iter = objects.find(id);
	return iter == std::end(objects) ? nullptr : iter->second.get();
}

const esl::object::Interface::Object* ObjectContext::findRawObject(const std::string& id) const {
	auto iter = objects.find(id);
	return iter == std::end(objects) ? nullptr : iter->second.get();
}

void ObjectContext::addRawObject(const std::string& id, std::unique_ptr<esl::object::Interface::Object> object) {
	objects[id] = std::move(object);
}

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */
