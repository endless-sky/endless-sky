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
#include "Dialog.h"
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

#include <sstream>

using namespace std;



PlanetPanel::PlanetPanel(PlayerInfo &player, function<void()> callback)
	: player(player), callback(callback),
	planet(*player.GetPlanet()), system(*player.GetSystem()),
	ui(*GameData::Interfaces().Get("planet"))
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




void PlanetPanel::Draw()
{
	if(player.IsDead())
		return;
	
	const Ship *flagship = player.Flagship();
	
	Information info;
	info.SetSprite("land", planet.Landscape());
	bool hasAccess = planet.CanUseServices();
	bool hasShip = false;
	for(const auto &it : player.Ships())
		if(it->GetSystem() == player.GetSystem() && !it->IsDisabled())
		{
			hasShip = true;
			break;
		}
	if(flagship && flagship->CanBeFlagship())
		info.SetCondition("has ship");
	if(planet.IsInhabited() && hasAccess)
		info.SetCondition("has bank");
	if(flagship && planet.IsInhabited() && hasAccess)
		info.SetCondition("is inhabited");
	if(flagship && planet.HasSpaceport() && hasAccess)
		info.SetCondition("has spaceport");
	if(planet.HasShipyard() && hasAccess)
		info.SetCondition("has shipyard");
	if(hasShip && planet.HasOutfitter() && hasAccess)
		info.SetCondition("has outfitter");
	
	ui.Draw(info, this);
	
	if(!selectedPanel)
		text.Draw(Point(-300., 80.), *GameData::Colors().Get("bright"));
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	Panel *oldPanel = selectedPanel;
	const Ship *flagship = player.Flagship();
	
	bool hasAccess = planet.CanUseServices();
	if(key == 'd' && flagship && flagship->CanBeFlagship())
	{
		// Check whether the player should be warned before taking off.
		if(player.ShouldLaunch())
			TakeOff();
		else
		{
			// Will you have to sell something other than regular cargo?
			int cargoToSell = -(player.Cargo().Free() + player.Cargo().CommoditiesSize());
			int droneCount = 0;
			int fighterCount = 0;
			for(const auto &it : player.Ships())
				if(!it->IsParked() && !it->IsDisabled() && it->GetSystem() == player.GetSystem())
				{
					const string &category = it->Attributes().Category();
					droneCount += (category == "Drone") - it->BaysFree(false);
					fighterCount += (category == "Fighter") - it->BaysFree(true);
				}
			
			if(fighterCount > 0 || droneCount > 0 || cargoToSell > 0)
			{
				ostringstream out;
				out << "If you take off now you will have to sell ";
				bool triple = (fighterCount > 0 && droneCount > 0 && cargoToSell > 0);
			
				if(fighterCount == 1)
					out << "a fighter";
				else if(fighterCount > 0)
					out << fighterCount << " fighters";
				if(fighterCount > 0 && (droneCount > 0 || cargoToSell > 0))
					out << (triple ? ", " : " and ");
			
				if(droneCount == 1)
					out << "a drone";
				else if(droneCount > 0)
					out << droneCount << " drones";
				if(droneCount > 0 && cargoToSell > 0)
					out << (triple ? ", and " : " and ");
			
				if(cargoToSell == 1)
					out << "a ton of cargo";
				else if(cargoToSell > 0)
					out << cargoToSell << " tons of cargo";
				out << " that you do not have space for. Are you sure you want to continue?";
				
				GetUI()->Push(new Dialog(this, &PlanetPanel::TakeOff, out.str()));
				return true;
			}
			else
				TakeOff();
		}
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
	else if(key == 'o' && planet.HasOutfitter() && hasAccess)
	{
		bool hasShip = false;
		for(const auto &it : player.Ships())
			if(it->GetSystem() == player.GetSystem() && !it->IsDisabled())
			{
				hasShip = true;
				break;
			}
		if(hasShip)
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



void PlanetPanel::TakeOff()
{
	player.Save();
	if(player.TakeOff(GetUI()))
	{
		if(callback)
			callback();
		if(selectedPanel)
			GetUI()->Pop(selectedPanel);
		GetUI()->Pop(this);
	}
}
