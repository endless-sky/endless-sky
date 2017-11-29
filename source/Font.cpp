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

#include "Color.h"
#include "ImageBuffer.h"
#include "Point.h"
#include "Screen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {
	bool showUnderlines = false;
	
	const char *vertexCode =
		// "scale" maps pixel coordinates to GL coordinates (-1 to 1).
		"uniform vec2 scale;\n"
		// The (x, y) coordinates of the top left corner of the glyph.
		"uniform vec2 position;\n"
		// The glyph to draw. (ASCII value - 32).
		"uniform int glyph;\n"
		// Aspect ratio of rendered glyph (unity by default).
		"uniform float aspect = 1.f;\n"
		
		// Inputs from the VBO.
		"in vec2 vert;\n"
		"in vec2 corner;\n"
		
		// Output to the fragment shader.
		"out vec2 texCoord;\n"
		
		// Pick the proper glyph out of the texture.
		"void main() {\n"
		"  texCoord = vec2((glyph + corner.x) / 98.f, corner.y);\n"
		"  gl_Position = vec4((aspect * vert.x + position.x) * scale.x, (vert.y + position.y) * scale.y, 0, 1);\n"
		"}\n";
	
	const char *fragmentCode =
		// The user must supply a texture and a color (white by default).
		"uniform sampler2D tex;\n"
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		
		// This comes from the vertex shader.
		"in vec2 texCoord;\n"
		
		// Output color.
		"out vec4 finalColor;\n"
		
		// Multiply the texture by the user-specified color (including alpha).
		"void main() {\n"
		"  finalColor = texture(tex, texCoord).a * color;\n"
		"}\n";
	
	const int KERN = 2;
	
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
	
	// Decodes a unicode codepoint in utf8.
	int DecodeCodePoint(const char *str)
	{
		// invalid (-1) or end (0)
		int bytes = CodePointBytes(str);
		if(bytes < 1)
			return bytes;
		
		// 1 byte
		if(bytes == 1)
			return (*str & 0x7f);
		
		// 2-4 bytes
		int c = (*str & ((1 << (7 - bytes)) - 1));
		for(int i = 1; i < bytes; ++i)
			c = (c << 6) + (*(str + i) & 0x3f);
		return c;
	}
	
	// Skips to the next unicode code point in utf8.
	const char *NextCodePoint(const char *str)
	{
		if(!str || !*str)
			return str;
		
		for(++str; *str; ++str)
			if((*str & 0x80) == 0 || (*str & 0xc0) == 0xc0)
				break;
		return str;
	}
	
	// Returns the start index of the unicode code point of index i in utf8.
	size_t CodePointStart(const char *str, size_t i)
	{
		if(!str || i <= 0)
			return 0;
		
		while(i > 0 && (*(str + i) & 0x80) != 0x00 && (*(str + i) & 0xc0) != 0xc0)
			--i;
		return i;
	}
}



Font::Font()
	: texture(0), vao(0), vbo(0), colorI(0), scaleI(0), glyphI(0), aspectI(0),
	  positionI(0), height(0), space(0), screenWidth(0), screenHeight(0)
{
}



Font::Font(const string &imagePath)
	: Font()
{
	Load(imagePath);
}



void Font::Load(const string &imagePath)
{
	// Load the texture.
	ImageBuffer image;
	if(!image.Read(imagePath))
		return;
	
	LoadTexture(image);
	CalculateAdvances(image);
	SetUpShader(image.Width() / GLYPHS, image.Height());
}



void Font::Draw(const string &str, const Point &point, const Color &color) const
{
	DrawAliased(str, round(point.X()), round(point.Y()), color);
}



void Font::DrawAliased(const string &str, double x, double y, const Color &color) const
{
	glUseProgram(shader.Object());
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(vao);
	
	glUniform4fv(colorI, 1, color.Get());
	
	// Update the scale, only if the screen size has changed.
	if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		GLfloat scale[2] = {2.f / screenWidth, -2.f / screenHeight};
		glUniform2fv(scaleI, 1, scale);
	}
	
	GLfloat textPos[2] = {
		static_cast<float>(x - 1.),
		static_cast<float>(y)};
	int previous = 0;
	bool isAfterSpace = true;
	bool underlineChar = false;
	const int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));
	
	for(const char *s = str.c_str(); *s; s = NextCodePoint(s))
	{
		// TODO handle composing codepoints
		int c = DecodeCodePoint(s);
		if(c == '_')
		{
			underlineChar = showUnderlines;
			continue;
		}
		
		int glyph = Glyph(c, isAfterSpace);
		if(c != '"' && c != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
		{
			textPos[0] += space;
			continue;
		}
		
		glUniform1i(glyphI, glyph);
		glUniform1f(aspectI, 1.f);
		
		textPos[0] += advance[previous * GLYPHS + glyph] + KERN;
		glUniform2fv(positionI, 1, textPos);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		if(underlineChar)
		{
			glUniform1i(glyphI, underscoreGlyph);
			glUniform1f(aspectI, static_cast<float>(advance[glyph * GLYPHS] + KERN)
				/ (advance[underscoreGlyph * GLYPHS] + KERN));
			
			glUniform2fv(positionI, 1, textPos);
			
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			underlineChar = false;
		}
		
		previous = glyph;
	}
}



int Font::Width(const string &str, char after) const
{
	return Width(str.c_str(), after);
}



