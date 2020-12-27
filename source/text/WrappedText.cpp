/* WrappedText.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WrappedText.h"

#include "DisplayText.h"
#include "Font.h"

#include <cstring>

using namespace std;



WrappedText::WrappedText(const Font &font)
{
	SetFont(font);
}



void WrappedText::SetAlignment(Alignment align)
{
	alignment = align;
}



// Set the truncate mode.
void WrappedText::SetTruncate(Truncate trunc)
{
	truncate = trunc;
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
void WrappedText::Wrap(const string &str)
{
	SetText(str.data(), str.length());
	
	Wrap();
}



void WrappedText::Wrap(const char *str)
{
	SetText(str, strlen(str));
	
	Wrap();
}



// Get the height of the wrapped text.
int WrappedText::Height() const
{
	return height;
}



// Draw the text.
void WrappedText::Draw(const Point &topLeft, const Color &color) const
{
	if(words.empty())
		return;
	
	if(truncate == Truncate::NONE)
		for(const Word &w : words)
			font->Draw(text.c_str() + w.Index(), w.Pos() + topLeft, color);
	else
	{
		// Currently, we only apply truncation to a line if it contains a single word.
		int h = words[0].y - 1;
		for(size_t i = 0; i < words.size(); ++i)
		{
			const Word &w = words[i];
			if(h == w.y && (i != words.size() - 1 && w.y == words[i+1].y))
				font->Draw(text.c_str() + w.Index(), w.Pos() + topLeft, color);
			else
				font->Draw({text.c_str() + w.Index(), {wrapWidth, truncate}}, w.Pos() + topLeft, color);
			h = w.y;
		}
	}
}



size_t WrappedText::Word::Index() const
{
	return index;
}



Point WrappedText::Word::Pos() const
{
	return Point(x, y);
}



void WrappedText::SetText(const char *it, size_t length)
{
	// Clear any previous word-wrapping data. It becomes invalid as soon as the
	// underlying text buffer changes.
	words.clear();
	
	// Reallocate that buffer.
	text.assign(it, length);
}



void WrappedText::Wrap()
{
	height = 0;
	if(text.empty() || !font)
		return;
	
	// Do this as a finite state machine.
	Word word;
	bool hasWord = false;
	
	// Keep track of how wide the current line is. This is just so we know how
	// much extra space must be allotted by the alignment code.
	int lineWidth = 0;
	// This is the index in the "words" vector of the first word on this line.
	size_t lineBegin = 0;
	
	// TODO: handle single words that are longer than the wrap width. Right now
	// they are simply drawn un-broken, and thus extend beyond the margin.
	// TODO: break words at hyphens, or even do automatic hyphenation. This
	// would require a different format for the buffer, though, because it means
	// inserting '\0' characters even where there is no whitespace.
	
	for(string::iterator it = text.begin(); it != text.end(); ++it)
	{
		char c = *it;
		
		// Whenever we encounter whitespace, the current word needs wrapping.
		if(c <= ' ' && hasWord)
		{
			// Break the string at this point, and measure the word's width.
			*it = '\0';
			int width = font->Width(text.c_str() + word.index);
			if(word.x + width > wrapWidth)
			{
				// If adding this word would overflow the length of the line, this
				// word will be the first on the next line.
				word.y += lineHeight;
				word.x = 0;
				
				// Adjust the spacing of words in the now-complete line.
				AdjustLine(lineBegin, lineWidth, false);
			}
			// Store this word, then advance the x position to the end of it.
			words.push_back(word);
			word.x += width;
			// Keep track of how wide this line is now that this word is added.
			lineWidth = word.x;
			// We currently are not inside a word.
			hasWord = false;
		}
		
		// If that whitespace was a newline, we must handle that, too.
		if(c == '\n')
		{
			// The next word will begin on a new line.
			word.y += lineHeight + paragraphBreak;
			word.x = 0;
			
			// Adjust the word spacings on the now-completed line.
			AdjustLine(lineBegin, lineWidth, true);
		}
		// Otherwise, whitespace just adds to the x position.
		else if(c <= ' ')
			word.x += Space(c);
		// If we've reached the start of a new word, remember where it begins.
		else if(!hasWord)
		{
			hasWord = true;
			word.index = it - text.begin();
		}
	}
	// Handle the final word.
	if(hasWord)
	{
		int width = font->Width(text.c_str() + word.index);
		if(word.x + width > wrapWidth)
		{
			// If adding this word would overflow the length of the line, this
			// final word will be the first (and only) on the next line.
			word.y += lineHeight;
			word.x = 0;
			
			// Adjust the spacing of words in the now-complete line.
			AdjustLine(lineBegin, lineWidth, false);
		}
		// Add this final word to the existing words.
		words.push_back(word);
		word.y += lineHeight + paragraphBreak;
		// Keep track of how wide this line is now that this word is added.
		word.x += width;
		lineWidth = word.x;
	}
	// Adjust the spacing of words in the final line of text.
	AdjustLine(lineBegin, lineWidth, true);
	
	height = word.y;
}



void WrappedText::AdjustLine(size_t &lineBegin, int &lineWidth, bool isEnd)
{
	int wordCount = static_cast<int>(words.size() - lineBegin);
	int extraSpace = wrapWidth - lineWidth;
	
	// Figure out how much space is left over. Depending on the alignment, we
	// will add that space to the left, to the right, to both sides, or to the
	// space in between the words. Exception: the last line of a "justified"
	// paragraph is left aligned, not justified.
	if(alignment == Alignment::JUSTIFIED && !isEnd && wordCount > 1)
	{
		for(int i = 0; i < wordCount; ++i)
			words[lineBegin + i].x += extraSpace * i / (wordCount - 1);
	}
	else if(alignment == Alignment::CENTER || alignment == Alignment::RIGHT)
	{
		int shift = (alignment == Alignment::CENTER) ? extraSpace / 2 : extraSpace;
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
