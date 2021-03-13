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

#include "Command.h"
#include "ConversationPanel.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Interface.h"
#include "Information.h"
#include "LoadPanel.h"
#include "MainPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PointerShader.h"
#include "PreferencesPanel.h"
#include "Ship.h"
#include "ShipyardPanel.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "StartConditionsPanel.h"
#include "System.h"
#include "UI.h"

#include "gl_header.h"

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace {
	float alpha = 1.f;
	const int scrollSpeed = 2;
}



MenuPanel::MenuPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), scroll(0)
{
	SetIsFullScreen(true);
	
	credits = Format::Split(Files::Read(Files::Resources() + "credits.txt"), "\n");
}



void MenuPanel::Step()
{
	if(GetUI()->IsTop(this) && alpha < 1.f)
	{
		++scroll;
		if(scroll >= (20 * credits.size() + 300) * scrollSpeed)
			scroll = 0;
	}
	progress = static_cast<int>(GameData::Progress() * 60.);
	if(GameData::IsLoaded() && gamePanels.IsEmpty())
	{
		gamePanels.Push(new MainPanel(player));
		// It takes one step to figure out the planet panel should be created, and
		// another step to actually place it. So, take two steps to avoid a flicker.
		gamePanels.StepAll();
		gamePanels.StepAll();
	}
}



void MenuPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	const Font &font = FontSet::Get(14);
	
	Information info;
	if(player.IsLoaded() && !player.IsDead())
	{
		info.SetCondition("pilot loaded");
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		if(player.Flagship())
		{
			const Ship &flagship = *player.Flagship();
			info.SetSprite("ship sprite", flagship.GetSprite());
			info.SetString("ship", flagship.Name());
		}
		if(player.GetSystem())
			info.SetString("system", player.GetSystem()->Name());
		if(player.GetPlanet())
			info.SetString("planet", player.GetPlanet()->Name());
		info.SetString("credits", Format::Credits(player.Accounts().Credits()));
		info.SetString("date", player.GetDate().ToString());
		info.SetString("playtime", Format::PlayTime(player.GetPlayTime()));
	}
	else if(player.IsLoaded())
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		info.SetString("ship", "You have died.");
	}
	else
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", "No Pilot Loaded");
	}
	
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("main menu")->Draw(info, this);
	GameData::Interfaces().Get("menu player info")->Draw(info, this);
	
	if(progress == 60)
		alpha -= .02f;
	if(alpha > 0.f)
	{
		Angle da(6.);
		Angle a(0.);
		for(int i = 0; i < progress; ++i)
		{
			Color color(.5f * alpha, 0.f);
			PointerShader::Draw(Point(), a.Unit(), 8.f, 20.f, 140.f * alpha, color);
			a += da;
		}
	}
	
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
			Color color(((line.empty() || line[0] == ' ') ? .2f : .4f) * fade, 0.f);
			font.Draw(line, Point(-470., y), color);
		}
		y += 20;
	}
}



bool MenuPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(!GameData::IsLoaded())
		return false;
	
	if(player.IsLoaded() && (key == 'e' || command.Has(Command::MENU)))
	{
		gamePanels.CanSave(true);
		GetUI()->Pop(this);
	}
	else if(key == 'p')
		GetUI()->Push(new PreferencesPanel());
	else if(key == 'l')
		GetUI()->Push(new LoadPanel(player, gamePanels));
	else if(key == 'n' && (!player.IsLoaded() || player.IsDead()))
	{
		// If no player is loaded, the "Enter Ship" button becomes "New Pilot."
		// Request that the player chooses a start scenario.
		// StartConditionsPanel also handles the case where there's no scenarios.
		GetUI()->Push(new StartConditionsPanel(player, gamePanels, GameData::StartOptions(), nullptr));
	}
	else if(key == 'q')
		GetUI()->Quit();
	else
		return false;
	
	return true;
}
