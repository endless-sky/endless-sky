/* GameEvent.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameEvent.h"

#include "DataWriter.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "System.h"

#include <algorithm>

using namespace std;

namespace {
	const set<string> DEFINITION_NODES = {
		"fleet",
		"galaxy",
		"government",
		"outfitter",
		"news",
		"planet",
		"shipyard",
		"system",
	};
}



// Determine the universe object definitions that are defined by the given list of changes.
map<string, set<string>> GameEvent::DeferredDefinitions(const list<DataNode> &changes)
{
	auto definitions = map<string, set<string>> {};
	
	for(auto &&node : changes)
		if(node.Size() >= 2 && node.HasChildren() && DEFINITION_NODES.count(node.Token(0)))
		{
			const string &key = node.Token(0);
			const string &name = node.Token(1);
			if(key == "system")
			{
				// A system is only actually defined by this change node if its position is set.
				if(any_of(node.begin(), node.end(), [](const DataNode &child) noexcept -> bool
						{
							return child.Size() >= 3 && child.Token(0) == "pos";
						}))
					definitions[key].emplace(name);
			}
			// Since this (or any other) event may be used to assign a planet to a system, we cannot
			// do a robust "planet definition" check. Similarly, all other GameEvent-createable objects
			// become valid once they appear as a root-level node that has at least one child node.
			else
				definitions[key].emplace(name);
		}
	
	return definitions;
}



// Construct and Load() at the same time.
GameEvent::GameEvent(const DataNode &node)
{
	Load(node);
}



void GameEvent::Load(const DataNode &node)
{
	// If the event has a name, a condition should be automatically created that
	// represents the fact that this event has occurred.
	if(node.Size() >= 2)
	{
		name = node.Token(1);
		conditionsToApply.Add("set", "event: " + name);
	}
	isDefined = true;
	
	static const auto allowedChanges = []() -> set<string>
		{
			auto allowed = DEFINITION_NODES;
			// Include other modifications that cannot create new universe objects.
			allowed.insert({
				"link",
				"unlink",
			});
			return allowed;
		}();
	
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "unvisit" && child.Size() >= 2)
			systemsToUnvisit.push_back(GameData::Systems().Get(child.Token(1)));
		else if(key == "visit" && child.Size() >= 2)
			systemsToVisit.push_back(GameData::Systems().Get(child.Token(1)));
		else if(key == "unvisit planet" && child.Size() >= 2)
			planetsToUnvisit.push_back(GameData::Planets().Get(child.Token(1)));
		else if(key == "visit planet" && child.Size() >= 2)
			planetsToVisit.push_back(GameData::Planets().Get(child.Token(1)));
		else if(allowedChanges.count(key))
			changes.push_back(child);
		else
			conditionsToApply.Add(child);
	}
}



void GameEvent::Save(DataWriter &out) const
{
	out.Write("event");
	out.BeginChild();
	{
		if(date)
			out.Write("date", date.Day(), date.Month(), date.Year());
		conditionsToApply.Save(out);
		
		for(auto &&system : systemsToUnvisit)
			out.Write("unvisit", system->Name());
		for(auto &&planet : planetsToUnvisit)
			out.Write("unvisit planet", planet->TrueName());
		
		for(auto &&system : systemsToVisit)
			out.Write("visit", system->Name());
		for(auto &&planet : planetsToVisit)
			out.Write("visit planet", planet->TrueName());
		
		for(auto &&change : changes)
			out.Write(change);
	}
	out.EndChild();
}



// All events held by GameData have a name, but those loaded from a save do not.
const string &GameEvent::Name() const
{
	return name;
}



// "Stock" GameEvents require a name to be serialized with an accepted mission.
void GameEvent::SetName(const string &name)
{
	this->name = name;
}



const Date &GameEvent::GetDate() const
{
	return date;
}



// Check that this GameEvent has been loaded from a file (vs. referred to only
// by name), and that the systems & planets it references are similarly defined.
bool GameEvent::IsValid() const
{
	// When Apply is called, we mutate the universe definition before we update
	// the player's knowledge of the universe. Thus, to determine if a system or
	// planet is invalid, we must first peek at what `changes` will do.
	auto deferred = DeferredDefinitions(changes);
	
	for(auto &&systems : {systemsToVisit, systemsToUnvisit})
		for(auto &&system : systems)
			if(!system->IsValid() && !deferred["system"].count(system->Name()))
				return false;
	for(auto &&planets : {planetsToVisit, planetsToUnvisit})
		for(auto &&planet : planets)
			if(!planet->IsValid() && !deferred["planet"].count(planet->TrueName()))
				return false;
	
	return isDefined;
}



void GameEvent::SetDate(const Date &date)
{
	this->date = date;
}



void GameEvent::Apply(PlayerInfo &player)
{
	// Serialize the current reputation with other governments.
	player.SetReputationConditions();
	
	// Apply this event's ConditionSet to the player's conditions.
	conditionsToApply.Apply(player.Conditions());
	// Apply (and store a record of applying) this event's other general
	// changes (e.g. updating an outfitter's inventory).
	player.AddChanges(changes);
	
	// Update the current reputation with other governments (e.g. this
	// event's ConditionSet may have altered some reputations).
	player.CheckReputationConditions();
	
	for(const System *system : systemsToUnvisit)
		player.Unvisit(*system);
	for(const Planet *planet : planetsToUnvisit)
		player.Unvisit(*planet);
	
	// Perform visits after unvisits, as "unvisit <system>"
	// will unvisit any planets in that system.
	for(const System *system : systemsToVisit)
		player.Visit(*system);
	for(const Planet *planet : planetsToVisit)
		player.Visit(*planet);
}



const list<DataNode> &GameEvent::Changes() const
{
	return changes;
}
