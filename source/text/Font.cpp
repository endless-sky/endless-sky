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
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace {
	bool showUnderlines = false;
	const int TOTAL_TAB_STOPS = 8;
	const uint32_t ENDIAN_DETECTOR = 1;
	const bool isLittleEndian = *reinterpret_cast<const unsigned char*>(&ENDIAN_DETECTOR) == 1;
	
	
	// Convert PANGO size to pixel's.
	int PixelFromPangoCeil(int pangoSize)
	{
		return ceil(static_cast<double>(pangoSize) / PANGO_SCALE);
	}
	
	// Type conversions.
	PangoEllipsizeMode ToPangoEllipsizeMode(Truncate truncate)
	{
		PangoEllipsizeMode ellipsize;
		switch(truncate)
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
		return ellipsize;
	}
	
	pair<PangoAlignment, gboolean> ToPangoAlignmentAndJustify(Alignment align)
	{
		PangoAlignment pangoAlign = PANGO_ALIGN_LEFT;
		gboolean justify = FALSE;
		switch(align)
		{
			case Alignment::LEFT:
				// Do nothing
				break;
			case Alignment::CENTER:
				pangoAlign = PANGO_ALIGN_CENTER;
				break;
			case Alignment::RIGHT:
				pangoAlign = PANGO_ALIGN_RIGHT;
				break;
			case Alignment::JUSTIFIED:
				justify = TRUE;
				break;
		}
		return make_pair(pangoAlign, justify);
	}
	
	
	// Replace straight quotation marks with curly ones, except in markup tags.
	string ReplaceCharacters(const string &str)
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
				else
					buf.append(1, str[pos]);
				isAfterWhitespace = (str[pos] == ' ');
				isTag = (str[pos] == '<');
			}
		}
		return buf;
	}
	
	
	// Remove the accelerator character '_', except in markup tags.
	string RemoveAccelerator(const string &str)
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
	
	
	// A wrapper function that makes the compiler estimate the type T and D
	// in order to reduce complexity of expression.
	template<class T, class D>
	unique_ptr<T, D> MakeUniq(T* ptr, D deleter)
	{
		return unique_ptr<T, D>(ptr, deleter);
	}
	
	
	// Copy a 2D region in a cairo surface to ImageBuffer and then clear the cairo surface.
	// 'dest' and 'src' point to the top-left corner.
	// The pixel format in the cairo surface is ARGB as a value of uint32_t;
	// the format on the memory depends on the endianness.
	void MoveCairoSurfaceToImageBufferForLittleEndian(uint32_t *dest, uint32_t *src,
		int height, int width, int stride)
	{
		for(int y = 0; y < height; ++y)
		{
			for(int x = 0; x < width; ++x)
			{
				*dest = ((*src >> 16) & 0xFF) | (*src & 0xFF00FF00U) | ((*src << 16) & 0xFF0000);
				*src = 0;
				++dest;
				++src;
			}
			src += stride;
		}
	}
	
	void MoveCairoSurfaceToImageBufferForBigEndian(uint32_t *dest, uint32_t *src,
		int height, int width, int stride)
	{
		for(int y = 0; y < height; ++y)
		{
			for(int x = 0; x < width; ++x)
			{
				*dest = ((*src << 8) & 0xFFFFFF00U) | ((*src >> 24) & 0xFF);
				*src = 0;
				++dest;
				++src;
			}
			src += stride;
		}
	}
}



Font::Font()
{
	SetUpShader();
	if(!UpdateSurfaceSize(256, 64, ""))
		throw runtime_error("Initializing error in a constructor of the class Font.");
	
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
	return ToTextCeilX(WidthInViewport({str, {}}));
}



int Font::Height() const
{
	return ToTextCeilY(viewportFontHeight);
}



int Font::FormattedWidth(const DisplayText &text) const
{
	return ToTextCeilX(WidthInViewport(text));
}



int Font::FormattedHeight(const DisplayText &text) const
{
	if(text.GetText().empty())
		return 0;
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return 0;
	return ToTextCeilY(renderedText.height);
}



Point Font::FormattedBounds(const DisplayText &text) const
{
	if(text.GetText().empty())
		return Point();
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return Point();
	return Point(ToTextCeilX(renderedText.width), ToTextCeilY(renderedText.height));
}



int Font::LineHeight(const Layout &layout) const
{
	if(layout.lineHeight == Layout::DEFAULT_LINE_HEIGHT)
		return ToTextCeilY(viewportDefaultLineHeight);
	else
		return layout.lineHeight;
}



int Font::ParagraphBreak(const Layout &layout) const
{
	if(layout.paragraphBreak == Layout::DEFAULT_PARAGRAPH_BREAK)
		return ToTextCeilY(viewportDefaultParagraphBreak);
	else
		return layout.paragraphBreak;
}



