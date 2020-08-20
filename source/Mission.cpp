/* Mission.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Mission.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "DistanceMap.h"
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

#include <cmath>
#include <sstream>

using namespace std;

namespace {
	// Pick a random commodity that would make sense to be exported from the
	// first system to the second.
	const Trade::Commodity *PickCommodity(const System &from, const System &to)
	{
		vector<int> weight;
		int total = 0;
		for(const Trade::Commodity &commodity : GameData::Commodities())
		{
			// For every 100 credits in profit you can make, double the chance
			// of this commodity being chosen.
			double profit = to.Trade(commodity.name) - from.Trade(commodity.name);
			int w = max<int>(1, 100. * pow(2., profit * .01));
			weight.push_back(w);
			total += w;
		}
		total += !total;
		// Pick a random commodity based on those weights.
		int r = Random::Int(total);
		for(unsigned i = 0; i < weight.size(); ++i)
		{
			r -= weight[i];
			if(r < 0)
				return &GameData::Commodities()[i];
		}
		// Control will never reach here, but to satisfy the compiler:
		return nullptr;
	}
	
	// If a source, destination, waypoint, or stopover supplies more than one explicit choice
	// or a mixture of explicit choice and location filter, print a warning.
	void ParseMixedSpecificity(const DataNode &node, string &&kind, int expected)
	{
		if(node.Size() >= expected + 1)
			node.PrintTrace("Warning: use a location filter to choose from multiple " + kind + "s:");
		if(node.HasChildren())
			node.PrintTrace("Warning: location filter ignored due to use of explicit " + kind + ":");
	}
}



// Construct and Load() at the same time.
Mission::Mission(const DataNode &node)
{
	Load(node);
}



// Load a mission, either from the game data or from a saved game.
void Mission::Load(const DataNode &node)
{
	// All missions need a name.
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for mission:");
		return;
	}
	// If a mission object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a mission with the same name as the base game
	// or another plugin). This class is not designed to allow merging or
	// overriding of mission data from two different definitions.
	if(!name.empty())
	{
		node.PrintTrace("Duplicate definition of mission:");
		return;
	}
	name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "name" && child.Size() >= 2)
			displayName = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
			description = child.Token(1);
		else if(child.Token(0) == "blocked" && child.Size() >= 2)
			blocked = child.Token(1);
		else if(child.Token(0) == "deadline" && child.Size() >= 4)
			deadline = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "deadline")
		{
			if(child.Size() == 1)
				deadlineMultiplier += 2;
			if(child.Size() >= 2)
				deadlineBase += child.Value(1);
			if(child.Size() >= 3)
				deadlineMultiplier += child.Value(2);
		}
		else if(child.Token(0) == "cargo" && child.Size() >= 3)
		{
			cargo = child.Token(1);
			cargoSize = child.Value(2);
			if(child.Size() >= 4)
				cargoLimit = child.Value(3);
			if(child.Size() >= 5)
				cargoProb = child.Value(4);
			
			for(const DataNode &grand : child)
			{
				if(!ParseContraband(grand))
					grand.PrintTrace("Skipping unrecognized attribute:");
				else
					grand.PrintTrace("Warning: \"stealth\" and \"illegal\" are now mission-level properties:");
			}
		}
		else if(child.Token(0) == "passengers" && child.Size() >= 2)
		{
			passengers = child.Value(1);
			if(child.Size() >= 3)
				passengerLimit = child.Value(2);
			if(child.Size() >= 4)
				passengerProb = child.Value(3);
		}
		else if(ParseContraband(child))
		{
			// This was an "illegal" or "stealth" entry. It has already been
			// parsed, so nothing more needs to be done here.
		}
		else if(child.Token(0) == "invisible")
			isVisible = false;
		else if(child.Token(0) == "priority")
			hasPriority = true;
		else if(child.Token(0) == "minor")
			isMinor = true;
		else if(child.Token(0) == "autosave")
			autosave = true;
		else if(child.Token(0) == "job")
			location = JOB;
		else if(child.Token(0) == "landing")
			location = LANDING;
		else if(child.Token(0) == "assisting")
			location = ASSISTING;
		else if(child.Token(0) == "boarding")
			location = BOARDING;
		else if(child.Token(0) == "repeat")
			repeat = (child.Size() == 1 ? 0 : static_cast<int>(child.Value(1)));
		else if(child.Token(0) == "clearance")
		{
			clearance = (child.Size() == 1 ? "auto" : child.Token(1));
			clearanceFilter.Load(child);
		}
		else if(child.Token(0) == "infiltrating")
			hasFullClearance = false;
		else if(child.Token(0) == "failed")
			hasFailed = true;
		else if(child.Token(0) == "to" && child.Size() >= 2)
		{
			if(child.Token(1) == "offer")
				toOffer.Load(child);
			else if(child.Token(1) == "complete")
				toComplete.Load(child);
			else if(child.Token(1) == "fail")
				toFail.Load(child);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(child.Token(0) == "source" && child.Size() >= 2)
		{
			source = GameData::Planets().Get(child.Token(1));
			ParseMixedSpecificity(child, "planet", 2);
		}
		else if(child.Token(0) == "source")
			sourceFilter.Load(child);
		else if(child.Token(0) == "destination" && child.Size() == 2)
		{
			destination = GameData::Planets().Get(child.Token(1));
			ParseMixedSpecificity(child, "planet", 2);
		}
		else if(child.Token(0) == "destination")
			destinationFilter.Load(child);
		else if(child.Token(0) == "waypoint" && child.Size() >= 2)
		{
			bool visited = child.Size() >= 3 && child.Token(2) == "visited";
			set<const System *> &set = visited ? visitedWaypoints : waypoints;
			set.insert(GameData::Systems().Get(child.Token(1)));
			ParseMixedSpecificity(child, "system", 2 + visited);
		}
		else if(child.Token(0) == "waypoint" && child.HasChildren())
			waypointFilters.emplace_back(child);
		else if(child.Token(0) == "stopover" && child.Size() >= 2)
		{
			bool visited = child.Size() >= 3 && child.Token(2) == "visited";
			set<const Planet *> &set = visited ? visitedStopovers : stopovers;
			set.insert(GameData::Planets().Get(child.Token(1)));
			ParseMixedSpecificity(child, "planet", 2 + visited);
		}
		else if(child.Token(0) == "stopover" && child.HasChildren())
			stopoverFilters.emplace_back(child);
		else if(child.Token(0) == "npc")
			npcs.emplace_back(child);
		else if(child.Token(0) == "on" && child.Size() >= 2 && child.Token(1) == "enter")
		{
			// "on enter" nodes may either name a specific system or use a LocationFilter
			// to control the triggering system.
			if(child.Size() >= 3)
			{
				MissionAction &action = onEnter[GameData::Systems().Get(child.Token(2))];
				action.Load(child, name);
			}
			else
				genericOnEnter.emplace_back(child, name);
		}
		else if(child.Token(0) == "on" && child.Size() >= 2)
		{
			static const map<string, Trigger> trigger = {
				{"complete", COMPLETE},
				{"offer", OFFER},
				{"accept", ACCEPT},
				{"decline", DECLINE},
				{"fail", FAIL},
				{"defer", DEFER},
				{"visit", VISIT},
				{"stopover", STOPOVER}
			};
			auto it = trigger.find(child.Token(1));
			if(it != trigger.end())
				actions[it->second].Load(child, name);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	if(displayName.empty())
		displayName = name;
}



// Save a mission. It is safe to assume that any mission that is being saved
// is already "instantiated," so only a subset of the data must be saved.
void Mission::Save(DataWriter &out, const string &tag) const
{
	out.Write(tag, name);
	out.BeginChild();
	{
		out.Write("name", displayName);
		if(!description.empty())
			out.Write("description", description);
		if(!blocked.empty())
			out.Write("blocked", blocked);
		if(deadline)
			out.Write("deadline", deadline.Day(), deadline.Month(), deadline.Year());
		if(cargoSize)
			out.Write("cargo", cargo, cargoSize);
		if(passengers)
			out.Write("passengers", passengers);
		if(illegalCargoFine)
			out.Write("illegal", illegalCargoFine, illegalCargoMessage);
		if(failIfDiscovered)
			out.Write("stealth");
		if(!isVisible)
			out.Write("invisible");
		if(hasPriority)
			out.Write("priority");
		if(isMinor)
			out.Write("minor");
		if(autosave)
			out.Write("autosave");
		if(location == LANDING)
			out.Write("landing");
		if(location == ASSISTING)
			out.Write("assisting");
		if(location == BOARDING)
			out.Write("boarding");
		if(location == JOB)
			out.Write("job");
		if(!clearance.empty())
		{
			out.Write("clearance", clearance);
			clearanceFilter.Save(out);
		}
		if(!hasFullClearance)
			out.Write("infiltrating");
		if(hasFailed)
			out.Write("failed");
		if(repeat != 1)
			out.Write("repeat", repeat);
		
		if(!toOffer.IsEmpty())
		{
			out.Write("to", "offer");
			out.BeginChild();
			{
				toOffer.Save(out);
			}
			out.EndChild();
		}
		if(!toComplete.IsEmpty())
		{
			out.Write("to", "complete");
			out.BeginChild();
			{
				toComplete.Save(out);
			}
			out.EndChild();
		}
		if(!toFail.IsEmpty())
		{
			out.Write("to", "fail");
			out.BeginChild();
			{
				toFail.Save(out);
			}
			out.EndChild();
		}
		if(destination)
			out.Write("destination", destination->Name());
		for(const System *system : waypoints)
			out.Write("waypoint", system->Name());
		for(const System *system : visitedWaypoints)
			out.Write("waypoint", system->Name(), "visited");
		
		for(const Planet *planet : stopovers)
			out.Write("stopover", planet->Name());
		for(const Planet *planet : visitedStopovers)
			out.Write("stopover", planet->Name(), "visited");
		
		// Save all NPCs, except those that have despawned. This is so that despawned
		// NPCs will not reappear should the player quit the game and return, and the
		// NPCs no lonager pass the despawn conditions.
		for(const NPC &npc : npcs)
			if(!npc.PassedDespawn())
				npc.Save(out);
		
		// Save all the actions, because this might be an "available mission" that
		// has not been received yet but must still be included in the saved game.
		for(const auto &it : actions)
			it.second.Save(out);
		// Save any "on enter" actions that have not been performed.
		for(const auto &it : onEnter)
			if(!didEnter.count(&it.second))
				it.second.Save(out);
		for(const MissionAction &action : genericOnEnter)
			if(!didEnter.count(&action))
				action.Save(out);
	}
	out.EndChild();
}



// Basic mission information.
const string &Mission::Name() const
{
	return displayName;
}



const string &Mission::Description() const
{
	return description;
}



// Check if this mission should be shown in your mission list. If not, the
// player will not know this mission exists (which is sometimes useful).
bool Mission::IsVisible() const
{
	return isVisible;
}



// Check if this mission has high priority. If any high-priority missions
// are available, no others will be shown at landing or in the spaceport.
// This is to be used for missions that are part of a series.
bool Mission::HasPriority() const
{
	return hasPriority;
}



// Check if this mission is a "minor" mission. Minor missions will only be
// offered if no other missions (minor or otherwise) are being offered.
bool Mission::IsMinor() const
{
	return isMinor;
}



bool Mission::IsAtLocation(Location location) const
{
	return (this->location == location);
}



// Information about what you are doing.
const Planet *Mission::Destination() const
{
	return destination;
}



const set<const System *> &Mission::Waypoints() const
{
	return waypoints;
}



const set<const System *> &Mission::VisitedWaypoints() const
{
	return visitedWaypoints;
}



const set<const Planet *> &Mission::Stopovers() const
{
	return stopovers;
}



const set<const Planet *> &Mission::VisitedStopovers() const
{
	return visitedStopovers;
}



const string &Mission::Cargo() const
{
	return cargo;
}



int Mission::CargoSize() const
{
	return cargoSize;
}



int Mission::IllegalCargoFine() const
{
	return illegalCargoFine;
}



string Mission::IllegalCargoMessage() const
{
	return illegalCargoMessage;
}



bool Mission::FailIfDiscovered() const
{
	return failIfDiscovered;
}



int Mission::Passengers() const
{
	return passengers;
}



// The mission must be completed by this deadline (if there is a deadline).
const Date &Mission::Deadline() const
{
	return deadline;
}



// If this mission's deadline was before the given date and it has not been
// marked as failing already, mark it and return true.
bool Mission::CheckDeadline(const Date &today)
{
	if(!hasFailed && deadline && deadline < today)
	{
		hasFailed = true;
		return true;
	}
	return false;
}



// Check if you have special clearance to land on your destination.
bool Mission::HasClearance(const Planet *planet) const
{
	if(clearance.empty())
		return false;
	if(planet == destination || stopovers.count(planet) || visitedStopovers.count(planet))
		return true;
	return (!clearanceFilter.IsEmpty() && clearanceFilter.Matches(planet));
}



// Get the string to be shown in the destination planet's hailing dialog. If
// this is "auto", you don't have to hail them to get landing permission.
const string &Mission::ClearanceMessage() const
{
	return clearance;
}



// Check whether we have full clearance to land and use the planet's
// services, or whether we are landing in secret ("infiltrating").
bool Mission::HasFullClearance() const
{
	return hasFullClearance;
}



// Check if it's possible to offer or complete this mission right now.
bool Mission::CanOffer(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	if(location == BOARDING || location == ASSISTING)
	{
		if(!boardingShip)
			return false;
		
		if(!sourceFilter.Matches(*boardingShip))
			return false;
	}
	else
	{
		if(source && source != player.GetPlanet())
			return false;
	
		if(!sourceFilter.Matches(player.GetPlanet()))
			return false;
	}
	
	if(!toOffer.Test(player.Conditions()))
		return false;
	
	if(!toFail.IsEmpty() && toFail.Test(player.Conditions()))
		return false;
	
	if(repeat)
	{
		auto cit = player.Conditions().find(name + ": offered");
		if(cit != player.Conditions().end() && cit->second >= repeat)
			return false;
	}
	
	auto it = actions.find(OFFER);
	if(it != actions.end() && !it->second.CanBeDone(player, boardingShip))
		return false;
	
	it = actions.find(ACCEPT);
	if(it != actions.end() && !it->second.CanBeDone(player, boardingShip))
		return false;
	
	it = actions.find(DECLINE);
	if(it != actions.end() && !it->second.CanBeDone(player, boardingShip))
		return false;
	
	return true;
}



bool Mission::HasSpace(const PlayerInfo &player) const
{
	int extraCrew = 0;
	if(player.Flagship())
		extraCrew = player.Flagship()->Crew() - player.Flagship()->RequiredCrew();
	return (cargoSize <= player.Cargo().Free() + player.Cargo().CommoditiesSize()
		&& passengers <= player.Cargo().BunksFree() + extraCrew);
}



// Check if this mission's cargo can fit entirely on the referenced ship.
bool Mission::HasSpace(const Ship &ship) const
{
	return (cargoSize <= ship.Cargo().Free() && passengers <= ship.Cargo().BunksFree());
}



bool Mission::CanComplete(const PlayerInfo &player) const
{
	if(player.GetPlanet() != destination)
		return false;
	
	return IsSatisfied(player);
}



// This function dictates whether missions on the player's map are shown in
// bright or dim text colors, and may be called while in-flight or landed.
bool Mission::IsSatisfied(const PlayerInfo &player) const
{
	if(!waypoints.empty() || !stopovers.empty())
		return false;
	
	// Test the completion conditions for this mission.
	if(!toComplete.Test(player.Conditions()))
		return false;
	
	// Determine if any fines or outfits that must be transferred, can.
	auto it = actions.find(COMPLETE);
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	// NPCs which must be accompanied or evaded must be present (or not),
	// and any needed scans, boarding, or assisting must also be completed.
	for(const NPC &npc : npcs)
		if(!npc.HasSucceeded(player.GetSystem()))
			return false;
	
	// If any of the cargo for this mission is being carried by a ship that is
	// not in this system, the mission cannot be completed right now.
	for(const auto &ship : player.Ships())
	{
		// Skip in-system ships, and carried ships whose parent is in-system.
		if(ship->GetSystem() == player.GetSystem() || (!ship->GetSystem() && ship->CanBeCarried()
				&& ship->GetParent() && ship->GetParent()->GetSystem() == player.GetSystem()))
			continue;
		
		if(ship->Cargo().GetPassengers(this))
			return false;
		// Check for all mission cargo, including that which has 0 mass.
		auto &cargo = ship->Cargo().MissionCargo();
		if(cargo.find(this) != cargo.end())
			return false;
	}
	
	return true;
}



bool Mission::HasFailed(const PlayerInfo &player) const
{
	if(!toFail.IsEmpty() && toFail.Test(player.Conditions()))
		return true;
	
	for(const NPC &npc : npcs)
		if(npc.HasFailed())
			return true;
	
	return hasFailed;
}



bool Mission::IsFailed() const
{
	return hasFailed;
}



// Mark a mission failed (e.g. due to a "fail" action in another mission).
void Mission::Fail()
{
	hasFailed = true;
}



// Get a string to show if this mission is "blocked" from being offered
// because it requires you to have more passenger or cargo space free. After
// calling this function, any future calls to it will return an empty string
// so that you do not display the same message multiple times.
string Mission::BlockedMessage(const PlayerInfo &player)
{
	if(blocked.empty())
		return "";
	
	int extraCrew = 0;
	const Ship *flagship = player.Flagship();
	// You cannot fire crew in space.
	if(flagship && player.GetPlanet())
		extraCrew = flagship->Crew() - flagship->RequiredCrew();
	
	int cargoNeeded = cargoSize;
	int bunksNeeded = passengers;
	if(player.GetPlanet())
	{
		cargoNeeded -= (player.Cargo().Free() + player.Cargo().CommoditiesSize());
		bunksNeeded -= (player.Cargo().BunksFree() + extraCrew);
	}
	else
	{
		// Boarding a ship, so only use the flagship's space.
		cargoNeeded -= flagship->Cargo().Free();
		bunksNeeded -= flagship->Cargo().BunksFree();
	}
	if(cargoNeeded < 0 && bunksNeeded < 0)
		return "";
	
	map<string, string> subs;
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	if(flagship)
		subs["<ship>"] = flagship->Name();
	
	ostringstream out;
	if(bunksNeeded > 0)
		out << (bunksNeeded == 1 ? "another bunk" : to_string(bunksNeeded) + " more bunks");
	if(bunksNeeded > 0 && cargoNeeded > 0)
		out << " and ";
	if(cargoNeeded > 0)
		out << (cargoNeeded == 1 ? "another ton" : to_string(cargoNeeded) + " more tons") << " of cargo space";
	subs["<capacity>"] = out.str();
	
	string message = Format::Replace(blocked, subs);
	blocked.clear();
	return message;
}



// Check if this mission recommends that the game be autosaved when it is
// accepted. This should be set for main story line missions that have a
// high chance of failing, such as escort missions.
bool Mission::RecommendsAutosave() const
{
	return autosave;
}



// Check if this mission is unique, i.e. not something that will be offered
// over and over again in different variants.
bool Mission::IsUnique() const
{
	return (repeat == 1);
}



// When the state of this mission changes, it may make changes to the player
// information or show new UI panels. PlayerInfo::MissionCallback() will be
// used as the callback for any UI panel that returns a value.
bool Mission::Do(Trigger trigger, PlayerInfo &player, UI *ui, const shared_ptr<Ship> &boardingShip)
{
	if(trigger == STOPOVER)
	{
		// If this is not one of this mission's stopover planets, or if it is
		// not the very last one that must be visited, do nothing.
		auto it = stopovers.find(player.GetPlanet());
		if(it == stopovers.end())
			return false;
		
		for(const NPC &npc : npcs)
			if(npc.IsLeftBehind(player.GetSystem()))
			{
				ui->Push(new Dialog("This is a stop for one of your missions, but you have left a ship behind."));
				return false;
			}
		
		visitedStopovers.insert(*it);
		stopovers.erase(it);
		if(!stopovers.empty())
			return false;
	}
	// Don't update any conditions if this action exists and can't be completed.
	auto it = actions.find(trigger);
	if(it != actions.end() && !it->second.CanBeDone(player, boardingShip))
		return false;
	
	if(trigger == ACCEPT)
	{
		++player.Conditions()[name + ": offered"];
		++player.Conditions()[name + ": active"];
		// Any potential on offer conversation has been finished, so update
		// the active NPCs for the first time.
		UpdateNPCs(player);
	}
	else if(trigger == DECLINE)
	{
		++player.Conditions()[name + ": offered"];
		++player.Conditions()[name + ": declined"];
	}
	else if(trigger == FAIL)
	{
		--player.Conditions()[name + ": active"];
		++player.Conditions()[name + ": failed"];
	}
	else if(trigger == COMPLETE)
	{
		--player.Conditions()[name + ": active"];
		++player.Conditions()[name + ": done"];
	}
	
	// "Jobs" should never show dialogs when offered, nor should they call the
	// player's mission callback.
	if(trigger == OFFER && location == JOB)
		ui = nullptr;
	
	// If this trigger has actions tied to it, perform them. Otherwise, check
	// if this is a non-job mission that just got offered and if so,
	// automatically accept it.
	if(it != actions.end())
		it->second.Do(player, ui, destination ? destination->GetSystem() : nullptr, boardingShip, IsUnique());
	else if(trigger == OFFER && location != JOB)
		player.MissionCallback(Conversation::ACCEPT);
	
	return true;
}



// Get a list of NPCs associated with this mission. Every time the player
// takes off from a planet, they should be added to the active ships.
const list<NPC> &Mission::NPCs() const
{
	return npcs;
}



// Update which NPCs are active based on their spawn and despawn conditions.
void Mission::UpdateNPCs(const PlayerInfo &player)
{
	for(auto &npc : npcs)
		npc.UpdateSpawning(player);
}



// Checks if the given ship belongs to one of the mission's NPCs.
bool Mission::HasShip(const shared_ptr<Ship> &ship) const
{
	for(const auto &npc : npcs)
		for(const auto &npcShip : npc.Ships())
			if(npcShip == ship)
				return true;
	return false;
}



// If any event occurs between two ships, check to see if this mission cares
// about it. This may affect the mission status or display a message.
void Mission::Do(const ShipEvent &event, PlayerInfo &player, UI *ui)
{
	if(event.TargetGovernment()->IsPlayer() && !hasFailed)
	{
		bool failed = false;
		string message = "Your ship '" + event.Target()->Name() + "' has been ";
		if(event.Type() & ShipEvent::DESTROY)
		{
			// Destroyed ships carrying mission cargo result in failed missions.
			// Mission cargo may have a quantity of zero (i.e. 0 mass).
			for(const auto &it : event.Target()->Cargo().MissionCargo())
				failed |= (it.first == this);
			// If any mission passengers were present, this mission is failed.
			for(const auto &it : event.Target()->Cargo().PassengerList())
				failed |= (it.first == this && it.second);
			if(failed)
				message += "lost. ";
		}
		else if(event.Type() & ShipEvent::BOARD)
		{
			// Fail missions whose cargo is stolen by a boarding vessel.
			for(const auto &it : event.Actor()->Cargo().MissionCargo())
				failed |= (it.first == this);
			if(failed)
				message += "plundered. ";
		}
		
		if(failed)
		{
			hasFailed = true;
			if(isVisible)
				Messages::Add(message + "Mission failed: \"" + displayName + "\".");
		}
	}
	
	// Jump events are only created for the player's flagship.
	if((event.Type() & ShipEvent::JUMP) && event.Actor())
	{
		const System *system = event.Actor()->GetSystem();
		// If this was a waypoint, clear it.
		if(waypoints.erase(system))
			visitedWaypoints.insert(system);
		
		// Perform an "on enter" action for this system, if possible.
		Enter(system, player, ui);
		
		// Update any potential NPCs for this mission, as an "on enter" action may have
		// changed the player's conditions.
		UpdateNPCs(player);
	}
	
	for(NPC &npc : npcs)
		npc.Do(event, player, ui, isVisible);
}



// Get the internal name used for this mission. This name is unique and is
// never modified by string substitution, so it can be used in condition
// variables, etc.
const string &Mission::Identifier() const
{
	return name;
}



// Get a specific mission action from this mission.
// If a mission action is not found for the given trigger, returns an empty
// mission action.
const MissionAction &Mission::GetAction(Trigger trigger) const
{
	auto ait = actions.find(trigger);
	static const MissionAction EMPTY;
	if(ait != actions.end())
		return ait->second;
	else
		return EMPTY;
}



// "Instantiate" a mission by replacing randomly selected values and places
// with a single choice, and then replacing any wildcard text as well.
Mission Mission::Instantiate(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	Mission result;
	// If anything goes wrong below, this mission should not be offered.
	result.hasFailed = true;
	result.isVisible = isVisible;
	result.hasPriority = hasPriority;
	result.isMinor = isMinor;
	result.autosave = autosave;
	result.location = location;
	result.repeat = repeat;
	result.name = name;
	result.waypoints = waypoints;
	// Handle waypoint systems that are chosen randomly.
	const System * const source = player.GetSystem();
	for(const LocationFilter &filter : waypointFilters)
	{
		const System *system = filter.PickSystem(source);
		if(!system)
			return result;
		result.waypoints.insert(system);
	}
	// If one of the waypoints is the current system, it is already visited.
	if(result.waypoints.erase(source))
		result.visitedWaypoints.insert(source);
	
	// Copy the stopover planet list, and populate the list based on the filters
	// that were given.
	result.stopovers = stopovers;
	// Make sure they all exist in a valid system.
	for(auto it = result.stopovers.begin(); it != result.stopovers.end(); )
	{
		if((*it)->GetSystem())
			++it;
		else
			it = result.stopovers.erase(it);
	}
	for(const LocationFilter &filter : stopoverFilters)
	{
		const Planet *planet = filter.PickPlanet(source, !clearance.empty());
		if(!planet)
			return result;
		result.stopovers.insert(planet);
	}
	
	// First, pick values for all the variables.
	
	// If a specific destination is not specified in the mission, pick a random
	// one out of all the destinations that satisfy the mission requirements.
	result.destination = destination;
	if(!result.destination && !destinationFilter.IsEmpty())
	{
		result.destination = destinationFilter.PickPlanet(source, !clearance.empty());
		if(!result.destination)
			return result;
	}
	// If no destination is specified, it is the same as the source planet. Also
	// use the source planet if the given destination is not a valid planet name.
	if(!result.destination || !result.destination->GetSystem())
	{
		if(player.GetPlanet())
			result.destination = player.GetPlanet();
		else
			return result;
	}
	
	// If cargo is being carried, see if we are supposed to replace a generic
	// cargo name with something more specific.
	if(!cargo.empty())
	{
		const Trade::Commodity *commodity = nullptr;
		if(cargo == "random")
			commodity = PickCommodity(*source, *result.destination->GetSystem());
		else
		{
			for(const Trade::Commodity &option : GameData::Commodities())
				if(option.name == cargo)
				{
					commodity = &option;
					break;
				}
			for(const Trade::Commodity &option : GameData::SpecialCommodities())
				if(option.name == cargo)
				{
					commodity = &option;
					break;
				}
		}
		if(commodity)
			result.cargo = commodity->items[Random::Int(commodity->items.size())];
		else
			result.cargo = cargo;
	}
	// Pick a random cargo amount, if requested.
	if(cargoSize || cargoLimit)
	{
		if(cargoProb)
			result.cargoSize = Random::Polya(cargoLimit, cargoProb) + cargoSize;
		else if(cargoLimit > cargoSize)
			result.cargoSize = cargoSize + Random::Int(cargoLimit - cargoSize + 1);
		else
			result.cargoSize = cargoSize;
	}
	// Pick a random passenger count, if requested.
	if(passengers | passengerLimit)
	{
		if(passengerProb)
			result.passengers = Random::Polya(passengerLimit, passengerProb) + passengers;
		else if(passengerLimit > passengers)
			result.passengers = passengers + Random::Int(passengerLimit - passengers + 1);
		else
			result.passengers = passengers;
	}
	result.illegalCargoFine = illegalCargoFine;
	result.illegalCargoMessage = illegalCargoMessage;
	result.failIfDiscovered = failIfDiscovered;
	
	// Estimate how far the player will have to travel to visit all the waypoints
	// and stopovers and then to land on the destination planet. Rather than a
	// full traveling salesman path, just calculate a greedy approximation.
	const System *path = source;
	list<const System *> destinations;
	for(const System *system : result.waypoints)
		destinations.push_back(system);
	for(const Planet *planet : result.stopovers)
		destinations.push_back(planet->GetSystem());
	
	int jumps = 0;
	while(!destinations.empty())
	{
		// Find the closest destination to this location.
		DistanceMap distance(path);
		auto it = destinations.begin();
		auto bestIt = it;
		for(++it; it != destinations.end(); ++it)
			if(distance.Days(*it) < distance.Days(*bestIt))
				bestIt = it;
		
		path = *bestIt;
		jumps += distance.Days(*bestIt);
		destinations.erase(bestIt);
	}
	DistanceMap distance(path);
	jumps += distance.Days(result.destination->GetSystem());
	int payload = result.cargoSize + 10 * result.passengers;
	
	// Set the deadline, if requested.
	if(deadlineBase || deadlineMultiplier)
		result.deadline = player.GetDate() + deadlineBase + deadlineMultiplier * jumps;
	
	// Copy the conditions. The offer conditions must be copied too, because they
	// may depend on a condition that other mission offers might change.
	result.toOffer = toOffer;
	result.toComplete = toComplete;
	result.toFail = toFail;
	
	// Generate the substitutions map.
	map<string, string> subs;
	subs["<commodity>"] = result.cargo;
	subs["<tons>"] = to_string(result.cargoSize) + (result.cargoSize == 1 ? " ton" : " tons");
	subs["<cargo>"] = subs["<tons>"] + " of " + subs["<commodity>"];
	subs["<bunks>"] = to_string(result.passengers);
	subs["<passengers>"] = (result.passengers == 1) ? "passenger" : "passengers";
	subs["<fare>"] = (result.passengers == 1) ? "a passenger" : (subs["<bunks>"] + " passengers");
	if(player.GetPlanet())
		subs["<origin>"] = player.GetPlanet()->Name();
	else if(boardingShip)
		subs["<origin>"] = boardingShip->Name();
	subs["<planet>"] = result.destination ? result.destination->Name() : "";
	subs["<system>"] = result.destination ? result.destination->GetSystem()->Name() : "";
	subs["<destination>"] = subs["<planet>"] + " in the " + subs["<system>"] + " system";
	subs["<date>"] = result.deadline.ToString();
	subs["<day>"] = result.deadline.LongString();
	// Stopover and waypoint substitutions: iterate by reference to the
	// pointers so we can check when we're at the very last one in the set.
	// Stopovers: "<name> in the <system name> system" with "," and "and".
	if(!result.stopovers.empty())
	{
		string planets;
		const Planet * const *last = &*--result.stopovers.end();
		int count = 0;
		for(const Planet * const &planet : result.stopovers)
		{
			if(count++)
				planets += (&planet != last) ? ", " : (count > 2 ? ", and " : " and ");
			planets += planet->Name() + " in the " + planet->GetSystem()->Name() + " system";
		}
		subs["<stopovers>"] = planets;
	}
	// Waypoints: "<system name>" with "," and "and".
	if(!result.waypoints.empty())
	{
		string systems;
		const System * const *last = &*--result.waypoints.end();
		int count = 0;
		for(const System * const &system : result.waypoints)
		{
			if(count++)
				systems += (&system != last) ? ", " : (count > 2 ? ", and " : " and ");
			systems += system->Name();
		}
		subs["<waypoints>"] = systems;
	}
	
	// Instantiate the NPCs. This also fills in the "<npc>" substitution.
	for(const NPC &npc : npcs)
		result.npcs.push_back(npc.Instantiate(subs, source, result.destination->GetSystem()));
	
	// Instantiate the actions. The "complete" action is always first so that
	// the "<payment>" substitution can be filled in.
	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(subs, source, jumps, payload);
	for(const auto &it : onEnter)
		result.onEnter[it.first] = it.second.Instantiate(subs, source, jumps, payload);
	for(const MissionAction &action : genericOnEnter)
		result.genericOnEnter.emplace_back(action.Instantiate(subs, source, jumps, payload));
	
	// Perform substitution in the name and description.
	result.displayName = Format::Replace(displayName, subs);
	result.description = Format::Replace(description, subs);
	result.clearance = Format::Replace(clearance, subs);
	result.blocked = Format::Replace(blocked, subs);
	result.clearanceFilter = clearanceFilter;
	result.hasFullClearance = hasFullClearance;
	
	result.hasFailed = false;
	return result;
}



// Perform an "on enter" MissionAction associated with the current system.
void Mission::Enter(const System *system, PlayerInfo &player, UI *ui)
{
	const auto eit = onEnter.find(system);
	if(eit != onEnter.end() && !didEnter.count(&eit->second) && eit->second.CanBeDone(player))
	{
		eit->second.Do(player, ui);
		didEnter.insert(&eit->second);
	}
	// If no specific `on enter` was performed, try matching to a generic "on enter,"
	// which may use a LocationFilter to govern which systems it can be performed in.
	else
		for(MissionAction &action : genericOnEnter)
			if(!didEnter.count(&action) && action.CanBeDone(player))
			{
				action.Do(player, ui);
				didEnter.insert(&action);
				break;
			}
}



// For legacy code, contraband definitions can be placed in two different
// locations, so move that parsing out to a helper function.
bool Mission::ParseContraband(const DataNode &node)
{
	if(node.Token(0) == "illegal" && node.Size() == 2)
		illegalCargoFine = node.Value(1);
	else if(node.Token(0) == "illegal" && node.Size() == 3)
	{
		illegalCargoFine = node.Value(1);
		illegalCargoMessage = node.Token(2);
	}
	else if(node.Token(0) == "stealth")
		failIfDiscovered = true;
	else
		return false;
	
	return true;
}
