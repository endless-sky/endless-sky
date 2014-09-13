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
#include "ConversationPanel.h"
#include "Dialog.h"
#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "HailPanel.h"
#include "InfoPanel.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "UI.h"

#include <functional>
#include <set>
#include <sstream>
#include <string>

using namespace std;



MainPanel::MainPanel(PlayerInfo &player)
	: player(player), engine(player), load(0.), loadSum(0.), loadCount(0)
{
	SetIsFullScreen(true);
}



void MainPanel::Step(bool isActive)
{
	// If the player just landed, pop up the planet panel. When it closes, it
	// will call this object's OnCallback() function;
	if(isActive && player.GetPlanet())
	{
		player.Land();
		GetUI()->Push(new PlanetPanel(player, bind(&MainPanel::OnCallback, this)));
		FinishNormalMissions();
		FinishSpecialMissions();
		isActive = false;
	}
	
	engine.Step(isActive);
	
	const Government *gov = GameData::PlayerGovernment();
	for(const ShipEvent &event : engine.Events())
	{
		if(event.Type() == ShipEvent::BOARD)
		{
			// TODO: handle player getting boarded.
			if(event.ActorGovernment() == gov)
				GetUI()->Push(new BoardingPanel(player, event.Target()));
		}
		if(event.Type() == ShipEvent::DESTROY && event.TargetGovernment() == gov)
		{
			// Some missions have both cargo and passengers. Avoid aborting them
			// twice, because the name may no longer be accessible the second time.
			set<const Mission *> failed;
			for(const auto &it : event.Target()->Cargo().MissionCargo())
				failed.insert(it.first);
			for(const auto &it : event.Target()->Cargo().PassengerList())
				failed.insert(it.first);
			
			for(const Mission *mission : failed)
			{
				Messages::Add("Ship lost. Mission failed: \"" + mission->Name() + "\".");
				player.AbortMission(*mission);
			}
		}
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(event.ActorGovernment() == gov)
				ShowScanDialog(event);
		}
	}
}



void MainPanel::Draw() const
{
	FrameTimer loadTimer;
	glClear(GL_COLOR_BUFFER_BIT);
	
	engine.Draw();
	
	if(GameData::ShouldShowLoad())
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% GPU";
		Color color = *GameData::Colors().Get("medium");
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
void MainPanel::OnCallback()
{
	engine.Place();
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == GameData::Keys().Get(Key::MAP))
		GetUI()->Push(new MapDetailPanel(player));
	else if(key == GameData::Keys().Get(Key::INFO))
		GetUI()->Push(new InfoPanel(player));
	else if(key == GameData::Keys().Get(Key::HAIL))
		ShowHailPanel();
	else
		return false;
	
	return true;
}



void MainPanel::FinishNormalMissions()
{
	auto it = player.Missions().begin();
	while(it != player.Missions().end())
	{
		const Mission &mission = *it;
		++it;
		
		if(mission.Destination() == player.GetPlanet())
		{
			GetUI()->Push(new Dialog(mission.SuccessMessage()));
			player.CompleteMission(mission);
		}
	}
	player.UpdateCargoCapacities();
}



void MainPanel::FinishSpecialMissions()
{
	auto it = player.SpecialMissions().begin();
	while(it != player.SpecialMissions().end())
	{
		const Mission &mission = **it;
		++it;
		
		if(mission.Destination() == player.GetPlanet())
		{
			// Check that all this mission's requirements are met.
			if(mission.Next() && !player.CanAccept(*mission.Next()))
				continue;
			// You can't complete the mission if some of the cargo is in a ship
			// stranded in another system.
			if(player.Cargo().Get(&mission) != mission.CargoSize())
				continue;
			if(player.Cargo().GetPassengers(&mission) != mission.Passengers())
				continue;
			
			if(!mission.SuccessMessage().empty())
				GetUI()->Push(new Dialog(mission.SuccessMessage()));
			if(mission.Next() && !mission.Next()->Introduction().IsEmpty())
			{
				GetUI()->Push(new ConversationPanel(
					player,
					mission.Next()->Introduction(),
					mission.Next()->Destination()->GetSystem()));
			}
			player.CompleteMission(mission);
		}
	}
	player.UpdateCargoCapacities();
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
	}
	GetUI()->Push(new Dialog(out.str()));
}



void MainPanel::ShowHailPanel()
{
	const Ship *ship = player.GetShip();
	if(ship)
	{
		// An exploding ship cannot communicate.
		if(ship->Hull() <= 0.)
			return;
		
		shared_ptr<Ship> target = ship->GetTargetShip();
		if(target)
		{
			if(target->IsHyperspacing())
				Messages::Add("Unable to send hail: ship is entering hyperspace.");
			else if(target->Hull() > 0. && target->GetSystem() == player.GetSystem())
				GetUI()->Push(new HailPanel(player, target));
			else
				Messages::Add("Unable to hail target ship.");
		}
		else if(ship->GetTargetPlanet())
		{
			if(ship->GetTargetPlanet()->GetPlanet())
				GetUI()->Push(new HailPanel(player, ship->GetTargetPlanet()));
			else
				Messages::Add("Unable to send hail: planet is not inhabited.");
		}
		else
			Messages::Add("Unable to send hail: no target selected.");
	}
}
