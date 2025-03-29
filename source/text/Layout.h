/* Layout.h
Copyright (c) 2020 by OOTA, Masato

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
#include "Truncate.h"



// Information for the font rendering code that controls how a given piece of text
// should behave on screen, especially with regards to its alignment and maximum width.
class Layout {
public:
	Layout() noexcept = default;
	Layout(int width) noexcept;
	Layout(Alignment alignment) noexcept;
	Layout(Truncate truncateType) noexcept;
	Layout(int width, Alignment alignment) noexcept;
	Layout(int width, Truncate truncateType) noexcept;
	Layout(int width, Alignment alignment, Truncate truncateType) noexcept;

	// A negative width implies infinite width is allowed (e.g. a wrappable string, or
	// one which we can guarantee will not overflow its drawing bounds).
	int width = -1;
	Alignment align = Alignment::LEFT;
	Truncate truncate = Truncate::NONE;
};



inline Layout::Layout(int width) noexcept
	: width(width)
{
}



inline Layout::Layout(Alignment alignment) noexcept
	: align(alignment)
{
}



inline Layout::Layout(Truncate truncateType) noexcept
	: truncate(truncateType)
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
