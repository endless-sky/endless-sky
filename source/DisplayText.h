/* DisplayText.h
Copyright (c) 2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DISPLAYTEXT_H_
#define DISPLAYTEXT_H_

#include <string>



// Class for holding a displaying text and layout.
class DisplayText {
public:
	// Layout parameters.
	enum class Align {LEFT, CENTER, RIGHT, JUSTIFIED};
	enum class Truncate {NONE, FRONT, MIDDLE, BACK};
	static const uint_fast8_t DEFAULT_LINE_HEIGHT = 255;
	static const uint_fast8_t DEFAULT_PARAGRAPH_BREAK = 255;
	struct Layout {
		// Width of text to align and truncation. Negative width means infinite width, that is not truncated.
		int width = -1;
		// Set the alignment mode.
		Align align = Align::LEFT;
		// Set the truncate mode.
		Truncate truncate = Truncate::NONE;
		// Minimum Line height in pixels.
		uint_fast8_t lineHeight = DEFAULT_LINE_HEIGHT;
		// Extra spacing in pixel between paragraphs.
		uint_fast8_t paragraphBreak = DEFAULT_PARAGRAPH_BREAK;
		
		Layout() noexcept = default;
		Layout(int w, Align a) noexcept;
		Layout(int w, Truncate t) noexcept;
		Layout(int w, Align a, Truncate t) noexcept;
		bool operator==(const Layout &other) const noexcept;
	};
	
public:
	DisplayText() = default;
	// The default layout is infinite width and left align.
	DisplayText(const char *text);
	DisplayText(const std::string &text);
	DisplayText(const std::string &text, const Layout &layout);
	bool operator==(const DisplayText &other) const;
	
	const std::string &GetText() const;
	const Layout &GetLayout() const;
	
private:
	Layout layout;
	std::string text;
};



inline
DisplayText::Layout::Layout(int w, Align a) noexcept
        : width(w), align(a)
{
}



inline
DisplayText::Layout::Layout(int w, Truncate t) noexcept
        : width(w), truncate(t)
{
}



inline
DisplayText::Layout::Layout(int w, Align a, Truncate t) noexcept
        : width(w), align(a), truncate(t)
{
}



inline
bool DisplayText::Layout::operator==(const DisplayText::Layout &other) const noexcept
{
	return width == other.width && align == other.align && truncate == other.truncate
		&& lineHeight == other.lineHeight && paragraphBreak == other.paragraphBreak;
}



inline
DisplayText::DisplayText(const char *text)
	: text(text)
{
}



inline
DisplayText::DisplayText(const std::string &text)
	: text(text)
{
}



inline
DisplayText::DisplayText(const std::string &text, const DisplayText::Layout &layout)
	: layout(layout), text(text)
{
}



inline
bool DisplayText::operator==(const DisplayText &other) const
{
	return layout == other.layout && text == other.text;
}



inline
const std::string &DisplayText::GetText() const
{
	return text;
}



inline
const DisplayText::Layout &DisplayText::GetLayout() const
{
	return layout;
}



#endif
