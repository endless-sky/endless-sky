/* Fleet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Fleet.h"

#include "GameData.h"
#include "Ship.h"
#include "ShipName.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>

using namespace std;



Fleet::Fleet()
	: government(nullptr), names(nullptr), total(0)
{
}



void Fleet::Load(const DataFile::Node &node, const GameData &data)
{
	// Provide defaults for these in case they are not specified.
	government = data.Governments().Get("Merchant");
	names = data.ShipNames().Get("civilian");
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "government" && child.Size() >= 2)
			government = data.Governments().Get(child.Token(1));
		else if(child.Token(0) == "names" && child.Size() >= 2)
			names = data.ShipNames().Get(child.Token(1));
		else if(child.Token(0) == "variant")
		{
			variants.emplace_back(child, data);
			total += variants.back().weight;
		}
	}
}



void Fleet::Enter(const System &system, list<shared_ptr<Ship>> &ships) const
{
	if(!total)
		return;
	
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = rand() % total; choice >= variants[index].weight; ++index)
		choice -= variants[index].weight;
	
	bool isEnemy = system.GetGovernment().IsEnemy(government);
	int links = system.Links().size();
	// Count the inhabited planets in this system.
	int planets = 0;
	if(!isEnemy)
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
				++planets;
	
	if(!(links + planets))
		return;
	
	int choice = rand() % (links + planets);
	
	const Planet *planet = nullptr;
	const System *source = &system;
	const System *target = &system;
	Point position;
	unsigned radius = 0;
	if(choice >= links)
	{
		choice -= links;
		
		planets = 0;
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
			{
				if(planets++ != choice)
					continue;
				
				position = object.Position();
				planet = object.GetPlanet();
				radius = max(0, static_cast<int>(object.Radius()));
				break;
			}
		target = system.Links()[rand() % links];
	}
	else
		source = system.Links()[choice];
	
	shared_ptr<Ship> flagship;
	for(const Ship *ship : variants[index].ships)
	{
		Angle angle = Angle::Random(360.);
		Point pos = position + angle.Unit() * (rand() % (radius + 1));
		
		ships.push_front(shared_ptr<Ship>(new Ship(*ship)));
		ships.front()->SetSystem(source);
		ships.front()->SetPlanet(planet);
		ships.front()->Place(pos, angle.Unit(), angle);
		ships.front()->SetTargetSystem(target);
		ships.front()->SetGovernment(government);
		ships.front()->SetName(names->Get());
		
		if(!flagship)
			flagship = ships.front();
		else
		{
			ships.front()->SetParent(flagship);
			flagship->AddEscort(ships.front());
		}
	}
}



Fleet::Variant::Variant(const DataFile::Node &node, const GameData &data)
{
	weight = (node.Size() < 2) ? 1 : static_cast<int>(node.Value(1));
	
	for(const DataFile::Node &child : node)
		ships.push_back(data.Ships().Get(child.Token(0)));
}
