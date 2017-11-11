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



void Fleet::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		fleetName = node.Token(1);
	
	// If Load() has already been called once on this fleet, any subsequent
	// calls will replace the variants instead of adding to them.
	bool resetVariants = !variants.empty();
	
	for(const DataNode &child : node)
	{
		// The "add" and "remove" keywords should never be alone on a line, and
		// are only valid with "variant" or "personality" definitions.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		bool hasValue = (child.Size() >= 2);
		if((add || remove) && (!hasValue || (child.Token(1) != "variant" && child.Token(1) != "personality")))
		{	
			child.PrintTrace("Skipping invalid \"" + child.Token(0) + "\" tag:");
			continue;
		}
		
		// If this line is an add or remove, the key is the token at index 1.
		const string &key = child.Token(add || remove);
		
		if(key == "government" && hasValue)
			government = GameData::Governments().Get(child.Token(1));
		else if(key == "names" && hasValue)
			names = GameData::Phrases().Get(child.Token(1));
		else if(key == "fighters" && hasValue)
			fighterNames = GameData::Phrases().Get(child.Token(1));
		else if(key == "cargo" && hasValue)
			cargo = static_cast<int>(child.Value(1));
		else if(key == "commodities" && hasValue)
		{
			commodities.clear();
			for(int i = 1; i < child.Size(); ++i)
				commodities.push_back(child.Token(i));
		}
		else if(key == "personality")
			personality.Load(child);
		else if(key == "variant" && !remove)
		{
			if(resetVariants && !add)
			{
				resetVariants = false;
				variants.clear();
				total = 0;
			}
			variants.emplace_back(child);
			total += variants.back().weight;
		}
		else if(key == "variant")
		{
			// If given a full ship definition of one of this fleet's variant members, remove the variant.
			bool didRemove = false;
			for(auto it = variants.begin(); it != variants.end(); ++it)
			{
				Variant toRemove = Variant(child);
				if(toRemove.ships.size() == it->ships.size() &&
					is_permutation(it->ships.begin(), it->ships.end(), toRemove.ships.begin()))
				{
					total -= it->weight;
					variants.erase(it);
					didRemove = true;
					break;
				}
			}
			if(!didRemove)
				child.PrintTrace("Did not find matching variant for specified operation:");
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
	if(!total || variants.empty())
		return;
	
	// Pick a fleet variant to instantiate.
	const Variant &variant = ChooseVariant();
	if(variant.ships.empty())
		return;
	
	// Figure out what system the ship is starting in, where it is going, and
	// what position it should start from in the system.
	const System *source = &system;
	const System *target = &system;
	Point position;
	double radius = 0.;
	
	// Only pick a random entry point for this ship if a source planet was not specified.
	if(!planet)
	{
		// Where this fleet can come from depends on whether it is friendly to any
		// planets in this system and whether it has jump drives.
		vector<const System *> linkVector;
		// Find out what the "best" jump method the fleet has is. Assume that if the
		// others don't have that jump method, they are being carried as fighters.
		// That is, content creators should avoid creating fleets with a mix of jump
		// drives and hyperdrives.
		bool hasJump = false;
		bool hasHyper = false;
		for(const Ship *ship : variant.ships)
		{
			if(ship->Attributes().Get("jump drive"))
				hasJump = true;
			if(ship->Attributes().Get("hyperdrive"))
				hasHyper = true;
		}
		// Don't try to make a fleet "enter" from another system if none of the
		// ships have jump drives.
		if(hasJump || hasHyper)
		{
			bool isWelcomeHere = !system.GetGovernment()->IsEnemy(government);
			for(const System *neighbor : (hasJump ? system.Neighbors() : system.Links()))
			{
				// If this ship is not "welcome" in the current system, prefer to have
				// it enter from a system that is friendly to it. (This is for realism,
				// so attack fleets don't come from what ought to be a safe direction.)
				if(isWelcomeHere || neighbor->GetGovernment()->IsEnemy(government))
					linkVector.push_back(neighbor);
				else
					linkVector.insert(linkVector.end(), 8, neighbor);
			}
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
		{
			// Prefer to launch from inhabited planets, but launch from
			// uninhabited ones if there is no other option.
			for(const StellarObject &object : system.Objects())
				if(object.GetPlanet() && !object.GetPlanet()->GetGovernment()->IsEnemy(government))
					planetVector.push_back(object.GetPlanet());
			options = planetVector.size();
			if(!options)
				return;
		}
		
		// Choose a random planet or star system to come from.
		size_t choice = Random::Int(options);
	
		// If a planet is chosen, also pick a system to travel to after taking off.
		if(choice >= linkVector.size())
		{
			planet = planetVector[choice - linkVector.size()];
			if(!linkVector.empty())
				target = linkVector[Random::Int(linkVector.size())];
		}
		else
		{
			// We are entering this system via hyperspace, not taking off from a planet.
			radius = 1000.;
			source = linkVector[choice];
		}
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
	if(!total || variants.empty())
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
	if(system.Links().empty())
	{
		Place(system, ship);
		return;
	}
	
	// Choose which system this ship is coming from.
	int choice = Random::Int(system.Links().size());
	set<const System *>::const_iterator it = system.Links().begin();
	while(choice--)
		++it;
	
	Angle angle = Angle::Random();
	Point pos = angle.Unit() * Random::Real() * 1000.;
	
	ship.Place(pos, angle.Unit(), angle);
	ship.SetSystem(*it);
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
	weight = 1;
	if(node.Token(0) == "variant" && node.Size() >= 2)
		weight = node.Value(1);
	else if(node.Token(0) == "add" && node.Size() >= 3)
		weight = node.Value(2);
	
	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() >= 2 && child.Value(1) >= 1.)
			n = child.Value(1);
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
		const Phrase *phrase = ((isFighter && fighterNames) ? fighterNames : names);
		if(phrase)
			ship->SetName(phrase->Get());
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
		if(!commodities.empty())
		{
			// If a list of possible commodities was given, pick one of them at
			// random and then double-check that it's a valid commodity name.
			const string &name = commodities[Random::Int(commodities.size())];
			for(const auto &it : GameData::Commodities())
				if(it.name == name)
				{
					index = &it - &GameData::Commodities().front();
					break;
				}
		}
		
		const Trade::Commodity &commodity = GameData::Commodities()[index];
		int amount = Random::Int(free) + 1;
		ship->Cargo().Add(commodity.name, amount);
	}
	int extraCrew = ship->Attributes().Get("bunks") - ship->RequiredCrew();
	if(extraCrew > 0)
		ship->AddCrew(Random::Int(extraCrew + 1));
}
