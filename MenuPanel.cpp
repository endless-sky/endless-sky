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
#include "Files.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Interface.h"
#include "Information.h"
#include "LoadPanel.h"
#include "MainPanel.h"
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



MenuPanel::MenuPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), scroll(0)
{
	SetIsFullScreen(true);
	
	ifstream in(Files::Resources() + "credits.txt");
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
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	if(player.IsLoaded())
	{
		info.SetCondition("pilot loaded");
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		if(player.GetShip())
		{
			const Ship &ship = *player.GetShip();
			info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
			info.SetString("ship", ship.Name());
		}
		if(player.GetSystem())
			info.SetString("system", player.GetSystem()->Name());
		if(player.GetPlanet())
			info.SetString("planet", player.GetPlanet()->Name());
		info.SetString("credits", Format::Number(player.Accounts().Credits()));
		info.SetString("date", player.GetDate().ToString());
	}
	else
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", "No Pilot Loaded");
	}
	
	const Interface *menu = GameData::Interfaces().Get("main menu");
	menu->Draw(info);
	
	int progress = static_cast<int>(GameData::Progress() * 60.);
	
	if(progress == 60)
	{
		if(!gamePanels.Root())
			gamePanels.Push(new MainPanel(player));
		alpha -= .02f;
	}
	if(alpha > 0.f)
	{
		Angle da(6.);
		Angle a(0.);
		for(int i = 0; i < progress; ++i)
		{
			Color color(alpha, 0.f);
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
			font.Draw(line, Point(-465., y), color);
		}
		y += 20;
	}
}



// New player "conversation" callback.
void MenuPanel::OnCallback(int)
{
	GetUI()->Pop(this);
	gamePanels.Reset();
	Panel *panel = new MainPanel(player);
	gamePanels.Push(panel);
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	panel->Step(true);
	gamePanels.Push(new ShipyardPanel(player));
}



bool MenuPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(GameData::Progress() < 1.)
		return false;
	
	if(player.IsLoaded() && (key == 'e' || key == GameData::Keys().Get(Key::MENU)))
		GetUI()->Pop(this);
	else if(key == 'p')
		GetUI()->Push(new PreferencesPanel());
	else if(key == 'l')
		GetUI()->Push(new LoadPanel(player, gamePanels));
	else if(key == 'n' || key == 'e')
	{
		// The "New Pilot" and "Enter Ship" buttons are in the same place.
		GameData::Revert();
		player.New();
		
		ConversationPanel *panel = new ConversationPanel(
			player, *GameData::Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(*this);
	}
	else if(key == 'q')
		GetUI()->Quit();
	else
		return false;
	
	return true;
}



bool MenuPanel::Click(int x, int y)
{
	char key = GameData::Interfaces().Get("main menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	
	return true;
}
