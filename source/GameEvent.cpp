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

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Government.h"
#include "PlayerInfo.h"
#include "System.h"

using namespace std;



void GameEvent::Load(const DataNode &node)
{
	// If the event has a name, a condition should be automatically created that
	// represents the fact that this event has occurred.
	if(node.Size() >= 2)
		conditionsToApply.Add("set", "event: " + node.Token(1));
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "unvisit" && child.Size() >= 2)
			systemsToUnvisit.push_back(GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "unvisit planet" && child.Size() >= 2)
			planetsToUnvisit.push_back(GameData::Planets().Get(child.Token(1)));
		else if(child.Token(0) == "system" || child.Token(0) == "planet" || child.Token(0) == "galaxy"
				|| child.Token(0) == "shipyard" || child.Token(0) == "outfitter"
				|| child.Token(0) == "fleet" || child.Token(0) == "government"
				|| child.Token(0) == "link" || child.Token(0) == "unlink")
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
		
		for(const DataNode &change : changes)
			out.Write(change);
	}
	out.EndChild();
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
	for(const auto &it : GameData::Governments())
	{
		int rep = it.second.Reputation();
		player.Conditions()["reputation: " + it.first] = rep;
	}
	
	conditionsToApply.Apply(player.Conditions());
	player.AddChanges(changes);
	
	for(const auto &it : GameData::Governments())
	{
		int rep = it.second.Reputation();
		int newRep = player.Conditions()["reputation: " + it.first];
		if(rep != newRep)
			it.second.AddReputation(newRep - rep);
	}
	
	for(const System *system : systemsToUnvisit)
		player.Unvisit(system);
	for(const Planet *planet : planetsToUnvisit)
		player.Unvisit(planet);
}
