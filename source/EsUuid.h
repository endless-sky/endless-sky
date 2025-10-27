/* EsUuid.h
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

#pragma once

#ifdef _WIN32
// Don't include <windows.h>, which will shadow our Rectangle class.
#define RPC_NO_WINDOWS_H
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <string>



// Class wrapping IETF v4 GUIDs, providing lazy initialization.
class EsUuid final {
public:
	// Used to represent a UUID across supported platforms.
	struct UuidType final {
		UuidType() = default;
		UuidType(UuidType &&) = default;
		UuidType &operator=(UuidType &&) = default;
		~UuidType() = default;
		UuidType &operator=(const UuidType &other) { return *this = UuidType(other); }
#ifdef _WIN32
		UuidType(const UuidType &other) = default;
		UUID id = {};
#else
		UuidType(const UuidType &other) { uuid_copy(id, other.id); }
		uuid_t id = {};
#endif
	};


public:
	static UuidType MakeUuid();
	static EsUuid FromString(const std::string &input);


public:
	EsUuid() noexcept = default;
	~EsUuid() noexcept = default;
	// Copying a UUID does not copy its value. (This allows us to use simple copy operations on stock
	// ship definitions when spawning fleets, etc.)
	EsUuid(const EsUuid &other) noexcept : value{} {};
	// Copy-assigning also results in an empty UUID.
	EsUuid &operator=(const EsUuid &other) noexcept { return *this = EsUuid(other); };
	// UUIDs can be move-constructed as-is.
	EsUuid(EsUuid &&) noexcept = default;
	// UUIDs can be move-assigned as-is.
	EsUuid &operator=(EsUuid &&) noexcept = default;

	// UUIDs can be compared against other UUIDs.
	bool operator==(const EsUuid &other) const noexcept(false);
	bool operator!=(const EsUuid &other) const noexcept(false);
	bool operator<(const EsUuid &other) const noexcept(false);

	// Explicitly clone this UUID.
	void Clone(const EsUuid &other);

	// Get a string representation of this ID, e.g. for serialization.
	std::string ToString() const noexcept(false);


private:
	// Internal constructor, from a string.
	explicit EsUuid(const std::string &input);
	// Lazy initialization getter.
	const UuidType &Value() const;


private:
	mutable UuidType value;
};
