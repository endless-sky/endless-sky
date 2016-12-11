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
#include "Command.h"
#include "Dialog.h"
#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "Government.h"
#include "HailPanel.h"
#include "InfoPanel.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Planet.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Screen.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <sstream>
#include <string>

using namespace std;



MainPanel::MainPanel(PlayerInfo &player)
	: player(player), engine(player), load(0.), loadSum(0.), loadCount(0)
{
	SetIsFullScreen(true);
}



void MainPanel::Step()
{
	engine.Wait();
	
	bool isActive = GetUI()->IsTop(this);
	
	if(show.Has(Command::MAP))
	{
		GetUI()->Push(new MapDetailPanel(player));
		isActive = false;
	}
	else if(show.Has(Command::INFO))
	{
		GetUI()->Push(new InfoPanel(player));
		isActive = false;
	}
	else if(show.Has(Command::HAIL))
		isActive = !ShowHailPanel();
	
	show = Command::NONE;
	
	// If the player just landed, pop up the planet panel. When it closes, it
	// will call this object's OnCallback() function;
	if(isActive && player.GetPlanet() && !player.GetPlanet()->IsWormhole())
	{
		GetUI()->Push(new PlanetPanel(player, bind(&MainPanel::OnCallback, this)));
		player.Land(GetUI());
		isActive = false;
	}
	if(isActive && player.Flagship() && player.Flagship()->IsTargetable()
			&& !Preferences::Has("help: navigation"))
	{
		Preferences::Set("help: navigation");
		GetUI()->Push(new Dialog(GameData::HelpMessage("navigation")));
		isActive = false;
	}
	if(isActive && player.Flagship() && player.Flagship()->IsDestroyed()
			&& !Preferences::Has("help: dead"))
	{
		Preferences::Set("help: dead");
		GetUI()->Push(new Dialog(GameData::HelpMessage("dead")));
		isActive = false;
	}
	if(isActive && player.Flagship() && player.Flagship()->IsDisabled()
			&& !player.Flagship()->IsDestroyed() && !Preferences::Has("help: disabled"))
	{
		Preferences::Set("help: disabled");
		GetUI()->Push(new Dialog(GameData::HelpMessage("disabled")));
		isActive = false;
	}
	
	engine.Step(isActive);
	
	for(const ShipEvent &event : engine.Events())
	{
		const Government *actor = event.ActorGovernment();
		
		player.HandleEvent(event, GetUI());
		if((event.Type() & (ShipEvent::BOARD | ShipEvent::ASSIST)) && isActive && actor->IsPlayer()
				&& event.Actor().get() == player.Flagship())
		{
			// Boarding events are only triggered by your flagship.
			Mission *mission = player.BoardingMission(event.Target());
			if(mission)
				mission->Do(Mission::OFFER, player, GetUI());
			else if(event.Type() == ShipEvent::BOARD)
			{
				GetUI()->Push(new BoardingPanel(player, event.Target()));
				isActive = false;
			}
		}
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(actor->IsPlayer() && isActive)
				ShowScanDialog(event);
			else if(event.TargetGovernment()->IsPlayer())
			{
				string message = actor->Fine(player, event.Type(), &*event.Target());
				if(!message.empty())
				{
					GetUI()->Push(new Dialog(message));
					isActive = false;
				}
			}
		}
		if((event.Type() & ShipEvent::JUMP) && player.Flagship() && !player.Flagship()->JumpsRemaining()
				&& !player.GetSystem()->IsInhabited() && !Preferences::Has("help: stranded"))
		{
			Preferences::Set("help: stranded");
			GetUI()->Push(new Dialog(GameData::HelpMessage("stranded")));
			isActive = false;
		}
	}
	
	if(isActive)
		engine.Go();
}



void MainPanel::Draw()
{
	FrameTimer loadTimer;
	glClear(GL_COLOR_BUFFER_BIT);
	
	engine.Draw();
	
	if(Preferences::Has("Show CPU / GPU load"))
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% GPU";
		Color color = *GameData::Colors().Get("medium");
		FontSet::Get(14).Draw(loadString, Point(10., Screen::Height() * -.5 + 5.), color);
	
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
void MainPanel::OnCallback()
{
	engine.Place();
	engine.Step(true);
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP | Command::INFO | Command::HAIL))
		show = command;
	else if(command.Has(Command::AMMO))
	{
		Preferences::ToggleAmmoUsage();
		Messages::Add("Your escorts will now expend ammo: " + Preferences::AmmoUsage() + ".");
	}
	else
		return false;
	
	return true;
}



bool MainPanel::Click(int x, int y)
{
	engine.Click(Point(x, y));
	return true;
}



void MainPanel::ShowScanDialog(const ShipEvent &event)
{
	shared_ptr<Ship> target = event.Target();
	
	ostringstream out;
	if(event.Type() & ShipEvent::SCAN_CARGO)
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
	if(event.Type() & ShipEvent::SCAN_OUTFITS)
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
		map<string, int> count;
		for(const Ship::Bay &bay : target->Bays())
			if(bay.ship)
				++count[bay.ship->ModelName()];
		if(!count.empty())
		{
			out << "This ship is carrying:\n";
			for(const auto &it : count)
				out << "\t" << it.second << " " << it.first << (it.second == 1 ? "\n" : "s\n");
		}
	}
	GetUI()->Push(new Dialog(out.str()));
}



bool MainPanel::ShowHailPanel()
{
	// An exploding ship cannot communicate.
	const Ship *flagship = player.Flagship();
	if(!flagship || flagship->IsDestroyed())
		return false;
	
	shared_ptr<Ship> target = flagship->GetTargetShip();
	if((SDL_GetModState() & KMOD_SHIFT) && flagship->GetTargetPlanet())
		target.reset();
	
	if(flagship->IsEnteringHyperspace())
		Messages::Add("Unable to send hail: your flagship is entering hyperspace.");
	else if(flagship->Cloaking() == 1.)
		Messages::Add("Unable to send hail: your flagship is cloaked.");
	else if(target)
	{
		// If the target is out of system, always report a generic response
		// because the player has no way of telling if it's presently jumping or
		// not. If it's in system and jumping, report that.
		if(target->Zoom() < 1. || target->IsDestroyed() || target->GetSystem() != player.GetSystem()
				|| target->Cloaking() == 1.)
			Messages::Add("Unable to hail target ship.");
		else if(target->IsEnteringHyperspace())
			Messages::Add("Unable to send hail: ship is entering hyperspace.");
		else
		{
			GetUI()->Push(new HailPanel(player, target));
			return true;
		}
	}
	else if(flagship->GetTargetPlanet())
	{
		const Planet *planet = flagship->GetTargetPlanet()->GetPlanet();
		if(planet && planet->IsInhabited())
		{
			GetUI()->Push(new HailPanel(player, flagship->GetTargetPlanet()));
			return true;
		}
		else
			Messages::Add("Unable to send hail: " + planet->Noun() + " is not inhabited.");
	}
	else
		Messages::Add("Unable to send hail: no target selected.");
	
	return false;
}
