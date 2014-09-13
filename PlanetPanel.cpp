/* PlanetPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PlanetPanel.h"

#include "Information.h"

#include "BankPanel.h"
#include "Color.h"
#include "ConversationPanel.h"
#include "GameData.h"
#include "FontSet.h"
#include "HiringPanel.h"
#include "InfoPanel.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MissionPanel.h"
#include "OutfitterPanel.h"
#include "PlayerInfo.h"
#include "ShipyardPanel.h"
#include "SpaceportPanel.h"
#include "TradingPanel.h"
#include "UI.h"

using namespace std;



PlanetPanel::PlanetPanel(PlayerInfo &player, function<void()> callback)
	: player(player), callback(callback),
	planet(*player.GetPlanet()), system(*player.GetSystem()),
	ui(*GameData::Interfaces().Get("planet")),
	selectedPanel(nullptr)
{
	trading.reset(new TradingPanel(player));
	bank.reset(new BankPanel(player));
	spaceport.reset(new SpaceportPanel(planet.SpaceportDescription()));
	hiring.reset(new HiringPanel(player));
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.Description());
	
	// Since the loading of landscape images is deferred, make sure that the
	// landscapes for this system are loaded before showing the planet panel.
	GameData::FinishLoading();
}



void PlanetPanel::Draw() const
{
	const Ship *ship = player.GetShip();
	
	Information info;
	info.SetSprite("land", planet.Landscape());
	if(player.GetShip())
		info.SetCondition("has ship");
	if(ship && planet.HasSpaceport())
		info.SetCondition("has spaceport");
	if(planet.HasShipyard())
		info.SetCondition("has shipyard");
	if(ship && (planet.HasOutfitter() || player.Cargo().HasOutfits()))
		info.SetCondition("has outfitter");
	
	ui.Draw(info);
	
	if(!selectedPanel)
		text.Draw(Point(-300., 80.), *GameData::Colors().Get("bright"));
}



// Conversation callback for new special missions.
void PlanetPanel::OnCallback(int value)
{
	if(value == Conversation::DIE)
	{
		// TODO: handle "DIE".
	}
	else if(value == Conversation::ACCEPT)
		player.AcceptSpecialMission();
	else
		player.DeclineSpecialMission();
	
	if(player.NextSpecialMission())
	{
		ConversationPanel *panel = new ConversationPanel(
			player,
			player.NextSpecialMission()->Introduction(),
			player.NextSpecialMission()->Destination()->GetSystem());
		panel->SetCallback(this, &PlanetPanel::OnCallback);
		GetUI()->Push(panel);
	}
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	Panel *oldPanel = selectedPanel;
	const Ship *ship = player.GetShip();
	
	if(key == 'd' && ship)
	{
		// Only save if this is a planet that refuels you, to avoid an auto-save
		// of a pilot who is out of fuel with no help in sight.
		if(planet.HasSpaceport())
			player.Save();
		player.TakeOff();
		if(callback)
			callback();
		GetUI()->Pop(this);
	}
	else if(key == 'l')
	{
		selectedPanel = nullptr;
	}
	else if(key == 't' && ship && planet.HasSpaceport())
	{
		selectedPanel = trading.get();
		GetUI()->Push(trading);
	}
	else if(key == 'b' && planet.HasSpaceport())
	{
		selectedPanel = bank.get();
		GetUI()->Push(bank);
	}
	else if(key == 'p' && ship && planet.HasSpaceport())
	{
		selectedPanel = spaceport.get();
		GetUI()->Push(spaceport);
		if(player.NextSpecialMission())
		{
			ConversationPanel *panel = new ConversationPanel(
				player,
				player.NextSpecialMission()->Introduction(),
			player.NextSpecialMission()->Destination()->GetSystem());
			panel->SetCallback(this, &PlanetPanel::OnCallback);
			GetUI()->Push(panel);
		}
	}
	else if(key == 's' && planet.HasShipyard())
	{
		GetUI()->Push(new ShipyardPanel(player));
		return true;
	}
	else if(key == 'o' && ship && (planet.HasOutfitter() || player.Cargo().HasOutfits()))
	{
		GetUI()->Push(new OutfitterPanel(player));
		return true;
	}
	else if(key == 'j' && ship)
	{
		GetUI()->Push(new MissionPanel(player));
		return true;
	}
	else if(key == 'h' && ship)
	{
		selectedPanel = hiring.get();
		GetUI()->Push(hiring);
	}
	else if(key == GameData::Keys().Get(Key::MAP))
	{
		GetUI()->Push(new MapDetailPanel(player));
		return true;
	}
	else if(key == GameData::Keys().Get(Key::INFO))
	{
		GetUI()->Push(new InfoPanel(player));
		return true;
	}
	else
		return false;
	
	// If we are here, it is because something happened to change the selected
	// panel. So, we need to pop the old selected panel:
	if(oldPanel)
		GetUI()->Pop(oldPanel);
	
	return true;
}



bool PlanetPanel::Click(int x, int y)
{
	char key = ui.OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	
	return true;
}
