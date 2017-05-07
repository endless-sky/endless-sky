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
#include "LineShader.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Planet.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Preferences.h"
#include "Random.h"
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
		GetUI()->Push(new PlayerInfoPanel(player));
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
	const Ship *flagship = player.Flagship();
	if(flagship)
	{
		// Check if any help messages should be shown.
		if(isActive && flagship->IsTargetable())
			isActive = !DoHelp("navigation");
		if(isActive && flagship->IsDestroyed())
			isActive = !DoHelp("dead");
		if(isActive && flagship->IsDisabled())
			isActive = !DoHelp("disabled");
		bool canRefuel = player.GetSystem()->IsInhabited(flagship);
		if(isActive && !flagship->IsHyperspacing() && !flagship->JumpsRemaining() && !canRefuel)
			isActive = !DoHelp("stranded");
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
	}
	
	if(isActive)
		engine.Go();
	else
		canDrag = false;
	canClick = isActive;
}



void MainPanel::Draw()
{
	FrameTimer loadTimer;
	glClear(GL_COLOR_BUFFER_BIT);
	
	engine.Draw();
	
	if(isDragging)
	{
		Color color(.2, 1., 0., 0.);
		LineShader::Draw(dragSource, Point(dragSource.X(), dragPoint.Y()), .8, color);
		LineShader::Draw(Point(dragSource.X(), dragPoint.Y()), dragPoint, .8, color);
		LineShader::Draw(dragPoint, Point(dragPoint.X(), dragSource.Y()), .8, color);
		LineShader::Draw(Point(dragPoint.X(), dragSource.Y()), dragSource, .8, color);
	}
	
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
	// Run one step of the simulation to fill in the new planet locations.
	engine.Go();
	engine.Wait();
	engine.Step(true);
	// Start the next step of the simulatip because Step() above still thinks
	// the planet panel is up and therefore will not start it.
	engine.Go();
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
	else if(key == '-')
		Preferences::ZoomViewOut();
	else if(key == '=')
		Preferences::ZoomViewIn();
	else if(key >= '0' && key <= '9')
		engine.SelectGroup(key - '0', mod & KMOD_SHIFT, mod & (KMOD_CTRL | KMOD_GUI));
	else
		return false;
	
	return true;
}



bool MainPanel::Click(int x, int y, int clicks)
{
	// Don't respond to clicks if another panel is active.
	if(!canClick)
		return true;
	// Only allow drags that start when clicking was possible.
	canDrag = true;
	
	dragSource = Point(x, y);
	dragPoint = dragSource;
	
	SDL_Keymod mod = SDL_GetModState();
	hasShift = (mod & KMOD_SHIFT);
	
	engine.Click(dragSource, dragSource, hasShift);
	
	return true;
}



bool MainPanel::RClick(int x, int y)
{
	engine.RClick(Point(x, y));
	
	return true;
}



bool MainPanel::Drag(double dx, double dy)
{
	if(!canDrag)
		return true;
	
	dragPoint += Point(dx, dy);
	isDragging = true;
	return true;
}



bool MainPanel::Release(int x, int y)
{
	if(isDragging)
	{
		dragPoint = Point(x, y);
		if(dragPoint.Distance(dragSource) > 5.)
			engine.Click(dragSource, dragPoint, hasShift);
	
		isDragging = false;
	}
	
	return true;
}



bool MainPanel::Scroll(double dx, double dy)
{
	if(dy < 0)
		Preferences::ZoomViewOut();
	else if(dy > 0)
		Preferences::ZoomViewIn();
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
					out << "This " + target->Noun() + " is carrying:\n";
				first = false;
		
				out << "\t" << it.second
					<< (it.second == 1 ? " ton of " : " tons of ")
					<< it.first << "\n";
			}
		for(const auto &it : target->Cargo().Outfits())
			if(it.second)
			{
				if(first)
					out << "This " + target->Noun() + " is carrying:\n";
				first = false;
		
				out << "\t" << it.second << " "
					<< (it.second == 1 ? it.first->Name(): it.first->PluralName()) << "\n";
			}
		if(first)
			out << "This " + target->Noun() + " is not carrying any cargo.\n";
	}
	if((event.Type() & ShipEvent::SCAN_OUTFITS) && target->Attributes().Get("inscrutable"))
		out << "Your scanners cannot make any sense of this " + target->Noun() + "'s interior.";
	else if(event.Type() & ShipEvent::SCAN_OUTFITS)
	{
		out << "This " + target->Noun() + " is equipped with:\n";
		for(const auto &it : target->Outfits())
			if(it.first && it.second)
				out << "\t" << it.second << " "
					<< (it.second == 1 ? it.first->Name() : it.first->PluralName()) << "\n";
		
		map<string, int> count;
		for(const Ship::Bay &bay : target->Bays())
			if(bay.ship)
			{
				int &value = count[bay.ship->ModelName()];
				if(value)
				{
					// If the name and the plural name are the same string, just
					// update the count. Otherwise, clear the count for the
					// singular name and set it for the plural.
					int &pluralValue = count[bay.ship->PluralModelName()];
					if(!pluralValue)
					{
						value = -1;
						pluralValue = 1;
					}
					++pluralValue;
				}
				else
					++value;
			}
		if(!count.empty())
		{
			out << "This " + target->Noun() + " is carrying:\n";
			for(const auto &it : count)
				if(it.second > 0)
					out << "\t" << it.second << " " << it.first << "\n";
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
	if((SDL_GetModState() & KMOD_SHIFT) && flagship->GetTargetStellar())
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
			Messages::Add("Unable to hail target " + target->Noun() + ".");
		else if(target->IsEnteringHyperspace())
			Messages::Add("Unable to send hail: " + target->Noun() + " is entering hyperspace.");
		else
		{
			GetUI()->Push(new HailPanel(player, target));
			return true;
		}
	}
	else if(flagship->GetTargetStellar())
	{
		const Planet *planet = flagship->GetTargetStellar()->GetPlanet();
		if(planet && planet->IsWormhole())
		{
			static const vector<string> messages = {
				"The gaping hole in the fabric of the universe does not respond to your hail.",
				"Wormholes do not understand the language of finite beings like yourself.",
				"You stare into the swirling abyss, but with appalling bad manners it refuses to stare back.",
				"All the messages you try to send disappear into the wormhole without a trace.",
				"The spatial anomaly pointedly ignores your attempts to engage it in conversation.",
				"Like most wormholes, this one does not appear to be very talkative.",
				"The wormhole says nothing, but silently beckons you to explore its mysteries.",
				"You can't talk to wormholes. Maybe you should try landing on it instead.",
				"Your words cannot travel through wormholes, but maybe your starship can.",
				"Unable to send hail: this unfathomable void is not inhabited."
			};
			Messages::Add(messages[Random::Int(messages.size())]);
		}
		else if(planet && planet->IsInhabited())
		{
			GetUI()->Push(new HailPanel(player, flagship->GetTargetStellar()));
			return true;
		}
		else
			Messages::Add("Unable to send hail: " + planet->Noun() + " is not inhabited.");
	}
	else
		Messages::Add("Unable to send hail: no target selected.");
	
	return false;
}
