/* layout.hpp
Copyright (c) 2020 by Masato Oota

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_LAYOUT_H_
#define ES_TEXT_LAYOUT_H_

#include "alignment.hpp"
#include "truncate.hpp"

#include <cstdint>



// Information for the font rendering code that controls how a given piece of text
// should behave on screen, especially with regards to its alignment and maximum width.
class Layout {
public:
	Layout() noexcept = default;
	Layout(int width) noexcept;
	Layout(int width, Alignment alignment) noexcept;
	Layout(int width, Truncate truncateType) noexcept;
	Layout(int width, Alignment alignment, Truncate truncateType) noexcept;
	bool operator==(const Layout &rhs) const noexcept;
	
public:
	static constexpr uint_fast8_t DEFAULT_LINE_HEIGHT = 255;
	static constexpr uint_fast8_t DEFAULT_PARAGRAPH_BREAK = 255;
	
	
	// A negative width implies infinite width is allowed (e.g. a wrappable string, or
	// one which we can guarantee will not overflow its drawing bounds).
	int width = -1;
	Alignment align = Alignment::LEFT;
	Truncate truncate = Truncate::NONE;
	// Minimum Line height in pixels.
	uint_fast8_t lineHeight = DEFAULT_LINE_HEIGHT;
	// Extra spacing in pixel between paragraphs.
	uint_fast8_t paragraphBreak = DEFAULT_PARAGRAPH_BREAK;
};



inline Layout::Layout(int width) noexcept
	: width(width)
{
}



inline Layout::Layout(int width, Alignment alignment) noexcept
	: width(width), align(alignment)
{
}



inline Layout::Layout(int width, Truncate truncateType) noexcept
	: width(width), truncate(truncateType)
{
}



inline Layout::Layout(int width, Alignment alignment, Truncate truncateType) noexcept
	: width(width), align(alignment), truncate(truncateType)
{
}



inline bool Layout::operator==(const Layout &rhs) const noexcept
{
	return width == rhs.width && align == rhs.align && truncate == rhs.truncate
		&& lineHeight == rhs.lineHeight && paragraphBreak == rhs.paragraphBreak;
}



#endif
