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
#include "DistanceMap.h"
#include "Format.h"
#include "GameData.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "UI.h"

#include <iostream>

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
			int w = max(1, static_cast<int>(100. * pow(2., profit * .01)));
			weight.push_back(w);
			total += w;
		}
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
			name = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
			description = child.Token(1);
		else if(child.Token(0) == "deadline" && child.Size() >= 4)
		{
			hasDeadline = true;
			deadline = Date(child.Value(1), child.Value(2), child.Value(3));
		}
		else if(child.Token(0) == "deadline" && child.Size() >= 2)
			daysToDeadline = child.Value(1);
		else if(child.Token(0) == "deadline")
			doDefaultDeadline = true;
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
				if(grand.Token(0) == "illegal" && grand.Size() >= 2)
				{
					if(grand.Size() >= 3)
						cargoIllegality[GameData::Governments().Get(grand.Token(1))] = grand.Value(2);
					else
						cargoBaseIllegality = grand.Value(1);
				}
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
		else if(child.Token(0) == "job")
			location = JOB;
		else if(child.Token(0) == "landing")
			location = LANDING;
		else if(child.Token(0) == "repeat")
			repeat = (child.Size() == 1 ? 0 : static_cast<int>(child.Value(1)));
		else if(child.Token(0) == "to" && child.Size() >= 2)
		{
			if(child.Token(1) == "offer")
				toOffer.Load(child);
			else if(child.Token(1) == "complete")
				toComplete.Load(child);
		}
		else if(child.Token(0) == "source" && child.Size() >= 2)
			source = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "source")
			sourceFilter.Load(child);
		else if(child.Token(0) == "destination" && child.Size() == 2)
			destination = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "destination")
			destinationFilter.Load(child);
		else if(child.Token(0) == "npc")
		{
			npcs.push_back(NPC());
			npcs.back().Load(child);
		}
		else if(child.Token(0) == "on" && child.Size() >= 2)
		{
			static const map<string, Trigger> trigger = {
				{"complete", COMPLETE},
				{"offer", OFFER},
				{"accept", ACCEPT},
				{"decline", DECLINE},
				{"fail", FAIL}};
			auto it = trigger.find(child.Token(1));
			if(it != trigger.end())
				actions[it->second].Load(child);
		}
	}
}



// Save a mission. It is safe to assume that any mission that is being saved
// is already "instantiated," so only a subset of the data must be saved.
void Mission::Save(DataWriter &out, const std::string &tag) const
{
	out.Write(tag, name);
	out.BeginChild();
	
	if(!description.empty())
		out.Write("description", description);
	if(hasDeadline)
		out.Write("deadline", deadline.Day(), deadline.Month(), deadline.Year());
	if(cargoSize)
	{
		out.Write("cargo", cargo, cargoSize);
		if(cargoBaseIllegality || !cargoIllegality.empty())
		{
			out.BeginChild();
			for(const auto &it : cargoIllegality)
				out.Write("illegal", it.first->GetName(), it.second);
			if(cargoBaseIllegality)
				out.Write("illegal", cargoBaseIllegality);
			out.EndChild();
		}
	}
	if(passengers)
		out.Write("passengers", passengers);
	if(!isVisible)
		out.Write("invisible");
	if(location == LANDING)
		out.Write("landing");
	
	if(!toComplete.IsEmpty())
	{
		out.Write("to", "complete");
		out.BeginChild();
		
		toComplete.Save(out);
		
		out.EndChild();
	}
	if(destination)
		out.Write("destination", destination->Name());
	
	for(const NPC &npc : npcs)
		npc.Save(out);
	
	// Save all the actions, because this might be an "available mission" that
	// has not been received yet but must still be included in the saved game.
	for(const auto &it : actions)
		it.second.Save(out);
	
	out.EndChild();
}



// Basic mission information.
const string &Mission::Name() const
{
	return name;
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



bool Mission::IsAtLocation(Location location) const
{
	return (this->location == location);
}



// Information about what you are doing.
const Planet *Mission::Destination() const
{
	return destination;
}



const string &Mission::Cargo() const
{
	return cargo;
}



int Mission::CargoSize() const
{
	return cargoSize;
}



int Mission::CargoIllegality(const Government *government) const
{
	auto it = cargoIllegality.find(government);
	return (it != cargoIllegality.end() ? it->second : cargoBaseIllegality);
}



int Mission::Passengers() const
{
	return passengers;
}



// The mission must be completed by this deadline (if there is a deadline).
bool Mission::HasDeadline() const
{
	return hasDeadline;
}



const Date &Mission::Deadline() const
{
	return deadline;
}



// If this mission's deadline was before the given date and it has not been
// marked as failing already, mark it and return true.
bool Mission::CheckDeadline(const Date &today)
{
	if(!hasFailed && hasDeadline && deadline < today)
	{
		hasFailed = true;
		return true;
	}
	return false;
}



// Check if it's possible to offer or complete this mission right now.
bool Mission::CanOffer(const PlayerInfo &player) const
{
	if(source && source != player.GetPlanet())
		return false;
	
	if(!sourceFilter.Matches(player.GetPlanet()))
		return false;
	
	if(!toOffer.Test(player.Conditions()))
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
	return (cargoSize <= player.Cargo().Free() && passengers <= player.Cargo().Bunks());
}



bool Mission::CanComplete(const PlayerInfo &player) const
{
	if(player.GetPlanet() != destination)
		return false;
	
	if(!toComplete.Test(player.Conditions()))
		return false;
	
	auto it = actions.find(COMPLETE);
	if(it != actions.end() && !it->second.CanBeDone(player))
		return false;
	
	for(const NPC &npc : npcs)
		if(!npc.HasSucceeded())
			return false;
	
	return true;
}



bool Mission::HasFailed() const
{
	for(const NPC &npc : npcs)
		if(npc.HasFailed())
			return true;
	
	return hasFailed;
}



// When the state of this mission changes, it may make changes to the player
// information or show new UI panels. PlayerInfo::MissionCallback() will be
// used as the callback for any UI panel that returns a value.
bool Mission::Do(Trigger trigger, PlayerInfo &player, UI *ui)
{
	auto it = actions.find(trigger);
	if(it == actions.end())
		return true;
	
	if(!it->second.CanBeDone(player))
		return false;
	
	// Set the "reputation" conditions so we can check if this action changed
	// any of them.
	Politics &politics = GameData::GetPolitics();
	for(const auto &it : GameData::Governments())
	{
		int rep = politics.Reputation(&it.second);
		player.Conditions()["reputation: " + it.first] = rep;
	}
	it->second.Do(player, ui, destination ? destination->GetSystem() : nullptr);
	if(trigger == OFFER)
		player.Conditions()[name + ": offered"] = true;
	if(trigger == COMPLETE)
		player.Conditions()[name + ": done"] = true;
	
	// Check if any reputation conditions were updated.
	for(const auto &it : GameData::Governments())
	{
		int rep = politics.Reputation(&it.second);
		int newRep = player.Conditions()["reputation: " + it.first];
		if(newRep != rep)
			politics.AddReputation(&it.second, newRep - rep);
	}
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
	if(event.TargetGovernment() == GameData::PlayerGovernment() && !hasFailed
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
			Messages::Add("Ship lost. Mission failed: \"" + name + "\".");
		}
	}
	
	for(NPC &npc : npcs)
		npc.Do(event, player, ui);
}



