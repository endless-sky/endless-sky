/* GameWindow.cpp
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

#include "GameWindow.h"

#include "Logger.h"
#include "Screen.h"

#ifdef _WIN32
#include "windows/WinWindow.h"
#endif

#include "opengl.h"
#include <SDL2/SDL.h>

#include <cstring>
#include <sstream>
#include <string>

using namespace std;

namespace {
	// The minimal screen resolution requirements.
	constexpr int minWidth = 1024;
	constexpr int minHeight = 768;

	SDL_Window *mainWindow = nullptr;
	SDL_GLContext context = nullptr;
	int width = 0;
	int height = 0;
	int drawWidth = 0;
	int drawHeight = 0;
	bool supportsAdaptiveVSync = false;

	// Logs SDL errors and returns true if found
	bool checkSDLerror()
	{
		string message = SDL_GetError();
		if(!message.empty())
		{
			Logger::Log("(SDL message: \"" + message + "\")", Logger::Level::ERROR);
			SDL_ClearError();
			return true;
		}

		return false;
	}
}



string GameWindow::SDLVersions()
{
	SDL_version built;
	SDL_version linked;
	SDL_VERSION(&built);
	SDL_GetVersion(&linked);

	auto toString = [](const SDL_version &v) -> string
	{
		return to_string(v.major) + "." + to_string(v.minor) + "." + to_string(v.patch);
	};
	return "Compiled against SDL v" + toString(built) + "\nUsing SDL v" + toString(linked);
}



bool GameWindow::Init(bool headless)
{
#ifdef _WIN32
	// Tell Windows this process is high dpi aware and doesn't need to get scaled.
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#elif defined(__linux__)
	// Set the class name for the window on Linux. Used to set the application icon.
	// This sets it for both X11 and Wayland.
	setenv("SDL_VIDEO_X11_WMCLASS", "io.github.endless_sky.endless_sky", true);
#endif

	// When running the integration tests, don't create a window nor an OpenGL context.
	if(headless)
#if defined(__linux__) && !SDL_VERSION_ATLEAST(2, 0, 22)
		setenv("SDL_VIDEODRIVER", "dummy", true);
#else
		SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
#endif

	// This needs to be called before any other SDL commands.
	if(SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		checkSDLerror();
		return false;
	}

	// Get details about the current display.
	SDL_DisplayMode mode;
	if(SDL_GetCurrentDisplayMode(0, &mode))
	{
		ExitWithError("Unable to query monitor resolution!");
		return false;
	}
	if(mode.refresh_rate && mode.refresh_rate < 60)
		Logger::Log("Low monitor frame rate detected (" + to_string(mode.refresh_rate) + ")."
			" The game will run more slowly.", Logger::Level::WARNING);

	// Make the window just slightly smaller than the monitor resolution.
	int maxWidth = mode.w;
	int maxHeight = mode.h;
	if(maxWidth < minWidth || maxHeight < minHeight)
		Logger::Log("Monitor resolution is too small! Minimal requirement is "
			+ to_string(minWidth) + 'x' + to_string(minHeight)
			+ ", while your resolution is " + to_string(maxWidth) + 'x' + to_string(maxHeight) + '.',
			Logger::Level::WARNING);

	int windowWidth = maxWidth - 100;
	int windowHeight = maxHeight - 100;

	// Decide how big the window should be.
	if(Screen::RawWidth() && Screen::RawHeight())
	{
		// Load the previously saved window dimensions.
		windowWidth = min(windowWidth, Screen::RawWidth());
		windowHeight = min(windowHeight, Screen::RawHeight());
	}

	if(!Preferences::Has("Block screen saver"))
		SDL_EnableScreenSaver();

	// Settings that must be declared before the window creation.
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if(Preferences::ScreenModeSetting() == "fullscreen")
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if(Preferences::Has("maximized"))
		flags |= SDL_WINDOW_MAXIMIZED;

	// The main window spawns visibly at this point.
	mainWindow = SDL_CreateWindow("Endless Sky", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, headless ? 0 : flags);

	if(!mainWindow)
	{
		ExitWithError("Unable to create window!");
		return false;
	}

	// Bail out early if we are in headless mode; no need to initialize all the OpenGL stuff.
	if(headless)
	{
		width = windowWidth;
		height = windowHeight;
		Screen::SetRaw(width, height, true);
		return true;
	}

	// Settings that must be declared before the context creation.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef _WIN32
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#ifdef ES_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	context = SDL_GL_CreateContext(mainWindow);
#ifndef ES_GLES
	if(!context)
	{
		Logger::Log("OpenGL context creation failed. Retrying with experimental OpenGL 2 support.",
			Logger::Level::WARNING);
		SDL_ClearError();
#ifdef _WIN32
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#endif
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
		context = SDL_GL_CreateContext(mainWindow);
	}
#endif
	if(!context)
	{
		ExitWithError("Unable to create OpenGL context! Check if your system supports OpenGL 3.0.");
		return false;
	}

	if(SDL_GL_MakeCurrent(mainWindow, context))
	{
		ExitWithError("Unable to set the current OpenGL context!");
		return false;
	}

	// Initialize GLEW.
#if !defined(__APPLE__) && !defined(ES_GLES)
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	if(err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY)
#else
	if(err != GLEW_OK)
#endif
	{
		ExitWithError("Unable to initialize GLEW!");
		return false;
	}
#endif

	// Check that the OpenGL version is high enough.
	const char *glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	if(!glVersion || !*glVersion)
	{
		ExitWithError("Unable to query the OpenGL version!");
		return false;
	}

	const char *glslVersion = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	if(!glslVersion || !*glslVersion)
	{
		ostringstream out;
		out << "Unable to query the GLSL version. OpenGL version is " << glVersion << ".";
		ExitWithError(out.str());
		return false;
	}

	if(*glVersion < '2')
	{
		ostringstream out;
		out << "Endless Sky requires OpenGL version 2.0 or higher, and 3.0 is recommended." << endl;
		out << "Your OpenGL version is " << glVersion << ", GLSL version " << glslVersion << "." << endl;
		out << "Please update your graphics drivers.";
		ExitWithError(out.str());
		return false;
	}
#ifndef ES_GLES
	else if(*glVersion == '2')
	{
		OpenGL::DisableOpenGL3();
		Logger::Log("Experimental OpenGL 2 support has been enabled.", Logger::Level::INFO);
	}
#endif

	// OpenGL settings
	glClearColor(0.f, 0.f, 0.0f, 1.f);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Check for support of various graphical features.
	supportsAdaptiveVSync = OpenGL::HasAdaptiveVSyncSupport();

	// Enable the user's preferred VSync state, otherwise update to an available
	// value (e.g. if an external program is forcing a particular VSync state).
	if(!SetVSync(Preferences::VSyncState()))
		Preferences::ToggleVSync();

	// Make sure the screen size and view-port are set correctly.
	AdjustViewport(true);

#ifdef _WIN32
	UpdateTitleBarTheme();
	UpdateWindowRounding();
#endif

	return true;
}



// Clean up the SDL context, window, and shut down SDL.
void GameWindow::Quit()
{
	// Make sure the cursor is visible.
	SDL_ShowCursor(true);

	// Clean up in the reverse order that everything is launched.
	if(context)
		SDL_GL_DeleteContext(context);
	if(mainWindow)
		SDL_DestroyWindow(mainWindow);

	SDL_Quit();
}



void GameWindow::Step()
{
	SDL_GL_SwapWindow(mainWindow);
}



void GameWindow::AdjustViewport(bool noResizeEvent)
{
	if(!mainWindow)
		return;

	// Get the window's size in screen coordinates.
	int windowWidth, windowHeight;
	SDL_GetWindowSize(mainWindow, &windowWidth, &windowHeight);

	// Only save the window size when not in fullscreen mode.
	if(!GameWindow::IsFullscreen())
	{
		width = windowWidth;
		height = windowHeight;
	}

	// Round the window size up to a multiple of 2, even if this
	// means one pixel of the display will be clipped.
	int roundWidth = (windowWidth + 1) & ~1;
	int roundHeight = (windowHeight + 1) & ~1;
	Screen::SetRaw(roundWidth, roundHeight, noResizeEvent);

	// Find out the drawable dimensions. If this is a high- DPI display, this
	// may be larger than the window.
	SDL_GL_GetDrawableSize(mainWindow, &drawWidth, &drawHeight);
	Screen::SetHighDPI(drawWidth > windowWidth || drawHeight > windowHeight);

	// Set the viewport to go off the edge of the window, if necessary, to get
	// everything pixel-aligned.
	drawWidth = (drawWidth * roundWidth) / windowWidth;
	drawHeight = (drawHeight * roundHeight) / windowHeight;
	glViewport(0, 0, drawWidth, drawHeight);
}



// Attempts to set the requested SDL Window VSync to the given state. Returns false
// if the operation could not be completed successfully.
bool GameWindow::SetVSync(Preferences::VSync state)
{
	if(!context)
		return false;

	const int originalState = SDL_GL_GetSwapInterval();
	int interval = 1;
	switch(state)
	{
		case Preferences::VSync::adaptive:
			interval = -1;
			break;
		case Preferences::VSync::off:
			interval = 0;
			break;
		case Preferences::VSync::on:
			interval = 1;
			break;
		default:
			return false;
	}
	// Do not attempt to enable adaptive VSync when unsupported,
	// as this can crash older video drivers.
	if(interval == -1 && !supportsAdaptiveVSync)
		return false;

	if(SDL_GL_SetSwapInterval(interval) == -1)
	{
		checkSDLerror();
		SDL_GL_SetSwapInterval(originalState);
		return false;
	}
	return SDL_GL_GetSwapInterval() == interval;
}



// Last window width, in windowed mode.
int GameWindow::Width()
{
	return width;
}



// Last window height, in windowed mode.
int GameWindow::Height()
{
	return height;
}



int GameWindow::DrawWidth()
{
	return drawWidth;
}



int GameWindow::DrawHeight()
{
	return drawHeight;
}



bool GameWindow::IsMaximized()
{
	return (SDL_GetWindowFlags(mainWindow) & SDL_WINDOW_MAXIMIZED);
}



bool GameWindow::IsFullscreen()
{
	return (SDL_GetWindowFlags(mainWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP);
}



void GameWindow::ToggleFullscreen()
{
	// This will generate a window size change event,
	// no need to adjust the viewport here.
	if(IsFullscreen())
	{
		SDL_SetWindowFullscreen(mainWindow, 0);
		SDL_SetWindowSize(mainWindow, width, height);
	}
	else
		SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
}



void GameWindow::ToggleBlockScreenSaver()
{
	if(SDL_IsScreenSaverEnabled())
		SDL_DisableScreenSaver();
	else
		SDL_EnableScreenSaver();
}



void GameWindow::ExitWithError(const string &message, bool doPopUp)
{
	// Print the error message in the terminal and the error file.
	Logger::Log(message, Logger::Level::ERROR);
	checkSDLerror();

	// Show the error message in a message box.
	if(doPopUp)
	{
		SDL_MessageBoxData box;
		box.flags = SDL_MESSAGEBOX_ERROR;
		box.window = nullptr;
		box.title = "Endless Sky: Error";
		box.message = message.c_str();
		box.colorScheme = nullptr;

		SDL_MessageBoxButtonData button;
		button.flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		button.buttonid = 0;
		button.text = "OK";
		box.numbuttons = 1;
		box.buttons = &button;

		int result = 0;
		SDL_ShowMessageBox(&box, &result);
	}

	GameWindow::Quit();
}



#ifdef _WIN32
void GameWindow::UpdateTitleBarTheme()
{
	WinWindow::UpdateTitleBarTheme(mainWindow);
}



void GameWindow::UpdateWindowRounding()
{
	WinWindow::UpdateWindowRounding(mainWindow);
}
#endif
