/* Bitset.h
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef BITSET_H_
#define BITSET_H_

#include <limits>
#include <cstddef>
#include <cstdint>
#include <vector>



// Class representing a bitset with a dynamic size. Used
// for turret control.
class Bitset {
public:
	static constexpr size_t BITS_PER_BLOCK = std::numeric_limits<uint64_t>::digits;


public:
	// Returns the number of bits this bitset can hold.
	size_t size() const noexcept;

	// Resizes the bitset to hold at least the specific amount of bits.
	void resize(size_t size);
	// Clears the bitset. After this call this bitset is empty.
	void clear() noexcept;

	// Whether the given bitset has any bits that are also set in this bitset.
	bool intersects(const Bitset &other) const noexcept;
	// Returns the value of the bit at the specified index.
	bool test(size_t index) const noexcept;
	// Sets the bit at the specified index.
	void set(size_t index) noexcept;
	// Whether any bits are set.
	bool any() const noexcept;
	// Whether no bits are set.
	bool none() const noexcept;


private:
	// Stores the bits of the bitset.
	std::vector<uint64_t> bits;
};



#endif
