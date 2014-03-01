/* MenuPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MenuPanel.h"

#include "ConversationPanel.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Interface.h"
#include "Information.h"
#include "LoadPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PointerShader.h"
#include "PreferencesPanel.h"
#include "ShipyardPanel.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "UI.h"

#include <fstream>

using namespace std;

namespace {
	static float alpha = 1.;
	static const int scrollSpeed = 2;
}



MenuPanel::MenuPanel(GameData &gameData, PlayerInfo &playerInfo, UI &gamePanels)
	: gameData(gameData), playerInfo(playerInfo), gamePanels(gamePanels), scroll(0)
{
	SetIsFullScreen(true);
	
	ifstream in(gameData.ResourcePath() + "credits.txt");
	string line;
	while(getline(in, line))
		credits.push_back(line);
}



void MenuPanel::Step(bool isActive)
{
	if(isActive && alpha < 1.)
	{
		++scroll;
		if(scroll >= (20 * credits.size() + 300) * scrollSpeed)
			scroll = 0;
	}
}



void MenuPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	gameData.Background().Draw(Point(), Point());
	
	Information info;
	if(playerInfo.IsLoaded())
	{
		info.SetCondition("pilot loaded");
		info.SetString("pilot", playerInfo.FirstName() + " " + playerInfo.LastName());
		if(playerInfo.GetShip())
		{
			const Ship &ship = *playerInfo.GetShip();
			info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
			info.SetString("ship", ship.Name());
		}
		if(playerInfo.GetSystem())
			info.SetString("system", playerInfo.GetSystem()->Name());
		if(playerInfo.GetPlanet())
			info.SetString("planet", playerInfo.GetPlanet()->Name());
		info.SetString("credits", to_string(playerInfo.Accounts().Credits()));
		info.SetString("date", playerInfo.GetDate().ToString());
	}
	else
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", "No Pilot Loaded");
	}
	
	const Interface *menu = gameData.Interfaces().Get("main menu");
	menu->Draw(info);
	
	int progress = static_cast<int>(gameData.Progress() * 60.);
	
	if(progress == 60)
		alpha -= .02f;
	if(alpha > 0.f)
	{
		Angle da(6.);
		Angle a(0.);
		for(int i = 0; i < progress; ++i)
		{
			float color[4] = {alpha, alpha, alpha, 0.f};
			PointerShader::Draw(Point(), a.Unit(), 8., 20., 140. * alpha, color);
			a += da;
		}
	}
	
	const Font &font = FontSet::Get(14);
	int y = 120 - scroll / scrollSpeed;
	for(const string &line : credits)
	{
		float fade = 1.f;
		if(y < -145)
			fade = max(0.f, (y + 165) / 20.f);
		else if(y > 95)
			fade = max(0.f, (115 - y) / 20.f);
		if(fade)
		{
			Color color(((line.empty() || line[0] == ' ') ? .2 : .4) * fade, 0.);
			font.Draw(line, Point(-465., y), color.Get());
		}
		y += 20;
	}
}



// New player "conversation" callback.
void MenuPanel::OnCallback(int)
{
	GetUI()->Pop(this);
	shared_ptr<Panel> saved = gamePanels.Root();
	gamePanels.Reset();
	gamePanels.Push(saved);
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	saved->Step(true);
	gamePanels.Push(new ShipyardPanel(gameData, playerInfo));
}



bool MenuPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(gameData.Progress() < 1.)
		return false;
	
	if(key == 'e' || key == gameData.Keys().Get(Key::MENU))
		GetUI()->Pop(this);
	else if(key == 'p')
		GetUI()->Push(new PreferencesPanel(gameData));
	else if(key == 'l')
		GetUI()->Push(new LoadPanel(gameData, playerInfo, gamePanels));
	else if(key == 'n')
	{
		playerInfo.New(gameData);
		
		ConversationPanel *panel = new ConversationPanel(
			playerInfo, *gameData.Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(*this);
	}
	else if(key == 'q')
		GetUI()->Quit();
	
	return true;
}



bool MenuPanel::Click(int x, int y)
{
	char key = gameData.Interfaces().Get("main menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	return true;
}
