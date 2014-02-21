/* MainPanel.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the MainPanel class.
*/

#include "MainPanel.h"

#include "Angle.h"
#include "DotShader.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "Interface.h"
#include "MapPanel.h"
#include "MenuPanel.h"
#include "PlanetPanel.h"
#include "Point.h"
#include "PointerShader.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "StarField.h"

#include <string>
#include <limits>

using namespace std;



MainPanel::MainPanel(GameData &gameData)
	: gameData(gameData), engine(gameData),
	load(0.), loadSum(0.), loadCount(0)
{
}



void MainPanel::Step(bool isActive)
{
	engine.Step(isActive);
	
	Panel *panel = engine.PanelToShow();
	if(panel)
		Push(panel);
}



void MainPanel::Draw() const
{
	FrameTimer loadTimer;
	glClear(GL_COLOR_BUFFER_BIT);
	
	engine.Draw();
	
	if(gameData.ShouldShowLoad())
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% GPU";
		const float color[4] = {.4f, .4f, .4f, 0.f};
		FontSet::Get(14).Draw(loadString, Point(0., Screen::Height() * -.5), color);
	
		loadSum += loadTimer.Time();
		if(++loadCount == 60)
		{
			load = loadSum;
			loadSum = 0.;
			loadCount = 0;
		}
	}
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'm')
		Push(engine.Map());
	else if(key == SDLK_ESCAPE)
		Push(new MenuPanel(gameData));
	
	return true;
}
