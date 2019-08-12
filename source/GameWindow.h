/* GameWindow.h
Copyright (c) 2014 by Michael Zahniser

Provides the ability to create window objects. Like the main game window.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAMEWINDOW_H_
#define GAMEWINDOW_H_

#include <string>

// This class is a collection of global functions for handling SDL_Windows.
class GameWindow
{
public:
	// Last known window width from windowed mode
	static int Width();
	
	// Last known height width from windowed mode
	static int Height();
	
	// The refresh rate of the current display
	static int MonitorHz();

	// Returns true if the main window is in full screen mode
	static bool IsFullscreen();
	
	// Returns true if the main window is maximized
	static bool IsMaximized();
	
	// Returns true if the system supports OpenGL texture_swizzle
	static bool HasSwizzle();

	// Initialize the main window
	static int Init();
	
	// Paint the next frame in the main window
	static void Step();
	
	// Ensure the proper icon is set on the main window
	static void SetIcon();
	
	// Handles resize events of the main window
	static void AdjustViewport();

	// Toggle full screen mode for the main window
	static void ToggleFullscreen();
	
	// Destroys all window system objects (because we're about to quit). 
	static void Quit();
	
	// Print the error message in the terminal, error file, and message box.
	// Checks for video system errors and records those as well.
	static int DoError(const std::string& message);
};



#endif
