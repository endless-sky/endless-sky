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
			if(system && !system->Name().empty())
				out.Write("unvisit", system->Name());
		for(const Planet *planet : planetsToUnvisit)
			if(planet && !planet->Name().empty())
				out.Write("unvisit planet", planet->Name());
		
		for(const System *system : systemsToVisit)
			if(system && !system->Name().empty())
				out.Write("visit", system->Name());
		for(const Planet *planet : planetsToVisit)
			if(planet && !planet->Name().empty())
				out.Write("visit planet", planet->Name());
		
		for(const DataNode &change : changes)
			out.Write(change);
	}
	out.EndChild();
}



const string &GameEvent::Name() const
{
	return name;
}



const Date &GameEvent::GetDate() const
{
	return date;
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
