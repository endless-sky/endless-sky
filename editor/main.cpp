/* main.cpp
Michael Zahniser, 26 Oct 2013

Main function for the Endless Sky editor.
*/

#include "Screen.h"

#include "Panel.h"
#include "MapPanel.h"
#include "SystemPanel.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "DotShader.h"
#include "LineShader.h"

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
		if(maxWidth < 640 || maxHeight < 480)
		{
			cerr << "Monitor resolution is too small!" << endl;
			return 1;
		}
		Screen::Set(maxWidth - 100, maxHeight - 100);
	
		// Create the window.
		SDL_WM_SetCaption("Endless Sky (Editor)", "Endless Sky (Editor)");
		Uint32 flags = SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF;
		SDL_SetVideoMode(Screen::Width(), Screen::Height(), 0, flags);
	
		// Initialize GLEW.
		if(glewInit() != GLEW_OK)
			return 1;
	
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		const char *path[] = {"..", nullptr};
		SpriteSet::SetPath(path);
		System::Init();
		FontSet::Add("../images/font/ubuntu14r.png", 14);
		SpriteShader::Init();
		DotShader::Init();
		LineShader::Init();
		
		Panel::Push(new MapPanel(argv[1] ? argv[1] : "map.txt"));
		
		FrameTimer timer(60);
		while(true)
		{
			bool done = false;
	
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
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
			
			Panel::StepAll();
		
			glClear(GL_COLOR_BUFFER_BIT);
		
			// Find the last panel that is full screen.
			Panel::DrawAll();
		
			SDL_GL_SwapBuffers();
			timer.Wait();
		}
		Panel::FreeAll();
		SDL_Quit();
	}
	catch(const runtime_error &error)
	{
		cerr << error.what() << endl;
	}
	
	return 0;
}
