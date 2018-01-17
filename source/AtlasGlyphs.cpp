/* AtlasGlyphs.cpp
Copyright (c) 2018 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "AtlasGlyphs.h"

#include "Color.h"
#include "Font.h"
#include "Point.h"
#include "Screen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {
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
}



AtlasGlyphs::AtlasGlyphs()
	: texture(0), vao(0), vbo(0), colorI(0), scaleI(0), glyphI(0), aspectI(0),
	  positionI(0), height(0), space(0), screenWidth(0), screenHeight(0)
{
}



bool AtlasGlyphs::Load(const string &imagePath)
{
	// Load the image.
	image.Clear();
	if(!image.Read(imagePath))
		return false;
	
	CalculateAdvances();
	glyphW = .5f * image.Width() / GLYPHS;
	glyphH = .5f * image.Height();
	return true;
}



void AtlasGlyphs::Draw(const string &str, double x, double y, const Color &color) const
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
	bool underlineChar = false;
	const int underscoreGlyph = Glyph('_');
	
	for(size_t pos = 0; pos < str.length(); pos = Font::NextCodePoint(str, pos))
	{
		// TODO handle composing codepoints
		char32_t c = Font::DecodeCodePoint(str, pos);
		if(c == '_')
		{
			underlineChar = Font::ShowUnderlines();
			continue;
		}
		
		int glyph = Glyph(c);
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
	
	glBindVertexArray(0);
	glUseProgram(0);
}



double AtlasGlyphs::Width(const string &str) const
{
	int width = 0;
	int previous = 0;
	
	for(size_t pos = 0; pos < str.length(); pos = Font::NextCodePoint(str, pos))
	{
		char32_t c = Font::DecodeCodePoint(str, pos);
		if(c == '_')
			continue;
		
		int glyph = Glyph(c);
		if(!glyph)
			width += space;
		else
		{
			width += advance[previous * GLYPHS + glyph] + KERN;
			previous = glyph;
		}
	}
	width += advance[previous * GLYPHS];
	
	return width;
}



double AtlasGlyphs::LineHeight() const
{
	return height;
}



double AtlasGlyphs::Space() const
{
	return space;
}



size_t AtlasGlyphs::FindUnsupported(const string &str, size_t pos) const
{
	if(Font::CodePointStart(str, pos) != pos)
		pos = Font::NextCodePoint(str, pos);
	for(; pos < str.length(); pos = Font::NextCodePoint(str, pos))
	{
		char32_t c = Font::DecodeCodePoint(str, pos);
		// ASCII
		if(c >= 32 && c < 32 + GLYPHS - 3 && c != '\'' && c != '"')
			continue;
		// Curly quotes
		if(c == Font::LEFT_SINGLE_QUOTATION_MARK)
			continue;
		if(c == Font::RIGHT_SINGLE_QUOTATION_MARK)
			continue;
		if(c == Font::LEFT_DOUBLE_QUOTATION_MARK)
			continue;
		if(c == Font::RIGHT_DOUBLE_QUOTATION_MARK)
			continue;
		// Vertical rectangle
		if(c == Font::WHITE_VERTICAL_RECTANGLE)
			continue;
		return pos;
	}
	return str.length();
}



int AtlasGlyphs::Glyph(char32_t c)
{
	// ASCII
	if(c >= 32 && c < 32 + GLYPHS - 3 && c != '\'' && c != '"')
		return c - 32;
	// Curly quotes
	if(c == Font::LEFT_SINGLE_QUOTATION_MARK)
		return 96;
	if(c == Font::RIGHT_SINGLE_QUOTATION_MARK)
		return '\'' - 32;
	if(c == Font::LEFT_DOUBLE_QUOTATION_MARK)
		return 97;
	if(c == Font::RIGHT_DOUBLE_QUOTATION_MARK)
		return '"' - 32;
	// Vertical rectangle
	return 95;
}



void AtlasGlyphs::LoadTexture()
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



void AtlasGlyphs::CalculateAdvances()
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



void AtlasGlyphs::SetUpShader()
{
	LoadTexture();
	image.Clear();
	
	shader = Shader(vertexCode, fragmentCode);
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);
	
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
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	// We must update the screen size next time we draw.
	screenWidth = 0;
	screenHeight = 0;
	
	// The texture always comes from texture unit 0.
	glUniform1i(shader.Uniform("tex"), 0);

	colorI = shader.Uniform("color");
	scaleI = shader.Uniform("scale");
	glyphI = shader.Uniform("glyph");
	aspectI = shader.Uniform("aspect");
	positionI = shader.Uniform("position");
}
