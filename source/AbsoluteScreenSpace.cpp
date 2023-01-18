/* AbsoluteScreenSpace.cpp
Copyright (c) 2023 by Daniel Weiner

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AbsoluteScreenSpace.h"

#include "Screen.h"



// Effective zoom level, as restricted by the current resolution / window size.
int AbsoluteScreenSpace::Zoom()
{
	return 100;
}



// This is true if the screen is high DPI, or if the zoom is above 100%.
bool AbsoluteScreenSpace::IsHighResolution()
{
	return Screen::IsHighResolution() && Screen::Zoom() <= 100;
}



Point AbsoluteScreenSpace::Dimensions()
{
	return Screen::RawDimensions();
}



int AbsoluteScreenSpace::Width() const
{
	return Screen::RawWidth();
}



int AbsoluteScreenSpace::Height() const
{
	return Screen::RawHeight();
}



// Get the positions of the edges and corners of the viewport.
int AbsoluteScreenSpace::Left()
{
	return Screen::RawLeft();
}



int AbsoluteScreenSpace::Top()
{
	return Screen::RawTop();
}



int AbsoluteScreenSpace::Right()
{
	return Screen::RawRight();
}



int AbsoluteScreenSpace::Bottom()
{
	return Screen::RawBottom();
}



Point AbsoluteScreenSpace::TopLeft()
{
	return Screen::RawTopLeft();
}



Point AbsoluteScreenSpace::TopRight()
{
	return Screen::RawTopRight();
}



Point AbsoluteScreenSpace::BottomLeft()
{
	return Screen::RawBottomLeft();
}



Point AbsoluteScreenSpace::BottomRight()
{
	return Screen::RawBottomRight();
}



std::shared_ptr<AbsoluteScreenSpace> AbsoluteScreenSpace::instance()
{
	static std::shared_ptr<AbsoluteScreenSpace> screenSpace(new AbsoluteScreenSpace);
	return screenSpace;
}
