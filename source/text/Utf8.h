/* Utf8.h
Copyright (c) 2017, 2018 by Flavio J. Saraiva

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_UTF8_H_
#define ES_TEXT_UTF8_H_

#include <cstddef>
#include <string>

namespace Utf8 {
	// Skip to the next unicode code point after pos in utf8.
	// Return string::npos when there are no more code points.
	std::size_t NextCodePoint(const std::string &str, std::size_t pos);
	
	// Returns the start of the unicode code point at pos in utf8.
	std::size_t CodePointStart(const std::string &str, std::size_t pos);
	
	// Decodes a unicode code point in utf8.
	// Invalid codepoints are converted to 0xFFFFFFFF.
	// pos skips to the next unicode code point after pos in utf8,
	// or is set string::npos when there are no more code points.
	char32_t DecodeCodePoint(const std::string &str, std::size_t &pos);
}

#endif
