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

#include <string>
#include <utility>
#include <vector>



// This class stores a mapping from character string keys to values, in a way
// that prioritizes fast lookup time at the expense of longer construction time
// compared to an STL map. That makes it suitable for ship attributes, which are
// changed much less frequently than they are queried.
class Dictionary : private std::vector<std::pair<const char *, double>> {
public:
	// Access a key for modifying it:
	double &operator[](const char *key);
	double &operator[](const std::string &key);
	// Get the value of a key, or 0 if it does not exist:
	double Get(const char *key) const;
	double Get(const std::string &key) const;
	// Erase the given element.
	void Erase(const char *key);

	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<const char *, double>>::empty;
	using std::vector<std::pair<const char *, double>>::begin;
	using std::vector<std::pair<const char *, double>>::end;
};
