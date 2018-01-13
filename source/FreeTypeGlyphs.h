/* FreeTypeGlyphs.h
Copyright (c) 2018 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FREE_TYPE_GLYPHS_H_
#define FREE_TYPE_GLYPHS_H_

#include "Font.h"
#include "Point.h"
#include "Shader.h"

#include "gl_header.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

class Color;
class ImageBuffer;
class Sprite;



// Class for drawing text in OpenGL.
// The glyphs come from a font file.
class FreeTypeGlyphs : public Font::IGlyphs {
public:
	FreeTypeGlyphs();
	~FreeTypeGlyphs();
	
	bool Load(const std::string &path, int size);
	
	virtual void Draw(const std::string &str, double x, double y, const Color &color) const override;
	virtual double Width(const std::string &str) const override;
	virtual double LineHeight() const override;
	virtual double Space() const override;
	virtual size_t FindUnsupported(const std::string &str, size_t pos = 0) const override;
	virtual void SetUpShader() override;
	
	
private:
	// Glyph translated from a string.
	struct GlyphData {
		// Index of the glyph.
		FT_UInt index;
		// Position in the string.
		size_t start;
		// Position in the screen.
		Point position;
	};
	
	// A key mapping the text, subpixel position and underline status to RenderedText.
	typedef std::pair<std::string,uint16_t> CacheKey;
	
	// Text rendered as a sprite.
	struct RenderedText {
		// Sprite with the rendered text.
		// Frame 0 is normal text.
		// Frame 1 is underlined text.
		std::shared_ptr<Sprite> sprite;
		// Offset from the floored origin to the center of the sprite.
		Point offset;
		// Last access time.
		time_t timestamp;
	};
	
	
private:
	// Translate a string to glyphs.
	std::vector<GlyphData> Translate(const std::string &str) const;
	// Shape the data, recording the position of each glyph.
	void Shape(std::vector<GlyphData> &arr, double x, double y) const;
	
	// Render the text, caching the result.
	RenderedText &Render(const std::string &str, double x, double y, bool showUnderlines) const;
	
	
private:
	Shader shader;
	GLuint vao;
	GLuint vbo;
	
	// Shader parameters.
	GLint scaleI;
	GLint centerI;
	GLint sizeI;
	GLint colorI;
	
	int size;
	double baseline;
	double space;
	FT_UInt underscoreIndex;
	
	FT_Library library;
	FT_Face face;
	
	mutable int screenWidth;
	mutable int screenHeight;
	
	// Cache of rendered text.
	mutable std::map<CacheKey,RenderedText> cache;
	// Time stamp for removing entries from the cache.
	mutable time_t timestamp;
};



#endif
