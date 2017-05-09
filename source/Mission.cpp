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
}



// Load a mission, either from the game data or from a saved game.
void Mission::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	else
		name = "Unnamed Mission";
	
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
				if(grand.Token(0) == "illegal" && grand.Size() == 2)
					illegalCargoFine = grand.Value(1);
				else if(grand.Token(0) == "illegal" && grand.Size() == 3)
				{
					illegalCargoFine = grand.Value(1);
					illegalCargoMessage = grand.Token(2);
				}
				else if(grand.Token(0) == "stealth")
					failIfDiscovered = true;
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
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
			source = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "source")
			sourceFilter.Load(child);
		else if(child.Token(0) == "destination" && child.Size() == 2)
			destination = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "destination")
			destinationFilter.Load(child);
		else if(child.Token(0) == "waypoint" && child.Size() >= 2)
			waypoints.insert(GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "waypoint")
		{
			waypointFilters.emplace_back();
			waypointFilters.back().Load(child);
		}
		else if(child.Token(0) == "stopover" && child.Size() >= 2)
			stopovers.insert(GameData::Planets().Get(child.Token(1)));
		else if(child.Token(0) == "stopover")
		{
			stopoverFilters.emplace_back();
			stopoverFilters.back().Load(child);
		}
		else if(child.Token(0) == "npc")
		{
			npcs.push_back(NPC());
			npcs.back().Load(child);
		}
		else if(child.Token(0) == "on" && child.Size() >= 2 && child.Token(1) == "enter")
		{
			const System *system = nullptr;
			if(child.Size() >= 3)
				system = GameData::Systems().Get(child.Token(2));
			MissionAction &action = onEnter[system];
			action.Load(child, name);
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
		{
			out.Write("cargo", cargo, cargoSize);
			if(illegalCargoFine)
			{
				out.BeginChild();
				{
					out.Write("illegal", illegalCargoFine, illegalCargoMessage);
				}
				out.EndChild();
			}
			if(failIfDiscovered)
			{
				out.BeginChild();
				{
					out.Write("stealth");
				}
				out.EndChild();
			}
		}
		if(passengers)
			out.Write("passengers", passengers);
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
		for(const Planet *planet : stopovers)
			out.Write("stopover", planet->Name());
		
		for(const NPC &npc : npcs)
			npc.Save(out);
		
		// Save all the actions, because this might be an "available mission" that
		// has not been received yet but must still be included in the saved game.
		for(const auto &it : actions)
			it.second.Save(out);
		for(const auto &it : onEnter)
			if(!didEnter.count(it.first))
				it.second.Save(out);
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



set<const System *> Mission::Waypoints() const
{
	return waypoints;
}



set<const Planet *> Mission::Stopovers() const
{
	return stopovers;
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
	if(planet == destination || stopovers.count(planet))
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
bool Mission::CanOffer(const PlayerInfo &player) const
{
	if(location == BOARDING || location == ASSISTING)
	{
		if(!player.BoardingShip())
			return false;
		
		if(!sourceFilter.Matches(*player.BoardingShip()))
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
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	it = actions.find(ACCEPT);
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	it = actions.find(DECLINE);
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	return true;
}



bool Mission::HasSpace(const PlayerInfo &player) const
{
	int extraCrew = 0;
	if(player.Flagship())
		extraCrew = player.Flagship()->Crew() - player.Flagship()->RequiredCrew();
	return (cargoSize <= player.Cargo().Free() + player.Cargo().CommoditiesSize()
		&& passengers <= player.Cargo().Bunks() + extraCrew);
}



bool Mission::CanComplete(const PlayerInfo &player) const
{
	if(player.GetPlanet() != destination || !waypoints.empty() || !stopovers.empty())
		return false;
	
	if(!toComplete.Test(player.Conditions()))
		return false;
	
	auto it = actions.find(COMPLETE);
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	for(const NPC &npc : npcs)
		if(!npc.HasSucceeded(player.GetSystem()))
			return false;
	
	// If any of the cargo for this mission is being carried by a ship that is
	// not in this system, the mission cannot be completed right now.
	for(const auto &ship : player.Ships())
		if(ship->GetSystem() != player.GetSystem() && ship->Cargo().Get(this))
			return false;
	
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
	if(player.Flagship())
		extraCrew = player.Flagship()->Crew() - player.Flagship()->RequiredCrew();
	
	int cargoNeeded = cargoSize - (player.Cargo().Free() + player.Cargo().CommoditiesSize());
	int bunksNeeded = passengers - (player.Cargo().Bunks() + extraCrew);
	if(cargoNeeded < 0 && bunksNeeded < 0)
		return "";
	
	map<string, string> subs;
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	if(player.Flagship())
		subs["<ship>"] = player.Flagship()->Name();
	
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
bool Mission::Do(Trigger trigger, PlayerInfo &player, UI *ui)
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
		
		stopovers.erase(it);
		if(!stopovers.empty())
			return false;
	}
	if(trigger == ACCEPT)
	{
		++player.Conditions()[name + ": offered"];
		++player.Conditions()[name + ": active"];
	}
	else if(trigger == DECLINE)
		++player.Conditions()[name + ": offered"];
	else if(trigger == FAIL)
		--player.Conditions()[name + ": active"];
	else if(trigger == COMPLETE)
	{
		--player.Conditions()[name + ": active"];
		++player.Conditions()[name + ": done"];
	}
	
	// "Jobs" should never show dialogs when offered, nor should they call the
	// player's mission callback.
	if(trigger == OFFER && location == JOB)
		ui = nullptr;
	
	auto it = actions.find(trigger);
	if(it == actions.end())
	{
		// If a mission has no "on offer" field, it is automatically accepted.
		if(trigger == OFFER && location != JOB)
			player.MissionCallback(Conversation::ACCEPT);
		return true;
	}
	
	if(!it->second.CanBeDone(player))
		return false;
	
	// Perform any actions tied to this event.
	it->second.Do(player, ui, destination ? destination->GetSystem() : nullptr);
	
	return true;
}



// Get a list of NPCs associated with this mission. Every time the player
// takes off from a planet, they should be added to the active ships.
const list<NPC> &Mission::NPCs() const
{
	return npcs;
}



// If any event occurs between two ships, check to see if this mission cares
// about it. This may affect the mission status or display a message.
void Mission::Do(const ShipEvent &event, PlayerInfo &player, UI *ui)
{
	if(event.TargetGovernment()->IsPlayer() && !hasFailed
			&& (event.Type() & ShipEvent::DESTROY))
	{
		bool failed = false;
		for(const auto &it : event.Target()->Cargo().MissionCargo())
			failed |= (it.first == this);
		for(const auto &it : event.Target()->Cargo().PassengerList())
			failed |= (it.first == this);
		
		if(failed)
		{
			hasFailed = true;
			if(isVisible)
				Messages::Add("Ship lost. Mission failed: \"" + displayName + "\".");
		}
	}
	
	// Jump events are only created for the player's flagship.
	if((event.Type() & ShipEvent::JUMP) && event.Actor())
	{
		const System *system = event.Actor()->GetSystem();
		// If this was a waypoint, clear it.
		waypoints.erase(system);
		
		Enter(system, player, ui);
		// Allow special "on enter" conditions that match any system.
		Enter(nullptr, player, ui);
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



// "Instantiate" a mission by replacing randomly selected values and places
// with a single choice, and then replacing any wildcard text as well.
Mission Mission::Instantiate(const PlayerInfo &player) const
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
	// If one of the waypoints is the current system, it is already visited.
	result.waypoints.erase(player.GetSystem());
	// Handle waypoint systems that are chosen randomly.
	for(const LocationFilter &filter : waypointFilters)
	{
		const System *system = PickSystem(filter, player);
		if(!system)
			return result;
		result.waypoints.insert(system);
	}
	
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
		const Planet *planet = PickPlanet(filter, player);
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
		result.destination = PickPlanet(destinationFilter, player);
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
			commodity = PickCommodity(*player.GetSystem(), *result.destination->GetSystem());
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
	const System *source = player.GetSystem();
	list<const System *> destinations;
	for(const System *system : result.waypoints)
		destinations.push_back(system);
	for(const Planet *planet : result.stopovers)
		destinations.push_back(planet->GetSystem());
	
	int jumps = 0;
	while(!destinations.empty())
	{
		// Find the closest destination to this location.
		DistanceMap distance(source);
		auto it = destinations.begin();
		auto bestIt = it;
		for(++it; it != destinations.end(); ++it)
			if(distance.Days(*it) < distance.Days(*bestIt))
				bestIt = it;
		
		source = *bestIt;
		destinations.erase(bestIt);
		jumps += distance.Days(*bestIt);
	}
	DistanceMap distance(source);
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
	else if(player.BoardingShip())
		subs["<origin>"] = player.BoardingShip()->Name();
	subs["<planet>"] = result.destination ? result.destination->Name() : "";
	subs["<system>"] = result.destination ? result.destination->GetSystem()->Name() : "";
	subs["<destination>"] = subs["<planet>"] + " in the " + subs["<system>"] + " system";
	subs["<date>"] = result.deadline.ToString();
	subs["<day>"] = result.deadline.LongString();
	if(!result.stopovers.empty())
	{
		string planets;
		const Planet * const *last = &*--result.stopovers.end();
		int count = 0;
		// Iterate by reference to the pointers so we can check when we're at
		// the very last one in the set.
		for(const Planet * const &planet : result.stopovers)
		{
			if(count++)
				planets += (&planet != last) ? ", " : (count > 2 ? ", and " : " and ");
			planets += planet->Name() + " in the " + planet->GetSystem()->Name() + " system";
		}
		subs["<stopovers>"] = planets;
	}
	
	// Instantiate the NPCs. This also fills in the "<npc>" substitution.
	for(const NPC &npc : npcs)
		result.npcs.push_back(npc.Instantiate(subs, player.GetSystem(), result.destination->GetSystem()));
	
	// Instantiate the actions. The "complete" action is always first so that
	// the "<payment>" substitution can be filled in.
	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(subs, jumps, payload);
	for(const auto &it : onEnter)
		result.onEnter[it.first] = it.second.Instantiate(subs, jumps, payload);
	
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



void Mission::Enter(const System *system, PlayerInfo &player, UI *ui)
{
	auto eit = onEnter.find(system);
	if(eit != onEnter.end() && !didEnter.count(eit->first))
	{
		eit->second.Do(player, ui);
		didEnter.insert(eit->first);
	}
}



const System *Mission::PickSystem(const LocationFilter &filter, const PlayerInfo &player) const
{
	// Find a planet that satisfies the filter.
	vector<const System *> options;
	for(const auto &it : GameData::Systems())
	{
		// Skip entries with incomplete data.
		if(it.second.Name().empty())
			continue;
		if(filter.Matches(&it.second, player.GetSystem()))
			options.push_back(&it.second);
	}
	return options.empty() ? nullptr : options[Random::Int(options.size())];
}



const Planet *Mission::PickPlanet(const LocationFilter &filter, const PlayerInfo &player) const
{
	// Find a planet that satisfies the filter.
	vector<const Planet *> options;
	for(const auto &it : GameData::Planets())
	{
		// Skip entries with incomplete data.
		if(it.second.Name().empty() || (clearance.empty() && !it.second.CanLand()))
			continue;
		if(it.second.IsWormhole() || !it.second.HasSpaceport())
			continue;
		if(filter.Matches(&it.second, player.GetSystem()))
			options.push_back(&it.second);
	}
	return options.empty() ? nullptr : options[Random::Int(options.size())];
}
