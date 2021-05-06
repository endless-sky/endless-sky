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
#include "CoreStartData.h"
#include "Dialog.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "Government.h"
#include "HailPanel.h"
#include "LineShader.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Mission.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Preferences.h"
#include "Random.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include "gl_header.h"

#include <cmath>
#include <sstream>
#include <string>

using namespace std;



MainPanel::MainPanel(PlayerInfo &player)
	: player(player), engine(player)
{
	SetIsFullScreen(true);
}



void MainPanel::Step()
{
	engine.Wait();
	
	// Depending on what UI element is on top, the game is "paused." This
	// checks only already-drawn panels.
	bool isActive = GetUI()->IsTop(this);
	
	// Display any requested panels.
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
	
	// Display any relevant help/tutorial messages.
	const Ship *flagship = player.Flagship();
	if(flagship)
	{
		// Check if any help messages should be shown.
		if(isActive && flagship->IsTargetable())
			isActive = !DoHelp("navigation");
		if(isActive && flagship->IsDestroyed())
			isActive = !DoHelp("dead");
		if(isActive && flagship->IsDisabled() && !flagship->IsDestroyed())
			isActive = !DoHelp("disabled");
		bool canRefuel = player.GetSystem()->HasFuelFor(*flagship);
		if(isActive && !flagship->IsHyperspacing() && !flagship->JumpsRemaining() && !canRefuel)
			isActive = !DoHelp("stranded");
		shared_ptr<Ship> target = flagship->GetTargetShip();
		if(isActive && target && target->IsDisabled() && !target->GetGovernment()->IsEnemy())
			isActive = !DoHelp("friendly disabled");
		if(isActive && player.Ships().size() > 1)
			isActive = !DoHelp("multiple ship controls");
		if(isActive && !flagship->IsHyperspacing() && flagship->Position().Length() > 10000.
				&& player.GetDate() <= player.StartData().GetDate() + 4)
		{
			++lostness;
			int count = 1 + lostness / 3600;
			if(count > lostCount && count <= 7)
			{
				string message = "lost 1";
				message.back() += lostCount;
				++lostCount;
				
				GetUI()->Push(new Dialog(GameData::HelpMessage(message)));
			}
		}
	}
	
	engine.Step(isActive);
	
	// Splice new events onto the eventQueue for (eventual) handling. No
	// other classes use Engine::Events() after Engine::Step() completes.
	eventQueue.splice(eventQueue.end(), engine.Events());
	// Handle as many ShipEvents as possible (stopping if no longer active
	// and updating the isActive flag).
	StepEvents(isActive);
	
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
		if(canDrag)
		{
			const Color &dragColor = *GameData::Colors().Get("drag select");
			LineShader::Draw(dragSource, Point(dragSource.X(), dragPoint.Y()), .8f, dragColor);
			LineShader::Draw(Point(dragSource.X(), dragPoint.Y()), dragPoint, .8f, dragColor);
			LineShader::Draw(dragPoint, Point(dragPoint.X(), dragSource.Y()), .8f, dragColor);
			LineShader::Draw(Point(dragPoint.X(), dragSource.Y()), dragSource, .8f, dragColor);
		}
		else
			isDragging = false;
	}
	
	if(Preferences::Has("Show CPU / GPU load"))
	{
		string loadString = to_string(lround(load * 100.)) + "% GPU";
		const Color &color = *GameData::Colors().Get("medium");
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
	// Start the next step of the simulation because Step() above still
	// thinks the planet panel is up and therefore will not start it.
	engine.Go();
}



bool MainPanel::AllowFastForward() const
{
	return true;
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::MAP | Command::INFO | Command::HAIL))
		show = command;
	else if(command.Has(Command::AMMO))
	{
		Preferences::ToggleAmmoUsage();
		Messages::Add("Your escorts will now expend ammo: " + Preferences::AmmoUsage() + ".");
	}
	else if((key == SDLK_MINUS || key == SDLK_KP_MINUS) && !command)
		Preferences::ZoomViewOut();
	else if((key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS) && !command)
		Preferences::ZoomViewIn();
	else if(key >= '0' && key <= '9' && !command)
		engine.SelectGroup(key - '0', mod & KMOD_SHIFT, mod & (KMOD_CTRL | KMOD_GUI));
	else
		return false;
	
	return true;
}



