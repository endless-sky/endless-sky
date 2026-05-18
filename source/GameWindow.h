/* GameWindow.h
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

#include "Preferences.h"

#include <string>

// This class is a collection of global functions for handling SDL_Windows.
class GameWindow {
public:
	static std::string SDLVersions();
	static bool Init(bool headless);
	static void Quit();

	// Paint the next frame in the main window.
	static void Step();

	// Handle resize events of the main window.
	static void AdjustViewport(bool noResizeEvent = false);

	// Attempt to set the game's VSync setting.
	static bool SetVSync(Preferences::VSync state);

	// Last known windowed-mode width & height.
	static int Width();
	static int Height();

	// Last known drawable width & height.
	static int DrawWidth();
	static int DrawHeight();

	static bool IsMaximized();
	static bool IsFullscreen();
	static void ToggleFullscreen();
	static void ToggleBlockScreenSaver();

	// Print the error message in the terminal, error file, and message box.
	// Checks for video system errors and records those as well.
	static void ExitWithError(const std::string &message, bool doPopUp = true);

#ifdef _WIN32
	// Set attributes of the main window according to the current preferences.
	static void UpdateTitleBarTheme();
	static void UpdateWindowRounding();
#endif
};
