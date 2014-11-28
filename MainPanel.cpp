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
#include "Preferences.h"
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



void MainPanel::Step()
{
	bool isActive = GetUI()->IsTop(this);
	
	// If the player just landed, pop up the planet panel. When it closes, it
	// will call this object's OnCallback() function;
	if(isActive && player.GetPlanet() && !player.GetPlanet()->IsWormhole())
	{
		GetUI()->Push(new PlanetPanel(player, bind(&MainPanel::OnCallback, this)));
		
		// Check if the player is doing anything illegal.
		const Government *gov = player.GetSystem()->GetGovernment();
		string message = GameData::GetPolitics().Fine(player, gov, 0, player.GetPlanet()->Security());
		if(!message.empty())
		{
			if(message == "atrocity")
			{
				const Conversation *conversation = gov->DeathSentence();
				if(conversation)
					GetUI()->Push(new ConversationPanel(player, *conversation));
				else
				{
					message = "Before you can leave your ship, the " + gov->GetName()
						+ " authorities show up and begin scanning it. They say, \"Captain "
						+ player.LastName()
						+ ", we detect highly illegal material on your ship.\""
						"\n\tYou are sentenced to lifetime imprisonment on a penal colony."
						" Your days of traveling the stars have come to an end.";
					GetUI()->Push(new Dialog(message));
				}
				player.Die();
			}
			else
				GetUI()->Push(new Dialog(message));
		}
		
		FinishMissions();
		player.Land();
		isActive = false;
	}
	
	engine.Step(isActive);
	
	const Government *playerGov = GameData::PlayerGovernment();
	for(const ShipEvent &event : engine.Events())
	{
		const Government *actor = event.ActorGovernment();
		
		player.HandleEvent(event, GetUI());
		if(event.Type() == ShipEvent::BOARD)
		{
			// TODO: handle player getting boarded.
			if(actor == playerGov)
				GetUI()->Push(new BoardingPanel(player, event.Target()));
		}
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(actor == playerGov)
				ShowScanDialog(event);
			else if(event.TargetGovernment() == playerGov)
			{
				string message = GameData::GetPolitics().Fine(player, actor, event.Type());
				if(!message.empty())
					GetUI()->Push(new Dialog(message));
			}
		}
	}
}



void MainPanel::Draw() const
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



void MainPanel::FinishMissions()
{
	auto it = player.Missions().begin();
	while(it != player.Missions().end())
	{
		const Mission &mission = *it;
		++it;
		
		if(mission.HasFailed(player))
			player.RemoveMission(Mission::FAIL, mission, GetUI());
		else if(mission.CanComplete(player))
			player.RemoveMission(Mission::COMPLETE, mission, GetUI());
		else if(mission.Destination() == player.GetPlanet())
			mission.Do(Mission::VISIT, player, GetUI());
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
		vector<shared_ptr<Ship>> carried = target->CarriedShips();
		if(!carried.empty())
		{
			out << "This ship is carrying:\n";
			map<string, int> count;
			for(const shared_ptr<Ship> &fighter : carried)
				++count[fighter->ModelName()];
			
			for(const auto &it : count)
				out << "\t" << it.second << " " << it.first << (it.second == 1 ? "\n" : "s\n");
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
		if(ship->IsDestroyed())
			return;
		
		shared_ptr<Ship> target = ship->GetTargetShip();
		if(target)
		{
			if(target->IsHyperspacing())
				Messages::Add("Unable to send hail: ship is entering hyperspace.");
			else if(!target->IsDestroyed() && target->GetSystem() == player.GetSystem())
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
