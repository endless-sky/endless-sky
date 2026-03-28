/* Bitset.h
Copyright (c) 2021 by quyykk

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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>



// Class representing a bitset with a dynamic size.
class Bitset {
public:
	// Returns the number of bits this bitset can hold.
	size_t Size() const noexcept;
	// Returns the number of bits this bitset has reserved.
	size_t Capacity() const noexcept;

	// Resizes the bitset to hold at least the specific amount of bits.
	void Resize(size_t size);
	// Clears the bitset. After this call this bitset is empty.
	void Clear() noexcept;

	// Whether the given bitset has any bits that are also set in this bitset.
	bool Intersects(const Bitset &other) const noexcept;
	// Returns the value of the bit at the specified index.
	bool Test(size_t index) const noexcept;
	// Sets the bit at the specified index.
	void Set(size_t index) noexcept;
	// Resets all bits in the bitset.
	void Reset() noexcept;
	// Whether any bits are set.
	bool Any() const noexcept;
	// Whether no bits are set.
	bool None() const noexcept;

	// Fills the current bitset with the bits of other.
	void UpdateWith(const Bitset &other);


private:
	static constexpr size_t BITS_PER_BLOCK = std::numeric_limits<uint64_t>::digits;

	// Stores the bits of the bitset.
	std::vector<uint64_t> bits;
};
