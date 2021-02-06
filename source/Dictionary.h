/* Dictionary.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <cstring>
#include <string>
#include <utility>
#include <vector>



// Needs to be a separate class if we want to share interning.
// This unfortunately makes the interning function more publicly visible.
class DictionaryGeneric{
	public:
		static const char *Intern(const char *key);
};



// This class stores a mapping from character string keys to values, in a way
// that prioritizes fast lookup time at the expense of longer construction time
// compared to an STL map. That makes it suitable for ship attributes, which are
// changed much less frequently than they are queried.
template <typename T>
class Dictionary : private std::vector<std::pair<const char *, T>> {
public:
	// Access a key for modifying it:
	T &operator[](const char *key);
	T &operator[](const std::string &key);
	// Get the value of a key, or 0 if it does not exist:
	T Get(const char *key) const;
	T Get(const std::string &key) const;
	
	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<const char *, T>>::empty;
	using std::vector<std::pair<const char *, T>>::begin;
	using std::vector<std::pair<const char *, T>>::end;
	
	
private:
	static std::pair<size_t, bool> Search(const char *key, const std::vector<std::pair<const char *, T>> &v);

};



template <typename T>
T& Dictionary<T>::operator[](const char *key)
{
	std::pair<size_t, bool> pos = Search(key, *this);
	if(pos.second)
		return std::vector<std::pair<const char *, T>>::data()[pos.first].second;
	
	return this->insert(begin() + pos.first, std::make_pair(DictionaryGeneric::Intern(key), T()))->second;
}



template <typename T>
T &Dictionary<T>::operator[](const std::string &key)
{
	return (*this)[key.c_str()];
}



template <typename T>
T Dictionary<T>::Get(const char *key) const
{
	std::pair<size_t, bool> pos = Search(key, *this);
	return (pos.second ? std::vector<std::pair<const char *, T>>::data()[pos.first].second : T());
}



template <typename T>
T Dictionary<T>::Get(const std::string &key) const
{
	return Get(key.c_str());
}



// Perform a binary search on a sorted vector. Return the key's location (or
// proper insertion spot) in the first element of the pair, and "true" in
// the second element if the key is already in the vector.
template <typename T>
std::pair<size_t, bool> Dictionary<T>::Search(const char *key, const std::vector<std::pair<const char *, T>> &v)
{
	// At each step of the search, we know the key is in [low, high).
	size_t low = 0;
	size_t high = v.size();
	
	while(low != high)
	{
		size_t mid = (low + high) / 2;
		int cmp = std::strcmp(key, v[mid].first);
		if(!cmp)
			return std::make_pair(mid, true);
		
		if(cmp < 0)
			high = mid;
		else
			low = mid + 1;
	}
	return std::make_pair(low, false);
}



#endif