// "Instantiate" a mission by replacing randomly selected values and places
// with a single choice, and then replacing any wildcard text as well.
Mission Mission::Instantiate(const PlayerInfo &player) const
{
	Mission result;
	// If anything goes wrong below, this mission should not be offered.
	result.hasFailed = true;
	result.isVisible = isVisible;
	result.location = location;
	
	// First, pick values for all the variables.
	
	// If a specific destination is not specified in the mission, pick a random
	// one out of all the destinations that satisfy the mission requirements.
	result.destination = destination;
	if(!result.destination && !destinationFilter.IsEmpty())
	{
		// Find a destination that satisfies the filter.
		vector<const Planet *> options;
		for(const auto &it : GameData::Planets())
		{
			// Skip entries with incomplete data.
			if(it.second.Name().empty() || !GameData::GetPolitics().CanLand(&it.second))
				continue;
			if(destinationFilter.Matches(&it.second, player.GetSystem()))
				options.push_back(&it.second);
		}
		if(!options.empty())
			result.destination = options[Random::Int(options.size())];
		else
			return result;
	}
	// If no destination is specified, it is the same as the source planet.
	if(!result.destination)
		result.destination = player.GetPlanet();
	
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
	// Pick a random passenger count, it requested.
	if(passengers | passengerLimit)
	{
		if(passengerProb)
			result.passengers = Random::Polya(passengerLimit, passengerProb) + passengers;
		else if(cargoLimit > cargoSize)
			result.passengers = passengers + Random::Int(passengerLimit - passengers + 1);
		else
			result.passengers = passengers;
	}
	result.cargoIllegality = cargoIllegality;
	result.cargoBaseIllegality = cargoBaseIllegality;
	
	// How far is it to the destination?
	DistanceMap distance(player.GetSystem());
	int jumps = distance.Distance(result.destination->GetSystem());
	int defaultPayment = (jumps + 1) * (150 * result.cargoSize + 1500 * result.passengers);
	int defaultDeadline = doDefaultDeadline ? (2 * jumps) : 0;
	
	// Set the deadline, if requested.
	if(daysToDeadline || defaultDeadline)
	{
		result.hasDeadline = true;
		result.deadline = player.GetDate() + (defaultDeadline + daysToDeadline);
	}
	
	// Copy the completion conditions. No need to copy the offer conditions,
	// because they have already been checked.
	result.toComplete = toComplete;
	
	// Generate the substitutions map.
	map<string, string> subs;
	subs["<commodity>"] = result.cargo;
	subs["<tons>"] = to_string(result.cargoSize) + (result.cargoSize == 1 ? " ton" : " tons");
	subs["<cargo>"] = subs["<tons>"] + " of " + subs["<commodity>"];
	subs["<bunks>"] = to_string(result.passengers);
	subs["<passengers>"] = (result.passengers == 1) ? "your passenger" : "your passengers";
	subs["<fare>"] = (result.passengers == 1) ? "a passenger" : (subs["<bunks>"] + " passengers");
	subs["<origin>"] = player.GetPlanet()->Name();
	subs["<planet>"] = result.destination ? result.destination->Name() : "";
	subs["<system>"] = result.destination ? result.destination->GetSystem()->Name() : "";
	subs["<destination>"] = subs["<planet>"] + " in the " + subs["<system>"] + " system";
	subs["<date>"] = result.deadline.ToString();
	
	// Instantiate the NPCs. This also fills in the "<npc>" substitution.
	for(const NPC &npc : npcs)
		result.npcs.push_back(npc.Instantiate(subs, player.GetSystem()));
	
	// Instantiate the actions. The "complete" action is always first so that
	// the "<payment>" substitution can be filled in.
	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(subs, defaultPayment);
	
	// Perform substitution in the name and description.
	result.name = Format::Replace(name, subs);
	result.description = Format::Replace(description, subs);
	
	result.hasFailed = false;
	return result;
}
