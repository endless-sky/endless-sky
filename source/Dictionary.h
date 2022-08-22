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

#include <string>
#include <utility>
#include <vector>



typedef std::vector<double>::size_type dict;

// This class stores a mapping from character string keys to values, in a way
// that prioritizes fast lookup time at the expense of longer construction time
// compared to an STL map. That makes it suitable for ship attributes, which are
// changed much less frequently than they are queried. For critical paths, it
// offers direct access to the attribute values stored in a vector.
class Dictionary : private std::vector<dict> {
public:
	// Access a key for modifying it:
	double &operator[](const char *name);
	double &operator[](const std::string &name);
	inline double &operator[](const dict &key)
	{
		return store.data()[key];
	}

	// Update a key if it exists. Faster if you know it to be present.
	void Update(const char *name, const double &value);
	void Update(const std::string &name, const double &value);
	inline void Update(const dict &key, const double &value)
	{
		store.data()[key] = value;
	}


	// Get the value of a key, or 0 if it does not exist:
	//double Get(const char *name) const;
	double Get(const std::string &name) const;
	inline double Get(const dict &key) const
	{
		return store.data()[key];
	}

	// Make the vector large enough to fit every attribute.
	void Grow();

	// Mark key as used for this dictionary.
	void UseKey(const dict &key);

	// Get the key id matching to a attribute name, or vice versa.
	static const char *GetName(const dict &key);
	static const dict GetKey(const std::string &name);

	// Expose certain functions from the underlying vector:
	using std::vector<dict>::empty;
	using std::vector<dict>::begin;
	using std::vector<dict>::end;


private:
	// The actual data stored. For every instance, the same position in the
	// vector holds the same kind of value.
	std::vector<double> store;
};



#endif
