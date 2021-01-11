/* StartConditions.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StartConditions.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Planet.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "System.h"

using namespace std;



StartConditions::StartConditions(const DataNode &node)
{
	Load(node);
}



void StartConditions::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			system = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			planet = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "name" && child.Size() >= 2)
			name = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
			description += child.Token(1) + "\n";
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
			sprite = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "account")
			accounts.Load(child);
		else if(child.Token(0) == "ship" && child.Size() >= 2)
			ships.emplace_back(child);
		else
			conditions.Add(child);
	}
	if(description.empty())
	{
		description = "No description provided";
	}
}



void StartConditions::Save(DataWriter &out) const
{
	out.Write("start");
	out.BeginChild();
	{
		out.Write("name", name);
					
		istringstream iss(description);
		for(string line; getline(iss, line); )
		{
			if(!line.empty())
			{
				out.Write("description", line);	
			}
			
		}
		out.Write("system", system->Name());
		out.Write("planet", planet->TrueName());
		out.Write("date", date.Year(), date.Month(), date.Day());
		accounts.Save(out);
	}
	out.EndChild();
}



// Finish loading the ship definitions.
void StartConditions::FinishLoading()
{
	for(Ship &ship : ships)
		ship.FinishLoading(true);
}



Date StartConditions::GetDate() const
{
	return date ? date : Date(16, 11, 3013);
}



const Planet *StartConditions::GetPlanet() const
{
	return planet ? planet : GameData::Planets().Get("New Boston");
}



const System *StartConditions::GetSystem() const
{
	return system ? system : GetPlanet() ? GetPlanet()->GetSystem() : GameData::Systems().Get("Rutilicus");
}



const Account &StartConditions::GetAccounts() const
{
	return accounts;
}



const ConditionSet &StartConditions::GetConditions() const
{
	return conditions;
}



const list<Ship> &StartConditions::Ships() const
{
	return ships;
}


const std::string StartConditions::GetName() const
{
	return name;
}



const std::string StartConditions::GetDescription() const
{
	return description;
}



const Sprite *StartConditions::GetSprite() const 
{
	return sprite;	
}



bool StartConditions::operator==(const StartConditions &other) const 
{
	return (
		GetPlanet() == other.GetPlanet() &&
		GetSystem() == other.GetSystem() &&
		GetAccounts() == other.GetAccounts() &&
		GetDescription() == other.GetDescription() &&
		GetDate() == other.GetDate()
	);
}
