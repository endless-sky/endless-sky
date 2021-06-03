/* CoreStartData.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CoreStartData.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Planet.h"
#include "System.h"

using namespace std;



void CoreStartData::Load(const DataNode &node)
{
	identifier = node.Size() >= 2 ? node.Token(1) : "Unidentified Start";
	for(const DataNode &child : node)
	{
		// Check for the "add" or "remove" keyword.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}
		LoadChild(child, add, remove);
	}
}



void CoreStartData::Save(DataWriter &out) const
{
	out.Write("start", identifier);
	out.BeginChild();
	{
		out.Write("system", system->Name());
		out.Write("planet", planet->TrueName());
		if(date)
			out.Write("date", date.Day(), date.Month(), date.Year());
		accounts.Save(out);
	}
	out.EndChild();
}



Date CoreStartData::GetDate() const
{
	return date ? date : Date(16, 11, 3013);
}



const Planet &CoreStartData::GetPlanet() const
{
	return planet ? *planet : *GameData::Planets().Get("New Boston");
}



const System &CoreStartData::GetSystem() const
{
	if(system)
		return *system;
	const System *planetSystem = GetPlanet().GetSystem();
	
	return planetSystem ? *planetSystem : *GameData::Systems().Get("Rutilicus");
}



const Account &CoreStartData::GetAccounts() const noexcept
{
	return accounts;
}



const string &CoreStartData::Identifier() const noexcept
{
	return identifier;
}



bool CoreStartData::LoadChild(const DataNode &child, bool isAdd, bool isRemove)
{
	const string &key = child.Token((isAdd || isRemove) ? 1 : 0);
	int valueIndex = (isAdd || isRemove) ? 2 : 1;
	bool hasValue = (child.Size() > valueIndex);
	const string &value = child.Token(hasValue ? valueIndex : 0);
	
	if(child.Token(0) == "date" && child.Size() >= 4)
		date = Date(child.Value(1), child.Value(2), child.Value(3));
	else if(key == "system" && hasValue)
		system = GameData::Systems().Get(value);
	else if(key == "planet" && hasValue)
		planet = GameData::Planets().Get(value);
	else if(key == "account")
		accounts.Load(child, !isAdd);
	else
		return false;
	
	return true;
}