void Font::ShowUnderlines(bool show) noexcept
{
	showUnderlines = show;
}



void Font::DeleterCairoT::operator()(cairo_t *ptr) const
{
	cairo_destroy(ptr);
}



void Font::DeleterPangoLayout::operator()(PangoLayout *ptr) const
{
	g_object_unref(ptr);
}



// Return true if the surface is updated.
bool Font::UpdateSurfaceSize(int width, int height, const string &renderingText) const
{
	// Too huge texture will truncate in order to avoid lack of memory.
	if((surfaceWidth < surfaceWidthLimit && width >= surfaceWidthLimit)
		|| (surfaceHeight < surfaceHeightLimit && height >= surfaceHeightLimit))
	{
		string message = "Warning: Reach the maximum limit of the texture size in class Font";
		if(renderingText.empty())
			message += '.';
		else
			message += " while drawing the text \"" + renderingText + "\".";
		Files::LogError(message);
	}
	
	width = min(width, surfaceWidthLimit);
	height = min(height, surfaceHeightLimit);
	
	if(surfaceWidth == width && surfaceHeight == height)
		return false;
	
	auto sf = MakeUniq(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height),
		cairo_surface_destroy);
	const cairo_status_t cairoStatus = cairo_surface_status(sf.get());
	if(cairoStatus == CAIRO_STATUS_SUCCESS)
	{
		cr = decltype(cr)(cairo_create(sf.get()));
		surfaceWidth = width;
		surfaceHeight = height;
	}
	else
	{
		string message = "Error: ";
		message += cairo_status_to_string(cairoStatus);
		if(renderingText.empty())
			message += " is detected in the allocation of the buffer to draw a text.";
		else
			message += " is detected in the expansion of the buffer to draw the text \"" + renderingText + "\".";
		Files::LogError(message);
		return false;
	}
	sf.reset();
	
	if(pangoLayout)
		pango_cairo_update_layout(cr.get(), pangoLayout.get());
	else
		pangoLayout = decltype(pangoLayout)(pango_cairo_create_layout(cr.get()));
	context = pango_layout_get_context(pangoLayout.get());
	
	pango_layout_set_wrap(pangoLayout.get(), PANGO_WRAP_WORD);
	
	UpdateFont();
	
	return true;
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
	const int fontSize = ToViewportFloorY(pixelSize) * PANGO_SCALE;
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
	viewportFontHeight = PixelFromPangoCeil(ascent + descent);
	metrics.reset();
	fontDesc.reset();
	
	if (drawingSettings.lineHeightScale >= 0.)
		viewportDefaultLineHeight = viewportFontHeight * drawingSettings.lineHeightScale;
	else
		viewportDefaultLineHeight = 0.;
	if (drawingSettings.paragraphBreakScale >= 0.)
		viewportDefaultParagraphBreak = viewportFontHeight * drawingSettings.paragraphBreakScale;
	else
		viewportDefaultParagraphBreak = 0.;
	
	// Tab Stop
	auto tb = MakeUniq(pango_tab_array_new(TOTAL_TAB_STOPS, FALSE), pango_tab_array_free);
	space = WidthInViewport(DisplayText(" ", {}));
	const int tabSize = 4 * space * PANGO_SCALE;
	for(int i = 0; i < TOTAL_TAB_STOPS; ++i)
		pango_tab_array_set_tab(tb.get(), i, PANGO_TAB_LEFT, i * tabSize);
	pango_layout_set_tabs(pangoLayout.get(), tb.get());
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
		viewportWidth = Screen::ViewportWidth();
		viewportHeight = Screen::ViewportHeight();
		
		// Use the view port size as a rough estimation of the RAM size of the VIDEO system.
		// The surface size is larger than the initial size because the surface is never shrunk.
		// UpdateFont() will clear all caches.
		surfaceWidthLimit = viewportWidth * 2;
		surfaceHeightLimit = viewportHeight * 2;
		
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
		GLfloat scale[2] = {2.f / viewportWidth, -2.f / viewportHeight};
		glUniform2fv(scaleI, 1, scale);
		
	}
	
	// Update the center.
	Point center = Point(ToViewportX(x), ToViewportY(y));
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
	
	// Use viewport coodinates in this function.
	const Layout layout = ToViewport(text.GetLayout());
	
	// Truncate
	const int layoutWidth = layout.width < 0 ? -1 : layout.width * PANGO_SCALE;
	pango_layout_set_width(pangoLayout.get(), layoutWidth);
	pango_layout_set_ellipsize(pangoLayout.get(), ToPangoEllipsizeMode(layout.truncate));
	
	// Align and justification
	const auto alignAndJustify = ToPangoAlignmentAndJustify(layout.align);
	pango_layout_set_alignment(pangoLayout.get(), alignAndJustify.first);
	pango_layout_set_justify(pangoLayout.get(), alignAndJustify.second);
	
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
		if(UpdateSurfaceSize(surfaceWidth * ((textWidth / surfaceWidth) + 1),
			surfaceHeight * ((textHeight / surfaceHeight) + 1), text.GetText()))
			return Render(text);
	
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
		int add = max(diffY, static_cast<int>(layout.lineHeight));
		if(index > 0 && layoutText[index - 1] == '\n')
			add += layout.paragraphBreak;
		baselineY += add;
		sumExtraY += add - diffY;
		pango_layout_iter_get_line_extents(iter.get(), nullptr, &logicalRect);
		cairo_move_to(cr.get(), PixelFromPangoCeil(logicalRect.x), baselineY);
		pango_cairo_update_layout(cr.get(), pangoLayout.get());
		line = pango_layout_iter_get_line_readonly(iter.get());
		pango_cairo_show_layout_line(cr.get(), line);
		y0 = y1;
	}
	textHeight += sumExtraY + layout.paragraphBreak;
	if (layout.lineHeight > viewportFontHeight)
		textHeight += layout.lineHeight - viewportFontHeight;
	iter.reset();
	
	// Check this surface has enough height.
	if(surfaceHeight < textHeight)
		if(UpdateSurfaceSize(surfaceWidth, surfaceHeight * ((textHeight / surfaceHeight) + 1), text.GetText()))
			return Render(text);
	
	// In case of the surface size is smaller than the text size because the text is too large to draw.
	textWidth = min(textWidth, surfaceWidth);
	textHeight = min(textHeight, surfaceHeight);
	
	// Copy to image buffer and clear the surface.
	cairo_surface_t *sf = cairo_get_target(cr.get());
	cairo_surface_flush(sf);
	ImageBuffer image;
	image.Allocate(textWidth, textHeight);
	uint32_t *src = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(sf));
	uint32_t *dest = image.Pixels();
	const int stride = surfaceWidth - textWidth;
	if(isLittleEndian)
		MoveCairoSurfaceToImageBufferForLittleEndian(dest, src, textHeight, textWidth, stride);
	else
		MoveCairoSurfaceToImageBufferForBigEndian(dest, src, textHeight, textWidth, stride);
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
		GL_RGBA, GL_UNSIGNED_BYTE, image.Pixels()); // input format, data type, data.
	
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
		"  gl_Position = vec4((center + vert * size) * scale, 0.f, 1.f);\n"
		"  texCoord = vert + vec2(.5, .5);\n"
		"}\n";
	
	static const char *fragmentCode =
		"// fragment font shader\n"
		"precision mediump float;\n"
		// Parameter: Texture with the text.
		"uniform sampler2D tex;\n"
		// Parameter: Color to apply to the text.
		"uniform vec4 color;\n"
		
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
	viewportWidth = 1;
	viewportHeight = 1;
}



