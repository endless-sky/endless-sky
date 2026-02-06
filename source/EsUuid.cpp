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
#ifdef _WIN32
#include "text/Utf8.h"

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <uuid/uuid.h>
#endif

#include <stdexcept>

using namespace std;

namespace {
#ifndef _WIN32
	constexpr size_t UUID_BUFFER_LENGTH = 37;
#endif

	EsUuid::UuidType ParseUuid(const string &str)
	{
		EsUuid::UuidType value;
#ifdef _WIN32
		auto data = Utf8::ToUTF16(str, false);
		RPC_STATUS status = UuidFromStringW(reinterpret_cast<RPC_WSTR>(&data[0]), &value.id);
		if(status == RPC_S_INVALID_STRING_UUID)
			throw invalid_argument("Cannot convert \"" + str + "\" into a UUID");
		else if(status != RPC_S_OK)
			throw runtime_error("Fatal error parsing \"" + str + "\" as a UUID");
#else
		auto result = uuid_parse(str.data(), value.id);
		if(result == -1)
			throw invalid_argument("Cannot convert \"" + str + "\" into a UUID");
		else if(result != 0)
			throw runtime_error("Fatal error parsing \"" + str + "\" as a UUID");
#endif
		return value;
	}

#ifdef _WIN32
	bool IsNil(const UUID &id) noexcept
	{
		RPC_STATUS status;
		return UuidIsNil(const_cast<UUID *>(&id), &status);
	}
#else
	bool IsNil(const uuid_t &id) noexcept
	{
		return uuid_is_null(id) == 1;
	}
#endif

#ifdef _WIN32
	string Serialize(const UUID &id)
	{
		wchar_t *buf = nullptr;
		RPC_STATUS status = UuidToStringW(&id, reinterpret_cast<RPC_WSTR *>(&buf));

		string result = (status == RPC_S_OK) ? Utf8::ToUTF8(buf) : string{};
		if(result.empty())
			Logger::Log("Failed to serialize UUID!", Logger::Level::WARNING);
		else
			RpcStringFreeW(reinterpret_cast<RPC_WSTR *>(&buf));

		return result;
	}
#else
	string Serialize(const uuid_t &id)
	{
		char buf[UUID_BUFFER_LENGTH];
		uuid_unparse_lower(id, buf);
		return buf;
	}
#endif

	signed int Compare(const EsUuid::UuidType &a, const EsUuid::UuidType &b)
	{
#ifdef _WIN32
		RPC_STATUS status;
		auto result = UuidCompare(const_cast<UUID *>(&a.id), const_cast<UUID *>(&b.id), &status);
		if(status != RPC_S_OK)
			throw runtime_error("Fatal error comparing UUIDs \"" + Serialize(a.id) + "\" and \"" + Serialize(b.id) + "\"");
		return result;
#else
		return uuid_compare(a.id, b.id);
#endif
	}
}



// Get a version 4 (random) Universally Unique Identifier (see IETF RFC 4122).
EsUuid::UuidType EsUuid::MakeUuid()
{
	EsUuid::UuidType value;
#ifdef _WIN32
	RPC_STATUS status = UuidCreate(&value.id);
	if(status == RPC_S_UUID_LOCAL_ONLY)
		Logger::Log("Created locally unique UUID only", Logger::Level::WARNING);
	else if(status == RPC_S_UUID_NO_ADDRESS)
		throw runtime_error("Failed to create UUID");
#else
	uuid_generate_random(value.id);
#endif
	return value;
}



EsUuid EsUuid::FromString(const string &input)
{
	return EsUuid(input);
}



// Explicitly copy the value of the other UUID.
void EsUuid::Clone(const EsUuid &other)
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



string EsUuid::ToString() const noexcept(false)
{
	return Serialize(Value().id);
}



// Internal constructor. Note that the provided value may not be a valid v4 UUID,
// in which case an error is logged and we return a new UUID.
EsUuid::EsUuid(const string &input)
{
	try {
		value = ParseUuid(input);
	}
	catch(const invalid_argument &err)
	{
		Logger::Log(err.what(), Logger::Level::WARNING);
	}
}



const EsUuid::UuidType &EsUuid::Value() const
{
	if(IsNil(value.id))
		value = MakeUuid();

	return value;
}
