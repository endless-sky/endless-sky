/* FreeTypeGlyphs.cpp
Copyright (c) 2018 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FreeTypeGlyphs.h"

#include "Color.h"
#include "Files.h"
#include "ImageBuffer.h"
#include "Screen.h"
#include "Sprite.h"

#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

// The FreeType library performs subpixel floating point calculations with integer logic.
// The most important formats are:
//   26.6 fractional format - 64 is 1 pixel (26 integer bits, 6 fractional bits)
//   16.16 fractional format - 65536 is 1 pixel (16 integer bits, 16 fractional bits)
//
// During ship movement the planet name and the government name will move along with the planet.
// Using hinting to shape the text would make some of the glyphs jump around as the text moved.
// To avoid this problem the text is shaped at subpixel positions with hinting disabled.
// Auto-hinting is enabled during render to make the glyphs look sharper.

namespace {
	void LogError(const std::string &str, FT_Error error = FT_Err_Ok)
	{
		if(error == FT_Err_Ok)
			Files::LogError(str);
		else
		{
			static const char *errorMsg[] = {
#undef FTERRORS_H_
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s ) s,
#include FT_ERRORS_H
				nullptr
			};
			int len = sizeof(errorMsg)/sizeof(errorMsg[0]) - 1;
			string buf = str + ": ";
			buf += (error >= 0 && error < len) ? errorMsg[error] : "???";
			buf += " (" + to_string(error) + ")";
			Files::LogError(buf);
		}
	}
	
	// 26.6 fractional format.
	inline double From26Dot6(FT_Long x)
	{
		return x / 64.;
	}
	
	inline FT_Long To26Dot6(double x)
	{
		return x * 64.;
	}
	
	// 16.16 fractional format.
	inline double From16Dot16(FT_Long x)
	{
		return x / 65536.;
	}
	
	inline FT_Long To16Dot16(double x)
	{
		return x * 65536.;
	}
	
	// User data for RasterFunc.
	struct RasterBuffer {
		ImageBuffer image;
		int left;
		int top;
	};
	
	// Called by FT_Outline_Render to update image pixels.
	// When outline pixels overlap, it will keep the biggest coverage value.
	void RasterFunc(int y, int count, const FT_Span *spans, void *user)
	{
		RasterBuffer *buffer = static_cast<RasterBuffer *>(user);
		int row = buffer->top - y;
		if(row < 0 || row >= buffer->image.Height())
		{
			LogError("RasterFunc: row out of range");
			return;
		}
		for(int i = 0; i < count; ++i)
		{
			int col = spans[i].x - buffer->left;
			int len = spans[i].len;
			if(col < 0 || len < 0 || col + len >= buffer->image.Width())
			{
				LogError("RasterFunc: col out of range");
				continue;
			}
			uint32_t color = 0x01010101 * spans[i].coverage;
			uint32_t *pixel = buffer->image.Begin(row) + col;
			for(int j = 0; j < len; ++j)
				pixel[j] = max(pixel[j], color);
		}
	}
}



FreeTypeGlyphs::FreeTypeGlyphs()
	: vao(0), vbo(0), scaleI(0), centerI(0), sizeI(0), colorI(0),
	  baseline(0.), space(0.),
	  underscoreIndex(0), library(nullptr), face(nullptr),
	  screenWidth(0), screenHeight(0), timestamp(time(nullptr))
{
}



FreeTypeGlyphs::~FreeTypeGlyphs()
{
	if(face)
		FT_Done_Face(face);
	
	if(library)
		FT_Done_FreeType(library);
}



bool FreeTypeGlyphs::Load(const string &path, int size)
{
	FT_Error error;
	
	// Ignore if already loaded.
	if(face)
		return false;
	
	// Load library.
	if(!library)
	{
		error = FT_Init_FreeType(&library);
		if(error != FT_Err_Ok)
		{
			LogError("FT_Init_FreeType", error);
			return false;
		}
	}
	
	// Load font face.
	error = FT_New_Face(library, path.c_str(), 0, &face);
	if(error != FT_Err_Ok)
	{
		LogError("FT_New_Face(\"" + path + "\")", error);
		return false;
	}
	
	// Load the requested size at 72 dpi.
	error = FT_Set_Char_Size(face, size << 6, 0, 72, 0);
	if(error != FT_Err_Ok)
	{
		LogError("FT_Set_Char_Size(" + to_string(size) + ")", error);
		FT_Done_Face(face);
		face = nullptr;
		return false;
	}
	
	// Validate the face:
	//  - must have a unicode charmap
	//  - must be scalable (bitmaps are not being handled)
	//  - must not be a tricky font (needs testing, might require hinting while shaping)
	//  - must support horizontal layout (until vertical layout is supported)
	bool valid = false;
	// By default it tries to load a 32-bit unicode charmap,
	// failing that it tries to load any unicode charmap (16-bit),
	// failing that it sets charmap to null.
	if(!face->charmap)
		LogError("\"" + path + "\" does not have a unicode charmap.");
	else if(!FT_IS_SCALABLE(face))
		LogError("\"" + path + "\" is not a scalable font.");
	else if(FT_IS_TRICKY(face))
		LogError("\"" + path + "\" is a tricky font.");
	else if(!FT_HAS_HORIZONTAL(face))
		LogError("\"" + path + "\" does not support horizontal layout.");
	else
		valid = true;
	if(!valid)
	{
		FT_Done_Face(face);
		face = nullptr;
		return false;
	}
	
	// Center the letter 'x' vertically in the line, rounded to the nearest pixel.
	baseline = .5 * From26Dot6(face->size->metrics.height);
	error = FT_Load_Char(face, 'x', FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP);
	if(error != FT_Err_Ok)
		LogError("FT_Load_Char('x')", error);
	else
	{
		FT_BBox bounds;
		FT_Outline_Get_CBox(&face->glyph->outline, &bounds);
		baseline += .5 * From26Dot6(bounds.yMax - bounds.yMin);
	}
	baseline = round(baseline);
	
	// Get the glyph index of an underscore for underlines.
	underscoreIndex = FT_Get_Char_Index(face, '_');
	
	// Get the advance of a space, rounded to the next pixel.
	space = 0.;
	error = FT_Load_Char(face, ' ', FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP);
	if(error != FT_Err_Ok)
		LogError("FT_Load_Char(' ')", error);
	else
		space = ceil(From16Dot16(face->glyph->linearHoriAdvance));
	
	return true;
}



void FreeTypeGlyphs::Draw(const string &str, double x, double y, const Color &color) const
{
	if(!face)
		return;
	
	y += baseline;
	RenderedText &text = Render(str, x, y, Font::ShowUnderlines());
	if(!text.sprite)
		return;
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	// Update the texture.
	glBindTexture(GL_TEXTURE_2D_ARRAY, text.sprite->Texture());
	
	// Update the scale, only if the screen size has changed.
	if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		GLfloat scale[2] = { 2.f / screenWidth, -2.f / screenHeight };
		glUniform2fv(scaleI, 1, scale);
	}
	
	// Update the center.
	Point center = Point(floor(x), floor(y)) + text.offset;
	glUniform2f(centerI, center.X(), center.Y());
	
	// Update the size.
	glUniform2f(sizeI, text.sprite->Width(), text.sprite->Height());
	
	// Update the color.
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	
	// Once per minute erase from the cache text that has been unused for 10 minutes.
	time_t now = time(nullptr);
	if(difftime(now, timestamp) >= 60.)
	{
		for(auto it = cache.begin(); it != cache.end(); )
			if(difftime(now, it->second.timestamp) >= 600.)
				it = cache.erase(it);
			else
				++it;
		timestamp = now;
	}
}



double FreeTypeGlyphs::Width(const string &str) const
{
	if(!face)
		return 0.;
	
	vector<GlyphData> arr = Translate(str + "_");
	Shape(arr, 0., 0.);
	return abs(arr.back().position.X());
}



double FreeTypeGlyphs::LineHeight() const
{
	if(!face)
		return 0.;
	
	return From26Dot6(face->size->metrics.height);
}



double FreeTypeGlyphs::Space() const
{
	return space;
}



size_t FreeTypeGlyphs::FindUnsupported(const string &str, size_t pos) const
{
	if(!face)
		return 0;
	
	vector<GlyphData> arr = Translate(str);
	for(GlyphData &data : arr)
		if(!data.index)
			return data.start;
	
	return str.length();
}



// Translate a string to glyphs.
vector<FreeTypeGlyphs::GlyphData> FreeTypeGlyphs::Translate(const string &str) const
{
	const bool hasVariantSelectors = FT_Face_GetVariantSelectors(face);
	vector<GlyphData> arr;
	for(size_t pos = 0; pos < str.length(); pos = Font::NextCodePoint(str, pos))
	{
		char32_t c = Font::DecodeCodePoint(str, pos);
		
		GlyphData data = {};
		data.start = pos;
		
		// Get the variant.
		if(hasVariantSelectors)
		{
			size_t next = Font::NextCodePoint(str, pos);
			if(next < str.length())
			{
				char32_t nextC = Font::DecodeCodePoint(str, next);
				data.index = FT_Face_GetCharVariantIndex(face, c, nextC);
				if(data.index)
					pos = next;
			}
		}
		
		// Get the glyph.
		if(!data.index)
			data.index = FT_Get_Char_Index(face, c);
		
		arr.push_back(data);
	}
	return arr;
}



// Shape the data, recording the position of each glyph.
void FreeTypeGlyphs::Shape(vector<GlyphData> &arr, double x, double y) const
{
	const bool hasKerning = FT_HAS_KERNING(face);
	Point origin = Point(floor(x), floor(y));
	FT_Vector pen; // 16.16
	pen.x = To16Dot16(x - origin.X());
	pen.y = To16Dot16(y - origin.Y());
	FT_Error error;
	FT_UInt prevIndex = 0;
	for(GlyphData &data : arr)
	{
		// Underscores apply the underline style to the next visible character.
		if(underscoreIndex && underscoreIndex == data.index)
		{
			data.position = origin + Point(From16Dot16(pen.x), From16Dot16(pen.y));
			continue;
		}
		
		// Apply kerning.
		if(hasKerning && prevIndex && data.index)
		{
			FT_Vector kerning = {}; // 26.6
			error = FT_Get_Kerning(face, prevIndex, data.index, FT_KERNING_UNFITTED, &kerning);
			if(error)
				LogError("FT_Get_Kerning", error);
			
			// Adjust pen.
			if(kerning.x || kerning.y)
			{
				pen.x += kerning.x << 10;
				pen.y += kerning.y << 10;
			}
		}
		
		data.position = origin + Point(From16Dot16(pen.x), From16Dot16(pen.y));
		
		// Advance pen.
		error = FT_Load_Glyph(face, data.index, FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP);
		if(error != FT_Err_Ok)
			LogError("FT_Load_Glyph", error);
		else
			pen.x += face->glyph->linearHoriAdvance;
		
		prevIndex = data.index;
	}
}



// Render the text, caching the result for some time.
FreeTypeGlyphs::RenderedText &FreeTypeGlyphs::Render(const string &str, double x, double y, bool showUnderlines) const
{
	FT_Error error;
	
	if(showUnderlines && str.find('_') == string::npos)
		showUnderlines = false;
	
	time_t timestamp = time(nullptr);
	FT_Vector origin = { To26Dot6(x - floor(x)), To26Dot6(ceil(y) - y) };
	
	// Return if already cached.
	CacheKey key = make_pair(str, origin.x + (origin.y << 6) + (showUnderlines << 12));
	auto it = cache.find(key);
	if(it != cache.end())
	{
		it->second.timestamp = timestamp;
		return it->second;
	}
	
	// Shape the text.
	vector<GlyphData> arr = Translate(str);
	Shape(arr, From26Dot6(origin.x), From26Dot6(origin.y));
	
	FT_BBox bounds;
	bounds.xMin = LONG_MAX;
	bounds.yMin = LONG_MAX;
	bounds.xMax = LONG_MIN;
	bounds.yMax = LONG_MIN;
	
	FT_GlyphSlot slot = face->glyph;
	vector<FT_Glyph> glyphs;
	vector<tuple<size_t,FT_Glyph> > underlines;
	
	// Load auto-hinted outlines at the target positions.
	for(GlyphData &data : arr)
	{
		FT_Vector delta = { To26Dot6(data.position.X()), To26Dot6(data.position.Y()) };
		
		// Get a copy of the glyph, which must be a non-empty outline.
		FT_Glyph glyph;
		FT_Set_Transform(face, nullptr, &delta);
		error = FT_Load_Glyph(face, data.index, FT_LOAD_FORCE_AUTOHINT | FT_LOAD_NO_BITMAP);
		if(error != FT_Err_Ok)
		{
			LogError("FT_Load_Glyph", error);
			continue;
		}
		if(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			if(face->glyph->outline.n_contours <= 0 || face->glyph->outline.n_points <= 0)
				continue;
		}
		else
			continue;
		error = FT_Get_Glyph(slot, &glyph);
		if(error != FT_Err_Ok)
		{
			LogError("FT_Get_Glyph", error);
			continue;
		}
		
		FT_BBox glyphBounds;
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &glyphBounds);
		bounds.yMin = min(bounds.yMin, glyphBounds.yMin);
		bounds.yMax = max(bounds.yMax, glyphBounds.yMax);
		
		// Underscores are placed below the next visible
		// glyph with a matching width and horizontal position.
		if(underscoreIndex && underscoreIndex == data.index)
		{
			if(showUnderlines && (underlines.empty() || get<0>(underlines.back()) != glyphs.size()))
				underlines.emplace_back(glyphs.size(), glyph);
			else
				FT_Done_Glyph(glyph);
			continue;
		}
		
		bounds.xMin = min(bounds.xMin, glyphBounds.xMin);
		bounds.xMax = max(bounds.xMax, glyphBounds.xMax);
		
		glyphs.push_back(glyph);
	}
	FT_Set_Transform(face, nullptr, nullptr);
	
	// Render the text.
	int width = bounds.xMax - bounds.xMin + 1;
	int height = bounds.yMax - bounds.yMin + 1;
	
	RasterBuffer buffer = {};
	buffer.image.Allocate(width, height);
	buffer.left = bounds.xMin;
	buffer.top = bounds.yMax;
	fill_n(buffer.image.Pixels(), buffer.image.Width() * buffer.image.Height(), 0);
	
	FT_Raster_Params rasterParams = {};
	rasterParams.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
	rasterParams.gray_spans = RasterFunc;
	rasterParams.user = &buffer;
	
	for(FT_Glyph glyph : glyphs)
	{
		// Render to image.
		FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
		error = FT_Outline_Render(library, &outlineGlyph->outline, &rasterParams);
		if(error != FT_Err_Ok)
			LogError("FT_Outline_Render", error);
	}
	
	// Render the underlines.
	for(auto &underline : underlines)
	{
		size_t idx = get<0>(underline);
		FT_Glyph underscoreGlyph = get<1>(underline);
		if(idx >= glyphs.size())
			continue;
		
		FT_Glyph targetGlyph = glyphs[idx];
		
		// Match width and horizontal position.
		FT_BBox target; // 26.6
		FT_BBox current; // 26.6
		FT_Glyph_Get_CBox(targetGlyph, FT_GLYPH_BBOX_SUBPIXELS, &target);
		FT_Glyph_Get_CBox(underscoreGlyph, FT_GLYPH_BBOX_SUBPIXELS, &current);
		FT_Matrix matrix = {}; // 16.16
		matrix.xx = FT_DivFix(target.xMax - target.xMin, current.xMax - current.xMin);
		matrix.yy = 0x10000L;
		FT_Vector delta = {}; // 26.6
		delta.x = target.xMin - FT_MulFix(current.xMin, matrix.xx);
		FT_Glyph_Transform(underscoreGlyph, &matrix, &delta);
		
		// Render to image.
		FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(underscoreGlyph);
		error = FT_Outline_Render(library, &outlineGlyph->outline, &rasterParams);
		if(error != FT_Err_Ok)
			LogError("FT_Outline_Render('_')", error);
	}
	
	// Cleanup.
	for(FT_Glyph glyph : glyphs)
		FT_Done_Glyph(glyph);
	glyphs.clear();
	for(auto &underline : underlines)
		FT_Done_Glyph(get<1>(underline));
	underlines.clear();
	
	// Record rendered text.
	RenderedText &text = cache[key];
	text.sprite = make_shared<Sprite>(str);
	text.offset.X() = .5 * buffer.image.Width() + bounds.xMin;
	text.offset.Y() = .5 * buffer.image.Height() - bounds.yMax;
	text.timestamp = timestamp;
	text.sprite->AddFrames(buffer.image, false);
	return text;
}



void FreeTypeGlyphs::SetUpShader()
{
	static const char *vertexCode =
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
		// Parameter: Texture array with the text in frame 0.
		"uniform sampler2DArray tex;\n"
		// Parameter: Color to apply to the text.
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		
		// Input: Texture coordinate from the vertex shader.
		"in vec2 texCoord;\n"
		
		// Output: Color for the screen.
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  finalColor = color * texture(tex, vec3(texCoord, 0));\n"
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
	screenWidth = 0;
	screenHeight = 0;
}
