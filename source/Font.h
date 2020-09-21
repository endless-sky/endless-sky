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

#include "Shader.h"

#include "gl_header.h"

#include <string>

class Color;
class ImageBuffer;
class Point;



// Class for drawing text in OpenGL. Each font is based on a single image with
// glyphs for each character in ASCII order (not counting control characters).
// The kerning between characters is automatically adjusted to look good. At the
// moment only plain ASCII characters are supported, not Unicode.
class Font {
public:
	Font();
	explicit Font(const std::string &imagePath);
	
	void Load(const std::string &imagePath);
	
	// Layout parameters.
	enum Align {LEFT, CENTER, RIGHT};
	enum Truncate {TRUNC_NONE, TRUNC_FRONT, TRUNC_MIDDLE, TRUNC_BACK};
	struct Layout {
			// Align and trancate width. No align or trancate if width is negative.
			int width = -1;
			// Set the alignment mode.
			Align align = LEFT;
			// Set the truncate mode.
			Truncate truncate = TRUNC_NONE;
			
			Layout() noexcept;
			Layout(int w, Align a) noexcept;
			Layout(int w, Truncate t) noexcept;
			Layout(int w, Truncate t, Align a) noexcept;
	};
	
	void Draw(const std::string &str, const Point &point, const Color &color,
		const Layout &layout = defaultLayout) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color,
		const Layout &layout = defaultLayout) const;
	
	int Width(const std::string &str, const Layout &layout = defaultLayout, char after = ' ') const;
	
	int Height() const;
	
	int Space() const;
	
	static void ShowUnderlines(bool show);
	
	
private:
	static int Glyph(char c, bool isAfterSpace);
	void LoadTexture(ImageBuffer &image);
	void CalculateAdvances(ImageBuffer &image);
	void SetUpShader(float glyphW, float glyphH);
	
	int WidthRawString(const char *str, char after = ' ') const;
	
	std::string TruncateText(const std::string &str, const Layout &layout, int &width) const;
	std::string TruncateBack(const std::string &str, int &width) const;
	std::string TruncateFront(const std::string &str, int &width) const;
	std::string TruncateMiddle(const std::string &str, int &width) const;
	
	
private:
	static const Layout defaultLayout;
	
	
	
	
	Shader shader;
	GLuint texture;
	GLuint vao;
	GLuint vbo;
	
	GLint colorI;
	GLint scaleI;
	GLint glyphI;
	GLint aspectI;
	GLint positionI;
	
	int height;
	int space;
	mutable int screenWidth;
	mutable int screenHeight;
	
	static const int GLYPHS = 98;
	int advance[GLYPHS * GLYPHS];
	int widthEllipses;
};



inline
Font::Layout::Layout() noexcept
{
}



inline
Font::Layout::Layout(int w, Align a) noexcept
        : width(w), align(a)
{
}



inline
Font::Layout::Layout(int w, Truncate t) noexcept
        : width(w), truncate(t)
{
}



inline
Font::Layout::Layout(int w, Truncate t, Align a) noexcept
        : width(w), align(a), truncate(t)
{
}



#endif
