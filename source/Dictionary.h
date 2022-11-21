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

#include <string>
#include <utility>
#include <vector>

// Code from:
// https://gist.github.com/filsinger/1255697/1972eb676b47116838edaacf923e60b9173199c2
namespace hash_fnv
{
	using def_type = uint32_t;

	template <typename S> struct fnv_internal;
	template <typename S> struct fnv1;
	template <typename S> struct fnv1a;

	template <> struct fnv_internal<def_type>
	{
		constexpr static def_type default_offset_basis = 0x811C9DC5;
		constexpr static def_type prime				   = 0x01000193;
	};

	template <> struct fnv1<def_type> : public fnv_internal<def_type>
	{
		constexpr static inline def_type hash(char const*const aString, const def_type val = default_offset_basis)
		{
			return (aString[0] == '\0') ? val : hash( &aString[1], ( val * prime ) ^ def_type(aString[0]) );
		}
	};

	template <> struct fnv1a<def_type> : public fnv_internal<def_type>
	{
		constexpr static inline def_type hash(char const*const aString, const def_type val = default_offset_basis)
		{
			return (aString[0] == '\0') ? val : hash( &aString[1], ( val ^ def_type(aString[0]) ) * prime);
		}
	};
} // namespace hash

class HashWrapper {
public:
	constexpr explicit HashWrapper(hash_fnv::def_type h) : hash(h) {}
	constexpr hash_fnv::def_type Get() const { return hash; }
private:
	hash_fnv::def_type hash;
};

inline constexpr HashWrapper operator "" _fnv1a (const char* aString, size_t aStrlen)
{
	return HashWrapper(hash_fnv::fnv1a<hash_fnv::def_type>::hash(aString));
}


class stringAndHash {
public:
	stringAndHash(const char *str)
		: str(str)
		, hash(hash_fnv::fnv1a<hash_fnv::def_type>::hash(str))
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
	double Get(const char *key) const;
	double Get(const std::string &key) const;

	// Expose certain functions from the underlying vector:
	using std::vector<std::pair<stringAndHash, double>>::empty;
	using std::vector<std::pair<stringAndHash, double>>::begin;
	using std::vector<std::pair<stringAndHash, double>>::end;
};



#endif
