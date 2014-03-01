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

#include "Color.h"
#include "Font.h"
#include "Point.h"

#include <cassert>
#include <cstring>

using namespace std;



WrappedText::WrappedText()
	: font(nullptr), wrapWidth(1000), alignment(JUSTIFIED), text(nullptr), height(0)
{
}



WrappedText::WrappedText(const Font &font)
	: font(nullptr), wrapWidth(1000), alignment(JUSTIFIED), text(nullptr), height(0)
{
	SetFont(font);
}



WrappedText::~WrappedText()
{
	if(text)
		delete [] text;
}



// Set the alignment mode.
WrappedText::Align WrappedText::Alignment() const
{
	return alignment;
}



void WrappedText::SetAlignment(Align align)
{
	alignment = align;
}



// Set the wrap width. This does not include any margins.
int WrappedText::WrapWidth() const
{
	return wrapWidth;
}



void WrappedText::SetWrapWidth(int width)
{
	wrapWidth = width;
}



// Set the font to use. This will also set sensible defaults for the tab
// width, line height, and paragraph break. You must specify the wrap width
// and the alignment separately.
void WrappedText::SetFont(const Font &font)
{
	this->font = &font;
	
	space = font.Space();
	SetTabWidth(4 * space);
	SetLineHeight(font.Height() * 120 / 100);
	SetParagraphBreak(font.Height() * 40 / 100);
}



// Set the width in pixels of a single '\t' character.
int WrappedText::TabWidth() const
{
	return tabWidth;
}



void WrappedText::SetTabWidth(int width)
{
	tabWidth = width;
}



// Set the height in pixels of one line of text within a paragraph.
int WrappedText::LineHeight() const
{
	return lineHeight;
}



void WrappedText::SetLineHeight(int height)
{
	lineHeight = height;
}



// Set the extra spacing in pixels to be added in between paragraphs.
int WrappedText::ParagraphBreak() const
{
	return paragraphBreak;
}



void WrappedText::SetParagraphBreak(int height)
{
	paragraphBreak = height;
}



// Get the word positions when wrapping the given text. The coordinates
// always begin at (0, 0).
const vector<WrappedText::Word> &WrappedText::Wrap(const string &str)
{
	SetText(str.data(), str.length());
	
	Wrap();
	return words;
}



const vector<WrappedText::Word> &WrappedText::Wrap(const char *str)
{
	SetText(str, strlen(str));
	
	Wrap();
	return words;
}



// Get the words from the most recent wrapping.
const vector<WrappedText::Word> &WrappedText::Words() const
{
	return words;
}



// Get the height of the wrapped text.
int WrappedText::Height() const
{
	return height;
}



// Draw the text.
void WrappedText::Draw(const Point &topLeft, const Color &color) const
{
	for(const Word &w : words)
		font->Draw(w.String(), Point(w.X(), w.Y()) + topLeft, color.Get());
}



const char *WrappedText::Word::String() const
{
	return str;
}



int WrappedText::Word::X() const
{
	return x;
}



int WrappedText::Word::Y() const
{
	return y;
}



void WrappedText::SetText(const char *it, size_t length)
{
	// Clear any previous word-wrapping data. It becomes invalid as soon as the
	// underlying text buffer changes.
	words.clear();
	
	// Reallocate that buffer.
	if(text)
		delete [] text;
	
	text = new char[length + 1];
	memcpy(text, it, length);
	text[length] = '\0';
}



void WrappedText::Wrap()
{
	assert(font);
	
	// Do this as a finite state machine.
	Word word;
	word.x = 0;
	word.y = 0;
	word.str = nullptr;
	
	// Keep track of how wide the current line is. This is just so we know how
	// much extra space must be allotted by the alignment code.
	int lineWidth = 0;
	// This is the index in the "words" vector of the first word on this line.
	unsigned lineBegin = 0;
	
	// TODO: handle single words that are longer than the wrap width. Right now
	// they are simply drawn un-broken, and thus extend beyond the margin.
	// TODO: break words at hyphens, or even do automatic hyphenation. This
	// would require a different format for the buffer, though, because it means
	// inserting '\0' characters even where there is no whitespace.
	
	for(char *it = text; *it; ++it)
	{
		char c = *it;
		
		// Whenever we encounter whitespace, the current word needs wrapping.
		if(c <= ' ' && word.str)
		{
			// Break the string at this point, and measure the word's width.
			*it = '\0';
			int width = font->Width(word.str);
			if(word.x + width > wrapWidth)
			{
				// If we just overflowed the length of the line, this word will
				// be the first on the next line, and the current line needs to
				// be adjusted for alignment.
				word.y += lineHeight;
				word.x = 0;
				
				AdjustLine(lineBegin, lineWidth, false);
			}
			// Store this word, then advance the x position to the end of it.
			words.push_back(word);
			word.x += width;
			// Keep track of how wide this line is now that this word is added.
			lineWidth = word.x;
			// We currently are not inside a word.
			word.str = nullptr;
		}
		
		// If that whitespace was a newline, we must handle that, too.
		if(c == '\n')
		{
			word.y += lineHeight + paragraphBreak;
			word.x = 0;
			word.str = nullptr;
			
			AdjustLine(lineBegin, lineWidth, true);
		}
		// Otherwise, whitespace just adds to the x position.
		else if(c <= ' ')
			word.x += Space(c);
		// If we've reached the start of a new word, remember where it begins.
		else if(!word.str)
			word.str = it;
	}
	// Handle the final word.
	if(word.str)
	{
		words.push_back(word);
		word.y += lineHeight + paragraphBreak;
	}
	AdjustLine(lineBegin, lineWidth, true);
	
	height = word.y;
}



void WrappedText::AdjustLine(unsigned &lineBegin, int &lineWidth, bool isEnd)
{
	int wordCount = words.size() - lineBegin;
	int extraSpace = wrapWidth - lineWidth;
	
	// Figure out how much space is left over. Depending on the alignment, we
	// will add that space to the left, to the right, to both sides, or to the
	// space in between the words. Exception: the last line of a "justified"
	// paragraph is left aligned, not justified.
	if(alignment == JUSTIFIED && !isEnd && wordCount > 1)
	{
		for(int i = 0; i < wordCount; ++i)
			words[lineBegin + i].x += extraSpace * i / (wordCount - 1);
	}
	else if(alignment == CENTER || alignment == RIGHT)
	{
		int shift = (alignment == CENTER) ? extraSpace / 2 : extraSpace;
		for(int i = 0; i < wordCount; ++i)
			words[lineBegin + i].x += shift;
	}
	
	lineBegin = words.size();
	lineWidth = 0;
}



int WrappedText::Space(char c) const
{
	return (c == ' ') ? space : (c == '\t') ? tabWidth : 0;
}