int Font::Width(const char *str, char after) const
{
	int width = 0;
	int previous = 0;
	bool isAfterSpace = true;
	
	for( ; *str; str = NextCodePoint(str))
	{
		// TODO handle composing codepoints
		int c = DecodeCodePoint(str);
		if(c == '_')
			continue;
		
		int glyph = Glyph(c, isAfterSpace);
		if(c != '"' && c != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
			width += space;
		else
		{
			width += advance[previous * GLYPHS + glyph] + KERN;
			previous = glyph;
		}
	}
	width += advance[previous * GLYPHS + max(0, min(GLYPHS - 1, after - 32))];
	
	return width;
}



string Font::Truncate(const string &str, int width, bool ellipsis) const
{
	int prevWidth = Width(str);
	if(prevWidth <= width)
		return str;
	
	if(ellipsis)
		width -= Width("...");
	
	// Find the last index that fits the width. [good,bad[
	int len = str.length();
	int prev = len;
	int good = 0;
	int bad = len;
	int tries = len + 1;
	const char *s = str.c_str();
	while(NextCodePoint(s + good) < s + bad && --tries)
	{
		// Interpolate next index from the width of the previous index.
		int next = CodePointStart(s, (prev * width) / prevWidth);
		if(next <= good)
			next = NextCodePoint(s + good) - s;
		else if(next >= bad)
			next = CodePointStart(s, bad - 1);
		
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
	int len = str.length();
	int prev = 0;
	int bad = 0;
	int good = len;
	int tries = len + 1;
	const char *s = str.c_str();
	while(NextCodePoint(s + bad) < s + good && --tries)
	{
		// Interpolate next index from the width of the previous index.
		int next = CodePointStart(s, len - ((len - prev) * width) / prevWidth);
		if(next <= bad)
			next = NextCodePoint(s + bad) - s;
		else if(next >= good)
			next = CodePointStart(s, good - 1);
		
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
	return height;
}



int Font::Space() const
{
	return space;
}



void Font::ShowUnderlines(bool show)
{
	showUnderlines = show;
}



int Font::Glyph(int c, bool isAfterSpace)
{
	// Curly quotes.
	if(c == '\'' && isAfterSpace)
		return 96;
	if(c == '"' && isAfterSpace)
		return 97;
	
	return max(0, min(GLYPHS - 3, c - 32));
}



void Font::LoadTexture(ImageBuffer &image)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.Width(), image.Height(), 0,
		GL_BGRA, GL_UNSIGNED_BYTE, image.Pixels());
}



void Font::CalculateAdvances(ImageBuffer &image)
{
	// Get the format and size of the surface.
	int width = image.Width() / GLYPHS;
	height = image.Height();
	unsigned mask = 0xFF000000;
	unsigned half = 0xC0000000;
	int pitch = image.Width();
	
	// advance[previous * GLYPHS + next] is the x advance for each glyph pair.
	// There is no advance if the previous value is 0, i.e. we are at the very
	// start of a string.
	memset(advance, 0, GLYPHS * sizeof(advance[0]));
	for(int previous = 1; previous < GLYPHS; ++previous)
		for(int next = 0; next < GLYPHS; ++next)
		{
			int maxD = 0;
			int glyphWidth = 0;
			uint32_t *begin = reinterpret_cast<uint32_t *>(image.Pixels());
			for(int y = 0; y < height; ++y)
			{
				// Find the last non-empty pixel in the previous glyph.
				uint32_t *pend = begin + previous * width;
				uint32_t *pit = pend + width;
				while(pit != pend && (*--pit & mask) < half) {}
				int distance = (pit - pend) + 1;
				glyphWidth = max(distance, glyphWidth);
				
				// Special case: if "next" is zero (i.e. end of line of text),
				// calculate the full width of this character. Otherwise:
				if(next)
				{
					// Find the first non-empty pixel in this glyph.
					uint32_t *nit = begin + next * width;
					uint32_t *nend = nit + width;
					while(nit != nend && (*nit++ & mask) < half) {}
					
					// How far apart do you want these glyphs drawn? If drawn at
					// an advance of "width", there would be:
					// pend + width - pit   <- pixels after the previous glyph.
					// nit - (nend - width) <- pixels before the next glyph.
					// So for zero kerning distance, you would want:
					distance += 1 - (nit - (nend - width));
				}
				maxD = max(maxD, distance);
				
				// Update the pointer to point to the beginning of the next row.
				begin += pitch;
			}
			// This is a fudge factor to avoid over-kerning, especially for the
			// underscore and for glyph combinations like AV.
			advance[previous * GLYPHS + next] = max(maxD, glyphWidth - 4) / 2;
		}
	
	// Set the space size based on the character width.
	width /= 2;
	height /= 2;
	space = (width + 3) / 6 + 1;
}



void Font::SetUpShader(float glyphW, float glyphH)
{
	glyphW *= .5f;
	glyphH *= .5f;
	
	shader = Shader(vertexCode, fragmentCode);
	glUseProgram(shader.Object());
	
	// Create the VAO and VBO.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertices[] = {
		   0.f,    0.f, 0.f, 0.f,
		   0.f, glyphH, 0.f, 1.f,
		glyphW,    0.f, 1.f, 0.f,
		glyphW, glyphH, 1.f, 1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	// connect the xy to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
	
	glEnableVertexAttribArray(shader.Attrib("corner"));
	glVertexAttribPointer(shader.Attrib("corner"), 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	
	// We must update the screen size next time we draw.
	screenWidth = 0;
	screenHeight = 0;
	
	// The texture always comes from texture unit 0.
	glUniform1ui(shader.Uniform("tex"), 0);

	colorI = shader.Uniform("color");
	scaleI = shader.Uniform("scale");
	glyphI = shader.Uniform("glyph");
	aspectI = shader.Uniform("aspect");
	positionI = shader.Uniform("position");
}
