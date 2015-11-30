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
	{
		ShowHailPanel();
		isActive = false;
	}
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
		isActive = false;
	}
	if(isActive && player.Flagship() && player.Flagship()->IsDestroyed()
			&& !Preferences::Has("help: dead"))
	{
		Preferences::Set("help: dead");
		ostringstream out;
		out << "Uh-oh! You just died. The universe can a dangerous place for new captains!\n"
			<< "\tFortunately, your game is automatically saved every time you leave a planet. "
			<< "To load your most recent saved game, press \"" + Command::MENU.KeyName()
			<< "\" to return to the main menu, then click on \"Load / Save\" and \"Enter Ship.\"";
		GetUI()->Push(new Dialog(out.str()));
		isActive = false;
	}
	if(isActive && player.Flagship() && player.Flagship()->IsDisabled()
			&& !player.Flagship()->IsDestroyed() && !Preferences::Has("help: disabled"))
	{
		Preferences::Set("help: disabled");
		ostringstream out;
		out << "Your ship just got disabled! "
				<< "Before an enemy ship finishes you off, you should find someone to help you.\n\tPress \""
				<< Command::TARGET.KeyName() << "\" to cycle through all the ships in this system. "
				<< "When you have a friendly one selected, press \""
				<< Command::HAIL.KeyName() << "\" to hail it. "
				<< "You can then ask for help, and the ship will come over and patch you up."
				<< "\n\tIf the ship that disabled you is still hanging around, "
				<< "you might need to hail them first and bribe them to leave you alone.";
		GetUI()->Push(new Dialog(out.str()));
		isActive = false;
	}
	
	engine.Step(isActive);
	
	for(const ShipEvent &event : engine.Events())
	{
		const Government *actor = event.ActorGovernment();
		
		player.HandleEvent(event, GetUI());
		if((event.Type() & (ShipEvent::BOARD | ShipEvent::ASSIST)) && isActive && actor->IsPlayer())
		{
			const Mission *mission = player.BoardingMission(event.Target());
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
		if((event.Type() & ShipEvent::JUMP) && player.Flagship() && !player.Flagship()->Fuel()
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
			isActive = false;
		}
	}
	
	if(isActive)
		engine.Go();
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
	if(command.Has(Command::MAP | Command::INFO | Command::HAIL))
		show = command;
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
	// An exploding ship cannot communicate.
	const Ship *flagship = player.Flagship();
	if(!flagship || flagship->IsDestroyed())
		return;
	
	shared_ptr<Ship> target = flagship->GetTargetShip();
	if((SDL_GetModState() & KMOD_SHIFT) && flagship->GetTargetPlanet())
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
	else if(flagship->GetTargetPlanet())
	{
		const Planet *planet = flagship->GetTargetPlanet()->GetPlanet();
		if(planet && planet->IsInhabited())
			GetUI()->Push(new HailPanel(player, flagship->GetTargetPlanet()));
		else
			Messages::Add("Unable to send hail: " + planet->Noun() + " is not inhabited.");
	}
	else
		Messages::Add("Unable to send hail: no target selected.");
}
