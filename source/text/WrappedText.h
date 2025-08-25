/* WrappedText.h
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

#pragma once

#include "Alignment.h"
#include "../Point.h"
#include "Truncate.h"

#include <string>
#include <vector>

class Color;
class Font;



// Class for calculating word positions in wrapped text. You can specify various
// parameters of the formatting, including text alignment.
class WrappedText {
public:
	WrappedText() = default;
	explicit WrappedText(const Font &font);

	// Set the alignment mode.
	void SetAlignment(Alignment align);

	// Set the truncate mode.
	// Apply the truncation to a word only if a line has a single word.
	void SetTruncate(Truncate trunc);

	// Set the wrap width. This does not include any margins.
	int WrapWidth() const;
	void SetWrapWidth(int width);

	// Set the font to use. This will also set sensible defaults for the tab
	// width, line height, and paragraph break. You must specify the wrap width
	// and the alignment separately.
	void SetFont(const Font &font);

	// Set the width in pixels of a single '\t' character.
	int TabWidth() const;
	void SetTabWidth(int width);

	// Set the height in pixels of one line of text within a paragraph.
	int LineHeight() const;
	void SetLineHeight(int height);

	// Set the extra spacing in pixels to be added in between paragraphs.
	int ParagraphBreak() const;
	void SetParagraphBreak(int height);

	// Wrap the given text. Use Draw() to draw it.
	void Wrap(const std::string &str);
	void Wrap(const char *str);

	/// Get the height of the wrapped text.
	/// With trailingBreak, include a paragraph break after the text.
	int Height(bool trailingBreak = true) const;

	// Return the width of the longest line of the wrapped text.
	int LongestLineWidth() const;

	// Draw the text.
	void Draw(const Point &topLeft, const Color &color) const;


private:
	void SetText(const char *it, size_t length);
	void Wrap();
	void AdjustLine(size_t &lineBegin, int &lineWidth, bool isEnd);
	int Space(char c) const;


private:
	// The returned text is a series of words and (x, y) positions:
	class Word {
	public:
		Word() = default;

		size_t Index() const;
		Point Pos() const;

	private:
		size_t index = 0;
		int x = 0;
		int y = 0;

		friend class WrappedText;
	};


private:
	const Font *font = nullptr;

	int space = 0;
	int wrapWidth = 1000;
	int tabWidth = 0;
	int lineHeight = 0;
	int paragraphBreak = 0;
	Alignment alignment = Alignment::JUSTIFIED;
	Truncate truncate = Truncate::NONE;

	std::string text;
	std::vector<Word> words;
	int height = 0;

	int longestLineWidth = 0;
};