int Font::WidthInViewport(const DisplayText &text) const
{
	if(text.GetText().empty())
		return 0;
	
	const RenderedText &renderedText = Render(text);
	if(!renderedText.texture)
		return 0;
	return renderedText.width;
}



// Convert Text coordinates to viewport's, and replace DEFAULT_LINE_HEIGHT and
// DEFAULT_PARAGRAPH_BREAK with the actual value in the viewport coordinates.
Layout Font::ToViewport(const Layout &textLayout) const
{
	Layout viewportLayout = textLayout;
	if(textLayout.width > 0)
		viewportLayout.width = ToViewportNearestX(textLayout.width);
	if(textLayout.lineHeight == Layout::DEFAULT_LINE_HEIGHT)
		viewportLayout.lineHeight = viewportDefaultLineHeight;
	else
		viewportLayout.lineHeight = ToViewportFloorY(textLayout.lineHeight);
	if(textLayout.paragraphBreak == Layout::DEFAULT_PARAGRAPH_BREAK)
		viewportLayout.paragraphBreak = viewportDefaultParagraphBreak;
	else
		viewportLayout.paragraphBreak = ToViewportFloorY(textLayout.paragraphBreak);
	return viewportLayout;
}



double Font::ToViewportX(double textX) const
{
	return textX * viewportWidth / screenWidth;
}



double Font::ToViewportY(double textY) const
{
	return textY * viewportHeight / screenHeight;
}



int Font::ToViewportNearestX(int textX) const
{
	return floor(static_cast<double>(textX * viewportWidth + screenWidth / 2.0) / screenWidth);
}



int Font::ToViewportFloorY(int textY) const
{
	return floor(static_cast<double>(textY * viewportHeight) / screenHeight);
}



int Font::ToTextCeilX(int viewportX) const
{
	return ceil(static_cast<double>(viewportX * screenWidth) / viewportWidth);
}



int Font::ToTextCeilY(int viewportY) const
{
	return ceil(static_cast<double>(viewportY * screenHeight) / viewportHeight);
}
