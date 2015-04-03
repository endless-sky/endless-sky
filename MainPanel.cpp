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
		player.Land(GetUI());
		isActive = false;
	}
	if(isActive && player.GetShip() && player.GetShip()->IsTargetable()
			&& !Preferences::Has("help: navigation"))
	{
		Preferences::Set("help: navigation");
		ostringstream out;
		out << "Welcome to the sky! To travel to another star system, press \""
			<< Command::MAP.KeyName() << "\" to view your map, "
			<< "and click on the system you want to travel to. "
			<< "Your hyperdrive can only travel along the \"links\" shown on your map. "
			<< "After selecting a destination, close your map and press \""
			<< Command::JUMP.KeyName() << "\" to jump to that system.\n"
			<< "\tYour ship does not jump until you release the jump key. Once you have escorts, "
			<< "you can hold the key to get them ready to jump, "
			<< "then release it to have them all jump simultaneously.\n"
			<< "\tWhen you reach a new system, you can press \""
			<< Command::LAND.KeyName() << "\" to land on any inhabited planets that are there.\n"
			<< "\tAlso, don't worry about crashing into asteroids or other ships; "
			<< "your ship will fly safely below or above them.";
		GetUI()->Push(new Dialog(out.str()));
	}
	if(isActive && player.GetShip() && player.GetShip()->IsDestroyed()
			&& !Preferences::Has("help: dead"))
	{
		Preferences::Set("help: dead");
		ostringstream out;
		out << "Uh-oh! You just died. The universe can a dangerous place for new captains!\n"
			<< "\tFortunately, your game is automatically saved every time you leave a planet. "
			<< "To load your most recent saved game, press \"" + Command::MENU.KeyName()
			<< "\" to return to the main menu, then click on \"Load / Save\" and \"Enter Ship.\"";
		GetUI()->Push(new Dialog(out.str()));
	}
	
	engine.Step(isActive);
	
	for(const ShipEvent &event : engine.Events())
	{
		const Government *actor = event.ActorGovernment();
		
		player.HandleEvent(event, GetUI());
		if(event.Type() == ShipEvent::BOARD && isActive)
		{
			// TODO: handle player getting boarded.
			if(actor->IsPlayer())
				GetUI()->Push(new BoardingPanel(player, event.Target()));
		}
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(actor->IsPlayer() && isActive)
				ShowScanDialog(event);
			else if(event.TargetGovernment()->IsPlayer())
			{
				string message = actor->Fine(player, event.Type());
				if(!message.empty())
					GetUI()->Push(new Dialog(message));
			}
		}
		if((event.Type() & ShipEvent::JUMP) && player.GetShip() && !player.GetShip()->Fuel()
				&& !player.GetSystem()->IsInhabited() && !Preferences::Has("help: stranded"))
		{
			Preferences::Set("help: stranded");
			ostringstream out;
			out << "Oops! You just ran out of fuel in an uninhabited system. "
				<< "Fortunately, other ships are willing to help you.\n\tPress \""
				<< Command::TARGET.KeyName() << "\" to cycle through all the ships in this system. "
				<< "When you have a friendly one selected, press \""
				<< Command::HAIL.KeyName() << "\" to hail it. "
				<< "You can then ask for help, "
				<< "and if it has fuel to spare it will fly over and transfer fuel to your ship. "
				<< "This is easiest for the other ship to do if your ship is nearly stationary.";
			GetUI()->Push(new Dialog(out.str()));
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
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command == Command::MAP)
		GetUI()->Push(new MapDetailPanel(player));
	else if(command == Command::INFO)
		GetUI()->Push(new InfoPanel(player));
	else if(command == Command::HAIL)
		ShowHailPanel();
	else
		return false;
	
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
		if((SDL_GetModState() & KMOD_SHIFT) && ship->GetTargetPlanet())
			target.reset();
		if(target)
		{
			if(target->IsEnteringHyperspace())
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
