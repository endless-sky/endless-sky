/* main.cpp
Copyright (c) 2014 by Michael Zahniser

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Audio.h"
#include "Command.h"
#include "Conversation.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataNode.h"
#include "Dialog.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Screen.h"
#include "UI.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

void PrintHelp();
void PrintVersion();
int DoError(string message, SDL_Window *window = nullptr, SDL_GLContext context = nullptr);
void Cleanup(SDL_Window *window, SDL_GLContext context);
Conversation LoadConversation();



int main(int argc, char *argv[])
{
	Conversation conversation;
	bool debugMode = false;
	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-h" || arg == "--help")
		{
			PrintHelp();
			return 0;
		}
		else if(arg == "-v" || arg == "--version")
		{
			PrintVersion();
			return 0;
		}
		else if(arg == "-t" || arg == "--talk")
			conversation = LoadConversation();
		else if(arg == "-d" || arg == "--debug")
			debugMode = true;
	}
	PlayerInfo player;
	
	try {
		SDL_Init(SDL_INIT_VIDEO);
		
		// Begin loading the game data.
		GameData::BeginLoad(argv);
		Audio::Init(GameData::Sources());
		
		// On Windows, make sure that the sleep timer has at least 1 ms resolution
		// to avoid irregular frame rates.
#ifdef _WIN32
		timeBeginPeriod(1);
#endif
		
		player.LoadRecent();
		player.ApplyChanges();
		
		// Check how big the window can be.
		SDL_DisplayMode mode;
		if(SDL_GetCurrentDisplayMode(0, &mode))
			return DoError("Unable to query monitor resolution!");
		
		Preferences::Load();
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
		if(Preferences::Has("fullscreen"))
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		
		// Make the window just slightly smaller than the monitor resolution.
		int maxWidth = mode.w;
		int maxHeight = mode.h;
		// Restore this after toggling fullscreen.
		int restoreWidth = 0;
		int restoreHeight = 0;
		if(maxWidth < 640 || maxHeight < 480)
			return DoError("Monitor resolution is too small!");
		
		if(Screen::RawWidth() && Screen::RawHeight())
		{
			// Never allow the saved screen width to be leaving less than 100
			// pixels free around the window. This avoids the problem where you
			// maximize without going full-screen, and next time the window pops
			// up you can't access the resize control because it is offscreen.
			Screen::SetRaw(
				min(Screen::RawWidth(), (maxWidth - 100)),
				min(Screen::RawHeight(), (maxHeight - 100)));
			if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			{
				restoreWidth = Screen::RawWidth();
				restoreHeight = Screen::RawHeight();
				Screen::SetRaw(maxWidth, maxHeight);
			}
		}
		else
			Screen::SetRaw(maxWidth - 100, maxHeight - 100);
		// Make sure the zoom factor is not set too high for the full UI to fit.
		if(Screen::Height() < 700)
			Screen::SetZoom(100);
		
		// Create the window.
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef _WIN32
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		
		SDL_Window *window = SDL_CreateWindow("Endless Sky",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			Screen::RawWidth(), Screen::RawHeight(), flags);
		if(!window)
			return DoError("Unable to create window!");
		
		SDL_GLContext context = SDL_GL_CreateContext(window);
		if(!context)
			return DoError("Unable to create OpenGL context! Check if your system supports OpenGL 3.0.", window);
		
		if(SDL_GL_MakeCurrent(window, context))
			return DoError("Unable to set the current OpenGL context!", window, context);
		
		SDL_GL_SetSwapInterval(1);
		
		// Initialize GLEW.
#ifndef __APPLE__
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
			return DoError("Unable to initialize GLEW!", window, context);
#endif
		
		// Check that the OpenGL version is high enough.
		const char *glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
		if(!glVersion || !*glVersion)
			return DoError("Unable to query the OpenGL version!", window, context);
		
		const char *glslVersion = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
		if(!glslVersion || !*glslVersion)
		{
			ostringstream out;
			out << "Unable to query the GLSL version. OpenGL version is " << glVersion << ".";
			return DoError(out.str(), window, context);
		}
		
		if(*glVersion < '3')
		{
			ostringstream out;
			out << "Endless Sky requires OpenGL version 3.0 or higher." << endl;
			out << "Your OpenGL version is " << glVersion << ", GLSL version " << glslVersion << "." << endl;
			out << "Please update your graphics drivers.";
			return DoError(out.str(), window, context);
		}
		
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		{
			// Check whether this is a high-DPI window.
			int width = 0;
			int height = 0;
			SDL_GL_GetDrawableSize(window, &width, &height);
			Screen::SetHighDPI(width > Screen::RawWidth() && height > Screen::RawHeight());
		}
		
		GameData::LoadShaders();
		
		
		UI gamePanels;
		UI menuPanels;
		menuPanels.Push(new MenuPanel(player, gamePanels));
		if(!conversation.IsEmpty())
			menuPanels.Push(new ConversationPanel(player, conversation));
		
		string swizzleName = "_texture_swizzle";
#ifndef __APPLE__
		const char *extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		if(!strstr(extensions, swizzleName.c_str()))
#else
		bool hasSwizzle = false;
		GLint extensionCount;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		for(GLint i = 0; i < extensionCount && !hasSwizzle; ++i)
		{
			const char *extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
			hasSwizzle = (extension && strstr(extension, swizzleName.c_str()));
		}
		if(!hasSwizzle)
#endif
			menuPanels.Push(new Dialog(
				"Note: your computer does not support the \"texture swizzling\" OpenGL feature, "
				"which Endless Sky uses to draw ships in different colors depending on which "
				"government they belong to. So, all human ships will be the same color, which "
				"may be confusing. Consider upgrading your graphics driver (or your OS)."));
		
		FrameTimer timer(60);
		bool isPaused = false;
		while(!menuPanels.IsDone())
		{
			// Handle any events that occurred in this frame.
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
				UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);
				
				// The caps lock key slows the game down (to make it easier to
				// see and debug things that are happening quickly).
				if(debugMode && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
						&& event.key.keysym.sym == SDLK_CAPSLOCK)
				{
					timer.SetFrameRate((event.key.keysym.mod & KMOD_CAPS) ? 10 : 60);
				}
				else if(debugMode && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKQUOTE)
				{
					isPaused = !isPaused;
				}
				else if(event.type == SDL_KEYDOWN && menuPanels.IsEmpty()
						&& Command(event.key.keysym.sym).Has(Command::MENU)
						&& !gamePanels.IsEmpty() && gamePanels.Top()->IsInterruptible())
				{
					menuPanels.Push(shared_ptr<Panel>(
						new MenuPanel(player, gamePanels)));
				}
				else if(event.type == SDL_QUIT)
				{
					menuPanels.Quit();
				}
				else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					int width = event.window.data1 & ~1;
					int height = event.window.data2 & ~1;
					if(width != Screen::RawWidth() || height != Screen::RawHeight())
					{
						Screen::SetRaw(width, height);
						if((event.window.data1 | event.window.data2) & 1)
							SDL_SetWindowSize(window, Screen::RawWidth(), Screen::RawHeight());
						SDL_GL_GetDrawableSize(window, &width, &height);
						glViewport(0, 0, width, height);
					}
				}
				else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
					Audio::Mute();
					isPaused = true;
#ifdef _WIN32
					// When paused, slow the interrupt rate to conserve power.
					timeEndPeriod(1);
#endif
				}
				else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESTORED)
				{
					Audio::Unmute();
					isPaused = false;
#ifdef _WIN32
					// When unpausing, restore the high interrupt rate.
					timeBeginPeriod(1);
#endif
				}
				else if(activeUI.Handle(event))
				{
					// No need to do anything more!
				}
				else if(event.type == SDL_KEYDOWN
						&& (Command(event.key.keysym.sym).Has(Command::FULLSCREEN)
						|| (event.key.keysym.sym == SDLK_RETURN && event.key.keysym.mod & KMOD_ALT)))
				{
					if(restoreWidth)
					{
						SDL_SetWindowFullscreen(window, 0);
						Screen::SetRaw(restoreWidth, restoreHeight);
						SDL_SetWindowSize(window, Screen::RawWidth(), Screen::RawHeight());
						restoreWidth = 0;
						restoreHeight = 0;
					}
					else
					{
						restoreWidth = Screen::RawWidth();
						restoreHeight = Screen::RawHeight();
						Screen::SetRaw(maxWidth, maxHeight);
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
					}
					int width, height;
					SDL_GL_GetDrawableSize(window, &width, &height);
					glViewport(0, 0, width, height);
				}
			}
			
			// Tell all the panels to step forward, then draw them.
			((!isPaused && menuPanels.IsEmpty()) ? gamePanels : menuPanels).StepAll();
			Audio::Step();
			// That may have cleared out the menu, in which case we should draw
			// the game panels instead:
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();
			
			SDL_GL_SwapWindow(window);
			timer.Wait();
		}
		
		// If you quit while landed on a planet, save the game.
		if(player.GetPlanet())
			player.Save();
		
		// The Preferences class reads the screen dimensions, so update them if
		// the window is full screen:
		bool isFullscreen = (restoreWidth != 0);
		Preferences::Set("fullscreen", isFullscreen);
		if(isFullscreen)
			Screen::SetRaw(restoreWidth, restoreHeight);
		Preferences::Save();
		
		Cleanup(window, context);
	}
	catch(const runtime_error &error)
	{
		DoError(error.what());
	}
	
	return 0;
}



void PrintHelp()
{
	cerr << endl;
	cerr << "Command line options:" << endl;
	cerr << "    -h, --help: print this help message." << endl;
	cerr << "    -v, --version: print version information." << endl;
	cerr << "    -s, --ships: print table of ship statistics." << endl;
	cerr << "    -w, --weapons: print table of weapon statistics." << endl;
	cerr << "    -t, --talk: read and display a conversation from STDIN." << endl;
	cerr << "    -r, --resources <path>: load resources from given directory." << endl;
	cerr << "    -c, --config <path>: save user's files to given directory." << endl;
	cerr << "    -d, --debug: turn on debugging features (e.g. caps lock slow motion)." << endl;
	cerr << endl;
	cerr << "Report bugs to: mzahniser@gmail.com" << endl;
	cerr << "Home page: <https://endless-sky.github.io>" << endl;
	cerr << endl;
}



void PrintVersion()
{
	cerr << endl;
	cerr << "Endless Sky 0.8.8" << endl;
	cerr << "License GPLv3+: GNU GPL version 3 or later: <https://gnu.org/licenses/gpl.html>" << endl;
	cerr << "This is free software: you are free to change and redistribute it." << endl;
	cerr << "There is NO WARRANTY, to the extent permitted by law." << endl;
	cerr << endl;
}



int DoError(string message, SDL_Window *window, SDL_GLContext context)
{
	Cleanup(window, context);
	
	// Check if SDL has more details.
	const char *sdlMessage = SDL_GetError();
	if(sdlMessage && sdlMessage[0])
	{
		message += " (SDL message: \"";
		message += sdlMessage;
		message += "\")";
	}
	
	// Print the error message in the terminal.
	cerr << message << endl;
	
	// Show the error message both in a message box and in the terminal.
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
	
	return 1;
}



void Cleanup(SDL_Window *window, SDL_GLContext context)
{
	// Clean up in the reverse order that everything is launched.
#ifndef _WIN32
	// Under windows, this cleanup code causes intermittent crashes.
	if(context)
		SDL_GL_DeleteContext(context);
#endif
	if(window)
		SDL_DestroyWindow(window);
	Audio::Quit();
	SDL_Quit();
}



Conversation LoadConversation()
{
	Conversation conversation;
	DataFile file(cin);
	for(const DataNode &node : file)
		if(node.Token(0) == "conversation")
		{
			conversation.Load(node);
			break;
		}
	
	const map<string, string> subs = {
		{"<bunks>", "[N]"},
		{"<cargo>", "[N tons of Commodity]"},
		{"<commodity>", "[Commodity]"},
		{"<date>", "[Day Mon Year]"},
		{"<day>", "[The Nth of Month]"},
		{"<destination>", "[Planet in the Star system]"},
		{"<fare>", "[N passengers]"},
		{"<first>", "[First]"},
		{"<last>", "[Last]"},
		{"<origin>", "[Origin Planet]"},
		{"<passengers>", "[your passengers]"},
		{"<planet>", "[Planet]"},
		{"<ship>", "[Ship]"},
		{"<system>", "[Star]"},
		{"<tons>", "[N tons]"}
	};
	return conversation.Substitute(subs);
}

