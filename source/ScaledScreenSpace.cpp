/* ScaledScreenSpace.cpp
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

#include "ScaledScreenSpace.h"



// Effective zoom level, as restricted by the current resolution / window size.
int ScaledScreenSpace::Zoom()
{
	return Screen::Zoom();
}



// This is true if the screen is high DPI, or if the zoom is above 100%.
bool ScaledScreenSpace::IsHighResolution()
{
	return Screen::IsHighResolution();
}



Point ScaledScreenSpace::Dimensions()
{
	return Screen::Dimensions();
}



int ScaledScreenSpace::Width() const
{
	return Screen::Width();
}



int ScaledScreenSpace::Height() const
{
	return Screen::Height();
}



// Get the positions of the edges and corners of the viewport.
int ScaledScreenSpace::Left()
{
	return Screen::Left();
}



int ScaledScreenSpace::Top()
{
	return Screen::Top();
}



int ScaledScreenSpace::Right()
{
	return Screen::Right();
}



int ScaledScreenSpace::Bottom()
{
	return Screen::Bottom();
}



Point ScaledScreenSpace::TopLeft()
{
	return Screen::TopLeft();
}



Point ScaledScreenSpace::TopRight()
{
	return Screen::TopRight();
}



Point ScaledScreenSpace::BottomLeft()
{
	return Screen::BottomLeft();
}



Point ScaledScreenSpace::BottomRight()
{
	return Screen::BottomRight();
}



std::shared_ptr<ScaledScreenSpace> ScaledScreenSpace::instance()
{
	static std::shared_ptr<ScaledScreenSpace> screenSpace(new ScaledScreenSpace);
	return screenSpace;
}
