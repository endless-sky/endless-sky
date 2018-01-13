/* Font.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FONT_H_
#define FONT_H_

#include <memory>
#include <string>

class Color;
class Point;



// Class for drawing text in OpenGL.
// The encoding of the text is utf8.
class Font {
public:
	Font();
	
	bool Load(const std::string &path, int size);
	
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color) const;
	
	int Width(const std::string &str) const;
	std::string Truncate(const std::string &str, int width, bool ellipsis = true) const;
	std::string TruncateFront(const std::string &str, int width, bool ellipsis = true) const;
	std::string TruncateMiddle(const std::string &str, int width, bool ellipsis = true) const;
	
	int Height() const;
	
	int Space() const;
	
	static void ShowUnderlines(bool show);
	static bool ShowUnderlines();

	// Replace straight quotation marks with curly ones.
	static std::string ReplaceCharacters(const std::string &str);
	
	// Convert between utf8 and utf32.
	// Invalid code points are converted to 0xFFFFFFFF in utf32 and 0xFF in utf8.
	static std::u32string Convert(const std::string &str);
	static std::string Convert(const std::u32string &str);
	
	// Skip to the next unicode code point after pos in utf8.
	// Returns the string length when there are no more code points.
	static size_t NextCodePoint(const std::string &str, size_t pos = 0);
	
	// Get the start of the unicode code point at pos in utf8.
	static size_t CodePointStart(const std::string &str, size_t pos = 0);
	
	// Decode a unicode code point in utf8.
	// Invalid codepoints are converted to 0xFFFFFFFF.
	static char32_t DecodeCodePoint(const std::string &str, size_t pos = 0);
	
	
public:
	enum {
		// Curly quotes
		LEFT_SINGLE_QUOTATION_MARK = 0x2018,
		RIGHT_SINGLE_QUOTATION_MARK = 0x2019,
		LEFT_DOUBLE_QUOTATION_MARK = 0x201C,
		RIGHT_DOUBLE_QUOTATION_MARK = 0x201D,
		// Default character
		WHITE_VERTICAL_RECTANGLE = 0x25AF,
	};
	
	// Interface of a glyph source.
	class IGlyphs {
	public:
		virtual ~IGlyphs();
		
		// Draw a string.
		virtual void Draw(const std::string &str, double x, double y, const Color &color) const = 0;
		
		// Get the width of a string.
		virtual double Width(const std::string &str) const = 0;
		
		// Get the height of the line.
		virtual double LineHeight() const = 0;
		
		// Get the width of a space.
		virtual double Space() const = 0;
		
		// Get the index of the first unsupported code point starting at pos.
		// Return the string length when there are no more code points.
		virtual size_t FindUnsupported(const std::string &str, size_t pos = 0) const = 0;
		
		// Set up the shader that will draw the text.
		virtual void SetUpShader() = 0;
	};
	
	
private:
	std::shared_ptr<IGlyphs> source;
};



#endif
