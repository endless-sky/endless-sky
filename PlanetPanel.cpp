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
#include "Font.h"
#include "FontSet.h"
#include "MapPanel.h"
#include "OutfitterPanel.h"
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
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.Description());
}



void PlanetPanel::Draw() const
{
	Information info;
	info.SetSprite("land", planet.Landscape());
	if(player.GetShip())
		info.SetCondition("has ship");
	if(planet.HasSpaceport() && player.GetShip())
		info.SetCondition("has spaceport");
	if(planet.HasShipyard())
		info.SetCondition("has shipyard");
	if(planet.HasOutfitter() && player.GetShip())
		info.SetCondition("has outfitter");
	
	ui.Draw(info);
	
	if(!selectedPanel)
		text.Draw(Point(-300., 80.), Color(.8));
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDLKey key, SDLMod mod)
{
	Panel *oldPanel = selectedPanel;
	
	if(key == 'd' && FlightCheck())
	{
		player.Save();
		callback();
		GetUI()->Pop(this);
	}
	else if(key == 'l')
	{
		selectedPanel = nullptr;
	}
	else if(key == 't' && planet.HasSpaceport())
	{
		selectedPanel = trading.get();
		GetUI()->Push(trading);
	}
	else if(key == 'b' && planet.HasSpaceport())
	{
		selectedPanel = bank.get();
		GetUI()->Push(bank);
	}
	else if(key == 'p' && planet.HasSpaceport())
	{
		selectedPanel = spaceport.get();
		GetUI()->Push(spaceport);
	}
	else if(key == 's' && planet.HasShipyard())
	{
		GetUI()->Push(new ShipyardPanel(data, player));
		return true;
	}
	else if(key == 'o' && planet.HasOutfitter())
	{
		GetUI()->Push(new OutfitterPanel(data, player));
		return true;
	}
	else if(key == 'j' || key == 'h')
	{
		GetUI()->Push(new ConversationPanel(player, *data.Conversations().Get("free worlds intro")));
		return true;
	}
	else if(key == data.Keys().Get(Key::MAP))
	{
		GetUI()->Push(new MapPanel(data, player));
		return true;
	}
	else
		return true;
	
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
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	return true;
}



bool PlanetPanel::FlightCheck()
{
	if(!player.GetShip())
		return false;
	
	const Outfit &attributes = player.GetShip()->Attributes();
	double energy = attributes.Get("energy generation") + attributes.Get("energy capacity");
	if(!attributes.Get("thrust"))
	{
		GetUI()->Push(new ConversationPanel(player,
			*data.Conversations().Get("flight check: no thrusters")));
		return false;
	}
	if(attributes.Get("thrusting energy") > energy)
	{
		GetUI()->Push(new ConversationPanel(player,
			*data.Conversations().Get("flight check: no thruster energy")));
		return false;
	}
	if(!attributes.Get("turn"))
	{
		GetUI()->Push(new ConversationPanel(player,
			*data.Conversations().Get("flight check: no steering")));
		return false;
	}
	if(attributes.Get("turning energy") > energy)
	{
		GetUI()->Push(new ConversationPanel(player,
			*data.Conversations().Get("flight check: no steering energy")));
		return false;
	}
	if(attributes.Get("heat generation") * 10. > player.GetShip()->Mass())
	{
		GetUI()->Push(new ConversationPanel(player,
			*data.Conversations().Get("flight check: overheating")));
		return false;
	}
	return true;
}
