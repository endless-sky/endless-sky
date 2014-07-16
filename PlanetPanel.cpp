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
#include "GameData.h"
#include "FontSet.h"
#include "HiringPanel.h"
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



PlanetPanel::PlanetPanel(const GameData &data, PlayerInfo &player, const Callback &callback)
	: data(data), player(player), callback(callback),
	planet(*player.GetPlanet()), system(*player.GetSystem()),
	ui(*data.Interfaces().Get("planet")),
	selectedPanel(nullptr)
{
	trading.reset(new TradingPanel(data, player));
	bank.reset(new BankPanel(player));
	spaceport.reset(new SpaceportPanel(planet.SpaceportDescription()));
	hiring.reset(new HiringPanel(data, player));
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.Description());
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
		text.Draw(Point(-300., 80.), Color(.8));
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	Panel *oldPanel = selectedPanel;
	const Ship *ship = player.GetShip();
	
	if(key == 'd')
	{
		// Only save if this is a planet that refuels you, to avoid an auto-save
		// of a pilot who is out of fuel with no help in sight.
		if(planet.HasSpaceport())
			player.Save();
		player.TakeOff();
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
	else if(key == 'b' && ship && planet.HasSpaceport())
	{
		selectedPanel = bank.get();
		GetUI()->Push(bank);
	}
	else if(key == 'p' && ship && planet.HasSpaceport())
	{
		selectedPanel = spaceport.get();
		GetUI()->Push(spaceport);
	}
	else if(key == 's' && planet.HasShipyard())
	{
		GetUI()->Push(new ShipyardPanel(data, player));
		return true;
	}
	else if(key == 'o' && ship && (planet.HasOutfitter() || player.Cargo().HasOutfits()))
	{
		GetUI()->Push(new OutfitterPanel(data, player));
		return true;
	}
	else if(key == 'j')
	{
		GetUI()->Push(new MissionPanel(data, player));
		return true;
	}
	else if(key == 'h')
	{
		selectedPanel = hiring.get();
		GetUI()->Push(hiring);
	}
	else if(key == data.Keys().Get(Key::MAP))
	{
		GetUI()->Push(new MapDetailPanel(data, player));
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
