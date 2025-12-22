/* Dictionary.cpp
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

#include "Dictionary.h"

#include "StringInterner.h"

#include <cstring>

using namespace std;

namespace {
	// Perform a binary search on a sorted vector. Return the key's location (or
	// proper insertion spot) in the first element of the pair, and "true" in
	// the second element if the key is already in the vector.
	template<class Type>
	pair<size_t, bool> Search(const char *key, const vector<pair<const char *, Type>> &v)
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
}



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



template class Dictionary<int>;
template class Dictionary<int64_t>;
template class Dictionary<double>;
