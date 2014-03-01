/* MainPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MainPanel.h"

#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "MapPanel.h"
#include "PlanetPanel.h"
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
	// If the player just landed, pop up the planet panel. When it closes, it
	// will call this object's OnCallback() function;
	if(isActive && playerInfo.GetPlanet())
	{
		GetUI()->Push(new PlanetPanel(gameData, playerInfo, *this));
		isActive = false;
	}
	
	engine.Step(isActive);
}



void MainPanel::Draw() const
{
	FrameTimer loadTimer;
	glClear(GL_COLOR_BUFFER_BIT);
	
	engine.Draw();
	
	if(gameData.ShouldShowLoad())
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% GPU";
		Color color(.4, 0.);
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



// The planet panel calls this when it closes.
void MainPanel::OnCallback(int)
{
	engine.Place();
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == gameData.Keys().Get(Key::MAP))
		GetUI()->Push(new MapPanel(gameData, playerInfo));
	
	return true;
}
