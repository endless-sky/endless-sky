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
#include "Government.h"
#include "OutfitGroup.h"
#include "Phrase.h"
#include "pi.h"
#include "Planet.h"
#include "Random.h"
#include "Ship.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>
#include <cmath>

using namespace std;



Fleet::Fleet()
{
	government = GameData::Governments().Get("Merchant");
	names = GameData::Phrases().Get("civilian");
	fighterNames = GameData::Phrases().Get("deep fighter");
}



void Fleet::Load(const DataNode &node)
{
	// If Load() has already been called once on this fleet, any subsequent
	// calls will replace the variants instead of adding to them.
	bool resetVariants = !variants.empty();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "names" && child.Size() >= 2)
			names = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "fighters" && child.Size() >= 2)
			fighterNames = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "cargo" && child.Size() >= 2)
			cargo = static_cast<int>(child.Value(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "variant")
		{
			if(resetVariants)
			{
				resetVariants = false;
				variants.clear();
				total = 0;
			}
			variants.emplace_back(child);
			total += variants.back().weight;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Get the government of this fleet.
const Government *Fleet::GetGovernment() const
{
	return government;
}



void Fleet::Enter(const System &system, list<shared_ptr<Ship>> &ships, const Planet *planet) const
{
	if(!total || !government)
		return;
	
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= variants[index].weight; ++index)
		choice -= variants[index].weight;
	
	if(variants[index].ships.empty())
		return;
	
	bool isEnemy = system.GetGovernment()->IsEnemy(government);
	bool hasJump = variants[index].ships.front()->Attributes().Get("jump drive");
	const vector<const System *> &linkVector = hasJump ? system.Neighbors() : system.Links();
	int links = linkVector.size();
	// Count the inhabited planets in this system.
	int planets = 0;
	if(!isEnemy && !personality.IsSurveillance())
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
				++planets;
	
	if(!(links + planets))
		return;
	
	int choice = Random::Int(links + planets);
	
	const System *source = &system;
	const System *target = &system;
	Point position;
	unsigned radius = 0;
	
	if(planet)
	{
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() == planet)
			{
				position = object.Position();
				radius = max(0, static_cast<int>(object.Radius()));
				break;
			}
	}
	else if(choice >= links)
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
		if(links)
			target = linkVector[Random::Int(links)];
	}
	else
	{
		radius = 1000;
		source = linkVector[choice];
	}
	
	vector<shared_ptr<Ship>> placed;
	for(const Ship *ship : variants[index].ships)
	{
		if(ship->CanBeCarried())
		{
			shared_ptr<Ship> fighter(Ship::MakeShip(*ship, OutfitGroup::PlunderAge()));
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
		
		ships.push_front(shared_ptr<Ship>(Ship::MakeShip(*ship, OutfitGroup::PlunderAge())));
		ships.front()->SetSystem(source);
		ships.front()->SetPlanet(planet);
		if(source == &system)
			ships.front()->Place(pos, angle.Unit(), angle);
		else
		{
			Point offset = system.Position() - source->Position();
			angle = TO_DEG * atan2(offset.X(), -offset.Y());
			ships.front()->Place(pos, Point(), angle);
		}
		if(target != source)
			ships.front()->SetTargetSystem(target);
		ships.front()->SetGovernment(government);
		ships.front()->SetName(names->Get());
		ships.front()->SetPersonality(personality);
		
		if(!placed.empty())
			ships.front()->SetParent(placed.front());
		
		placed.push_back(ships.front());
		
		SetCargo(&*ships.front());
	}
}



// Place a fleet in the given system, already "in action."
void Fleet::Place(const System &system, list<shared_ptr<Ship>> &ships, bool carried) const
{
	if(!total || !government)
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
		if(carried && ship->CanBeCarried())
		{
			shared_ptr<Ship> fighter( Ship::MakeShip(*ship, OutfitGroup::PlunderAge()) );
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
		
		ships.push_front(shared_ptr<Ship>( Ship::MakeShip(*ship, OutfitGroup::PlunderAge()) ));
		ships.front()->SetSystem(&system);
		ships.front()->Place(pos, velocity * angle.Unit(), angle);
		ships.front()->SetGovernment(government);
		ships.front()->SetName(names->Get());
		ships.front()->SetPersonality(personality);
		
		if(!placed.empty())
			ships.front()->SetParent(placed.front());
		
		placed.push_back(ships.front());
		
		SetCargo(&*ships.front());
	}
}



// Do the randomization to make a ship enter or be in the given system.
void Fleet::Enter(const System &system, Ship &ship)
{
	if(!system.Links().size())
	{
		Place(system, ship);
		return;
	}
	
	const System *source = system.Links()[Random::Int(system.Links().size())];
	Angle angle = Angle::Random();
	Point pos = angle.Unit() * Random::Real() * 1000.;
	
	ship.Place(pos, angle.Unit(), angle);
	ship.SetSystem(source);
	ship.SetTargetSystem(&system);
}



void Fleet::Place(const System &system, Ship &ship)
{
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
	center += Angle::Random().Unit() * (Random::Real() * 2. - 1.);
	Point pos = center + Angle::Random().Unit() * Random::Real() * 400.;
	
	double velocity = Random::Real() * ship.MaxVelocity();
	
	ship.SetSystem(&system);
	Angle angle = Angle::Random();
	ship.Place(pos, velocity * angle.Unit(), angle);
}



void Fleet::SetCargo(Ship *ship) const
{
	for(int i = 0; i < cargo; ++i)
	{
		int free = ship->Cargo().Free();
		if(!free)
			break;
		
		int index = Random::Int(GameData::Commodities().size());
		const Trade::Commodity &commodity = GameData::Commodities()[index];
		int amount = Random::Int(free) + 1;
		ship->Cargo().Transfer(commodity.name, -amount);
	}
	int extraCrew = ship->Attributes().Get("bunks") - ship->RequiredCrew();
	if(extraCrew > 0)
		ship->AddCrew(Random::Int(extraCrew + 1));
}



Fleet::Variant::Variant(const DataNode &node)
{
	weight = (node.Size() < 2) ? 1 : static_cast<int>(node.Value(1));
	
	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() > 1 && child.Value(1) >= 1.)
			n = static_cast<int>(child.Value(1));
		ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
	}
}
