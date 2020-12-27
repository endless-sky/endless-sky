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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <pango/pangocairo.h>
#pragma GCC diagnostic pop

class Color;
class ImageBuffer;



// Class for drawing text in OpenGL.
// The encoding of the text is utf8.
class Font {
public:
	// Font and laying out settings except the pixel size.
	struct DrawingSettings {
		// A font description is a comma separated list of font families.
		std::string description = "Ubuntu";
		// The language for laying out.
		std::string language = "en";
		// The line height is lineHeightScale times larger than the font height.
		double lineHeightScale = 1.20;
		// The paragraph break is paragraphBreakScale times larger than the font height.
		double paragraphBreakScale = 0.40;
	};
	
	// Layout parameters.
	enum class Align {LEFT, CENTER, RIGHT, JUSTIFIED};
	enum class Truncate {NONE, FRONT, MIDDLE, BACK};
	static const uint_fast8_t DEFAULT_LINE_HEIGHT = 255;
	static const uint_fast8_t DEFAULT_PARAGRAPH_BREAK = 255;
	struct Layout {
		// Wrap and trancate width. No wrap or trancate if width is negative.
		int width = -1;
		// Set the alignment mode.
		Align align = Align::LEFT;
		// Set the truncate mode.
		Truncate truncate = Truncate::NONE;
		// Minimum Line height in pixels.
		uint_fast8_t lineHeight = DEFAULT_LINE_HEIGHT;
		// Extra spacing in pixel between paragraphs.
		uint_fast8_t paragraphBreak = DEFAULT_PARAGRAPH_BREAK;
		
		Layout() noexcept = default;
		Layout(int w, Align a) noexcept;
		Layout(int w, Truncate t) noexcept;
		Layout(int w, Align a, Truncate t) noexcept;
		bool operator==(const Layout &a) const noexcept;
	};
	
public:
	Font();
	~Font();
	Font(const Font &a) = delete;
	Font &operator=(const Font &a) = delete;
	
	// Set the font size by pixel size of the text coordinate.
	// It's a rough estimate of the actual font size.
	void SetPixelSize(int size);
	
	// Set the font and laying out settings except the pixel size.
	void SetDrawingSettings(const DrawingSettings &drawingSettings);
	
	void Draw(const std::string &str, const Point &point, const Color &color,
		const Layout &layout = defaultLayout) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color,
		const Layout &params = defaultLayout) const;
	
	// Get the height and width of the rendered text.
	int Width(const std::string &str, const Layout &layout = defaultLayout) const;
	int Height(const std::string &str, const Layout &layout = defaultLayout) const;
	
	// Get the height of the fonts.
	int Height() const;
	
	// Get the line height and paragraph break.
	int LineHeight(const Layout &layout = defaultLayout) const;
	int ParagraphBreak(const Layout &layout = defaultLayout) const;
	
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
		Layout layout;
		bool showUnderline;
		
		CacheKey(const std::string &s, const Layout &l, bool underline) noexcept;
		bool operator==(const CacheKey &a) const noexcept;
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
	void UpdateFont() const;
	
	static std::string ReplaceCharacters(const std::string &str);
	static std::string RemoveAccelerator(const std::string &str);
		
	void DrawCommon(const std::string &str, double x, double y, const Color &color,
		const Layout &layout, bool alignToDot) const;
	const RenderedText &Render(const std::string &str, const Layout &layout) const;
	void SetUpShader();
	
	int ViewWidth(const std::string &str, const Layout &layout = defaultLayout) const;
	
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
	static const Layout defaultLayout;
	
	
	
	Shader shader;
	GLuint vao = 0;
	GLuint vbo = 0;
	
	// Shader parameters.
	GLint scaleI = 0;
	GLint centerI = 0;
	GLint sizeI = 0;
	GLint colorI = 0;
	
	// Screen settings.
	mutable int screenWidth = 1;
	mutable int screenHeight = 1;
	mutable int viewWidth = 1;
	mutable int viewHeight = 1;
	mutable int viewFontHeight = 0;
	mutable unsigned int viewDefaultLineHeight = 0;
	mutable unsigned int viewDefaultParagraphBreak = 0;
	
	// Variables related to the font.
	int pixelSize = 0;
	DrawingSettings drawingSettings;
	mutable int space = 0;
	
	// For rendering.
	mutable cairo_t *cr = nullptr;
	mutable PangoContext *context = nullptr;
	mutable PangoLayout *pangoLayout = nullptr;
	mutable int surfaceWidth = 256;
	mutable int surfaceHeight = 64;
	
	// Cache of rendered text.
	mutable Cache<CacheKey, RenderedText, true, CacheKeyHash, AtRecycleForRenderedText> cache;
};



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
Font::Layout::Layout(int w, Align a, Truncate t) noexcept
	: width(w), align(a), truncate(t)
{
}



inline
bool Font::Layout::operator==(const Layout &a) const noexcept
{
	return width == a.width && align == a.align && truncate == a.truncate
		&& lineHeight == a.lineHeight && paragraphBreak == a.paragraphBreak;
}



inline
Font::CacheKey::CacheKey(const std::string &s, const Layout &l, bool underline) noexcept
	: text(s), layout(l), showUnderline(underline)
{
}



inline
bool Font::CacheKey::operator==(const CacheKey &a) const noexcept
{
	return text == a.text && layout == a.layout && showUnderline == a.showUnderline;
}



inline
Font::CacheKeyHash::result_type Font::CacheKeyHash::operator() (argument_type const &s) const noexcept
{
	const result_type h1 = std::hash<std::string>()(s.text);
	const result_type h2 = std::hash<int>()(s.layout.width);
	const unsigned int pack = s.showUnderline | (static_cast<unsigned int>(s.layout.align) << 1)
		| (static_cast<unsigned int>(s.layout.truncate) << 3)
		| (s.layout.lineHeight << 5) | (s.layout.paragraphBreak << 13);
	const result_type h3 = std::hash<unsigned int>()(pack);
	return h1 ^ (h2 << 1) ^ (h3 << 2);
}



inline
void Font::AtRecycleForRenderedText::operator()(RenderedText &v) const
{
	if(v.texture)
		glDeleteTextures(1, &v.texture);
}

#endif
