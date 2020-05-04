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

#include <set>
#include <string>

using namespace std;



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
	
	static const set<string> allowedChanges = {
		"fleet",
		"galaxy",
		"government",
		"link",
		"outfitter",
		"news",
		"planet",
		"shipyard",
		"system",
		"unlink"
	};
	
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
		
		for(const System *system : systemsToUnvisit)
			if(system)
				out.Write("unvisit", system->Name());
		for(const Planet *planet : planetsToUnvisit)
			if(planet)
				out.Write("unvisit planet", planet->TrueName());
		
		for(const System *system : systemsToVisit)
			if(system)
				out.Write("visit", system->Name());
		for(const Planet *planet : planetsToVisit)
			if(planet)
				out.Write("visit planet", planet->TrueName());
		
		for(const DataNode &change : changes)
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
	// Systems without a position in the map are not valid.
	for(const auto &systems : {systemsToVisit, systemsToUnvisit})
		for(const auto &system : systems)
			if(!system || !system->IsValid())
				return false;
	for(const auto &planets : {planetsToVisit, planetsToUnvisit})
		for(const auto &planet : planets)
			if(!planet || !planet->IsValid())
				return false;
	
	// TODO: the DataNodes in `changes` generate definitions for elements
	// they operate on, but these changes may not be well-defined.
	
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
		player.Unvisit(system);
	for(const Planet *planet : planetsToUnvisit)
		player.Unvisit(planet);
	
	// Perform visits after unvisits, as "unvisit <system>"
	// will unvisit any planets in that system.
	for(const System *system : systemsToVisit)
		player.Visit(system);
	for(const Planet *planet : planetsToVisit)
		player.Visit(planet);
}



const list<DataNode> &GameEvent::Changes() const
{
	return changes;
}
