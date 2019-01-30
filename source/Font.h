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

#include "Cache.h"
#include "Point.h"
#include "Shader.h"

#include "gl_header.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <pango/pangocairo.h>

class Color;
class ImageBuffer;



// Class for drawing text in OpenGL.
// The encoding of the text is utf8.
class Font {
public:
	Font();
	~Font();
	Font(const Font &a) = delete;
	Font &operator=(const Font &a) = delete;
	
	// Font settings.
	void SetFontDescription(const std::string &desc);
	void SetLayoutReference(const std::string &desc);
	void SetPixelSize(int size);
	void SetLanguage(const std::string &langCode);
	
	// Layout parameters.
	enum Align {LEFT, CENTER, RIGHT, JUSTIFIED};
	enum Truncate {TRUNC_NONE, TRUNC_FRONT, TRUNC_MIDDLE, TRUNC_BACK};
	static const uint_fast8_t DEFAULT_LINE_HEIGHT = 255;
	struct Layout {
		// Set the alignment mode.
		Align align = LEFT;
		// Set the truncate mode.
		Truncate truncate = TRUNC_NONE;
		// Wrap and trancate width. No wrap or trancate if width is negative.
		int width = -1;
		// Line height in pixels.
		uint_fast8_t lineHeight = DEFAULT_LINE_HEIGHT;
		// Extra spacing in pixel between paragraphs.
		uint_fast8_t paragraphBreak = 0;
		
		Layout() noexcept = default;
		Layout(Truncate t, int w) noexcept;
		Layout(const Layout& a) noexcept = default;
		Layout &operator=(const Layout& a) noexcept = default;
		bool operator==(const Layout &a) const;
	};
	
	void Draw(const std::string &str, const Point &point, const Color &color,
		const Layout *params = nullptr) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color,
		const Layout *params = nullptr) const;
	
	// Get the height and width of the rendered text.
	int Width(const std::string &str, const Layout *params = nullptr) const;
	int Height(const std::string &str, const Layout *params = nullptr) const;
	
	// Get the height of the fonts.
	int Height() const;
	
	static void ShowUnderlines(bool show);
	
	// Escape markup characters if it causes some errors.
	static std::string EscapeMarkupHasError(const std::string &str);
	
private:
	// Text rendered as a sprite.
	struct RenderedText {
		// Texture with rendered text.
		GLuint texture;
		int width;
		int height;
		// Offset from the floored origin to the center of the sprite.
		Point center;
	};
	
	// A key mapping the text and layout parameters, underline status to RenderedText.
	struct CacheKey {
		std::string text;
		Layout params;
		bool showUnderline;
		
		CacheKey(const std::string &s, const Layout &p, bool underline) noexcept;
		bool operator==(const CacheKey &a) const;
	};
	
	// Hash function of CacheKey.
	struct CacheKeyHash {
		typedef CacheKey argument_type;
		typedef std::size_t result_type;
		result_type operator() (argument_type const &s) const noexcept;
	};
	
	// Function to recycle it for RenderedText.
	class AtRecycleForRenderedText {
	public:
		void operator()(RenderedText &v) const;
	};
	
	
private:
	void UpdateSurfaceSize() const;
	void UpdateFontDesc() const;
	
	static std::string ReplaceCharacters(const std::string &str);
	static std::string RemoveAccelerator(const std::string &str);
		
	void DrawCommon(const std::string &str, double x, double y, const Color &color,
		const Layout *params, bool alignToDot) const;
	const RenderedText &Render(const std::string &str, const Layout *params) const;
	void SetUpShader();
	
	int ViewWidth(const std::string &str, const Layout *params = nullptr) const;
	
	// Convert Viewport to/from Text coordinates.
	double ViewFromTextX(double x) const;
	double ViewFromTextY(double y) const;
	int ViewFromTextX(int x) const;
	int ViewFromTextY(int y) const;
	int ViewFromTextCeilX(int x) const;
	int ViewFromTextCeilY(int y) const;
	int ViewFromTextFloorX(int x) const;
	int ViewFromTextFloorY(int y) const;
	double TextFromViewX(double x) const;
	double TextFromViewY(double y) const;
	int TextFromViewX(int x) const;
	int TextFromViewY(int y) const;
	int TextFromViewCeilX(int x) const;
	int TextFromViewCeilY(int y) const;
	int TextFromViewFloorX(int x) const;
	int TextFromViewFloorY(int y) const;
	
	
private:
	
	Shader shader;
	GLuint vao;
	GLuint vbo;
	
	// Shader parameters.
	GLint scaleI;
	GLint centerI;
	GLint sizeI;
	GLint colorI;
	
	mutable int screenWidth;
	mutable int screenHeight;
	mutable int viewWidth;
	mutable int viewHeight;
	
	mutable cairo_t *cr;
	std::string fontDescName;
	std::string refDescName;
	mutable PangoContext *context;
	mutable PangoLayout *layout;
	PangoLanguage *lang;
	int pixelSize;
	mutable int fontViewHeight;
	mutable int space;
	mutable int surfaceWidth;
	mutable int surfaceHeight;
	
	// Cache of rendered text.
	mutable Cache<CacheKey, RenderedText, true, CacheKeyHash, AtRecycleForRenderedText> cache;
};



inline
Font::Layout::Layout(Truncate t, int w) noexcept
	: truncate(t), width(w)
{
}



inline
bool Font::Layout::operator==(const Layout &a) const
{
	return align == a.align && truncate == a.truncate && width == a.width
		&& lineHeight == a.lineHeight && paragraphBreak == a.paragraphBreak;
}



inline
Font::CacheKey::CacheKey(const std::string &s, const Layout &p, bool underline) noexcept
	: text(s), params(p), showUnderline(underline)
{
}



inline
bool Font::CacheKey::operator==(const CacheKey &a) const
{
	return text == a.text && params == a.params && showUnderline == a.showUnderline;
}



inline
Font::CacheKeyHash::result_type Font::CacheKeyHash::operator() (argument_type const &s) const noexcept
{
	const result_type h1 = std::hash<std::string>()(s.text);
	const result_type h2 = std::hash<unsigned int>()(s.params.width);
	const std::uint_fast32_t pack = s.showUnderline | (s.params.align << 1) | (s.params.truncate << 3)
		| (s.params.lineHeight << 5) | (s.params.paragraphBreak << 13);
	const result_type h3 = std::hash<uint_fast32_t>()(pack);
	return h1 ^ (h2 << 1) ^ (h3 << 2);
}



inline
void Font::AtRecycleForRenderedText::operator()(RenderedText &v) const
{
	if(v.texture)
		glDeleteTextures(1, &v.texture);
}

#endif
