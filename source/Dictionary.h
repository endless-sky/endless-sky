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

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include "fnv1a.h"

#include <set>
#include <string>
#include <utility>
#include <vector>



// A(nother) tiny wrapper which is used as a key in the Dictionary class:
// this allow using a fast hash for lookup without loosing the possibility
// of getting the key as a string
class stringAndHash {
public:
	stringAndHash(const char *str)
		: str(str)
		, hash(hash_fnv1a::fnv1a<hash_fnv1a::def_type>::hash(str))
	{}
	stringAndHash(HashWrapper h)
		: str(nullptr)
		, hash(std::move(h))
	{}

	const char *GetString() const { return str; }
	const HashWrapper GetHash() const { return hash; }


private:
	const char *str;
	HashWrapper hash;
};



// This class stores a mapping from character string keys to values, in a way
// that prioritizes fast lookup time at the expense of longer construction time
// compared to an STL map. That makes it suitable for ship attributes, which are
// changed much less frequently than they are queried.
class Dictionary : private std::vector<std::pair<stringAndHash, double>> {
public:
	// Access a key for modifying it:
	double &operator[](const char *key);
	double &operator[](const std::string &key);
	// Get the value of a key, or 0 if it does not exist:
	double Get(HashWrapper hash_wr) const;
	double Get(const char *key) const;
	double Get(const std::string &key) const;

	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<stringAndHash, double>>::empty;
	using std::vector<std::pair<stringAndHash, double>>::begin;
	using std::vector<std::pair<stringAndHash, double>>::end;
};



// This class find hash collisions between keys in handed dictionaries.
// If a collision is found 'AddKeysWhileChecking' throw a 'runtime_error'.
class DictionaryCollisionChecker {
public:
	DictionaryCollisionChecker() = default;

	// Add the keys from the given Dictionary and
	// ensure there aren't collisions, or throw
	void AddKeysWhileChecking(const Dictionary &dict);


private:
	class hash_comparator {
	public:
		bool operator()(const stringAndHash &a, const stringAndHash &b)
		{
			return a.GetHash().Get() < b.GetHash().Get();
		}
	};

	std::set<stringAndHash, hash_comparator> collected_keys;
};

#endif
