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
#include "PlayerInfo.h"

using namespace std;



void GameEvent::Load(const DataNode &node)
{
	// If the event has a name, a condition should be automatically created that
	// represents the fact that this event has occurred.
	conditionsToApply.Load(node);
	if(node.Size() >= 2)
		conditionsToApply.Add("set", "event: " + node.Token(1));
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "system" || child.Token(0) == "planet"
				|| child.Token(0) == "shipyard" || child.Token(0) == "outfitter"
				|| child.Token(0) == "fleet" || child.Token(0) == "government"
				|| child.Token(0) == "link" || child.Token(0) == "unlink")
			changes.push_back(child);
	}
}



void GameEvent::Save(DataWriter &out) const
{
	out.Write("event");
	out.BeginChild();
	
	if(date)
		out.Write("date", date.Day(), date.Month(), date.Year());
	conditionsToApply.Save(out);
	
	for(const DataNode &change : changes)
		out.Write(change);
	
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
	conditionsToApply.Apply(player.Conditions());
	player.AddChanges(changes);
}
