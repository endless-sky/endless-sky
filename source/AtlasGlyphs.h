/* AtlasGlyphs.h
Copyright (c) 2018 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ATLAS_GLYPHS_H_
#define ATLAS_GLYPHS_H_

#include "Font.h"
#include "Shader.h"

#include "gl_header.h"

#include <string>

class Color;
class ImageBuffer;



// Class for drawing glyphs in OpenGL from an altas image.
// The kerning between glyphs is automatically adjusted to look good.
// The glyphs are hardcoded.
class AtlasGlyphs : public Font::IGlyphs {
public:
	AtlasGlyphs();
	
	bool Load(const std::string &imagePath);
	
	virtual void Draw(const std::string &str, double x, double y, const Color &color) const override;
	virtual double Width(const std::string &str) const override;
	virtual double LineHeight() const override;
	virtual double Space() const override;
	virtual size_t FindUnsupported(const std::string &str, size_t pos = 0) const override;
	virtual void SetUpShader() override;
	
	
private:
	static int Glyph(char32_t c);
	void LoadTexture(ImageBuffer &image);
	void CalculateAdvances(ImageBuffer &image);
	
	
private:
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
	
	float glyphW;
	float glyphH;
	static const int GLYPHS = 98;
	int advance[GLYPHS * GLYPHS];
};



#endif
