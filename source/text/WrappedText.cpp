/* WrappedText.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WrappedText.h"

#include "DisplayText.h"
#include "Font.h"

#include <cstring>

#include "../Logger.h"

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
	hyphen = font.Width("-");
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



/// Get the height of the wrapped text.
/// With trailingBreak, include a paragraph break after the text.
int WrappedText::Height(bool trailingBreak) const
{
	if(!height)
		return 0;
	return height + trailingBreak * paragraphBreak;
}



// Return the width of the longest line of the wrapped text.
int WrappedText::LongestLineWidth() const
{
	return longestLineWidth;
}



// Draw the text.
void WrappedText::Draw(const Point &topLeft, const Color &color) const
{
	if(words.empty())
		return;

	if(truncate == Truncate::NONE)
		for(const Word &w : words)
			font->Draw(w.Value(text), w.Pos() + topLeft, color);
	else
	{
		// Currently, we only apply truncation to a line if it contains a single word.
		int h = words[0].y - 1;
		for(size_t i = 0; i < words.size(); ++i)
		{
			const Word &w = words[i];
			if(h == w.y && (i != words.size() - 1 && w.y == words[i + 1].y))
				font->Draw(w.Value(text), w.Pos() + topLeft, color);
			else
				font->Draw({w.Value(text), {wrapWidth, truncate}}, w.Pos() + topLeft, color);
			h = w.y;
		}
	}
}



string WrappedText::Word::Value(const string &text) const
{
	return text.substr(index, length) + suffix;
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
	longestLineWidth = 0;

	if(text.empty() || !font)
		return;

	// Do this as a finite state machine.
	Word word;
	bool traversingWord = false;

	// Keep track of how wide the current line is. This is just so we know how
	// much extra space must be allotted by the alignment code.
	int lineWidth = 0;
	// This is the index in the "words" vector of the first word on this line.
	size_t lineBegin = 0;

	// TODO: break words at hyphens, or even do automatic hyphenation.

	bool done = false;
	string::iterator it = text.begin();
	while(!done)
	{
		char c;
		char cLast = '\0';
		if(it == text.end())
		{
			c = '\n';
			if(cLast == '\n')
				word.y -= lineHeight + paragraphBreak;
			done = true;
		}
		else
			c = *it;

		// Whitespace signals a word end - mark it and wrap the text if needed.
		if(c <= ' ' && traversingWord)
		{
			traversingWord = false;
			// Break the string at this point, and measure the word's width.
			// It may take more than one attempt to break extremely long words.
			bool breakingWord = true;
			while(breakingWord)
			{
				word.length = (it - text.begin()) - word.index;
				int width = font->Width(word.Value(text));
				// If adding this word would overflow the length of the line,
				// this word will be the first on the next line.
				// However, if this word itself is too long to fit on it's own
				// line: we have no choice but to break the word.
				if(width > wrapWidth)
				{
					// How much space is left on the current line?
					int chunkWidth = wrapWidth - lineWidth - hyphen;
					// When we break the word, we may be able to do so in such a
					// way that a part of it will fit on this line afterall, and
					// the remainder on the next line or lines.
					int numChars = BreakWord(word, chunkWidth);
					if(numChars)
					{
						// First portion will fit on this line, with a hyphen.
						word.length = numChars;
						word.suffix = '-';
						words.push_back(word);
						word.suffix.clear();
						word.index += word.length;
						word.length = (it - text.begin()) - word.index;
						word.x += chunkWidth;
						// Keep track of how wide this line is now that this word is added.
						lineWidth = word.x + hyphen;

						// Then we need to place the remainder of the word on the next line.
						word.y += lineHeight;
						word.x = 0;

						// Adjust the spacing of words in the now-complete line.
						AdjustLine(lineBegin, lineWidth, false);
					}
					else
					{
						// There is not enough room to reasonably put any part of the
						// word on the existing line, move the problem to the next line.
						word.y += lineHeight;
						word.x = 0;

						// Adjust the spacing of words in the now-complete line.
						AdjustLine(lineBegin, lineWidth, false);
					}
				}
				else if(word.x + width > wrapWidth)
				{
					// If adding this word would overflow the length of the line,
					// this word will be the first on the next line.
					word.y += lineHeight;
					word.x = 0;

					// Adjust the spacing of words in the now-complete line.
					AdjustLine(lineBegin, lineWidth, false);

					breakingWord = false;
					// Store this word, then advance the x position to the end of it.
					words.push_back(word);
					word.x += width;
					// Keep track of how wide this line is now that this word is added.
					lineWidth = word.x;
				}
				else
				{
					breakingWord = false;
					// Store this word, then advance the x position to the end of it.
					words.push_back(word);
					word.x += width;
					// Keep track of how wide this line is now that this word is added.
					lineWidth = word.x;
				}
			}
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
		else if(!traversingWord)
		{
			traversingWord = true;
			word.index = it - text.begin();
		}
		cLast = c;
		++it;
	}
	height = max(0, word.y - paragraphBreak);
}



void WrappedText::AdjustLine(size_t &lineBegin, int &lineWidth, bool isEnd)
{
	int wordCount = static_cast<int>(words.size() - lineBegin);
	int extraSpace = wrapWidth - lineWidth;

	if(lineWidth > longestLineWidth)
		longestLineWidth = lineWidth;

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



// Given `chunkWidth` pixels, determine where to split `word`, if possible.
int WrappedText::BreakWord(const WrappedText::Word &word, int &chunkWidth)
{
	Word temp;
	temp.index = word.index;
	temp.length = word.length;
	int width = font->Width(temp.Value(text));
	while(width > chunkWidth && temp.length > 0)
	{
		--temp.length;
		width = font->Width(temp.Value(text));
	}
	chunkWidth = width;
	// Note: there should be a minimum enforced wrap width. Here we expect at lesat two char with a hyphen to fit.
	return temp.length > 2 ? temp.length : 0;
}



int WrappedText::Space(char c) const
{
	return (c == ' ') ? space : (c == '\t') ? tabWidth : 0;
}