// Send a command through the main-panel to the engine
void MainPanel::GiveCommand(const Command &command)
{
	engine.GiveCommand(command);
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
		
				out << "\t" << it.second;
				if(it.first->Get("installable") < 0.)
				{
					int tons = ceil(it.second * it.first->Mass());
					out << (tons == 1 ? " ton of " : " tons of ") << Format::LowerCase(it.first->PluralName()) << "\n";
				}
				else
					out << " " << (it.second == 1 ? it.first->Name(): it.first->PluralName()) << "\n";
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
	
	// Player cannot hail while landing / departing.
	if(flagship->Zoom() < 1.)
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
		if(!planet)
			Messages::Add("Unable to send hail.");
		else if(planet->IsWormhole())
		{
			static const Phrase *wormholeHail = GameData::Phrases().Get("wormhole hail");
			Messages::Add(wormholeHail->Get());
		}
		else if(planet->IsInhabited())
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



// Handle ShipEvents from this and previous Engine::Step calls. Start with the
// oldest and then process events until any create a new UI element.
void MainPanel::StepEvents(bool &isActive)
{
	while(isActive && !eventQueue.empty())
	{
		const ShipEvent &event = eventQueue.front();
		const Government *actor = event.ActorGovernment();
		
		// Pass this event to the player, to update conditions and make
		// any new UI elements (e.g. an "on enter" dialog) from their
		// active missions.
		if(!handledFront)
			player.HandleEvent(event, GetUI());
		handledFront = true;
		isActive = (GetUI()->Top().get() == this);
		
		// If we can't safely display a new UI element (i.e. an active
		// mission created a UI element), then stop processing events
		// until the current Conversation or Dialog is resolved. This
		// will keep the current event in the queue, so we can still
		// check it for various special cases involving the player.
		if(!isActive)
			break;
		
		// Handle boarding events.
		// 1. Boarding an NPC may "complete" it (i.e. "npc board"). Any UI element that
		// completion created has now closed, possibly destroying the event target.
		// 2. Boarding an NPC may create a mission (e.g. it thanks you for the repair/refuel,
		// asks you to complete a quest, bribes you into leaving it alone, or silently spawns
		// hostile ships). If boarding creates a mission with an "on offer" conversation, the
		// ConversationPanel will only let the player plunder a hostile NPC if the mission is
		// declined or deferred - an "accept" is assumed to have bought the NPC its life.
		// 3. Boarding a hostile NPC that does not display a mission UI element will display
		// the BoardingPanel, allowing the player to plunder it.
		const Ship *flagship = player.Flagship();
		if((event.Type() & (ShipEvent::BOARD | ShipEvent::ASSIST)) && actor->IsPlayer()
				&& !event.Target()->IsDestroyed() && flagship && event.Actor().get() == flagship)
		{
			auto boardedShip = event.Target();
			Mission *mission = player.BoardingMission(boardedShip);
			if(mission && mission->HasSpace(*flagship))
				mission->Do(Mission::OFFER, player, GetUI(), boardedShip);
			else if(mission)
				player.HandleBlockedMissions((event.Type() & ShipEvent::BOARD)
						? Mission::BOARDING : Mission::ASSISTING, GetUI());
			// Determine if a Dialog or ConversationPanel is being drawn next frame.
			isActive = (GetUI()->Top().get() == this);
			
			// Confirm that this event's target is not destroyed and still an
			// enemy before showing the BoardingPanel (as a mission NPC's
			// completion conversation may have allowed it to be destroyed or
			// captured).
			// TODO: This BoardingPanel should not be displayed if a mission NPC
			// completion conversation creates a BoardingPanel for it, or if the
			// NPC completion conversation ends via `accept,` even if the ship is
			// still hostile.
			if(isActive && (event.Type() == ShipEvent::BOARD) && !boardedShip->IsDestroyed()
					&& boardedShip->GetGovernment()->IsEnemy())
			{
				// Either no mission activated, or the one that did was "silent."
				GetUI()->Push(new BoardingPanel(player, boardedShip));
				isActive = false;
			}
		}
		
		// Handle scan events of or by the player.
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(actor->IsPlayer())
			{
				ShowScanDialog(event);
				isActive = false;
			}
			else if(event.TargetGovernment() && event.TargetGovernment()->IsPlayer())
			{
				string message = actor->Fine(player, event.Type(), &*event.Target());
				if(!message.empty())
				{
					GetUI()->Push(new Dialog(message));
					isActive = false;
				}
			}
		}
		
		// Remove the fully-handled event.
		eventQueue.pop_front();
		handledFront = false;
	}
}
