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

#include <memory>
#include <string>

#if defined(_WIN32)
/// Don't include <windows.h>, which will shadow our Rectangle class.
///
#define RPC_NO_WINDOWS_H
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif



/// Class wrapping IETF v4 GUIDs, providing lazy initialization.
///
class EsUuid final {
public:
	/// Used to represent a UUID across supported platforms.
	///
	struct UuidType final {
		UuidType() = default;
		UuidType(UuidType &&) = default;
		UuidType &operator=(UuidType &&) = default;
		~UuidType() = default;
		UuidType &operator=(const UuidType &other) { return *this = UuidType(other); }
#if defined(_WIN32)
		UuidType(const UuidType &other) = default;
		UUID id = {};
#else
		UuidType(const UuidType &other) { uuid_copy(id, other.id); }
		uuid_t id = {};
#endif
	};


public:
	static EsUuid FromString(const std::string &input);
	EsUuid() noexcept = default;
	~EsUuid() noexcept = default;
	/// Copying a UUID does not copy its value. (This allows us to use simple copy operations on stock
	/// ship definitions when spawning fleets, etc.)
	EsUuid(const EsUuid &other) noexcept : value{} {};
	/// Copy-assigning also results in an empty UUID.
	///
	EsUuid &operator=(const EsUuid &other) noexcept { return *this = EsUuid(other); };
	/// UUIDs can be move-constructed as-is.
	///
	EsUuid(EsUuid &&) noexcept = default;
	/// UUIDs can be move-assigned as-is.
	///
	EsUuid &operator=(EsUuid &&) noexcept = default;

	/// UUIDs can be compared against other UUIDs.
	///
	bool operator==(const EsUuid &other) const noexcept(false);
	bool operator!=(const EsUuid &other) const noexcept(false);
	bool operator<(const EsUuid &other) const noexcept(false);

	/// Explicitly clone this UUID.
	///
	void clone(const EsUuid &other);

	/// Get a string representation of this ID, e.g. for serialization.
	///
	std::string ToString() const noexcept(false);


private:
	/// Internal constructor, from a string.
	///
	explicit EsUuid(const std::string &input);
	/// Lazy initialization getter.
	///
	const UuidType &Value() const;


private:
	mutable UuidType value;
};



template <class T>
struct UUIDComparator {
	/// Comparator for collections of shared_ptr<T>
	///
	bool operator() (const std::shared_ptr<T> &a, const std::shared_ptr<T> &b) const noexcept(false)
	{
		return a->UUID() < b->UUID();
	}

	/// Comparator for collections of T*, e.g. set<T *>
	///
	bool operator()(const T *a, const T *b) const noexcept(false)
	{
		return a->UUID() < b->UUID();
	}
	/// No comparator for collections of T, as std containers generally perform copy operations
	/// and copying this class will eventually be disabled.
};
