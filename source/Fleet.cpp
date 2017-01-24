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
#include "Files.h"
#include "GameData.h"
#include "Government.h"
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
}



void Fleet::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		fleetName = node.Token(1);
	
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
	if(!total || !government || variants.empty())
		return;
	
	// Pick a fleet variant to instantiate.
	const Variant &variant = ChooseVariant();
	if(variant.ships.empty())
		return;
	
	// Where this ship can come from depends on whether it is friendly to any
	// planets in this system and whether it has a jump drive.
	bool hasJump = variant.ships.front()->Attributes().Get("jump drive");
	vector<const System *> linkVector;
	bool isWelcomeHere = !system.GetGovernment()->IsEnemy(government);
	for(const System *neighbor : (hasJump ? system.Neighbors() : system.Links()))
	{
		// If this ship is not "welcome" in the current system, prefer to have
		// it enter from a system that is friendly to it. (This is for realism,
		// so attack fleets don't come from what ought to be a safe direction.)
		if(isWelcomeHere || neighbor->GetGovernment()->IsEnemy(government))
			linkVector.push_back(neighbor);
		else
			linkVector.insert(linkVector.end(), 4, neighbor);
	}
	
	// Find all the inhabited planets this fleet could take off from.
	vector<const Planet *> planetVector;
	if(!personality.IsSurveillance())
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport()
					&& !object.GetPlanet()->GetGovernment()->IsEnemy(government))
				planetVector.push_back(object.GetPlanet());
	
	// If there is nowhere for this fleet to come from, don't create it.
	size_t options = linkVector.size() + planetVector.size();
	if(!options)
		return;
	
	// Choose a random planet or star system to come from.
	size_t choice = Random::Int(options);
	
	// Figure out what system the ship is starting in, where it is going, and
	// what position it should start from in the system.
	const System *source = &system;
	const System *target = &system;
	Point position;
	double radius = 0.;
	
	// If a planet is chosen, also pick a system to travel to after taking off.
	if(choice >= linkVector.size())
	{
		planet = planetVector[choice - linkVector.size()];
		if(!linkVector.empty())
			target = linkVector[Random::Int(linkVector.size())];
	}
	
	// Find the stellar object for the given planet, and place the ships there.
	if(planet)
	{
		for(const StellarObject &object : system.Objects())
			if(object.GetPlanet() == planet)
			{
				position = object.Position();
				radius = object.Radius();
				break;
			}
	}
	else if(choice < linkVector.size())
	{
		// We are entering this system via hyperspace, not taking off from a planet.
		radius = 1000.;
		source = linkVector[choice];
	}
	
	// Place all the ships in the chosen fleet variant.
	shared_ptr<Ship> flagship;
	vector<shared_ptr<Ship>> placed = Instantiate(variant);
	for(shared_ptr<Ship> &ship : placed)
	{
		// If this is a fighter and someone can carry it, no need to position it.
		if(PlaceFighter(ship, placed))
			continue;
		
		Angle angle = Angle::Random(360.);
		Point pos = position + angle.Unit() * (Random::Real() * radius);
		
		ships.push_front(ship);
		ship->SetSystem(source);
		ship->SetPlanet(planet);
		if(source == &system)
			ship->Place(pos, angle.Unit(), angle);
		else
		{
			// Place the ship stationary and pointed in the right direction.
			angle = Angle(system.Position() - source->Position());
			ship->Place(pos, Point(), angle);
		}
		if(target != source)
			ship->SetTargetSystem(target);
		
		if(flagship)
			ship->SetParent(flagship);
		else
			flagship = ship;
		
		SetCargo(&*ship);
	}
}



// Place a fleet in the given system, already "in action."
void Fleet::Place(const System &system, list<shared_ptr<Ship>> &ships, bool carried) const
{
	if(!total || !government || variants.empty())
		return;
	
	// Pick a fleet variant to instantiate.
	const Variant &variant = ChooseVariant();
	if(variant.ships.empty())
		return;
	
	// Determine where the fleet is going to or coming from.
	Point center = ChooseCenter(system);
	
	// Place all the ships in the chosen fleet variant.
	shared_ptr<Ship> flagship;
	vector<shared_ptr<Ship>> placed = Instantiate(variant);
	for(shared_ptr<Ship> &ship : placed)
	{
		// If this is a fighter and someone can carry it, no need to position it.
		if(carried && PlaceFighter(ship, placed))
			continue;
		
		Angle angle = Angle::Random();
		Point pos = center + Angle::Random().Unit() * Random::Real() * 400.;
		double velocity = Random::Real() * ship->MaxVelocity();
		
		ships.push_front(ship);
		ship->SetSystem(&system);
		ship->Place(pos, velocity * angle.Unit(), angle);
		
		if(flagship)
			ship->SetParent(flagship);
		else
			flagship = ship;
		
		SetCargo(&*ship);
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
	// Move out a random distance from that object, facing toward it or away.
	Point pos = ChooseCenter(system) + Angle::Random().Unit() * Random::Real() * 400.;
	
	double velocity = Random::Real() * ship.MaxVelocity();
	
	ship.SetSystem(&system);
	Angle angle = Angle::Random();
	ship.Place(pos, velocity * angle.Unit(), angle);
}


	
int64_t Fleet::Strength() const
{
	int64_t sum = 0;
	for(const Variant &variant : variants)
	{
		int64_t thisSum = 0;
		for(const Ship *ship : variant.ships)
			thisSum += ship->Cost();
		sum += thisSum * variant.weight;
	}
	return sum / total;
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



const Fleet::Variant &Fleet::ChooseVariant() const
{
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= variants[index].weight; ++index)
		choice -= variants[index].weight;
	
	return variants[index];
}



Point Fleet::ChooseCenter(const System &system)
{
	vector<Point> centers;
	for(const StellarObject &object : system.Objects())
		if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
			centers.push_back(object.Position());
	
	if(centers.empty())
		return Point();
	return centers[Random::Int(centers.size())];
}



vector<shared_ptr<Ship>> Fleet::Instantiate(const Variant &variant) const
{
	vector<shared_ptr<Ship>> placed;
	for(const Ship *model : variant.ships)
	{
		if(model->ModelName().empty())
		{
			Files::LogError("Skipping unknown ship in fleet \"" + fleetName + "\".");
			continue;
		}
		
		shared_ptr<Ship> ship(new Ship(*model));
		
		bool isFighter = ship->CanBeCarried();
		ship->SetName(((isFighter && fighterNames) ? fighterNames : names)->Get());
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		
		placed.push_back(ship);
	}
	return placed;
}



bool Fleet::PlaceFighter(shared_ptr<Ship> fighter, vector<shared_ptr<Ship>> &placed) const
{
	if(!fighter->CanBeCarried())
		return false;
	
	for(const shared_ptr<Ship> &parent : placed)
		if(parent->Carry(fighter))
			return true;
	
	return false;
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
		ship->Cargo().Add(commodity.name, amount);
	}
	int extraCrew = ship->Attributes().Get("bunks") - ship->RequiredCrew();
	if(extraCrew > 0)
		ship->AddCrew(Random::Int(extraCrew + 1));
}
