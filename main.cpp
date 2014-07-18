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


#include "FrameTimer.h"
#include "GameData.h"
#include "MainPanel.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "UI.h"

#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;



int main(int argc, char *argv[])
{
	PlayerInfo player;
	
	try {
		SDL_Init(SDL_INIT_VIDEO);
		
		// Begin loading the game data.
		GameData::BeginLoad(argv);
		
		player.LoadRecent();
		
		// Check how big the window can be.
		SDL_DisplayMode mode;
		if(SDL_GetCurrentDisplayMode(0, &mode))
		{
			cerr << "Unable to query monitor resolution!" << endl;
			return 1;
		}
		
		// Make the window just slightly smaller than the monitor resolution.
		int maxWidth = mode.w;
		int maxHeight = mode.h;
		// Restore this after toggling fullscreen.
		int restoreWidth = 0;
		int restoreHeight = 0;
		if(maxWidth - 100 < 1024 || maxHeight - 100 < 768)
		{
			cerr << "Monitor resolution is too small!" << endl;
			return 1;
		}
		Screen::Set(maxWidth - 100, maxHeight - 100);
		
		// Create the window.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
#ifdef __APPLE__
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
		SDL_Window *window = SDL_CreateWindow("Endless Sky",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			Screen::Width(), Screen::Height(), flags);
		if(!window)
		{
			cerr << "Unable to create window!" << endl;
			return 1;
		}
		
		SDL_GLContext context = SDL_GL_CreateContext(window);
		if(!context)
		{
			cerr << "Unable to create OpenGL context!" << endl;
			return 1;
		}
		
		if(SDL_GL_MakeCurrent(window, context))
		{
			cerr << "Unable to set the current OpenGL context!" << endl;
			return 1;
		}
		SDL_GL_SetSwapInterval(1);
		
		// Initialize GLEW.
#ifndef __APPLE__
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
		{
			cerr << "Unable to initialize GLEW!" << endl;
			return 1;
		}
#endif
		
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		GameData::LoadShaders();
		
		
		UI gamePanels;
		UI menuPanels;
		menuPanels.Push(new MenuPanel(player, gamePanels));
		
		FrameTimer timer(60);
		while(!menuPanels.IsDone())
		{
			// Handle any events that occurred in this frame.
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
				UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);
				
				// The caps lock key slows the game down (to make it easier to
				// see and debug things that are happening quickly).
				if((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
						&& event.key.keysym.sym == SDLK_CAPSLOCK)
				{
					timer.SetFrameRate((event.type == SDL_KEYDOWN) ? 10 : 60);
				}
				else if(event.type == SDL_KEYDOWN && menuPanels.IsEmpty()
						&& event.key.keysym.sym == GameData::Keys().Get(Key::MENU))
				{
					menuPanels.Push(shared_ptr<Panel>(
						new MenuPanel(player, gamePanels)));
				}
				else if(event.type == SDL_QUIT)
				{
					menuPanels.Quit();
				}
				else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					Screen::Set(event.window.data1 & ~1, event.window.data2 & ~1);
					SDL_SetWindowSize(window, Screen::Width(), Screen::Height());
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
				else if(activeUI.Handle(event))
				{
					// No need to do anything more!
				}
				else if(event.type == SDL_KEYDOWN
						&& event.key.keysym.sym == GameData::Keys().Get(Key::FULLSCREEN))
				{
					if(restoreWidth)
					{
						SDL_SetWindowFullscreen(window, 0);
						Screen::Set(restoreWidth, restoreHeight);
						restoreWidth = 0;
						restoreHeight = 0;
					}
					else
					{
						restoreWidth = Screen::Width();
						restoreHeight = Screen::Height();
						Screen::Set(maxWidth, maxHeight);
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
					}
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
			}
			
			// Tell all the panels to step forward, then draw them.
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).StepAll();
			// That may have cleared out the menu, in which case we should draw
			// the game panels instead:
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();
			
			SDL_GL_SwapWindow(window);
			timer.Wait();
		}
		
		// If you quit while landed on a planet, save the game.
		if(player.GetPlanet())
			player.Save();
		
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	catch(const runtime_error &error)
	{
		cerr << error.what() << endl;
	}
	
	return 0;
}
