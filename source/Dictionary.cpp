/* Dictionary.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Dictionary.h"

#include <cstring>
#include <mutex>
#include <set>
#include <string>

using namespace std;

namespace {
	// Perform a binary search on a sorted vector. Return the key's location (or
	// proper insertion spot) in the first element of the pair, and "true" in
	// the second element if the key is already in the vector.
	pair<size_t, bool> Search(const char *key, const vector<pair<const char *, double>> &v)
	{
		// At each step of the search, we know the key is in [low, high).
		size_t low = 0;
		size_t high = v.size();
		
		while(low != high)
		{
			size_t mid = (low + high) / 2;
			int cmp = strcmp(key, v[mid].first);
			if(!cmp)
				return make_pair(mid, true);
			
			if(cmp < 0)
				high = mid;
			else
				low = mid + 1;
		}
		return make_pair(low, false);
	}
	
	// String interning: return a pointer to a character string that matches the
	// given string but has static storage duration.
	const char *Intern(const char *key)
	{
		static set<string> interned;
		static mutex m;
		
		// Just in case this function is accessed from multiple threads:
		lock_guard<mutex> lock(m);
		return interned.insert(key).first->c_str();
	}
}



double &Dictionary::operator[](const char *key)
{
	pair<size_t, bool> pos = Search(key, *this);
	if(pos.second)
		return data()[pos.first].second;
	
	return insert(begin() + pos.first, make_pair(Intern(key), 0.))->second;
}



double &Dictionary::operator[](const string &key)
{
	return (*this)[key.c_str()];
}



double Dictionary::Get(const char *key) const
{
	pair<size_t, bool> pos = Search(key, *this);
	return (pos.second ? data()[pos.first].second : 0.);
}



double Dictionary::Get(const string &key) const
{
	return Get(key.c_str());
}
