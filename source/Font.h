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
	Font() = default;
	explicit Font(const std::string &imagePath);
	
	void Load(const std::string &imagePath);
	
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color) const;
	
	int Width(const std::string &str, char after = ' ') const;
	int Width(const char *str, char after = ' ') const;
	std::string Truncate(const std::string &str, int width) const;
	std::string TruncateFront(const std::string &str, int width) const;
	std::string TruncateMiddle(const std::string &str, int width) const;
	
	int Height() const;
	
	int Space() const;
	
	static void ShowUnderlines(bool show);
	
	
private:
	static int Glyph(char c, bool isAfterSpace);
	void LoadTexture(ImageBuffer &image);
	void CalculateAdvances(ImageBuffer &image);
	void SetUpShader(float glyphW, float glyphH);
	
	
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
};



#endif
