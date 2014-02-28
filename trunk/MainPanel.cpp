/* MainPanel.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the MainPanel class.
*/

#include "MainPanel.h"

#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "Screen.h"
#include "UI.h"

#include <string>

using namespace std;



MainPanel::MainPanel(const GameData &gameData, PlayerInfo &playerInfo)
	: gameData(gameData), playerInfo(playerInfo),
	engine(gameData, playerInfo),
	load(0.), loadSum(0.), loadCount(0)
{
	SetIsFullScreen(true);
}



void MainPanel::Step(bool isActive)
{
	engine.Step(isActive);
	
	Panel *panel = engine.PanelToShow();
	if(panel)
		GetUI()->Push(panel);
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
	if(key == gameData.Keys().Get(Key::MAP))
		GetUI()->Push(engine.Map());
	
	return true;
}
