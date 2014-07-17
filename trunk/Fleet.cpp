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

#include "DataNode.h"
#include "GameData.h"
#include "Random.h"
#include "Ship.h"
#include "ShipName.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>

using namespace std;



Fleet::Fleet()
	: data(nullptr), government(nullptr), names(nullptr), cargo(3), total(0)
{
}



void Fleet::Load(const DataNode &node, const GameData &data)
{
	this->data = &data;
	
	// Provide defaults for these in case they are not specified.
	government = data.Governments().Get("Merchant");
	names = data.ShipNames().Get("civilian");
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "government" && child.Size() >= 2)
			government = data.Governments().Get(child.Token(1));
		else if(child.Token(0) == "names" && child.Size() >= 2)
			names = data.ShipNames().Get(child.Token(1));
		else if(child.Token(0) == "fighter names" && child.Size() >= 2)
			fighterNames = data.ShipNames().Get(child.Token(1));
		else if(child.Token(0) == "cargo" && child.Size() >= 2)
			cargo = static_cast<int>(child.Value(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "variant")
		{
			variants.emplace_back(child, data);
			total += variants.back().weight;
		}
	}
}



void Fleet::Enter(const System &system, list<shared_ptr<Ship>> &ships) const
{
	if(!total || !data || !government)
		return;
	
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= variants[index].weight; ++index)
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
	
	int choice = Random::Int(links + planets);
	
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
		target = system.Links()[Random::Int(links)];
	}
	else
		source = system.Links()[choice];
	
	vector<shared_ptr<Ship>> placed;
	for(const Ship *ship : variants[index].ships)
	{
		if(ship->IsFighter())
		{
			shared_ptr<Ship> fighter(new Ship(*ship));
			fighter->SetGovernment(government);
			fighter->SetName((fighterNames ? fighterNames : names)->Get());
			fighter->SetPersonality(personality);
			for(const shared_ptr<Ship> &parent : placed)
				if(parent->AddFighter(fighter))
					break;
			continue;
		}
		Angle angle = Angle::Random(360.);
		Point pos = position + angle.Unit() * (Random::Int(radius + 1));
		
		ships.push_front(shared_ptr<Ship>(new Ship(*ship)));
		ships.front()->SetSystem(source);
		ships.front()->SetPlanet(planet);
		ships.front()->Place(pos, angle.Unit(), angle);
		ships.front()->SetTargetSystem(target);
		ships.front()->SetGovernment(government);
		ships.front()->SetName(names->Get());
		ships.front()->SetPersonality(personality);
		
		if(!placed.empty())
		{
			ships.front()->SetParent(placed.front());
			placed.front()->AddEscort(ships.front());
		}
		placed.push_back(ships.front());
		
		SetCargo(&*ships.front());
	}
}



// Place a fleet in the given system, already "in action."
void Fleet::Place(const System &system, std::list<std::shared_ptr<Ship>> &ships) const
{
	if(!total || !data || !government)
		return;
	
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= variants[index].weight; ++index)
		choice -= variants[index].weight;
	
	// Count the inhabited planets in this system.
	int planets = 0;
	for(const StellarObject &object : system.Objects())
		if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
			++planets;
	
	// Determine where the fleet is going to or coming from.
	Point center;
	if(planets)
	{
		int index = Random::Int(planets);
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
				if(!index--)
					center = object.Position();
	}
	
	// Move out a random distance from that object, facing toward it or away.
	Angle angle = Angle::Random();
	center += angle.Unit() * (Random::Real() * 2. - 1.);
	
	vector<shared_ptr<Ship>> placed;
	for(const Ship *ship : variants[index].ships)
	{
		if(ship->IsFighter())
		{
			shared_ptr<Ship> fighter(new Ship(*ship));
			fighter->SetGovernment(government);
			fighter->SetName((fighterNames ? fighterNames : names)->Get());
			fighter->SetPersonality(personality);
			for(const shared_ptr<Ship> &parent : placed)
				if(parent->AddFighter(fighter))
					break;
			continue;
		}
		Angle angle = Angle::Random();
		Point pos = center + Angle::Random().Unit() * Random::Real() * 400.;
		
		double velocity = Random::Real() * ship->MaxVelocity();
		
		ships.push_front(shared_ptr<Ship>(new Ship(*ship)));
		ships.front()->SetSystem(&system);
		ships.front()->Place(pos, velocity * angle.Unit(), angle);
		ships.front()->SetGovernment(government);
		ships.front()->SetName(names->Get());
		ships.front()->SetPersonality(personality);
		
		if(!placed.empty())
		{
			ships.front()->SetParent(placed.front());
			placed.front()->AddEscort(ships.front());
		}
		placed.push_back(ships.front());
		
		SetCargo(&*ships.front());
	}
}



void Fleet::SetCargo(Ship *ship) const
{
	for(int i = 0; i < cargo; ++i)
	{
		int free = ship->Cargo().Free();
		if(!free)
			break;
		
		int index = Random::Int(data->Commodities().size());
		const Trade::Commodity &commodity = data->Commodities()[index];
		int amount = Random::Int(free) + 1;
		ship->Cargo().Transfer(commodity.name, -amount);
	}
}



Fleet::Variant::Variant(const DataNode &node, const GameData &data)
{
	weight = (node.Size() < 2) ? 1 : static_cast<int>(node.Value(1));
	
	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() > 1 && child.Value(1) >= 1.)
			n = static_cast<int>(child.Value(1));
		ships.insert(ships.end(), n, data.Ships().Get(child.Token(0)));
	}
}
