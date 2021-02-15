/* BasicStartData.cpp
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
		LoadChild(child);
}



void CoreStartData::Save(DataWriter &out) const
{
	out.Write("start", identifier);
	out.BeginChild();
	{
		out.Write("system", system->Name());
		out.Write("planet", planet->TrueName());
		if(date)
			out.Write("date", date.Year(), date.Month(), date.Day());
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



bool CoreStartData::LoadChild(const DataNode &child)
{
	if(child.Token(0) == "date" && child.Size() >= 4)
		date = Date(child.Value(1), child.Value(2), child.Value(3));
	else if(child.Token(0) == "system" && child.Size() >= 2)
		system = GameData::Systems().Get(child.Token(1));
	else if(child.Token(0) == "planet" && child.Size() >= 2)
		planet = GameData::Planets().Get(child.Token(1));
	else if(child.Token(0) == "account")
		accounts.Load(child);
	else
		return false;
	
	return true;
}
