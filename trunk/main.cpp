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

#include <GL/glew.h>
#include <SDL/SDL.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;



int main(int argc, char *argv[])
{
	// Game data should persist until after the UIs cease to exist.
	GameData gameData;
	PlayerInfo playerInfo;
	
	try {
		SDL_Init(SDL_INIT_VIDEO);
		
		// Begin loading the game data.
		gameData.BeginLoad(argv + 1);
		
		playerInfo.LoadRecent(gameData);
		
		// Check how big the window can be.
		const SDL_VideoInfo *info = SDL_GetVideoInfo();
		if(!info)
		{
			cerr << "Unable to query monitor resolution!" << endl;
			return 1;
		}
		
		// Make the window just slightly smaller than the monitor resolution.
		int maxWidth = info->current_w;
		int maxHeight = info->current_h;
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
		SDL_WM_SetCaption("Endless Sky", "Endless Sky");
		Uint32 flags = SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF;
		SDL_SetVideoMode(Screen::Width(), Screen::Height(), 0, flags);
		
		// Initialize GLEW.
		if(glewInit() != GLEW_OK)
			return 1;
		
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		gameData.LoadShaders();
		
		
		UI gamePanels;
		UI menuPanels;
		menuPanels.Push(new MenuPanel(gameData, playerInfo, gamePanels));
		
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
						&& event.key.keysym.sym == gameData.Keys().Get(Key::MENU))
				{
					menuPanels.Push(shared_ptr<Panel>(
						new MenuPanel(gameData, playerInfo, gamePanels)));
				}
				else if(event.type == SDL_QUIT)
				{
					menuPanels.Quit();
				}
				else if(event.type == SDL_VIDEORESIZE)
				{
					Screen::Set(event.resize.w & ~1, event.resize.h & ~1);
					SDL_SetVideoMode(Screen::Width(), Screen::Height(), 0, flags);
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
				else if(activeUI.Handle(event))
				{
					// No need to do anything more!
				}
				else if(event.type == SDL_KEYDOWN
						&& event.key.keysym.sym == gameData.Keys().Get(Key::FULLSCREEN))
				{
					if(restoreWidth)
					{
						Screen::Set(restoreWidth, restoreHeight);
						restoreWidth = 0;
						restoreHeight = 0;
					}
					else
					{
						restoreWidth = Screen::Width();
						restoreHeight = Screen::Height();
						Screen::Set(maxWidth, maxHeight);
					}
					// TODO: When toggling out of full-screen mode in unity,
					// the window is left maximized. Find a way to fix that.
					SDL_SetVideoMode(Screen::Width(), Screen::Height(), 0,
						restoreWidth ? (flags | SDL_FULLSCREEN) : flags);
					
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
			}
			
			// Tell all the panels to step forward, then draw them.
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).StepAll();
			// That may have cleared out the menu, in which case we should draw
			// the game panels instead:
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();
			
			SDL_GL_SwapBuffers();
			timer.Wait();
		}
		
		// If you quit while landed on a planet, save the game.
		if(playerInfo.GetPlanet())
			playerInfo.Save();
		
		SDL_Quit();
	}
	catch(const runtime_error &error)
	{
		cerr << error.what() << endl;
	}
	
	return 0;
}
