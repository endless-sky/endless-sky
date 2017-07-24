/* SHA1.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHA1_H_
#define SHA1_H_

#include <stdint.h>
#include <string>
#include <vector>

class SHA1 {
public:
	// Initializes an empty hash.
	inline SHA1() { Reset(); }
	// Hashes additional binary or string data.
	void Add(uint8_t byteValue);
	void Add(const void *pData, size_t length);
	void Add(const char *pStr);
	inline void Add(const std::string &str) { Add(str.c_str(), str.length()*sizeof(char)); }
	// Returns the hash as a vector of 20 bytes.
	std::vector<uint8_t> GetHash() const;
	// Returns the hash a lowercase hexadecimal string.
	std::string GetHashString() const;
	// Resets the hash as if it was newly constructed (i.e. no data added).
	void Reset();
	
	
private:
	void ProcessBlock();
	uint32_t hash[5];
	uint8_t block[64];
	size_t totalByteCount;
	int blockIndex;
};

#endif // SHA1_H_
