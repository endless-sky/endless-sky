/* Set.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SET_H_
#define SET_H_

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
	
	bool Has(const std::string &name) const { return data.count(name); }
	
	typename std::map<std::string, Type>::iterator begin() { return data.begin(); }
	typename std::map<std::string, Type>::const_iterator begin() const { return data.begin(); }
	typename std::map<std::string, Type>::iterator end() { return data.end(); }
	typename std::map<std::string, Type>::const_iterator end() const { return data.end(); }
	
	int size() const { return data.size(); }
	
	
private:
	mutable std::map<std::string, Type> data;
};



#endif
