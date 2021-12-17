/* Bitset.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Bitset.h"



using namespace std;



// Returns the number of bits this bitset can hold.
size_t Bitset::size() const noexcept
{
	return bits.size() * BITS_PER_BLOCK;
}



// Resizes the bitset to hold at least the specific amount of bits.
void Bitset::resize(size_t size)
{
	bits.resize(size / BITS_PER_BLOCK + 1);
}



// Clears the bitset. After this call this bitset is empty.
void Bitset::clear() noexcept
{
	bits.clear();
}



// Whether the given bitset has any bits that are also set in this bitset.
bool Bitset::intersects(const Bitset &other) const noexcept
{
	const auto size = bits.size() < other.bits.size() ? bits.size() : other.bits.size();
	for(size_t i = 0; i < size; ++i)
		if(bits[i] & other.bits[i])
			return true;
	return false;
}



// Returns the value of the bit at the specified index.
bool Bitset::test(size_t index) const noexcept
{
	const auto blockIndex = index / BITS_PER_BLOCK;
	const auto pos = index % BITS_PER_BLOCK;
	return bits[blockIndex] & (uint64_t(1) << pos);
}



// Sets the bit at the specified index.
void Bitset::set(size_t index) noexcept
{
	const auto blockIndex = index / BITS_PER_BLOCK;
	const auto pos = index % BITS_PER_BLOCK;
	bits[blockIndex] |= (uint64_t(1) << pos);
}



// Whether any bits are set.
bool Bitset::any() const noexcept
{
	for(uint64_t block : bits)
		if(block)
			return true;
	return false;
}



// Whether no bits are set.
bool Bitset::none() const noexcept
{
	return !any();
}
