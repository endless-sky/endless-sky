/* main.cpp
Michael Zahniser, 26 Oct 2013

Main function for Endless Sky.
*/


#include "FrameTimer.h"
#include "GameData.h"
#include "MainPanel.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Screen.h"

#include <GL/glew.h>
#include <SDL/SDL.h>

#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;



int main(int argc, char *argv[])
{
	try {
		SDL_Init(SDL_INIT_VIDEO);
		
		// Begin loading the game data.
		GameData gameData;
		gameData.BeginLoad(argv + 1);
		PlayerInfo playerInfo;
		
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
		
		
		Panel::Push(new MainPanel(gameData, playerInfo));
		Panel::Push(new MenuPanel(gameData, playerInfo));
		
		FrameTimer timer(60);
		while(!Panel::IsDone())
		{
			bool done = false;
			
			// Handle any events that occurred in this frame.
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
				// The caps lock key slows the game down (to make it easier to
				// see and debug things that are happening quickly).
				if((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
						&& event.key.keysym.sym == SDLK_CAPSLOCK)
					timer.SetFrameRate((event.type == SDL_KEYDOWN) ? 10 : 60);
				
				if(event.type == SDL_QUIT)
					done = true;
				else if(event.type == SDL_VIDEORESIZE)
				{
					Screen::Set(event.resize.w, event.resize.h);
					SDL_SetVideoMode(Screen::Width(), Screen::Height(), 0, flags);
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
				else
					Panel::Handle(event);
			}
			if(done)
				break;
			
			// Tell all the panels to step forward, then draw them.
			Panel::StepAll();
			
			Panel::DrawAll();
			
			SDL_GL_SwapBuffers();
			timer.Wait();
		}
		
		SDL_Quit();
	}
	catch(const runtime_error &error)
	{
		cerr << error.what() << endl;
	}
	
	return 0;
}
