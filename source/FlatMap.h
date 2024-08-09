/* FlatMap.h
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

#pragma once

#include <functional>
#include <utility>
#include <vector>



// A generic flat map providing fast lookup and slower insertion.
template < class Key, class Value, class ThreeWayComp = std::compare_three_way, class Alloc =
		std::allocator<std::pair<Key, Value>>>
class FlatMap : private std::vector<std::pair<Key, Value>, Alloc> {
private:
	// Use references or copies for passing keys and values, depending on what's more efficient.
	typedef std::conditional_t<std::is_fundamental_v<Key> || std::is_pointer_v<Key> || std::is_enum_v<Key>,
			Key, const Key &> KeyRef;


public:
	// Access a key for modifying it:
	inline Value &operator[](KeyRef key);
	// Get the value of a key, or a default-constructed value if it does not exist:
	inline Value Get(KeyRef key) const;

	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<Key, Value>, Alloc>::empty;
	using std::vector<std::pair<Key, Value>, Alloc>::size;
	using std::vector<std::pair<Key, Value>, Alloc>::begin;
	using std::vector<std::pair<Key, Value>, Alloc>::end;
	using typename std::vector<std::pair<Key, Value>, Alloc>::iterator;
	using typename std::vector<std::pair<Key, Value>, Alloc>::const_iterator;


protected:
	// Look up a value in the vector.
	inline std::pair<size_t, bool> Search(KeyRef key) const;

	using std::vector<std::pair<Key, Value>, Alloc>::at;
	using std::vector<std::pair<Key, Value>, Alloc>::data;
	using std::vector<std::pair<Key, Value>, Alloc>::insert;
};



// Access a key for modifying it:
template <class Key, class Value, class ThreeWayComp, class Alloc>
Value &FlatMap<Key, Value, ThreeWayComp, Alloc>::operator[](KeyRef key)
{
	std::pair<size_t, bool> pos = Search(key);
	if(pos.second)
		return data()[pos.first].second;

	return insert(begin() + pos.first, std::make_pair(key, Value()))->second;
}



// Get the value of a key, or a default-constructed value if it does not exist:
template <class Key, class Value, class ThreeWayComp, class Alloc>
Value FlatMap<Key, Value, ThreeWayComp, Alloc>::Get(KeyRef key) const
{
	std::pair<size_t, bool> pos = Search(key);
	return (pos.second ? data()[pos.first].second : Value());
}



// Perform a binary search on a sorted vector. Return the key's location (or
// proper insertion spot) in the first element of the pair, and "true" in
// the second element if the key is already in the vector.
template <class Key, class Value, class ThreeWayComp, class Alloc>
std::pair<size_t, bool> FlatMap<Key, Value, ThreeWayComp, Alloc>::Search(KeyRef key) const
{
	// At each step of the search, we know the key is in [low, high).
	size_t low = 0;
	size_t high = size();

	while(low != high)
	{
		size_t mid = (low + high) / 2;
		const auto cmp = ThreeWayComp{}(key, at(mid).first);

		if(cmp < 0)
			high = mid;
		else if(cmp > 0)
			low = mid + 1;
		else
			return std::make_pair(mid, true);
	}
	return std::make_pair(low, false);
}
