/* EsUuid.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "EsUuid.h"

#include "Logger.h"
#if defined(_WIN32)
#include "text/Utf8.h"
#endif


#include <stdexcept>

#if defined(_WIN32)
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <uuid/uuid.h>
#endif

namespace es_uuid {
namespace detail {
#if defined(_WIN32)
// Get a version 4 (random) Universally Unique Identifier (see IETF RFC 4122).
EsUuid::UuidType MakeUuid()
{
	EsUuid::UuidType value;
	RPC_STATUS status = UuidCreate(&value.id);
	if(status == RPC_S_UUID_LOCAL_ONLY)
		Logger::LogError("Created locally unique UUID only");
	else if(status == RPC_S_UUID_NO_ADDRESS)
		throw std::runtime_error("Failed to create UUID");

	return value;
}

EsUuid::UuidType ParseUuid(const std::string &str)
{
	EsUuid::UuidType value;
	auto data = Utf8::ToUTF16(str, false);
	RPC_STATUS status = UuidFromStringW(reinterpret_cast<RPC_WSTR>(&data[0]), &value.id);
	if(status == RPC_S_INVALID_STRING_UUID)
		throw std::invalid_argument("Cannot convert \"" + str + "\" into a UUID");
	else if(status != RPC_S_OK)
		throw std::runtime_error("Fatal error parsing \"" + str + "\" as a UUID");

	return value;
}

bool IsNil(const UUID &id) noexcept
{
	RPC_STATUS status;
	return UuidIsNil(const_cast<UUID *>(&id), &status);
}

std::string Serialize(const UUID &id)
{
	wchar_t *buf = nullptr;
	RPC_STATUS status = UuidToStringW(const_cast<UUID *>(&id), reinterpret_cast<RPC_WSTR *>(&buf));

	std::string result = (status == RPC_S_OK) ? Utf8::ToUTF8(buf) : "";
	if(result.empty())
		Logger::LogError("Failed to serialize UUID!");
	else
		RpcStringFreeW(reinterpret_cast<RPC_WSTR *>(&buf));

	return result;
}

signed int Compare(const EsUuid::UuidType &a, const EsUuid::UuidType &b)
{
	RPC_STATUS status;
	auto result = UuidCompare(const_cast<UUID *>(&a.id), const_cast<UUID *>(&b.id), &status);
	if(status != RPC_S_OK)
		throw std::runtime_error("Fatal error comparing UUIDs \"" + Serialize(a.id) + "\" and \"" + Serialize(b.id) + "\"");
	return result;
}
#else
constexpr std::size_t UUID_BUFFER_LENGTH = 37;

// Get a version 4 (random) Universally Unique Identifier (see IETF RFC 4122).
EsUuid::UuidType MakeUuid()
{
	EsUuid::UuidType value;
	uuid_generate_random(value.id);
	return value;
}

EsUuid::UuidType ParseUuid(const std::string &str)
{
	EsUuid::UuidType value;
	auto result = uuid_parse(str.data(), value.id);
	if(result == -1)
		throw std::invalid_argument("Cannot convert \"" + str + "\" into a UUID");
	else if(result != 0)
		throw std::runtime_error("Fatal error parsing \"" + str + "\" as a UUID");

	return value;
}

bool IsNil(const uuid_t &id) noexcept
{
	return uuid_is_null(id) == 1;
}

std::string Serialize(const uuid_t &id)
{
	char buf[UUID_BUFFER_LENGTH];
	uuid_unparse_lower(id, buf);
	return std::string(buf);
}

signed int Compare(const EsUuid::UuidType &a, const EsUuid::UuidType &b) noexcept
{
	return uuid_compare(a.id, b.id);
}
#endif
}
}
using namespace es_uuid::detail;



EsUuid EsUuid::FromString(const std::string &input)
{
	return EsUuid(input);
}



// Explicitly copy the value of the other UUID.
void EsUuid::clone(const EsUuid &other)
{
	value = other.Value();
}



bool EsUuid::operator==(const EsUuid &other) const noexcept(false)
{
	return Compare(Value(), other.Value()) == 0;
}



bool EsUuid::operator!=(const EsUuid &other) const noexcept(false)
{
	return !(*this == other);
}



bool EsUuid::operator<(const EsUuid &other) const noexcept(false)
{
	return Compare(Value(), other.Value()) < 0;
}



std::string EsUuid::ToString() const noexcept(false)
{
	return Serialize(Value().id);
}



// Internal constructor. Note that the provided value may not be a valid v4 UUID,
// in which case an error is logged and we return a new UUID.
EsUuid::EsUuid(const std::string &input)
{
	try {
		value = ParseUuid(input);
	}
	catch(const std::invalid_argument &err)
	{
		Logger::LogError(err.what());
	}
}



const EsUuid::UuidType &EsUuid::Value() const
{
	if(IsNil(value.id))
		value = MakeUuid();

	return value;
}
