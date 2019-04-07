/* WrappedText.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WRAPPED_TEXT_H_
#define WRAPPED_TEXT_H_

#include "Font.h"
#include "Point.h"

#include <string>

class Color;



// Class for holding wrapped parameters.
class WrappedText {
public:
	WrappedText();
	explicit WrappedText(const Font &font);
	
	// Set the alignment mode.
	Font::Align Alignment() const;
	void SetAlignment(Font::Align align);
	
	// Set the truncate mode.
	Font::Truncate Truncate() const;
	void SetTruncate(Font::Truncate trunc);
	
	// Set the wrap width. This does not include any margins.
	int WrapWidth() const;
	void SetWrapWidth(int width);
	
	// Set the font to use. This will also set sensible defaults for the tab
	// width, line height, and paragraph break. You must specify the wrap width
	// and the alignment separately.
	void SetFont(const Font &font);
	
	// Set the height in pixels of one line of text within a paragraph.
	int LineHeight() const;
	void SetLineHeight(int height);
	
	// Set the extra spacing in pixels to be added in between paragraphs.
	int ParagraphBreak() const;
	void SetParagraphBreak(int height);
	
	// Wrap the given text. Use Draw() to draw it.
	void Wrap(const std::string &str);
	void Wrap(const char *str);
	
	// Get the height of the wrapped text.
	int Height() const;
	
	// Draw the text.
	void Draw(const Point &topLeft, const Color &color) const;
	
	// Bottom margin.
	int BottomMargin() const;
	
private:
	const Font *font;
	std::string text;
	Font::Layout layout;
};



#endif
