/* Screen.h
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

#pragma once

#include "Point.h"



// Class that simply holds the screen dimensions. This is really just a wrapper
// around some global variables, which means that no one but the drawing thread
// is allowed to use it.
class Screen {
public:
	// Use RAII to define the scope of a temporary screen size change.
	class ScreenDimensionsGuard final
	{
	public:
		// Constructor that changes the screen size on creation.
		ScreenDimensionsGuard(int width, int height);
		// Destructor, which restores the expected screen size.
		~ScreenDimensionsGuard();
		// Restore the screen settings.
		void Deactivate();

	private:
		bool valid = false;
		int oldWidth = 0;
		int oldHeight = 0;
	};


public:
	static void SetRaw(int width, int height, bool noResizeEvent = false);

	// Zoom level as specified by the user.
	static int UserZoom();
	// Effective zoom level, as restricted by the current resolution / window size.
	static int Zoom();
	static void SetZoom(int percent, bool noEvent = false);

	// Specify that this is a high-DPI window.
	static void SetHighDPI(bool isHighDPI = true);
	// This is true if the screen is high DPI, or if the zoom is above 100%.
	static bool IsHighResolution();

	static Point Dimensions();
	static int Width();
	static int Height();

	static int RawWidth();
	static int RawHeight();

	// Get the positions of the edges and corners of the viewport.
	static int Left();
	static int Top();
	static int Right();
	static int Bottom();

	static Point TopLeft();
	static Point TopRight();
	static Point BottomLeft();
	static Point BottomRight();
};
