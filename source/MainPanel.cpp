/* MainPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MainPanel.h"

#include "BoardingPanel.h"
#include "comparators/ByGivenOrder.h"
#include "CategoryList.h"
#include "CoreStartData.h"
#include "Dialog.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "HailPanel.h"
#include "shader/LineShader.h"
#include "MapDetailPanel.h"
#include "MessageLogPanel.h"
#include "Messages.h"
#include "Mission.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Preferences.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include "opengl.h"

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

	// If the player is dead, don't show anything.
	if(player.IsDead())
		show = Command::NONE;

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
	else if(show.Has(Command::MESSAGE_LOG))
	{
		GetUI()->Push(new MessageLogPanel());
		isActive = false;
	}
	else if(show.Has(Command::HAIL))
		isActive = !ShowHailPanel();
	else if(show.Has(Command::HELP))
		isActive = !ShowHelp(true);
	show = Command::NONE;

	// If the player just landed, pop up the planet panel. When it closes, it
	// will call this object's OnCallback() function;
	if(isActive && player.GetPlanet() && !player.GetPlanet()->IsWormhole())
	{
		GetUI()->Push(new PlanetPanel(player, bind(&MainPanel::OnCallback, this)));
		player.Land(GetUI());
		isActive = false;
	}

	// Offer the next available entering mission.
	if(isActive && player.HasAvailableEnteringMissions() && player.Flagship())
	{
		Mission *mission = player.EnteringMission();
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		else
			player.HandleBlockedEnteringMissions(GetUI());
		// Determine if a Dialog or ConversationPanel is being drawn next frame.
		isActive = (GetUI()->Top().get() == this);
	}

	// Display any relevant help/tutorial messages.
	if(isActive)
		isActive = !ShowHelp(false);

	engine.Step(isActive);

	if(isActive && !engine.IsPaused())
		player.StepMissionTimers(GetUI());

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



// The hail panel calls this when it closes.
void MainPanel::OnBribeCallback(const Government *bribed)
{
	engine.BreakTargeting(bribed);
}



bool MainPanel::AllowsFastForward() const noexcept
{
	return true;
}



Engine &MainPanel::GetEngine()
{
	return engine;
}



// Only override the ones you need; the default action is to return false.
bool MainPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(player.IsDead())
		return true;

	if(command.Has(Command::MAP | Command::INFO | Command::MESSAGE_LOG | Command::HAIL | Command::HELP))
		show = command;
	else if(command.Has(Command::TURRET_TRACKING))
	{
		bool newValue = !Preferences::Has("Turrets focus fire");
		Preferences::Set("Turrets focus fire", newValue);
		Messages::Add(*GameData::Messages().Get(newValue ?
			"turret tracking focused" : "turret tracking opportunistic"));
	}
	else if(command.Has(Command::AMMO))
	{
		Preferences::ToggleAmmoUsage();
		Messages::Add(*GameData::Messages().Get(
			Preferences::Has("Escorts expend ammo") ? (Preferences::Has("Escorts use ammo frugally") ?
			"expend ammo frugally" : "expend ammo always") : "expend ammo never"));
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



bool MainPanel::Click(int x, int y, MouseButton button, int clicks)
{
	switch(button)
	{
		case MouseButton::MIDDLE:
		case MouseButton::RIGHT:
			engine.RightOrMiddleClick(Point(x, y), button);
			return true;
		case MouseButton::LEFT:
			break;
		default:
			return false;
	}

	// Don't respond to clicks if another panel is active.
	if(!canClick)
		return true;
	// Only allow drags that start when clicking was possible.
	canDrag = true;

	dragSource = Point(x, y);
	dragPoint = dragSource;

	SDL_Keymod mod = SDL_GetModState();
	hasShift = (mod & KMOD_SHIFT);
	hasControl = (mod & KMOD_CTRL);

	engine.Click(dragSource, dragSource, hasShift, hasControl);

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



bool MainPanel::Release(int x, int y, MouseButton button)
{
	if(button != MouseButton::LEFT)
		return false;

	if(isDragging)
	{
		dragPoint = Point(x, y);
		if(dragPoint.Distance(dragSource) > 5.)
			engine.Click(dragSource, dragPoint, hasShift, hasControl);

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

				out << "\t" << Format::CargoString(it.second, it.first) << "\n";
			}
		for(const auto &it : target->Cargo().Outfits())
			if(it.second)
			{
				if(first)
					out << "This " + target->Noun() + " is carrying:\n";
				first = false;

				out << "\t";
				if(it.first->Get("installable") < 0.)
				{
					int tons = ceil(it.second * it.first->Mass());
					out << Format::CargoString(tons, Format::LowerCase(it.first->PluralName())) << "\n";
				}
				else
					out << it.second << " " << (it.second == 1 ? it.first->DisplayName() : it.first->PluralName()) << "\n";
			}
		if(first)
			out << "This " + target->Noun() + " is not carrying any cargo.\n";
	}
	if((event.Type() & ShipEvent::SCAN_OUTFITS) && target->Attributes().Get("inscrutable"))
		out << "Your scanners cannot make any sense of this " + target->Noun() + "'s interior.";
	else if(event.Type() & ShipEvent::SCAN_OUTFITS)
	{
		if(!target->Outfits().empty())
			out << "This " + target->Noun() + " is equipped with:\n";
		else
			out << "This " + target->Noun() + " is not equipped with any outfits.\n";

		// Split target->Outfits() into categories, then iterate over them in order.
		vector<string> categories;
		for(const auto &category : GameData::GetCategory(CategoryType::OUTFIT))
			categories.push_back(category.Name());
		auto comparator = ByGivenOrder<string>(categories);
		map<string, map<const string, int>, ByGivenOrder<string>> outfitsByCategory(comparator);
		for(const auto &it : target->Outfits())
		{
			string outfitNameForDisplay = (it.second == 1 ? it.first->DisplayName() : it.first->PluralName());
			if(it.first->IsDefined() && !it.first->Category().empty() && !outfitNameForDisplay.empty())
				outfitsByCategory[it.first->Category()].emplace(std::move(outfitNameForDisplay), it.second);
		}
		for(const auto &it : outfitsByCategory)
		{
			if(it.second.empty())
				continue;

			// Print the category's name and outfits in it.
			out << "\t" << (it.first.empty() ? "Unknown" : it.first) << "\n";
			for(const auto &it2 : it.second)
				if(!it2.first.empty() && it2.second > 0)
					out << "\t\t" << it2.second << " " << it2.first << "\n";
		}

		map<string, int> count;
		for(const Ship::Bay &bay : target->Bays())
			if(bay.ship)
			{
				int &value = count[bay.ship->DisplayModelName()];
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
		Messages::Add(*GameData::Messages().Get("cannot hail while jumping"));
	else if(flagship->IsCloaked() && !flagship->Attributes().Get("cloaked communication"))
		Messages::Add(*GameData::Messages().Get("cannot hail while cloaked"));
	else if(target)
	{
		// If the target is out of system, always report a generic response
		// because the player has no way of telling if it's presently jumping or
		// not. If it's in system and jumping, report that.
		if(target->Zoom() < 1. || target->IsDestroyed() || target->GetSystem() != player.GetSystem()
				|| target->IsCloaked())
			Messages::Add({"Unable to hail target " + target->Noun() + ".", GameData::MessageCategories().Get("high")});
		else if(target->IsEnteringHyperspace())
			Messages::Add({"Unable to send hail: " + target->Noun() + " is entering hyperspace.",
				GameData::MessageCategories().Get("high")});
		else
		{
			GetUI()->Push(new HailPanel(player, target,
				[&](const Government *bribed) { MainPanel::OnBribeCallback(bribed); }));
			return true;
		}
	}
	else if(flagship->GetTargetStellar())
	{
		const Planet *planet = flagship->GetTargetStellar()->GetPlanet();
		if(!planet)
			Messages::Add(*GameData::Messages().Get("cannot hail"));
		else if(planet->IsWormhole())
			Messages::Add(*GameData::Messages().Get("wormhole hail"));
		else if(planet->IsInhabited())
		{
			GetUI()->Push(new HailPanel(player, flagship->GetTargetStellar()));
			return true;
		}
		else
			Messages::Add({"Unable to send hail: " + planet->Noun() + " is not inhabited.",
				GameData::MessageCategories().Get("high")});
	}
	else
		Messages::Add(*GameData::Messages().Get("cannot hail without target"));

	return false;
}



bool MainPanel::ShowHelp(bool force)
{
	const Ship *flagship = player.Flagship();
	if(!flagship)
		return false;

	vector<string> forced;
	// Check if any help messages should be shown.
	if(Preferences::Has("Control ship with mouse"))
	{
		if(force)
			forced.push_back("control ship with mouse");
		else if(DoHelp("control ship with mouse"))
			return true;
	}
	if(flagship->IsTargetable())
	{
		if(force)
			forced.push_back("navigation");
		else if(DoHelp("navigation"))
			return true;
	}
	if(flagship->IsDestroyed())
	{
		if(force)
			forced.push_back("dead");
		else if(DoHelp("dead"))
			return true;
	}
	else if(flagship->IsDisabled())
	{
		if(force)
			forced.push_back("disabled");
		else if(DoHelp("disabled"))
			return true;
	}
	bool canRefuel = player.GetSystem()->HasFuelFor(*flagship);
	if(!flagship->IsHyperspacing() && !flagship->JumpsRemaining() && !canRefuel)
	{
		if(force)
			forced.push_back("stranded");
		else if(DoHelp("stranded"))
			return true;
	}
	shared_ptr<Ship> target = flagship->GetTargetShip();
	if(target && target->IsDisabled() && !target->GetGovernment()->IsEnemy())
	{
		if(force)
			forced.push_back("friendly disabled");
		else if(DoHelp("friendly disabled"))
			return true;
	}
	if(player.Ships().size() > 1)
	{
		if(force)
			forced.push_back("multiple ship controls");
		else if(DoHelp("multiple ship controls"))
			return true;
	}
	if(flagship->IsTargetable() && player.Ships().size() > 1)
	{
		if(force)
			forced.push_back("fleet harvest tutorial");
		else if(DoHelp("fleet harvest tutorial"))
			return true;
	}
	if(flagship->IsTargetable() &&
			flagship->Attributes().Get("asteroid scan power") &&
			player.Ships().size() > 1)
	{
		// Different order of these messages is intentional,
		// because we're displaying the forced messages in reverse order.
		if(force)
		{
			forced.push_back("fleet asteroid mining");
			forced.push_back("fleet asteroid mining shortcuts");
		}
		else if(DoHelp("fleet asteroid mining shortcuts") && DoHelp("fleet asteroid mining"))
			return true;
	}
	if(player.DisplayCarrierHelp())
	{
		if(force)
			forced.push_back("try out fighters transfer cargo");
		else if(DoHelp("try out fighters transfer cargo"))
			return true;
	}
	if(Preferences::Has("Fighters transfer cargo"))
	{
		if(force)
			forced.push_back("fighters transfer cargo");
		else if(DoHelp("fighters transfer cargo"))
			return true;
	}
	if(!flagship->IsHyperspacing() && flagship->Position().Length() > 10000.
			&& player.GetDate() <= player.StartData().GetDate() + 4)
	{
		++lostness;
		int count = 1 + lostness / 3600;
		if(count > lostCount && count <= 7)
		{
			string message = "lost 1";
			message.back() += lostCount;
			++lostCount;
			if(force)
				forced.push_back(message);
			else if(DoHelp(message))
				return true;
		}
	}

	if(!force || forced.empty())
		return false;

	bool hasValidHelp = false;
	// Reverse-iterate so that the player will see the basic messages first.
	for(auto it = forced.rbegin(); it != forced.rend(); ++it)
		hasValidHelp |= DoHelp(*it, true);
	return hasValidHelp;
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
				string message = actor->Fine(player, event.Type(), &*event.Target()).second;
				if(!message.empty())
				{
					GetUI()->Push(new Dialog(message));
					isActive = false;
				}
			}
		}

		// Handle jump events from the player's flagship. This means we should check
		// for entering missions that can be offered.
		if((event.Type() & ShipEvent::JUMP) && flagship && event.Actor().get() == flagship)
			player.CreateEnteringMissions();

		// Remove the fully-handled event.
		eventQueue.pop_front();
		handledFront = false;
	}
}
