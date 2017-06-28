/* NPC.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "NPC.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "UI.h"

using namespace std;



// Construct and Load() at the same time.
NPC::NPC(const DataNode &node)
{
	Load(node);
}



void NPC::Load(const DataNode &node)
{
	// Any tokens after the "npc" tag list the things that must happen for this
	// mission to succeed.
	for(int i = 1; i < node.Size(); ++i)
	{
		if(node.Token(i) == "save")
			failIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "kill")
			succeedIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "board")
			succeedIf |= ShipEvent::BOARD;
		else if(node.Token(i) == "assist")
			succeedIf |= ShipEvent::ASSIST;
		else if(node.Token(i) == "disable")
			succeedIf |= ShipEvent::DISABLE;
		else if(node.Token(i) == "scan cargo")
			succeedIf |= ShipEvent::SCAN_CARGO;
		else if(node.Token(i) == "scan outfits")
			succeedIf |= ShipEvent::SCAN_OUTFITS;
		else if(node.Token(i) == "land")
			succeedIf |= ShipEvent::LAND;
		else if(node.Token(i) == "evade")
			mustEvade = true;
		else if(node.Token(i) == "accompany")
		{
			mustAccompany = true;
			failIf |= ShipEvent::DESTROY;
		}
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
		{
			if(child.Size() >= 2)
			{
				if(child.Token(1) == "destination")
					isAtDestination = true;
				else
					system = GameData::Systems().Get(child.Token(1));
			}
			else
				location.Load(child);
		}
		else if(child.Token(0) == "waypoint" || child.Token(0) == "patrol")
		{
			// Waypoints can be added one-by-one (e.g. if from LocationFilters) or
			// from multiple "waypoint" / "patrol" nodes. If any are "patrol", all
			// waypoints are patrolled. If no system or filter to pick a system is
			// passed to the node, the mission's destination system will be used.
			doPatrol |= child.Token(0) == "patrol";
			if(!child.HasChildren())
			{
				// Given "waypoint/patrol" or "waypoint/patrol <system 1> .... <system N>"
				if(child.Size() == 1)
					needsWaypoint = true;
				else
					for(int i = 1; i < child.Size(); ++i)
						waypoints.push_back(GameData::Systems().Get(child.Token(i)));
			}
			// Given "waypoint/patrol" and child nodes. These get processed during NPC instantiation.
			else
				for(const DataNode &grand : child)
					waypointFilters.emplace_back(grand);
		}
		else if(child.Token(0) == "land" || child.Token(0) == "visit")
		{
			// Stopovers can be added one-by-one (e.g. if from LocationFilters) or
			// from multiple "visit" / "land" nodes. If any nodes are "visit", all
			// stopovers are visited (no permanent landing on the stopovers). If no
			// planet is passed to the node, the mission's destination will be used.
			doVisit |= child.Token(0) == "visit";
			if(!child.HasChildren())
			{
				// Given "land/visit" or "land/visit <planet 1> ... <planet N>".
				if(child.Size() == 1)
					needsStopover = true;
				else
					for(int i = 1; i < child.Size(); ++i)
						stopovers.push_back(GameData::Planets().Get(child.Token(i)));
			}
			// Given "land/visit" and child nodes. These get processed during NPC instantiation.
			else
				for(const DataNode &grand : child)
					stopoverFilters.emplace_back(grand);
		}
		else if(child.Token(0) == "succeed" && child.Size() >= 2)
			succeedIf = child.Value(1);
		else if(child.Token(0) == "fail" && child.Size() >= 2)
			failIf = child.Value(1);
		else if(child.Token(0) == "evade")
			mustEvade = true;
		else if(child.Token(0) == "accompany")
			mustAccompany = true;
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "dialog")
		{
			for(int i = 1; i < child.Size(); ++i)
			{
				if(!dialogText.empty())
					dialogText += "\n\t";
				dialogText += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!dialogText.empty())
						dialogText += "\n\t";
					dialogText += grand.Token(i);
				}
		}
		else if(child.Token(0) == "conversation" && child.HasChildren())
			conversation.Load(child);
		else if(child.Token(0) == "conversation" && child.Size() > 1)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else if(child.Token(0) == "ship")
		{
			if(child.HasChildren() && child.Size() == 2)
			{
				// Loading an NPC from a save file, or an entire ship specification.
				// The latter may result in references to non-instantiated outfits.
				ships.emplace_back(make_shared<Ship>(child));
				for(const DataNode &grand : child)
					if(grand.Token(0) == "actions" && grand.Size() >= 2)
						actions[ships.back().get()] = grand.Value(1);
			}
			else if(child.Size() >= 2)
			{
				// Loading a ship managed by GameData, i.e. "base models" and variants.
				stockShips.push_back(GameData::Ships().Get(child.Token(1)));
				shipNames.push_back(child.Token(1 + (child.Size() > 2)));
			}
			else
			{
				string message = "Skipping unsupported use of a ship token and child nodes: ";
				if(child.Size() >= 3)
					message += "to both name and customize a ship, create a variant and then reference it here.";
				else
					message += "the \'ship\' token must be followed by the name of a ship, e.g. ship \"Bulk Freighter\"";
				child.PrintTrace(message);
			}
		}
		else if(child.Token(0) == "fleet")
		{
			if(child.HasChildren())
			{
				fleets.emplace_back(child);
				if(child.Size() >= 2)
				{
					// Copy the custom fleet in lieu of reparsing the same DataNode.
					size_t numAdded = child.Value(1);
					for(size_t i = 1; i < numAdded; ++i)
						fleets.push_back(fleets.back());
				}
			}
			else if(child.Size() >= 3 && child.Value(2) > 1.)
				stockFleets.insert(stockFleets.end(), child.Value(2), GameData::Fleets().Get(child.Token(1)));
			else if(child.Size() >= 2)
				stockFleets.push_back(GameData::Fleets().Get(child.Token(1)));
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	// Since a ship's government is not serialized, set it now.
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		ship->SetIsSpecial();
		ship->FinishLoading(false);
		if(!waypoints.empty())
			ship->SetWaypoints(waypoints, doPatrol);
		if(!stopovers.empty())
			ship->SetStopovers(stopovers, doVisit);
	}
}



// Note: the Save() function can assume this is an instantiated mission, not
// a template, so fleets will be replaced by individual ships already.
void NPC::Save(DataWriter &out) const
{
	out.Write("npc");
	out.BeginChild();
	{
		if(succeedIf)
			out.Write("succeed", succeedIf);
		if(failIf)
			out.Write("fail", failIf);
		if(mustEvade)
			out.Write("evade");
		if(mustAccompany)
			out.Write("accompany");
		
		if(government)
			out.Write("government", government->GetName());
		personality.Save(out);
		
		if(!waypoints.empty())
		{
			out.WriteToken(doPatrol ? "patrol" : "waypoint");
			for(const auto &waypoint : waypoints)
				out.WriteToken(waypoint->Name());
			out.Write();
		}
		
		if(!stopovers.empty())
		{
			out.WriteToken(doVisit ? "visit" : "land");
			for(const auto &stopover : stopovers)
				out.WriteToken(stopover->Name());
			out.Write();
		}
		
		if(!dialogText.empty())
		{
			out.Write("dialog");
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(dialogText, "\n\t"))
					out.Write(line);
			}
			out.EndChild();
		}
		if(!conversation.IsEmpty())
			conversation.Save(out);
		
		for(const shared_ptr<Ship> &ship : ships)
		{
			ship->Save(out);
			auto it = actions.find(ship.get());
			if(it != actions.end() && it->second)
			{
				// Append an "actions" tag to the end of the ship data.
				out.BeginChild();
				{
					out.Write("actions", it->second);
				}
				out.EndChild();
			}
		}
	}
	out.EndChild();
}



// Get the ships associated with this set of NPCs.
const list<shared_ptr<Ship>> NPC::Ships() const
{
	return ships;
}



// Handle the given ShipEvent.
void NPC::Do(const ShipEvent &event, PlayerInfo &player, UI *ui, bool isVisible)
{
	// First, check if this ship is part of this NPC. If not, do nothing. If it
	// is an NPC and it just got captured, replace it with a destroyed copy of
	// itself so that this class thinks the ship is destroyed.
	shared_ptr<Ship> ship;
	int type = event.Type();
	for(shared_ptr<Ship> &ptr : ships)
		if(ptr == event.Target())
		{
			// If a mission ship is captured, let it live on under its new
			// ownership but mark our copy of it as destroyed. This must be done
			// before we check the mission's success status because otherwise
			// momentarily reactivating a ship you're supposed to evade would
			// clear the success status and cause the success message to be
			// displayed a second time below. 
			if(event.Type() & ShipEvent::CAPTURE)
			{
				Ship *copy = new Ship(*ptr);
				copy->Destroy();
				actions[copy] = actions[ptr.get()];
				// Count this ship as destroyed, as well as captured.
				type |= ShipEvent::DESTROY;
				ptr.reset(copy);
			}
			ship = ptr;
			break;
		}
	if(!ship)
		return;
	
	// Check if this NPC is already in the succeeded state.
	bool hasSucceeded = HasSucceeded(player.GetSystem());
	bool hasFailed = HasFailed();
	
	// If this event was "ASSIST", the ship is now known as not disabled.
	if(type == ShipEvent::ASSIST)
		actions[ship.get()] &= ~(ShipEvent::DISABLE);
	
	// Certain events only count towards the NPC's status if originated by
	// the player: scanning, boarding, or assisting.
	if(!event.ActorGovernment()->IsPlayer())
		type &= ~(ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS
				| ShipEvent::ASSIST | ShipEvent::BOARD);
	
	// Apply this event to the ship and any ships it is carrying.
	actions[ship.get()] |= type;
	for(const Ship::Bay &bay : ship->Bays())
		if(bay.ship)
			actions[bay.ship.get()] |= type;
	
	// Check if the success status has changed. If so, display a message.
	if(HasFailed() && !hasFailed && isVisible)
		Messages::Add("Mission failed.");
	else if(ui && HasSucceeded(player.GetSystem()) && !hasSucceeded)
	{
		// If "completing" this NPC displays a conversation, reference
		// it, to allow the completing event's target to be destroyed.
		if(!conversation.IsEmpty())
			ui->Push(new ConversationPanel(player, conversation, nullptr, ship));
		else if(!dialogText.empty())
			ui->Push(new Dialog(dialogText));
	}
}



bool NPC::HasSucceeded(const System *playerSystem) const
{
	if(HasFailed())
		return false;
	
	// Evaluate the status of each ship in this NPC block. If it has `accompany`,
	// it cannot be disabled or destroyed, and must be in the player's system.
	// Destroyed `accompany` are handled in HasFailed(). If the NPC block has
	// `evade`, the ship can be disabled, destroyed, captured, or not present.
	if(mustEvade || mustAccompany)
		for(const shared_ptr<Ship> &ship : ships)
		{
			auto it = actions.find(ship.get());
			// If a derelict ship has not received any ShipEvents, it is immobile.
			bool isImmobile = ship->GetPersonality().IsDerelict();
			// The success status calculation can only be based on recorded
			// events (and the current system).
			if(it != actions.end())
			{
				// A ship that was disabled, captured, or destroyed is considered 'immobile'.
				isImmobile = (it->second
					& (ShipEvent::DISABLE | ShipEvent::CAPTURE | ShipEvent::DESTROY));
				// if this NPC is 'derelict' and has no ASSIST on record, it is immobile.
				isImmobile |= ship->GetPersonality().IsDerelict()
					&& !(it->second & ShipEvent::ASSIST);
			}
			bool isHere = (!ship->GetSystem() || ship->GetSystem() == playerSystem);
			if((isHere && !isImmobile) ^ mustAccompany)
				return false;
		}
	
	if(!succeedIf)
		return true;
	
	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = actions.find(ship.get());
		if(it == actions.end() || (it->second & succeedIf) != succeedIf)
			return false;
	}
	
	return true;
}



// Check if the NPC is supposed to be accompanied and is not.
bool NPC::IsLeftBehind(const System *playerSystem) const
{
	if(HasFailed())
		return true;
	if(!mustAccompany)
		return false;
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->IsDisabled() || ship->GetSystem() != playerSystem)
			return true;
	
	return false;
}



bool NPC::HasFailed() const
{
	for(const auto &it : actions)
	{
		if(it.second & failIf)
			return true;
	
		// If we still need to perform an action that requires the NPC ship be
		// alive, then that ship being destroyed or landed causes the mission to fail.
		if((~it.second & succeedIf) && (it.second & (ShipEvent::DESTROY | ShipEvent::LAND)))
			return true;
		
		// If this ship has landed permanently, the NPC has failed if
		// 1) it must accompany and is not in the destination system, or
		// 2) it must evade, and is in the destination system.
		if((it.second & ShipEvent::LAND) && !doVisit && it.first->GetSystem()
				&& ((mustAccompany && it.first->GetSystem() != destination)
					|| (mustEvade && it.first->GetSystem() == destination)))
			return true;
	}
	
	return false;
}



// Create a copy of this NPC but with the fleets replaced by the actual
// ships they represent, wildcards in the conversation text replaced, etc.
NPC NPC::Instantiate(map<string, string> &subs, const System *origin, const Planet *destinationPlanet) const
{
	NPC result;
	result.destination = destinationPlanet->GetSystem();
	result.government = government;
	if(!result.government)
		result.government = GameData::PlayerGovernment();
	result.personality = personality;
	result.succeedIf = succeedIf;
	result.failIf = failIf;
	result.mustEvade = mustEvade;
	result.mustAccompany = mustAccompany;
	result.waypoints = waypoints;
	result.stopovers = stopovers;
	result.doPatrol = doPatrol;
	result.doVisit = doVisit;
	
	// Pick the system for this NPC to start out in.
	result.system = system;
	if(!result.system && !location.IsEmpty())
		result.system = location.PickSystem(origin);
	if(!result.system)
		result.system = (isAtDestination && destination) ? destination : origin;
	
	if(needsWaypoint && doPatrol)
	{
		// Create a patrol between the mission's origin and destination.
		result.waypoints.push_back(origin);
		result.waypoints.push_back(result.destination);
	}
	else if(needsWaypoint)
		result.waypoints.push_back(result.destination);
	for(const LocationFilter &filter : waypointFilters)
	{
		// Each new waypoint calculates "distance X Y" from the previous waypoint / origin.
		const System *choice = filter.PickSystem(result.waypoints.empty() ?
				origin : result.waypoints.back());
		if(choice)
			result.waypoints.push_back(choice);
	}
	
	if(needsStopover)
		result.stopovers.push_back(destinationPlanet);
	for(const LocationFilter &filter : stopoverFilters)
	{
		// Each new stopover's "distance X Y" is calculated from the previous one's system.
		const Planet *choice = filter.PickPlanet(result.stopovers.empty() ?
				origin : result.stopovers.back()->GetSystem(), true);
		if(choice)
			result.stopovers.push_back(choice);
	}
	
	// Convert fleets into instances of ships.
	for(const shared_ptr<Ship> &ship : ships)
	{
		// This ship is being defined from scratch.
		result.ships.push_back(make_shared<Ship>(*ship));
		result.ships.back()->FinishLoading(true);
	}
	auto shipIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for( ; shipIt != stockShips.end() && nameIt != shipNames.end(); ++shipIt, ++nameIt)
	{
		result.ships.push_back(make_shared<Ship>(**shipIt));
		result.ships.back()->SetName(*nameIt);
	}
	for(const Fleet &fleet : fleets)
		fleet.Place(*result.system, result.ships, false);
	for(const Fleet *fleet : stockFleets)
		fleet->Place(*result.system, result.ships, false);
	// Ships should either "enter" the system or start out there.
	for(const shared_ptr<Ship> &ship : result.ships)
	{
		ship->SetGovernment(result.government);
		ship->SetIsSpecial();
		ship->SetPersonality(result.personality);
		if(result.personality.IsDerelict())
			ship->Disable();
		
		// Use the destinations stored in the NPC copy, in case they were auto-generated.
		if(!result.stopovers.empty())
			ship->SetStopovers(result.stopovers, result.doVisit);
		if(!result.waypoints.empty())
			ship->SetWaypoints(result.waypoints, result.doPatrol);
		
		if(personality.IsEntering())
			Fleet::Enter(*result.system, *ship);
		else
			Fleet::Place(*result.system, *ship);
	}
	
	// String replacement:
	if(!result.ships.empty())
		subs["<npc>"] = result.ships.front()->Name();
	
	// Do string replacement on any dialog or conversation.
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Substitute(subs);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Substitute(subs);
	
	return result;
}
