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
#include "Command.h"
#include "GameData.h"
#include "FontSet.h"
#include "HiringPanel.h"
#include "InfoPanel.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MissionPanel.h"
#include "OutfitterPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Ship.h"
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
	spaceport.reset(new SpaceportPanel(player));
	hiring.reset(new HiringPanel(player));
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.Description());
	
	// Since the loading of landscape images is deferred, make sure that the
	// landscapes for this system are loaded before showing the planet panel.
	GameData::Preload(planet.Landscape());
	GameData::FinishLoading();
}



void PlanetPanel::Step()
{
	// If the previous mission callback resulted in a "launch", take off now.
	if(player.ShouldLaunch())
	{
		DoKey('d');
		return;
	}
	if(GetUI()->IsTop(this))
	{
		Mission *mission = player.MissionToOffer(Mission::LANDING);
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		else
			player.HandleBlockedMissions(Mission::LANDING, GetUI());
	}
}




void PlanetPanel::Draw() const
{
	if(player.IsDead())
		return;
	
	const Ship *flagship = player.Flagship();
	
	Information info;
	info.SetSprite("land", planet.Landscape());
	bool hasAccess = planet.CanUseServices();
	if(flagship && !flagship->CanBeCarried())
		info.SetCondition("has ship");
	if(planet.IsInhabited() && hasAccess)
		info.SetCondition("has bank");
	if(flagship && planet.IsInhabited() && hasAccess)
		info.SetCondition("is inhabited");
	if(flagship && planet.HasSpaceport() && hasAccess)
		info.SetCondition("has spaceport");
	if(planet.HasShipyard() && hasAccess)
		info.SetCondition("has shipyard");
	if(flagship && planet.HasOutfitter() && hasAccess)
		info.SetCondition("has outfitter");
	
	ui.Draw(info);
	
	if(!selectedPanel)
		text.Draw(Point(-300., 80.), *GameData::Colors().Get("bright"));
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	Panel *oldPanel = selectedPanel;
	const Ship *flagship = player.Flagship();
	
	bool hasAccess = planet.CanUseServices();
	if(key == 'd' && flagship && !flagship->CanBeCarried())
	{
		player.Save();
		player.TakeOff(GetUI());
		if(callback)
			callback();
		GetUI()->Pop(this);
	}
	else if(key == 'l')
	{
		selectedPanel = nullptr;
	}
	else if(key == 't' && flagship && planet.IsInhabited() && hasAccess)
	{
		selectedPanel = trading.get();
		GetUI()->Push(trading);
	}
	else if(key == 'b' && planet.IsInhabited() && hasAccess)
	{
		selectedPanel = bank.get();
		GetUI()->Push(bank);
	}
	else if(key == 'p' && flagship && planet.HasSpaceport() && hasAccess)
	{
		selectedPanel = spaceport.get();
		GetUI()->Push(spaceport);
	}
	else if(key == 's' && planet.HasShipyard() && hasAccess)
	{
		GetUI()->Push(new ShipyardPanel(player));
		return true;
	}
	else if(key == 'o' && flagship && planet.HasOutfitter() && hasAccess)
	{
		GetUI()->Push(new OutfitterPanel(player));
		return true;
	}
	else if(key == 'j' && flagship && planet.IsInhabited() && hasAccess)
	{
		GetUI()->Push(new MissionPanel(player));
		return true;
	}
	else if(key == 'h' && flagship && planet.IsInhabited() && hasAccess)
	{
		selectedPanel = hiring.get();
		GetUI()->Push(hiring);
	}
	else if(command.Has(Command::MAP))
	{
		GetUI()->Push(new MapDetailPanel(player));
		return true;
	}
	else if(command.Has(Command::INFO))
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
	if(key)
		return DoKey(key);
	
	return true;
}
