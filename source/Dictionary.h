/* Dictionary.h
Copyright (c) 2017 by Michael Zahniser

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

#include "StringInterner.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>



// This class stores a mapping from character string keys to values, in a way
// that prioritizes fast lookup time at the expense of longer construction time
// compared to an STL map. That makes it suitable for ship attributes, which are
// changed much less frequently than they are queried.
template<class Type>
class Dictionary : private std::vector<std::pair<const char *, Type>> {
public:
	// Access a key for modifying it:
	Type &operator[](const char *key);
	Type &operator[](const std::string &key);
	// Get the value of a key, or 0 if it does not exist:
	Type Get(const char *key) const;
	Type Get(const std::string &key) const;
	// Erase the given element.
	void Erase(const char *key);
	void Clear();

	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<const char *, Type>>::empty;
	using std::vector<std::pair<const char *, Type>>::begin;
	using std::vector<std::pair<const char *, Type>>::end;


private:
	static std::pair<size_t, bool> Search(const char *key, const std::vector<std::pair<const char *, Type>> &v);
};



template<class Type>
Type &Dictionary<Type>::operator[](const char *key)
{
	std::pair<size_t, bool> pos = Search(key, *this);
	if(pos.second)
		return this->data()[pos.first].second;

	return this->insert(begin() + pos.first, std::make_pair(StringInterner::Intern(key), 0.))->second;
}



template<class Type>
Type &Dictionary<Type>::operator[](const std::string &key)
{
	return (*this)[key.c_str()];
}



template<class Type>
Type Dictionary<Type>::Get(const char *key) const
{
	std::pair<size_t, bool> pos = Search(key, *this);
	return (pos.second ? this->data()[pos.first].second : 0.);
}



template<class Type>
Type Dictionary<Type>::Get(const std::string &key) const
{
	return Get(key.c_str());
}



template<class Type>
void Dictionary<Type>::Erase(const char *key)
{
	auto [pos, exists] = Search(key, *this);
	if(exists)
		this->erase(next(this->begin(), pos));
}



template<class Type>
void Dictionary<Type>::Clear()
{
	this->clear();
}



// Perform a binary search on a sorted vector. Return the key's location (or
// proper insertion spot) in the first element of the pair, and "true" in
// the second element if the key is already in the vector.
template <class Type>
std::pair<size_t, bool> Dictionary<Type>::Search(const char *key, const std::vector<std::pair<const char *, Type>> &v)
{
	// At each step of the search, we know the key is in [low, high).
	size_t low = 0;
	size_t high = v.size();

	while(low != high)
	{
		size_t mid = (low + high) / 2;
		int cmp = strcmp(key, v[mid].first);
		if(!cmp)
			return std::make_pair(mid, true);

		if(cmp < 0)
			high = mid;
		else
			low = mid + 1;
	}
	return std::make_pair(low, false);
}
