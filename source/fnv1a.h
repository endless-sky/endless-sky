/* fnv1a.h
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

#ifndef FNV1A_HASHING_H_
#define FNV1A_HASHING_H_

#include <cstdint>

// Code initially grabbed from:
// https://gist.github.com/filsinger/1255697/1972eb676b47116838edaacf923e60b9173199c2
namespace hash_fnv1a
{
	using def_type = uint32_t;

	template <typename S> struct fnv1a;

	template <> struct fnv1a<def_type>
	{
		constexpr static def_type default_offset_basis = 0x811C9DC5;
		constexpr static def_type prime = 0x01000193;

		constexpr static inline def_type hash(char const *const aString, const def_type val = default_offset_basis)
		{
			return (aString[0] == '\0') ? val : hash(&aString[1], (val ^ def_type(aString[0])) * prime);
		}
	};
} // namespace hash_fnv1a



// A tiny wrapper to prevent risks of type mismatch
// when passing an hash to 'Dictionary::Get' method
class HashWrapper {
public:
	constexpr explicit HashWrapper(hash_fnv1a::def_type h)
		: hash(h)
	{}
	constexpr hash_fnv1a::def_type Get() const { return hash; }
private:
	hash_fnv1a::def_type hash;
};


inline constexpr HashWrapper operator "" _fnv1a (const char *aString, std::size_t aStrlen)
{
	return HashWrapper(hash_fnv1a::fnv1a<hash_fnv1a::def_type>::hash(aString));
}


#endif
