/* Hasher.h
Copyright (c) 2025 by Amazinite

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

#include <functional>



// A helper class for generating hashes.
class Hasher {
public:
	template<class T>
	/// Combine a current hash value with the hash value of the given object.
	/// @param seed The current hash value. Will be updated by this function.
	/// @param v The object to add to the hash. Must be compatible with std::hash.
	static void Hash(std::size_t &seed, const T &v);
};



template<class T>
void Hasher::Hash(std::size_t &seed, const T &v)
{
	// https://stackoverflow.com/questions/6899392/generic-hash-function-for-all-stl-containers
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
