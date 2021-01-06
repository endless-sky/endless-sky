/* Font.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Font.h"

#include "alignment.hpp"
#include "../Color.h"
#include "../Files.h"
#include "../ImageBuffer.h"
#include "../Screen.h"
#include "truncate.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	bool showUnderlines = false;
	const int TOTAL_TAB_STOPS = 8;
	
	const array<string, 3> acceptableCharacterReferences{ "gt;", "lt;", "amp;" };
	
	// Convert from PANGO to pixel scale.
	int PixelFromPangoCeil(int pangoSize)
	{
		return ceil(static_cast<double>(pangoSize) / PANGO_SCALE);
	}
	
	// A wrapper function that makes the compiler estimate the type T and D
	// in order to reduce complexity of expression.
	template<class T, class D>
	unique_ptr<T, D> MakeUniq(T* ptr, D deleter)
	{
		return unique_ptr<T, D>(ptr, deleter);
	}
}



Font::Font()
{
	SetUpShader();
	UpdateSurfaceSize();
	
	cache.SetUpdateInterval(3600);
}



void Font::SetPixelSize(int size)
{
	pixelSize = size;
	UpdateFont();
}



void Font::SetDrawingSettings(const DrawingSettings &newSettings)
{
	drawingSettings = newSettings;
	UpdateFont();
}



void Font::Draw(const DisplayText &text, const Point &point, const Color &color) const
{
	DrawCommon(text, point.X(), point.Y(), color, true);
}



void Font::DrawAliased(const DisplayText &text, double x, double y, const Color &color) const
{
	DrawCommon(text, x, y, color, false);
}



void Font::Draw(const string &str, const Point &point, const Color &color) const
{
	DrawCommon({str, {}}, point.X(), point.Y(), color, true);
}



void Font::DrawAliased(const string &str, double x, double y, const Color &color) const
{
	DrawCommon({str, {}}, x, y, color, false);
}



int Font::Width(const string &str) const
{
	return TextFromViewCeilX(ViewWidth({str, {}}));
}



int Font::Height() const
{
	return TextFromViewCeilY(viewFontHeight);
}



int Font::FormattedWidth(const DisplayText &text) const
{
	return TextFromViewCeilX(ViewWidth(text));
}



int Font::FormattedHeight(const DisplayText &text) const
{
	if(text.GetText().empty())
		return 0;
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return 0;
	return TextFromViewCeilY(renderedText.height);
}



Point Font::FormattedBounds(const DisplayText &text) const
{
	if(text.GetText().empty())
		return Point();
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return Point();
	return Point(TextFromViewCeilX(renderedText.width), TextFromViewCeilY(renderedText.height));
}



int Font::LineHeight(const Layout &layout) const
{
	if(layout.lineHeight == Layout::DEFAULT_LINE_HEIGHT)
		return TextFromViewCeilY(viewDefaultLineHeight);
	else
		return layout.lineHeight;
}



int Font::ParagraphBreak(const Layout &layout) const
{
	if(layout.paragraphBreak == Layout::DEFAULT_PARAGRAPH_BREAK)
		return TextFromViewCeilY(viewDefaultParagraphBreak);
	else
		return layout.paragraphBreak;
}



void Font::ShowUnderlines(bool show) noexcept
{
	showUnderlines = show;
}



string Font::EscapeMarkupHasError(const string &str)
{
	const string text = ReplaceCharacters(str);
	if(pango_parse_markup(text.c_str(), -1, '_', nullptr, nullptr, nullptr, nullptr))
		return str;
	else
	{
		string result;
		for(const auto &c : text)
		{
			if(c == '<')
				result += "&lt;";
			else if(c == '>')
				result += "&gt;";
			else
				result += c;
		}
		return result;
	}
}



void Font::DeleterCairoT::operator()(cairo_t *ptr) const
{
	cairo_destroy(ptr);
}



void Font::DeleterPangoLayout::operator()(PangoLayout *ptr) const
{
	g_object_unref(ptr);
}



void Font::UpdateSurfaceSize() const
{
	auto sf = MakeUniq(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surfaceWidth, surfaceHeight),
		cairo_surface_destroy);
	cr = decltype(cr)(cairo_create(sf.get()));
	sf.reset();
	
	if(pangoLayout)
		pango_cairo_update_layout(cr.get(), pangoLayout.get());
	else
		pangoLayout = decltype(pangoLayout)(pango_cairo_create_layout(cr.get()));
	context = pango_layout_get_context(pangoLayout.get());
	
	pango_layout_set_wrap(pangoLayout.get(), PANGO_WRAP_WORD);
	
	UpdateFont();
}



void Font::UpdateFont() const
{
	if(pixelSize <= 0)
		return;
	
	cache.Clear();
	
	// Get font descriptions.
	auto fontDesc = MakeUniq(pango_font_description_from_string(drawingSettings.description.c_str()),
		pango_font_description_free);
	
	// Set the pixel size.
	const int fontSize = ViewFromTextFloorY(pixelSize) * PANGO_SCALE;
	pango_font_description_set_absolute_size(fontDesc.get(), fontSize);
	
	// Update the context.
	PangoLanguage *lang = pango_language_from_string(drawingSettings.language.c_str());
	pango_context_set_language(context, lang);
	
	// Update the layout.
	pango_layout_set_font_description(pangoLayout.get(), fontDesc.get());
	
	// Update layout parameters.
	auto metrics = MakeUniq(pango_context_get_metrics(context, fontDesc.get(), lang),
		pango_font_metrics_unref);
	const int ascent = pango_font_metrics_get_ascent(metrics.get());
	const int descent = pango_font_metrics_get_descent(metrics.get());
	viewFontHeight = PixelFromPangoCeil(ascent + descent);
	metrics.reset();
	fontDesc.reset();
	
	if (drawingSettings.lineHeightScale >= 0.)
		viewDefaultLineHeight = viewFontHeight * drawingSettings.lineHeightScale;
	else
		viewDefaultLineHeight = 0.;
	if (drawingSettings.paragraphBreakScale >= 0.)
		viewDefaultParagraphBreak = viewFontHeight * drawingSettings.paragraphBreakScale;
	else
		viewDefaultParagraphBreak = 0.;
	
	// Tab Stop
	auto tb = MakeUniq(pango_tab_array_new(TOTAL_TAB_STOPS, FALSE), pango_tab_array_free);
	space = ViewWidth(DisplayText(" ", {}));
	const int tabSize = 4 * space * PANGO_SCALE;
	for(int i = 0; i < TOTAL_TAB_STOPS; ++i)
		pango_tab_array_set_tab(tb.get(), i, PANGO_TAB_LEFT, i * tabSize);
	pango_layout_set_tabs(pangoLayout.get(), tb.get());
}



// Replaces straight quotation marks with curly ones,
// and escapes "&" except for minimum necessaries because a pilot name may
// contain "&" and that representation should be the same as the old version.
string Font::ReplaceCharacters(const string &str)
{
	string buf;
	buf.reserve(str.length());
	bool isAfterWhitespace = true;
	bool isTag = false;
	const size_t len = str.length();
	for(size_t pos = 0; pos < len; ++pos)
	{
		if(isTag)
		{
			if(str[pos] == '>')
				isTag = false;
			buf.append(1, str[pos]);
		}
		else
		{
			// U+2018 LEFT_SINGLE_QUOTATION_MARK
			// U+2019 RIGHT_SINGLE_QUOTATION_MARK
			// U+201C LEFT_DOUBLE_QUOTATION_MARK
			// U+201D RIGHT_DOUBLE_QUOTATION_MARK
			if(str[pos] == '\'')
				buf.append(isAfterWhitespace ? "\xE2\x80\x98" : "\xE2\x80\x99");
			else if(str[pos] == '"')
				buf.append(isAfterWhitespace ? "\xE2\x80\x9C" : "\xE2\x80\x9D");
			else if(str[pos] == '&')
			{
				buf.append(1, '&');
				bool hit = false;
				for(const auto &s : acceptableCharacterReferences)
				{
					const size_t slen = s.length();
					if(len - pos > slen && !str.compare(pos + 1, slen, s))
					{
						hit = true;
						break;
					}
				}
				if(!hit)
					buf.append("amp;");
			}
			else
				buf.append(1, str[pos]);
			isAfterWhitespace = (str[pos] == ' ');
			isTag = (str[pos] == '<');
		}
	}
	return buf;
}



string Font::RemoveAccelerator(const string &str)
{
	string dest;
	bool isTag = false;
	bool isAccel = false;
	for(char c : str)
	{
		const bool isAfterAccel = isAccel;
		isAccel = false;
		if(isTag)
		{
			dest += c;
			isTag = (c != '>');
		}
		else if(c == '<')
		{
			dest += c;
			isTag = true;
		}
		else if(c != '_' || isAfterAccel)
			dest += c;
		else
			isAccel = true;
	}
	return dest;
}



void Font::DrawCommon(const DisplayText &text, double x, double y, const Color &color, bool alignToDot) const
{
	if(text.GetText().empty())
		return;
	
	const bool screenChanged = Screen::Width() != screenWidth || Screen::Height() != screenHeight;
	if(screenChanged)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		GLint xyhw[4] = {};
		glGetIntegerv(GL_VIEWPORT, xyhw);
		viewWidth = xyhw[2];
		viewHeight = xyhw[3];
		
		UpdateFont();
	}
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return;
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	// Update the texture.
	glBindTexture(GL_TEXTURE_2D, renderedText.texture);
	
	// Update the scale, only if the screen size has changed.
	if(screenChanged)
	{
		GLfloat scale[2] = {2.f / viewWidth, -2.f / viewHeight};
		glUniform2fv(scaleI, 1, scale);
		
	}
	
	// Update the center.
	Point center = Point(ViewFromTextX(x), ViewFromTextY(y));
	if(alignToDot)
		center = Point(floor(center.X()), floor(center.Y()));
	center += renderedText.center;
	glUniform2f(centerI, center.X(), center.Y());
	
	// Update the size.
	glUniform2f(sizeI, renderedText.width, renderedText.height);
	
	// Update the color.
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}



// Render the text.
const Font::RenderedText &Font::Render(const DisplayText &text) const
{
	// Return if already cached.
	const CacheKey key(text, showUnderlines);
	auto cached = cache.Use(key);
	if(cached.second)
		return *cached.first;
	
	// Convert to viewport coodinates except align and truncate.
	const Layout &layout = text.GetLayout();
	Layout viewLayout = layout;
	if(layout.width > 0)
		viewLayout.width = ViewFromTextX(layout.width);
	if(layout.lineHeight == Layout::DEFAULT_LINE_HEIGHT)
		viewLayout.lineHeight = viewDefaultLineHeight;
	else
		viewLayout.lineHeight = ViewFromTextFloorY(layout.lineHeight);
	if(layout.paragraphBreak == Layout::DEFAULT_PARAGRAPH_BREAK)
		viewLayout.paragraphBreak = viewDefaultParagraphBreak;
	else
		viewLayout.paragraphBreak = ViewFromTextFloorY(layout.paragraphBreak);
	
	// Truncate
	const int layoutWidth = viewLayout.width < 0 ? -1 : viewLayout.width * PANGO_SCALE;
	pango_layout_set_width(pangoLayout.get(), layoutWidth);
	PangoEllipsizeMode ellipsize;
	switch(viewLayout.truncate)
	{
		case Truncate::NONE:
			ellipsize = PANGO_ELLIPSIZE_NONE;
			break;
		case Truncate::FRONT:
			ellipsize = PANGO_ELLIPSIZE_START;
			break;
		case Truncate::MIDDLE:
			ellipsize = PANGO_ELLIPSIZE_MIDDLE;
			break;
		case Truncate::BACK:
			ellipsize = PANGO_ELLIPSIZE_END;
			break;
		default:
			ellipsize = PANGO_ELLIPSIZE_NONE;
	}
	pango_layout_set_ellipsize(pangoLayout.get(), ellipsize);
	
	// Align and justification
	PangoAlignment align = PANGO_ALIGN_LEFT;
	gboolean justify = FALSE;
	switch(viewLayout.align)
	{
		case Alignment::LEFT:
			// Do nothing
			break;
		case Alignment::CENTER:
			align = PANGO_ALIGN_CENTER;
			break;
		case Alignment::RIGHT:
			align = PANGO_ALIGN_RIGHT;
			break;
		case Alignment::JUSTIFIED:
			justify = TRUE;
			break;
	}
	pango_layout_set_justify(pangoLayout.get(), justify);
	pango_layout_set_alignment(pangoLayout.get(), align);
	
	// Replaces straight quotation marks with curly ones.
	const string replacedText = ReplaceCharacters(text.GetText());
	
	// Keyboard Accelerator
	{
		char *textRemovedMarkup = nullptr;
		const char *drawingText = nullptr;
		PangoAttrList *al = nullptr;
		GError *error = nullptr;
		const char accel = showUnderlines ? '_' : '\0';
		const string nonAccelText = RemoveAccelerator(replacedText);
		const string &parseText = showUnderlines ? replacedText : nonAccelText;
		if(pango_parse_markup(parseText.c_str(), -1, accel, &al, &textRemovedMarkup, 0, &error))
			drawingText = textRemovedMarkup;
		else
		{
			if(error->message)
				Files::LogError(error->message);
			drawingText = nonAccelText.c_str();
			g_error_free(error);
		}

		// Set the text and attributes to layout.
		pango_layout_set_text(pangoLayout.get(), drawingText, -1);
		pango_layout_set_attributes(pangoLayout.get(), al);
		pango_attr_list_unref(al);
		if(textRemovedMarkup)
			g_free(textRemovedMarkup);
	}
	
	// Check the image buffer size.
	int textWidth;
	int textHeight;
	pango_layout_get_pixel_size(pangoLayout.get(), &textWidth, &textHeight);
	// Pango draws a PANGO_UNDERLINE_LOW under the logical rectangle,
	// and an underline may be longer than a text width.
	PangoRectangle ink_rect;
	pango_layout_get_pixel_extents(pangoLayout.get(), &ink_rect, nullptr);
	textHeight = max(textHeight, ink_rect.y + ink_rect.height);
	textWidth = max(textWidth, ink_rect.x + ink_rect.width);
	// Check this surface has enough width.
	if(surfaceWidth < textWidth)
	{
		surfaceWidth *= (textWidth / surfaceWidth) + 1;
		UpdateSurfaceSize();
		return Render(text);
	}
	
	// Render
	cairo_set_source_rgb(cr.get(), 1.0, 1.0, 1.0);
	
	// Control line skips and paragraph breaks manually.
	const char *layoutText = pango_layout_get_text(pangoLayout.get());
	auto iter = MakeUniq(pango_layout_get_iter(pangoLayout.get()), pango_layout_iter_free);
	int y0 = pango_layout_iter_get_baseline(iter.get());
	int baselineY = PixelFromPangoCeil(y0);
	int sumExtraY = 0;
	PangoRectangle logicalRect;
	pango_layout_iter_get_line_extents(iter.get(), nullptr, &logicalRect);
	cairo_move_to(cr.get(), PixelFromPangoCeil(logicalRect.x), baselineY);
	pango_cairo_update_layout(cr.get(), pangoLayout.get());
	PangoLayoutLine *line = pango_layout_iter_get_line_readonly(iter.get());
	pango_cairo_show_layout_line(cr.get(), line);
	while(pango_layout_iter_next_line(iter.get()))
	{
		const int y1 = pango_layout_iter_get_baseline(iter.get());
		const int index = pango_layout_iter_get_index(iter.get());
		const int diffY = PixelFromPangoCeil(y1 - y0);
		if(layoutText[index] == '\0')
		{
			sumExtraY -= diffY;
			break;
		}
		int add = max(diffY, static_cast<int>(viewLayout.lineHeight));
		if(index > 0 && layoutText[index - 1] == '\n')
			add += viewLayout.paragraphBreak;
		baselineY += add;
		sumExtraY += add - diffY;
		pango_layout_iter_get_line_extents(iter.get(), nullptr, &logicalRect);
		cairo_move_to(cr.get(), PixelFromPangoCeil(logicalRect.x), baselineY);
		pango_cairo_update_layout(cr.get(), pangoLayout.get());
		line = pango_layout_iter_get_line_readonly(iter.get());
		pango_cairo_show_layout_line(cr.get(), line);
		y0 = y1;
	}
	textHeight += sumExtraY + viewLayout.paragraphBreak;
	if (viewLayout.lineHeight > viewFontHeight)
		textHeight += viewLayout.lineHeight - viewFontHeight;
	iter.reset();
	
	// Check this surface has enough height.
	if(surfaceHeight < textHeight)
	{
		surfaceHeight *= (textHeight / surfaceHeight) + 1;
		UpdateSurfaceSize();
		return Render(text);
	}
	
	// Copy to image buffer and clear the surface.
	cairo_surface_t *sf = cairo_get_target(cr.get());
	cairo_surface_flush(sf);
	ImageBuffer image;
	image.Allocate(textWidth, textHeight);
	uint32_t *src = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(sf));
	uint32_t *dest = image.Pixels();
	const int stride = surfaceWidth - textWidth;
	for(int y = 0; y < textHeight; ++y)
	{
		for(int x = 0; x < textWidth; ++x)
		{
			*dest = *src;
			*src = 0;
			++dest;
			++src;
		}
		src += stride;
	}
	cairo_surface_mark_dirty(sf);
	
	// Try to reuse an old texture.
	GLuint texture = 0;
	auto recycled = cache.Recycle();
	if(recycled.second)
		texture = recycled.first.texture;
	
	// Record rendered text.
	RenderedText &renderedText = recycled.first;
	renderedText.texture = texture;
	renderedText.width = textWidth;
	renderedText.height = textHeight;
	renderedText.center = Point(.5 * textWidth, .5 * textHeight);
	
	// Upload the image as a texture.
	if(!renderedText.texture)
		glGenTextures(1, &renderedText.texture);
	glBindTexture(GL_TEXTURE_2D, renderedText.texture);
	const auto &cachedText = cache.New(key, std::move(renderedText));
	
	// Use linear interpolation and no wrapping.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Upload the image data.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, // target, mipmap level, internal format,
		textWidth, textHeight, 0, // width, height, border,
		GL_BGRA, GL_UNSIGNED_BYTE, image.Pixels()); // input format, data type, data.
	
	// Unbind the texture.
	glBindTexture(GL_TEXTURE_2D, 0);
	
	return cachedText;
}



void Font::SetUpShader()
{
	static const char *vertexCode =
		"// vertex font shader\n"
		// Parameter: Convert pixel coordinates to GL coordinates (-1 to 1).
		"uniform vec2 scale;\n"
		// Parameter: Position of the top left corner of the texture in pixels.
		"uniform vec2 center;\n"
		// Parameter: Size of the texture in pixels.
		"uniform vec2 size;\n"
		
		// Input: Coordinate from the VBO.
		"in vec2 vert;\n"
		
		// Output: Texture coordinate for the fragment shader.
		"out vec2 texCoord;\n"
		
		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"  texCoord = vert + vec2(.5, .5);\n"
		"}\n";
	
	static const char *fragmentCode =
		"// fragment font shader\n"
		// Parameter: Texture with the text.
		"uniform sampler2D tex;\n"
		// Parameter: Color to apply to the text.
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		
		// Input: Texture coordinate from the vertex shader.
		"in vec2 texCoord;\n"
		
		// Output: Color for the screen.
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  finalColor = color * texture(tex, texCoord);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	sizeI = shader.Uniform("size");
	colorI = shader.Uniform("color");
	
	// The texture always comes from texture unit 0.
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);
	
	// Create the VAO and VBO.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// Triangle strip.
	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	
	// Unbind the VBO and VAO.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	// We must update the screen size next time we draw.
	screenWidth = 1;
	screenHeight = 1;
	viewWidth = 1;
	viewHeight = 1;
}



int Font::ViewWidth(const DisplayText &text) const
{
	if(text.GetText().empty())
		return 0;
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return 0;
	return renderedText.width;
}



double Font::ViewFromTextX(double x) const
{
	return x * viewWidth / screenWidth;
}



double Font::ViewFromTextY(double y) const
{
	return y * viewHeight / screenHeight;
}



int Font::ViewFromTextX(int x) const
{
	return floor(static_cast<double>(x * viewWidth + screenWidth / 2.0) / screenWidth);
}



int Font::ViewFromTextY(int y) const
{
	return floor(static_cast<double>(y * viewHeight + screenHeight / 2.0) / screenHeight);
}



int Font::ViewFromTextCeilX(int x) const
{
	return ceil(static_cast<double>(x * viewWidth) / screenWidth);
}



int Font::ViewFromTextCeilY(int y) const
{
	return ceil(static_cast<double>(y * viewHeight) / screenHeight);
}



int Font::ViewFromTextFloorX(int x) const
{
	return floor(static_cast<double>(x * viewWidth) / screenWidth);
}



int Font::ViewFromTextFloorY(int y) const
{
	return floor(static_cast<double>(y * viewHeight) / screenHeight);
}



double Font::TextFromViewX(double x) const
{
	return x * screenWidth / viewWidth;
}



double Font::TextFromViewY(double y) const
{
	return y * screenHeight / viewHeight;
}



int Font::TextFromViewX(int x) const
{
	return floor(static_cast<double>(x * screenWidth + viewWidth / 2.0) / viewWidth);
}



int Font::TextFromViewY(int y) const
{
	return floor(static_cast<double>(y * screenHeight + viewHeight / 2.0) / viewHeight);
}



int Font::TextFromViewCeilX(int x) const
{
	return ceil(static_cast<double>(x * screenWidth) / viewWidth);
}



int Font::TextFromViewCeilY(int y) const
{
	return ceil(static_cast<double>(y * screenHeight) / viewHeight);
}



int Font::TextFromViewFloorX(int x) const
{
	return floor(static_cast<double>(x * screenWidth) / viewWidth);
}



int Font::TextFromViewFloorY(int y) const
{
	return floor(static_cast<double>(y * screenHeight) / viewHeight);
}
