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

#include "BoardingPanel.h"
#include "Dialog.h"
#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "MapPanel.h"
#include "Messages.h"
#include "PlanetPanel.h"
#include "Screen.h"
#include "UI.h"

#include <sstream>
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
		playerInfo.Land();
		GetUI()->Push(new PlanetPanel(gameData, playerInfo, *this));
		isActive = false;
	}
	
	engine.Step(isActive);
	
	if(engine.Boarding())
		GetUI()->Push(new BoardingPanel(gameData, playerInfo, engine.Boarding()));
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
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == gameData.Keys().Get(Key::MAP))
		GetUI()->Push(new MapPanel(gameData, playerInfo));
	else if(key == gameData.Keys().Get(Key::SCAN))
	{
		const Ship *player = playerInfo.GetShip();
		if(player)
		{
			double cargoRange = player->Attributes().Get("cargo scan");
			double outfitRange = player->Attributes().Get("outfit scan");
			shared_ptr<const Ship> target = player->GetTargetShip().lock();
			if(target && (cargoRange || outfitRange))
			{
				double distance = (player->Position() - target->Position()).Length();
				
				ostringstream out;
				if(distance < cargoRange)
				{
					bool first = true;
					for(const auto &it : target->Cargo().Commodities())
						if(it.second)
						{
							if(first)
								out << "This ship is carrying:\n";
							first = false;
					
							out << "\t" << it.second
								<< (it.second == 1 ? " ton of " : " tons of ")
								<< it.first << "\n";
						}
					if(first)
						out << "This ship is not carrying any cargo.\n";
				}
				if(distance < outfitRange)
				{
					out << "This ship is equipped with:\n";
					for(const auto &it : target->Outfits())
						if(it.first && it.second)
						{
							out << "\t" << it.first->Name();
							if(it.second != 1)
								out << " (" << it.second << ")";
							out << "\n";
						}
				}
				if(out.str().empty())
					Messages::Add("You are too far away to scan this ship.");
				else
					GetUI()->Push(new Dialog(out.str()));
			}
			else if(target)
				Messages::Add("You do not have any scanners installed.");
		}
	}
	else
		return false;
	
	return true;
}
