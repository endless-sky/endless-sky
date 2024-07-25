/* PartiallyGuardedMap.h
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PARTIALLY_GUARDED_MAP_H_
#define PARTIALLY_GUARDED_MAP_H_

#include <map>
#include <mutex>



// Vectors that guard against most concurrent modifications. Use with caution.
// For most purposes, this can be treated and passed around like any stl container,
// but any function modifying it inside a concurrent context MUST receive it as a PartiallyGuarded object
// to ensure proper handling.
template<class Key, class Value, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, Value>>>
class PartiallyGuardedMap : public std::map<Key, Value, Compare, Allocator> {
public:
	template<class... Args>
	inline auto emplace(Args &&... args)
	{
		const std::lock_guard<std::mutex> lock(write_mutex);
		return std::map<Key, Value, Compare, Allocator>::emplace(args...);
	}

	inline auto &operator[](const Key &key)
	{
		const std::lock_guard<std::mutex> lock(write_mutex);
		return std::map<Key, Value, Compare, Allocator>::operator[](key);
	}

	inline auto erase(const Key &key)
	{
		const std::lock_guard<std::mutex> lock(write_mutex);
		return std::map<Key, Value, Compare, Allocator>::erase(key);
	}

	// Prevent accidental unsafe operations.
	// These can be implemented later, if necessary.
	void insert(const Value &value) = delete;
	template<class P>
	void insert(P &&value) = delete;
	void insert(typename std::map<Key, Value, Compare, Allocator>::iterator pos, const Value &value) = delete;

private:
	std::mutex write_mutex;
};

#endif //ENDLESS_SKY_PARTIALLYGUARDEDMAP_H
