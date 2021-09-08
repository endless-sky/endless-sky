/* Font.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/text/Font.h"

#include "../../../source/text/alignment.hpp"
#include "../../../source/Color.h"
#include "../../../source/text/DisplayText.h"
#include "../../../source/ImageBuffer.h"
#include "../../../source/Point.h"
#include "../../../source/Screen.h"
#include "../../../source/text/truncate.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {
	const int KERN = 2;
}



Font::Font(const string &imagePath)
{
	Load(imagePath);
}



void Font::Load(const string &imagePath)
{
	static_cast<void>(shader);
	static_cast<void>(texture);
	static_cast<void>(vao);
	static_cast<void>(vbo);
	static_cast<void>(colorI);
	static_cast<void>(scaleI);
	static_cast<void>(glyphI);
	static_cast<void>(aspectI);
	static_cast<void>(positionI);
	static_cast<void>(screenWidth);
	static_cast<void>(screenHeight);
	widthEllipses = WidthRawString("...");
}



void Font::Draw(const DisplayText &text, const Point &point, const Color &color) const
{
}



void Font::DrawAliased(const DisplayText &text, double x, double y, const Color &color) const
{
}



void Font::Draw(const string &str, const Point &point, const Color &color) const
{
}



void Font::DrawAliased(const string &str, double x, double y, const Color &color) const
{
}



int Font::Width(const string &str, char after) const
{
	return WidthRawString(str.c_str(), after);
}



int Font::FormattedWidth(const DisplayText &text, char after) const
{
	int width = -1;
	const string truncText = TruncateText(text, width);
	return width < 0 ? WidthRawString(truncText.c_str(), after) : width;
}



int Font::Height() const noexcept
{
	return height;
}



int Font::Space() const noexcept
{
	return space;
}



void Font::ShowUnderlines(bool show) noexcept
{
}



int Font::Glyph(char c, bool isAfterSpace) noexcept
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
}



void Font::CalculateAdvances(ImageBuffer &image)
{
}



void Font::SetUpShader(float glyphW, float glyphH)
{
}



int Font::WidthRawString(const char *str, char after) const noexcept
{
	int width = 0;
	int previous = 0;
	bool isAfterSpace = true;

	for( ; *str; ++str)
	{
		if(*str == '_')
			continue;

		int glyph = Glyph(*str, isAfterSpace);
		if(*str != '"' && *str != '\'')
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



// Param width will be set to the width of the return value, unless the layout width is negative.
string Font::TruncateText(const DisplayText &text, int &width) const
{
	width = -1;
	const auto &layout = text.GetLayout();
	const string &str = text.GetText();
	if(layout.width < 0 || (layout.align == Alignment::LEFT && layout.truncate == Truncate::NONE))
		return str;
	width = layout.width;
	switch(layout.truncate)
	{
		case Truncate::NONE:
			width = WidthRawString(str.c_str());
			return str;
		case Truncate::FRONT:
			return TruncateFront(str, width);
		case Truncate::MIDDLE:
			return TruncateMiddle(str, width);
		case Truncate::BACK:
		default:
			return TruncateBack(str, width);
	}
}



string Font::TruncateBack(const string &str, int &width) const
{
	int firstWidth = WidthRawString(str.c_str());
	if(firstWidth <= width)
	{
		width = firstWidth;
		return str;
	}

	int prevChars = str.size();
	int prevWidth = firstWidth;

	width -= widthEllipses;
	// As a safety against infinite loops (even though they won't be possible if
	// this implementation is correct) limit the number of loops to the number
	// of characters in the string.
	for(size_t i = 0; i < str.length(); ++i)
	{
		// Loop until the previous width we tried was too long and this one is
		// too short, or vice versa. Each time, the next string length we try is
		// interpolated from the previous width.
		int nextChars = (prevChars * width) / prevWidth;
		bool isSame = (nextChars == prevChars);
		bool prevWorks = (prevWidth <= width);
		nextChars += (prevWorks ? isSame : -isSame);

		int nextWidth = WidthRawString(str.substr(0, nextChars).c_str(), '.');
		bool nextWorks = (nextWidth <= width);
		if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
		{
			if(prevWorks)
			{
				width = prevWidth + widthEllipses;
				return str.substr(0, prevChars) + "...";
			}
			else
			{
				width = nextWidth + widthEllipses;
				return str.substr(0, nextChars) + "...";
			}
		}

		prevChars = nextChars;
		prevWidth = nextWidth;
	}
	width = firstWidth;
	return str;
}



string Font::TruncateFront(const string &str, int &width) const
{
	int firstWidth = WidthRawString(str.c_str());
	if(firstWidth <= width)
	{
		width = firstWidth;
		return str;
	}

	int prevChars = str.size();
	int prevWidth = firstWidth;

	width -= widthEllipses;
	// As a safety against infinite loops (even though they won't be possible if
	// this implementation is correct) limit the number of loops to the number
	// of characters in the string.
	for(size_t i = 0; i < str.length(); ++i)
	{
		// Loop until the previous width we tried was too long and this one is
		// too short, or vice versa. Each time, the next string length we try is
		// interpolated from the previous width.
		int nextChars = (prevChars * width) / prevWidth;
		bool isSame = (nextChars == prevChars);
		bool prevWorks = (prevWidth <= width);
		nextChars += (prevWorks ? isSame : -isSame);

		int nextWidth = WidthRawString(str.substr(str.size() - nextChars).c_str());
		bool nextWorks = (nextWidth <= width);
		if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
		{
			if(prevWorks)
			{
				width = prevWidth + widthEllipses;
				return "..." + str.substr(str.size() - prevChars);
			}
			else
			{
				width = nextWidth + widthEllipses;
				return "..." + str.substr(str.size() - nextChars);
			}
		}

		prevChars = nextChars;
		prevWidth = nextWidth;
	}
	width = firstWidth;
	return str;
}



string Font::TruncateMiddle(const string &str, int &width) const
{
	int firstWidth = WidthRawString(str.c_str());
	if(firstWidth <= width)
	{
		width = firstWidth;
		return str;
	}

	int prevChars = str.size();
	int prevWidth = firstWidth;

	width -= widthEllipses;
	// As a safety against infinite loops (even though they won't be possible if
	// this implementation is correct), limit the number of loops to the number
	// of characters in the string.
	for(size_t i = 0; i < str.length(); ++i)
	{
		// Loop until the previous width we tried was too long and this one is
		// too short, or vice versa. Each time, the next string length we try is
		// interpolated from the previous width.
		int nextChars = (prevChars * width) / prevWidth;
		bool isSame = (nextChars == prevChars);
		bool prevWorks = (prevWidth <= width);
		nextChars += (prevWorks ? isSame : -isSame);

		int leftChars = nextChars / 2;
		int rightChars = nextChars - leftChars;
		int nextWidth = WidthRawString((str.substr(0, leftChars) + str.substr(str.size() - rightChars)).c_str(), '.');
		bool nextWorks = (nextWidth <= width);
		if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
		{
			if(prevWorks)
			{
				leftChars = prevChars / 2;
				rightChars = prevChars - leftChars;
				width = prevWidth + widthEllipses;
			}
			else
				width = nextWidth + widthEllipses;
			return str.substr(0, leftChars) + "..." + str.substr(str.size() - rightChars);
		}

		prevChars = nextChars;
		prevWidth = nextWidth;
	}
	width = firstWidth;
	return str;
}
