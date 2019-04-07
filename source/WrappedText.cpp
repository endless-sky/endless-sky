/* WrappedText.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WrappedText.h"

#include "Font.h"

#include <cstring>

using namespace std;



WrappedText::WrappedText()
	: font(nullptr), text(), layout()
{
	layout.align = Font::JUSTIFIED;
	layout.truncate = Font::TRUNC_NONE;
	layout.width = 1000;
	layout.lineHeight = 0;
	layout.paragraphBreak = 0;
}



WrappedText::WrappedText(const Font &font)
	: WrappedText()
{
	SetFont(font);
}



// Set the alignment mode.
Font::Align WrappedText::Alignment() const
{
	return layout.align;
}



void WrappedText::SetAlignment(Font::Align align)
{
	layout.align = align;
}



// Set the truncate mode.
Font::Truncate WrappedText::Truncate() const
{
	return layout.truncate;
}



void WrappedText::SetTruncate(Font::Truncate trunc)
{
	layout.truncate = trunc;
}



// Set the wrap width. This does not include any margins.
int WrappedText::WrapWidth() const
{
	return layout.width;
}



void WrappedText::SetWrapWidth(int width)
{
	layout.width = width;
}



// Set the font to use. This will also set sensible defaults for the tab
// width, line height, and paragraph break. You must specify the wrap width
// and the alignment separately.
void WrappedText::SetFont(const Font &font)
{
	this->font = &font;
	
	SetLineHeight(font.Height() * 112 / 100);
	SetParagraphBreak(font.Height() * 36 / 100);
}



// Set the height in pixels of one line of text within a paragraph.
int WrappedText::LineHeight() const
{
	return layout.lineHeight;
}



void WrappedText::SetLineHeight(int height)
{
	layout.lineHeight = height;
}



// Set the extra spacing in pixels to be added in between paragraphs.
int WrappedText::ParagraphBreak() const
{
	return layout.paragraphBreak;
}



void WrappedText::SetParagraphBreak(int height)
{
	layout.paragraphBreak = height;
}



// Get the word positions when wrapping the given text. The coordinates
// always begin at (0, 0).
void WrappedText::Wrap(const string &str)
{
	text = str;
}



void WrappedText::Wrap(const char *str)
{
	text = str;
}



// Get the height of the wrapped text.
int WrappedText::Height() const
{
	return font->Height(text, &layout);
}



// Draw the text.
void WrappedText::Draw(const Point &topLeft, const Color &color) const
{
	font->Draw(text, topLeft, color, &layout);
}



// Bottom Margin
int WrappedText::BottomMargin() const
{
	return ParagraphBreak() + LineHeight() - font->Height();
}
