/* GameWindow.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameWindow.h"

#include "Files.h"
#include "ImageBuffer.h"
#include "Screen.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <cstring>
#include <string>
#include <sstream>

using namespace std;

namespace {
	SDL_Window *mainWindow = nullptr;
	SDL_GLContext context = nullptr;
	int width = 0;
	int height = 0;
	bool hasSwizzle = false;
	bool supportsAdaptiveVSync = false;
	
	// Logs SDL errors and returns true if found
	bool checkSDLerror()
	{
		string message = SDL_GetError();
		if(!message.empty())
		{
			Files::LogError("(SDL message: \"" + message + "\")");
			SDL_ClearError();
			return true;
		}
		
		return false;
	}
	
	bool HasOpenGLExtension(const char *name) {
#ifndef __APPLE__
		auto extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		return strstr(extensions, name);
#else
		bool value = false;
		GLint extensionCount = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		for(GLint i = 0; i < extensionCount && !value; ++i)
		{
			auto extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
			value = (extension && strstr(extension, name));
		}
		return value;
#endif
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



bool GameWindow::Init()
{
	// This needs to be called before any other SDL commands.
	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
		checkSDLerror();
		return false;
	}
	
	// Get details about the current display.
	SDL_DisplayMode mode;
	if(SDL_GetCurrentDisplayMode(0, &mode))	{	
		ExitWithError("Unable to query monitor resolution!");
		return false;
	}
	if(mode.refresh_rate && mode.refresh_rate < 60)
		Files::LogError("Warning: low monitor frame rate detected (" + to_string(mode.refresh_rate) + "). The game will run more slowly.");
	
	// Make the window just slightly smaller than the monitor resolution.
	int minWidth = 640;
	int minHeight = 480;
	int maxWidth = mode.w;
	int maxHeight = mode.h;
	if(maxWidth < minWidth || maxHeight < minHeight){
		ExitWithError("Monitor resolution is too small!");
		return false;
	}
	
	int windowWidth = maxWidth - 100;
	int windowHeight = maxHeight - 100;
						
	// Decide how big the window should be.
	if(Screen::RawWidth() && Screen::RawHeight())
	{
		// Load the previously saved window dimensions.
		windowWidth = min(windowWidth, Screen::RawWidth());
		windowHeight = min(windowHeight, Screen::RawHeight());
	}
	
	// Settings that must be declared before the window creation.
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if(Preferences::Has("fullscreen"))
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if(Preferences::Has("maximized"))
		flags |= SDL_WINDOW_MAXIMIZED;
	
	// The main window spawns visibly at this point.
	mainWindow = SDL_CreateWindow("Endless Sky", SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, flags);
		
	if(!mainWindow){
		ExitWithError("Unable to create window!");
		return false;
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
	if(!context){
		ExitWithError("Unable to create OpenGL context! Check if your system supports OpenGL 3.0.");
		return false;
	}
	
	if(SDL_GL_MakeCurrent(mainWindow, context)){
		ExitWithError("Unable to set the current OpenGL context!");
		return false;
	}
			
	// Initialize GLEW.
#if !defined(__APPLE__) && !defined(ES_GLES)
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK){
		ExitWithError("Unable to initialize GLEW!");
		return false;
	}
#endif
	
	// Check that the OpenGL version is high enough.
	const char *glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	if(!glVersion || !*glVersion){
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
	
	if(*glVersion < '3')
	{
		ostringstream out;
		out << "Endless Sky requires OpenGL version 3.0 or higher." << endl;
		out << "Your OpenGL version is " << glVersion << ", GLSL version " << glslVersion << "." << endl;
		out << "Please update your graphics drivers.";
		ExitWithError(out.str());
		return false;
	}
	
	// OpenGL settings
	glClearColor(0.f, 0.f, 0.0f, 1.f);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	// Check for support of various graphical features.
	hasSwizzle = HasOpenGLExtension("_texture_swizzle");
	supportsAdaptiveVSync = HasOpenGLExtension("_swap_control_tear");
	
	// Enable the user's preferred VSync state, otherwise update to an available
	// value (e.g. if an external program is forcing a particular VSync state).
	if(!SetVSync(Preferences::VSyncState()))
		Preferences::ToggleVSync();
	
	// Make sure the screen size and view-port are set correctly.
	AdjustViewport();
	
#ifndef __APPLE__
	// On OS X, setting the window icon will cause that same icon to be used
	// in the dock and the application switcher. That's not something we
	// want, because the ".icns" icon that is used automatically is prettier.
	SetIcon();
#endif
	
	return true;
}



// Clean up the SDL context, window, and shut down SDL.
void GameWindow::Quit()
{
	// Make sure the cursor is visible.
	SDL_ShowCursor(true);
	
	// Clean up in the reverse order that everything is launched.
//#ifndef _WIN32
	// Under windows, this cleanup code causes intermittent crashes.
	if(context)
		SDL_GL_DeleteContext(context);
//#endif

	if(mainWindow)
		SDL_DestroyWindow(mainWindow);
		
	SDL_Quit();
}	



void GameWindow::Step()
{
	SDL_GL_SwapWindow(mainWindow);
}



void GameWindow::SetIcon()
{
	if(!mainWindow)
		return;
		
	// Load the icon file.
	ImageBuffer buffer;
	if(!buffer.Read(Files::Resources() + "icon.png"))
		return;
	if(!buffer.Pixels() || !buffer.Width() || !buffer.Height())
		return;
	
	// Convert the icon to an SDL surface.
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(buffer.Pixels(), buffer.Width(), buffer.Height(),
		32, 4 * buffer.Width(), 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if(surface)
	{
		SDL_SetWindowIcon(mainWindow, surface);
		SDL_FreeSurface(surface);
	}
}



void GameWindow::AdjustViewport()
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
	Screen::SetRaw(roundWidth, roundHeight);
	
	// Find out the drawable dimensions. If this is a high- DPI display, this
	// may be larger than the window.
	int drawWidth, drawHeight;
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



bool GameWindow::HasSwizzle()
{
	return hasSwizzle;
}



void GameWindow::ExitWithError(const string& message, bool doPopUp)
{
	// Print the error message in the terminal and the error file.
	Files::LogError(message);		
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


