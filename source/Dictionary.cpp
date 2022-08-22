/* Dictionary.cpp
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
#include <map>
#include <mutex>
#include <string>

using namespace std;

namespace {
	static map<string, dict> nameToKey;
	static vector<const char *> names;

	// Perform a binary search on a sorted vector. Return the key's location (or
	// proper insertion spot) in the first element of the pair, and "true" in
	// the second element if the key is already in the vector.
	pair<size_t, bool> Search(const char *key, const vector<dict> &v)
	{
		// At each step of the search, we know the key is in [low, high).
		size_t low = 0;
		size_t high = v.size();

		while(low != high)
		{
			size_t mid = (low + high) / 2;
			int cmp = strcmp(key, names[v[mid]]);
			if(!cmp)
				return make_pair(mid, true);

			if(cmp < 0)
				high = mid;
			else
				low = mid + 1;
		}
		return make_pair(low, false);
	}

	// Creates new if missing.
	static dict NameToKey(const string &name)
	{
		static mutex m;

		lock_guard<mutex> lock(m);
		map<string, dict>::iterator it = nameToKey.lower_bound(name);
		if(it == nameToKey.end() || nameToKey.key_comp()(name, it->first))
		{
			it = nameToKey.insert(it, pair<string, dict>(name, names.size()));
			names.push_back(it->first.c_str());
		}
		return it->second;
	}
}



double &Dictionary::operator[](const char *name)
{
	pair<size_t, bool> pos = Search(name, *this);
	if(pos.second)
		return store.data()[data()[pos.first]];

	dict key = NameToKey(name);
	Grow();

	insert(begin() + pos.first, key);
	return store[key];
}



double &Dictionary::operator[](const string &name)
{
	return (*this)[name.c_str()];
}



void Dictionary::Update(const char *name, const double &value)
{
	pair<size_t, bool> pos = Search(name, *this);
	if(pos.second)
		store.data()[pos.first] = value;
}



void Dictionary::Update(const string &name, const double &value)
{
	return Update(name.c_str(), value);
}



double Dictionary::Get(const string &name) const
{
	pair<size_t, bool> pos = Search(name.c_str(), *this);
	return pos.second ? store.data()[data()[pos.first]] : 0.;
}



void Dictionary::Grow()
{
	if(store.size() <= names.size())
		store.resize(names.size()+1);
}



void Dictionary::UseKey(const dict &key)
{
	pair<size_t, bool> pos = Search(names[key], *this);
	if(!pos.second)
		insert(begin() + pos.first, key);
}



const char *Dictionary::GetName(const dict &key)
{
	return names.data()[key];
}



// Inserts the name if not found.
const dict Dictionary::GetKey(const string &name)
{
	return NameToKey(name);
}
