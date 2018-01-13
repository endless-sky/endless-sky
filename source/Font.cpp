/* Font.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Font.h"

#include "AtlasGlyphs.h"
#include "Files.h"
#include "FreeTypeGlyphs.h"
#include "Point.h"

#include <algorithm>

using namespace std;

namespace {
	bool showUnderlines = false;
	
	// Determines the number of bytes used by the unicode code point in utf8.
	int CodePointBytes(const char *str)
	{
		// end - 00000000
		if(!str || !*str)
			return 0;
		
		// 1 byte - 0xxxxxxx
		if((*str & 0x80) == 0)
			return 1;
		
		// invalid - 10?????? or 11?????? invalid
		if((*str & 0x40) == 0 || (*(str + 1) & 0xc0) != 0x80)
			return -1;
		
		// 2 bytes - 110xxxxx 10xxxxxx
		if((*str & 0x20) == 0)
			return 2;
		
		// invalid - 111????? 10?????? invalid
		if((*(str + 2) & 0xc0) != 0x80)
			return -1;
		
		// 3 bytes - 1110xxxx 10xxxxxx 10xxxxxx
		if((*str & 0x10) == 0)
			return 3;
		
		// invalid - 1111???? 10?????? 10?????? invalid
		if((*(str + 3) & 0xc0) != 0x80)
			return -1;
		
		// 4 bytes - 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		if((*str & 0x8) == 0)
			return 4;
		
		// not unicode - 11111??? 10?????? 10?????? 10??????
		return -1;
	}
}



Font::Font()
{
}



bool Font::Load(const string &path, int size)
{
	// Ignore if already loaded.
	if(source)
		return false;
	
	string extension = Files::Extension(path);
	bool isPNG = (extension.compare(".png") == 0 || extension.compare(".PNG") == 0);
	bool isJPG = (extension.compare(".jpg") == 0 || extension.compare(".JPG") == 0);
	if(isPNG || isJPG)
	{
		auto glyphs = make_shared<AtlasGlyphs>();
		if(glyphs->Load(path))
		{
			glyphs->SetUpShader();
			source = glyphs;
			return true;
		}
	}
	else
	{
		auto glyphs = make_shared<FreeTypeGlyphs>();
		if(glyphs->Load(path, size))
		{
			glyphs->SetUpShader();
			source = glyphs;
			return true;
		}
	}
	return false;
}



void Font::Draw(const string &str, const Point &point, const Color &color) const
{
	DrawAliased(str, round(point.X()), round(point.Y()), color);
}



void Font::DrawAliased(const string &str, double x, double y, const Color &color) const
{
	if(!source || str.empty())
		return;
	
	string buf = ReplaceCharacters(str);
	source->Draw(buf, x, y, color);
}



int Font::Width(const string &str) const
{
	if(!source)
		return 0;
	
	string buf = ReplaceCharacters(str);
	return source->Width(buf);
}



string Font::Truncate(const string &str, int width, bool ellipsis) const
{
	int prevWidth = Width(str);
	if(prevWidth <= width)
		return str;
	
	if(ellipsis)
		width -= Width("...");
	
	// Find the last index that fits the width. [good,bad[
	size_t len = str.length();
	size_t prev = len;
	size_t good = 0;
	size_t bad = len;
	size_t tries = len + 1;
	while(NextCodePoint(str, good) < bad && --tries)
	{
		// Interpolate next index from the width of the previous index.
		size_t next = CodePointStart(str, (prev * width) / prevWidth);
		if(next <= good)
			next = NextCodePoint(str, good);
		else if(next >= bad)
			next = CodePointStart(str, bad - 1);
		
		int nextWidth = Width(str.substr(0, next));
		if(nextWidth <= width)
			good = next;
		else
			bad = next;
		prev = next;
		prevWidth = nextWidth;
	}
	return str.substr(0, good) + (ellipsis ? "..." : "");
}



string Font::TruncateFront(const string &str, int width, bool ellipsis) const
{
	int prevWidth = Width(str);
	if(prevWidth <= width)
		return str;
	
	if(ellipsis)
		width -= Width("...");
	
	// Find the first index that fits the width. ]bad,good]
	size_t len = str.length();
	size_t prev = 0;
	size_t bad = 0;
	size_t good = len;
	int tries = len + 1;
	while(NextCodePoint(str, bad) < good && --tries)
	{
		// Interpolate next index from the width of the previous index.
		size_t next = CodePointStart(str, len - ((len - prev) * width) / prevWidth);
		if(next <= bad)
			next = NextCodePoint(str, bad);
		else if(next >= good)
			next = CodePointStart(str, good - 1);
		
		int nextWidth = Width(str.substr(next));
		if(nextWidth <= width)
			good = next;
		else
			bad = next;
		prev = next;
		prevWidth = nextWidth;
	}
	return (ellipsis ? "..." : "") + str.substr(good);
}



string Font::TruncateMiddle(const string &str, int width, bool ellipsis) const
{
	if(Width(str) <= width)
		return str;
	
	if(ellipsis)
		width -= Width("...");
	
	string right = TruncateFront(str, width / 2, false);
	width -= Width(right);
	string left = Truncate(str, width, false);
	return left + (ellipsis ? "..." : "") + right;
}



int Font::Height() const
{
	if(!source)
		return 0;
	
	return source->LineHeight();
}



int Font::Space() const
{
	if(!source)
		return 0;
	
	return round(source->Space());
}



void Font::ShowUnderlines(bool show)
{
	showUnderlines = show;
}



bool Font::ShowUnderlines()
{
	return showUnderlines;
}



// Replaces straight quotation marks with curly ones.
string Font::ReplaceCharacters(const string &str)
{
	string buf;
	buf.reserve(str.length());
	bool isAfterWhitespace = true;
	for(size_t pos = 0; pos < str.length(); ++pos)
	{
		// U+2018 LEFT_SINGLE_QUOTATION_MARK
		// U+2019 RIGHT_SINGLE_QUOTATION_MARK
		// U+201C LEFT_DOUBLE_QUOTATION_MARK
		// U+201D RIGHT_DOUBLE_QUOTATION_MARK
		if(str[pos] == '\'')
			buf.append(isAfterWhitespace ? "\xE2\x80\x98" : "\xE2\x80\x99");
		else if(str[pos] == '"')
			buf.append(isAfterWhitespace ? "\xE2\x80\x9C" : "\xE2\x80\x9D");
		else
			buf.append(1, str[pos]);
		isAfterWhitespace = (str[pos] == ' ');
	}
	return buf;
}



// Convert between utf8 and utf32.
// Invalid code points are converted to 0xFFFFFFFF in utf32 and 0xFF in utf8.
u32string Font::Convert(const string &str)
{
	u32string buf;
	for(size_t pos = 0; pos < str.length(); pos = NextCodePoint(str, pos))
		buf.append(1, DecodeCodePoint(str, pos));
	return buf;
}



string Font::Convert(const u32string &str)
{
	string buf;
	for(char32_t c : str)
	{
		if(c < 0x80)
			buf.append(1, c);
		else if(c < 0x800)
		{
			buf.append(1, 0xC0 | (c >> 6));
			buf.append(1, 0x80 | (c & 0x3f));
		}
		else if(c < 0x10000)
		{
			buf.append(1, 0xE0 | (c >> 12));
			buf.append(1, 0x80 | ((c >> 6) & 0x3f));
			buf.append(1, 0x80 | (c & 0x3f));
		}
		else if(c < 0x110000)
		{
			buf.append(1, 0xF0 | (c >> 18));
			buf.append(1, 0x80 | ((c >> 12) & 0x3f));
			buf.append(1, 0x80 | ((c >> 6) & 0x3f));
			buf.append(1, 0x80 | (c & 0x3f));
		}
		else
			buf.append(1, 0xFF); // not unicode
	}
	return buf;
}



// Skip to the next unicode code point after pos in utf8.
// Return the string length when there are no more code points.
size_t Font::NextCodePoint(const string &str, size_t pos)
{
	if(pos >= str.length())
		return str.length();
	
	for(++pos; pos < str.length(); ++pos)
		if((str[pos] & 0x80) == 0 || (str[pos] & 0xc0) == 0xc0)
			break;
	return pos;
}



// Returns the start of the unicode code point at pos in utf8.
size_t Font::CodePointStart(const string &str, size_t pos)
{
	// 0xxxxxxx and 11?????? start a code point
	while(pos > 0 && (str[pos] & 0x80) != 0x00 && (str[pos] & 0xc0) != 0xc0)
		--pos;
	return pos;
}



// Decodes a unicode code point in utf8.
// Invalid codepoints are converted to 0xFFFFFFFF.
char32_t Font::DecodeCodePoint(const string &str, size_t pos)
{
	if(pos >= str.length())
		return 0;
	
	// invalid (-1) or end (0)
	int bytes = CodePointBytes(str.c_str() + pos);
	if(bytes < 1)
		return bytes;
	
	// 1 byte
	if(bytes == 1)
		return (str[pos] & 0x7f);
	
	// 2-4 bytes
	char32_t c = (str[pos] & ((1 << (7 - bytes)) - 1));
	for(int i = 1; i < bytes; ++i)
		c = (c << 6) + (str[pos + i] & 0x3f);
	return c;
}



Font::IGlyphs::~IGlyphs()
{
}
