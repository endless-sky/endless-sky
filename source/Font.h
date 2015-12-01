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
	Font(const std::string &imagePath);
	
	void Load(const std::string &imagePath);
	
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	
	int Width(const std::string &str, char after = ' ') const;
	int Width(const char *str, char after = ' ') const;
	
	int Height() const;
	
	int Space() const;
	
	static void ShowUnderlines(bool show);
	
	
private:
	void LoadTexture(ImageBuffer *image);
	void CalculateAdvances(ImageBuffer *image);
	void SetUpShader(float glyphW, float glyphH);
	
	
private:
	Shader shader;
	GLuint texture;
	GLuint vao;
	GLuint vbo;
	
	int height;
	int space;
	mutable int screenWidth;
	mutable int screenHeight;
	
	static const int GLYPHS = 96;
	int advance[GLYPHS * GLYPHS];
};



#endif
