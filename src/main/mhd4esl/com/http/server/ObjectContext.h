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

#ifndef MHD4ESL_COM_HTTP_SERVER_OBJECTCONTEXT_H_
#define MHD4ESL_COM_HTTP_SERVER_OBJECTCONTEXT_H_

#include <esl/object/Interface.h>
#include <esl/object/ObjectContext.h>

#include <string>
#include <map>
#include <memory>

namespace mhd4esl {
namespace com {
namespace http {
namespace server {

class ObjectContext final : public esl::object::ObjectContext {
public:
	void addObject(const std::string& id, std::unique_ptr<esl::object::Interface::Object> object) override;

protected:
	esl::object::Interface::Object* findRawObject(const std::string& id) override;
	const esl::object::Interface::Object* findRawObject(const std::string& id) const override;

private:
	std::map<std::string, std::unique_ptr<esl::object::Interface::Object>> objects;
};

} /* namespace server */
} /* namespace http */
} /* namespace com */
} /* namespace mhd4esl */

#endif /* MHD4ESL_COM_HTTP_SERVER_OBJECTCONTEXT_H_ */
