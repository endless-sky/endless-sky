/* Screen.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Screen.h"

#include "CustomEvents.h"

#include <algorithm>

using namespace std;

namespace {
	int RAW_WIDTH = 0;
	int RAW_HEIGHT = 0;
	int WIDTH = 0;
	int HEIGHT = 0;
	int USER_ZOOM = 100;
	int EFFECTIVE_ZOOM = 100;
	bool HIGH_DPI = false;
}



Screen::ScreenDimensionsGuard::ScreenDimensionsGuard(int width, int height)
	: valid(true), oldWidth(WIDTH), oldHeight(HEIGHT)
{
	WIDTH = width;
	HEIGHT = height;
}



Screen::ScreenDimensionsGuard::~ScreenDimensionsGuard()
{
	Deactivate();
}



void Screen::ScreenDimensionsGuard::Deactivate()
{
	if(!valid)
		return;

	WIDTH = oldWidth;
	HEIGHT = oldHeight;
	valid = false;
}



void Screen::SetRaw(int width, int height, bool noResizeEvent)
{
	RAW_WIDTH = width;
	RAW_HEIGHT = height;
	SetZoom(USER_ZOOM, noResizeEvent);
}



int Screen::UserZoom()
{
	return USER_ZOOM;
}



int Screen::Zoom()
{
	return EFFECTIVE_ZOOM;
}



void Screen::SetZoom(int percent, bool noEvent)
{
	if(!noEvent)
		CustomEvents::SendResize();

	USER_ZOOM = max(100, min(200, percent));

	// Make sure the zoom factor is not set too high for the full UI to fit.
	static const int MIN_WIDTH = 1000; // Width of main menu
	static const int MIN_HEIGHT = 500; // Height of preferences panel
	int minZoomX = 100 * RAW_WIDTH / MIN_WIDTH;
	int minZoomY = 100 * RAW_HEIGHT / MIN_HEIGHT;
	int minZoom = min(minZoomX, minZoomY);
	// Never go below 100% zoom, no matter how small the window is
	minZoom = max(minZoom, 100);
	// Use increments of 10, like the user setting
	minZoom -= minZoom % 10;
	EFFECTIVE_ZOOM = min(minZoom, UserZoom());

	WIDTH = RAW_WIDTH * 100 / EFFECTIVE_ZOOM;
	HEIGHT = RAW_HEIGHT * 100 / EFFECTIVE_ZOOM;
}



// Specify that this is a high-DPI window.
void Screen::SetHighDPI(bool isHighDPI)
{
	HIGH_DPI = isHighDPI;
}



// This is true if the screen is high DPI, or if the zoom is above 100%.
bool Screen::IsHighResolution()
{
	return HIGH_DPI || (EFFECTIVE_ZOOM > 100);
}



Point Screen::Dimensions()
{
	return Point(WIDTH, HEIGHT);
}



int Screen::Width()
{
	return WIDTH;
}



int Screen::Height()
{
	return HEIGHT;
}



int Screen::RawWidth()
{
	return RAW_WIDTH;
}



int Screen::RawHeight()
{
	return RAW_HEIGHT;
}



int Screen::Left()
{
	return WIDTH / -2;
}



int Screen::Top()
{
	return HEIGHT / -2;
}



int Screen::Right()
{
	return WIDTH / 2;
}



int Screen::Bottom()
{
	return HEIGHT / 2;
}



Point Screen::TopLeft()
{
	return Point(-.5 * WIDTH, -.5 * HEIGHT);
}



Point Screen::TopRight()
{
	return Point(.5 * WIDTH, -.5 * HEIGHT);
}



Point Screen::BottomLeft()
{
	return Point(-.5 * WIDTH, .5 * HEIGHT);
}



Point Screen::BottomRight()
{
	return Point(.5 * WIDTH, .5 * HEIGHT);
}
