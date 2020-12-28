/* Font.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_FONT_H_
#define ES_TEXT_FONT_H_

#include "../Shader.h"

#include "../gl_header.h"

#include <string>

class Color;
class DisplayText;
class ImageBuffer;
class Point;



// Class for drawing text in OpenGL. Each font is based on a single image with
// glyphs for each character in ASCII order (not counting control characters).
// The kerning between characters is automatically adjusted to look good. At the
// moment only plain ASCII characters are supported, not Unicode.
class Font {
public:
	Font() noexcept = default;
	explicit Font(const std::string &imagePath);
	
	void Load(const std::string &imagePath);
	
	// Draw a text string, subject to the given layout and truncation strategy.
	void Draw(const DisplayText &text, const Point &point, const Color &color) const;
	void DrawAliased(const DisplayText &text, double x, double y, const Color &color) const;
	// Draw the given text string, e.g. post-formatting (or without regard to formatting).
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color) const;
	
	// Determine the string's width, without considering formatting.
	int Width(const std::string &str, char after = ' ') const;
	// Get the width of the text while accounting for the desired layout and truncation strategy.
	int FormattedWidth(const DisplayText &text, char after = ' ') const;
	
	int Height() const noexcept;
	
	int Space() const noexcept;
	
	static void ShowUnderlines(bool show) noexcept;
	
	
private:
	static int Glyph(char c, bool isAfterSpace) noexcept;
	void LoadTexture(ImageBuffer &image);
	void CalculateAdvances(ImageBuffer &image);
	void SetUpShader(float glyphW, float glyphH);
	
	int WidthRawString(const char *str, char after = ' ') const noexcept;
	
	std::string TruncateText(const DisplayText &text, int &width) const;
	std::string TruncateBack(const std::string &str, int &width) const;
	std::string TruncateFront(const std::string &str, int &width) const;
	std::string TruncateMiddle(const std::string &str, int &width) const;
	
	
private:
	Shader shader;
	GLuint texture = 0;
	GLuint vao = 0;
	GLuint vbo = 0;
	
	GLint colorI = 0;
	GLint scaleI = 0;
	GLint glyphI = 0;
	GLint aspectI = 0;
	GLint positionI = 0;
	
	int height = 0;
	int space = 0;
	mutable int screenWidth = 0;
	mutable int screenHeight = 0;
	
	static const int GLYPHS = 98;
	int advance[GLYPHS * GLYPHS] = {};
	int widthEllipses = 0;
};



#endif
