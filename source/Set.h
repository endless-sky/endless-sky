/* Set.h
Copyright (c) 2014 by Michael Zahniser

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

#include <map>
#include <string>



// Template representing a set of named objects of a given type, where you can
// query it for a pointer to any object and it will return one, whether or not that
// object has been loaded yet. (This allows cyclic pointers.)
template<class Type>
class Set {
public:
	// Allow non-const access to the owner of this set; it can hand off only
	// const references to avoid anyone else modifying the objects.
	Type *Get(const std::string &name) { return &data[name]; }
	const Type *Get(const std::string &name) const { return &data[name]; }
	// If an item already exists in this set, get it. Otherwise, return a null
	// pointer rather than creating the item.
	const Type *Find(const std::string &name) const;

	bool Has(const std::string &name) const { return data.contains(name); }

	typename std::map<std::string, Type>::iterator begin() { return data.begin(); }
	typename std::map<std::string, Type>::const_iterator begin() const { return data.begin(); }
	typename std::map<std::string, Type>::const_iterator find(const std::string &key) const { return data.find(key); }
	typename std::map<std::string, Type>::iterator end() { return data.end(); }
	typename std::map<std::string, Type>::const_iterator end() const { return data.end(); }

	int size() const { return data.size(); }
	bool empty() const { return data.empty(); }
	// Remove any objects in this set that are not in the given set, and for
	// those that are in the given set, revert to their contents.
	void Revert(const Set<Type> &other);


private:
	mutable std::map<std::string, Type> data;
};



template<class Type>
const Type *Set<Type>::Find(const std::string &name) const
{
	auto it = data.find(name);
	return (it == data.end() ? nullptr : &it->second);
}



template<class Type>
void Set<Type>::Revert(const Set<Type> &other)
{
	auto it = data.begin();
	auto oit = other.data.begin();

	while(it != data.end())
	{
		if(oit == other.data.end() || it->first < oit->first)
			it = data.erase(it);
		else if(it->first == oit->first)
		{
			// If this is an entry that is in the set we are reverting to, copy
			// the state we are reverting to.
			it->second = oit->second;
			++it;
			++oit;
		}

		// There should never be a case when an entry in the set we are
		// reverting to has a name that is not also in this set.
	}
}
