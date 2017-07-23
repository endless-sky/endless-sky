/* SHA1.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SHA1.h"

#include <algorithm>
#include <array>
#include <string.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// Byte-swaps an integer. Only used on little-endian machines.
static inline uint32_t ByteSwap(uint32_t x)
{
	uint32_t y = (x >> 16) | ((x & 0xFFFF) << 16);
	return ((y >> 8) & 0xFF00FF) | ((y & 0xFF00FF) << 8);
}
#endif

// Left-rotates an integer in a bitwise sense.
static inline uint32_t LeftRotate(uint32_t value, int shift)
{
	return (value << shift) | (value >> (32-shift));
}

// Shuffles and rotates the words in the hash.
static void Shuffle(uint32_t temp[5], uint32_t add);



// Add a c-style string to the hash.
void SHA1::Add(const char *pStr)
{
	Add(pStr, strlen(pStr)*sizeof(char));
}



// Add a byte to the hash.
void SHA1::Add(uint8_t byteValue)
{
	block[blockIndex] = byteValue;
	if(++blockIndex == sizeof(block))
	{
		ProcessBlock();
		blockIndex = 0;
	}
	
	totalByteCount++;
}



// Add a sequence of bytes to the hash.
void SHA1::Add(const void *pData, size_t length)
{
	while(length > 0)
	{
		size_t toCopy = std::min(length, sizeof(block) - blockIndex);
		memcpy(block+blockIndex, pData, toCopy);
		blockIndex += toCopy;
		if(blockIndex == sizeof(block))
		{
			ProcessBlock();
			blockIndex = 0;
		}
		
		totalByteCount += toCopy;
		pData   = reinterpret_cast<const uint8_t*>(pData) + toCopy;
		length -= toCopy;
	}
}



// Returns the hash as a 20-byte vector.
std::vector<uint8_t> SHA1::GetHash() const
{
	SHA1 local = *this; // copy the hash so we don't change it
	uint64_t bitLength = (uint64_t)local.totalByteCount * 8; // grab the length in bits before it changes
	local.Add(0x80); // start by adding a 1 bit to the message
	
	// then pad the block with zeros until there's just enough space to store the length
	size_t spaceLeft = sizeof(block) - local.blockIndex;
	if(spaceLeft > sizeof(uint64_t)) // if there's space beyond that needed to store the length...
	{
		uint8_t zeros[spaceLeft-sizeof(uint64_t)]; // add zeros until we have exactly enough space
		memset(zeros, 0, spaceLeft-sizeof(uint64_t));
		local.Add(zeros, spaceLeft-sizeof(uint64_t));
	}
	else if(spaceLeft < 8) // otherwise if there's no space to store the length...
	{
		uint8_t zeros[sizeof(block)-sizeof(uint64_t)];
		memset(zeros, 0, sizeof(zeros));
		local.Add(zeros, spaceLeft); // then finish the block
		local.Add(zeros, sizeof(block)-sizeof(uint64_t)); // and add zeros until we have the right length
	}

	// now add the length to the hash. we add the bytes in big-endian order
	for(int shift=(sizeof(uint64_t)-1)*8; shift >= 0; shift -= 8)
		local.Add(static_cast<uint8_t>(bitLength >> shift));
	
	// convert the hash to a vector<uint8>
	std::vector<uint8_t> v(sizeof(hash)); // allocate a vector of the right size
	// copy the data into the vector, byte-swapping it along the way if necessary
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint32_t *pVector = reinterpret_cast<uint32_t*>(v.data());
	for(size_t i=0; i < sizeof(hash)/sizeof(hash[0]); i++)
		pVector[i] = ByteSwap(local.hash[i]);
	#else
	memcpy(v.data(), local.hash, sizeof(hash));
	#endif
	
	return v; // and return the final vector
}



// Returns the hash a hexadecimal string.
std::string SHA1::GetHashString() const
{
	const char *alphabet = "0123456789abcdef";
	std::vector<uint8_t> hash = GetHash();
	char hashStr[hash.size()*2];
	for (size_t i=0; i<hash.size(); i++)
	{
		uint8_t value = hash[i];
		hashStr[i*2]   = alphabet[value >> 4];
		hashStr[i*2+1] = alphabet[value & 15];
	}
	
	return std::string(hashStr, sizeof(hashStr));
}



// Processes a complete block, adding it to the hash.
void SHA1::ProcessBlock()
{
	// expand the 64-byte block into 80 words
	uint32_t expanded[80];
	for(int i=0; i<16; i++)
		expanded[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
	for(int i=16; i<80; i++)
		expanded[i] = LeftRotate(expanded[i-3] ^ expanded[i-8] ^ expanded[i-14] ^ expanded[i-16], 1);
	
	// now mix the expanded block into a copy of the hash
	uint32_t temp[5];
	memcpy(temp, hash, sizeof(temp));
	for(int i=0; i<20; i++)
		Shuffle(temp, expanded[i] + 0x5A827999 + (((temp[2] ^ temp[3]) & temp[1]) ^ temp[3]));
	for(int i=20; i<40; i++)
		Shuffle(temp, expanded[i] + 0x6ED9EBA1 + (temp[1] ^ temp[2] ^ temp[3]));
	for(int i=40; i<60; i++)
		Shuffle(temp, expanded[i] + 0x8F1BBCDC + ((temp[1] & temp[2]) | ((temp[1] | temp[2]) & temp[3])));
	for(int i=60; i<80; i++)
		Shuffle(temp, expanded[i] + 0xCA62C1D6 + (temp[1] ^ temp[2] ^ temp[3]));
	
	// and add the mixed-up copy back into the original hash
	for(int i=0; i<5; i++)
		hash[i] += temp[i];
}



// Removes all data from the hash, resetting it.
void SHA1::Reset()
{
	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;
	hash[4] = 0xC3D2E1F0;
	totalByteCount = 0;
	blockIndex     = 0;
}



// Shuffles and rotates the words in the hash.
static void Shuffle(uint32_t temp[5], uint32_t add)
{
	uint32_t t = LeftRotate(temp[0], 5) + temp[4] + add;
	temp[4] = temp[3];
	temp[3] = temp[2];
	temp[2] = LeftRotate(temp[1], 30);
	temp[1] = temp[0];
	temp[0] = t;
}
