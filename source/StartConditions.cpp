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
#include "GameData.h"
#include "Planet.h"
#include "Ship.h"
#include "System.h"

using namespace std;



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
		else if(child.Token(0) == "account")
			accounts.Load(child);
		else if(child.Token(0) == "ship" && child.Size() >= 2)
		{
			// TODO: support named stock ships.
			// Assume that child nodes introduce a full ship definition. Even without child nodes,
			// Ship::Load + Ship::FinishLoading will create the expected ship instance if there is
			// a 3rd token (i.e. this will be treated as though it were a ship variant definition,
			// without making the variant available to the rest of GameData).
			if(child.HasChildren() || child.Size() >= 3)
				ships.emplace_back(child);
			// If there's only 2 tokens & there's no child nodes, the created instance would be ill-formed.
			else
				child.PrintTrace("Skipping unsupported use of a \"stock\" ship (a full definition is required):");
		}
		else
			conditions.Add(child);
	}
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



const Planet &StartConditions::GetPlanet() const
{
	return planet ? *planet : *GameData::Planets().Get("New Boston");
}



const System &StartConditions::GetSystem() const
{
	if(system)
		return *system;
	const System *planetSystem = GetPlanet().GetSystem();
	
	return planetSystem ? *planetSystem : *GameData::Systems().Get("Rutilicus");
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
