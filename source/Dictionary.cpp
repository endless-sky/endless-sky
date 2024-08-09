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
#include <mutex>
#include <set>
#include <string>

using namespace std;


double &Dictionary::operator[](const char *key)
{
	pair<size_t, bool> pos = Search(key);
	if(pos.second)
		return data()[pos.first].second;

	return insert(begin() + pos.first, make_pair(StringInterner::Intern(key), 0.))->second;
}



double &Dictionary::operator[](const string &key)
{
	return (*this)[key.c_str()];
}



double Dictionary::Get(const char *key) const
{
	pair<size_t, bool> pos = Search(key);
	return (pos.second ? data()[pos.first].second : 0.);
}



double Dictionary::Get(const string &key) const
{
	return Get(key.c_str());
}
