/* System.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "System.h"

#include "Angle.h"
#include "DataNode.h"
#include "Date.h"
#include "Fleet.h"
#include "GameData.h"
#include "Government.h"
#include "Hazard.h"
#include "Minable.h"
#include "Planet.h"
#include "Random.h"
#include "SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Dynamic economy parameters: how much of its production each system keeps
	// and exports each day:
	const double KEEP = .89;
	const double EXPORT = .10;
	// Standard deviation of the daily production of each commodity:
	const double VOLUME = 2000.;
	// Above this supply amount, price differences taper off:
	const double LIMIT = 20000.;
}

const double System::DEFAULT_NEIGHBOR_DISTANCE = 100.;



System::Asteroid::Asteroid(const string &name, int count, double energy)
	: name(name), count(count), energy(energy)
{
}



System::Asteroid::Asteroid(const Minable *type, int count, double energy)
	: type(type), count(count), energy(energy)
{
}



const string &System::Asteroid::Name() const
{
	return name;
}



const Minable *System::Asteroid::Type() const
{
	return type;
}



int System::Asteroid::Count() const
{
	return count;
}



double System::Asteroid::Energy() const
{
	return energy;
}



System::FleetProbability::FleetProbability(const Fleet *fleet, int period)
	: fleet(fleet), period(period > 0 ? period : 200)
{
}



const Fleet *System::FleetProbability::Get() const
{
	return fleet;
}



int System::FleetProbability::Period() const
{
	return period;
}



System::HazardProbability::HazardProbability(const Hazard *hazard, int period)
	: hazard(hazard), period(period > 0 ? period : 200)
{
}



const Hazard *System::HazardProbability::Get() const
{
	return hazard;
}



int System::HazardProbability::Period() const
{
	return period;
}



// Load a system's description.
void System::Load(const DataNode &node, Set<Planet> &planets)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	isDefined = true;
	
	// For the following keys, if this data node defines a new value for that
	// key, the old values should be cleared (unless using the "add" keyword).
	set<string> shouldOverwrite = {"asteroids", "attributes", "fleet", "link", "object", "hazard"};
	
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
		
		// Get the key and value (if any).
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);
		
		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		bool removeAll = (remove && !hasValue);
		// If this is the first entry for the given key, and we are not in "add"
		// or "remove" mode, its previous value should be cleared.
		bool overwriteAll = (!add && !remove && shouldOverwrite.count(key));
		overwriteAll |= (!add && !remove && key == "minables" && shouldOverwrite.count("asteroids"));
		// Clear the data of the given type.
		if(removeAll || overwriteAll)
		{
			// Clear the data of the given type.
			if(key == "government")
				government = nullptr;
			else if(key == "music")
				music.clear();
			else if(key == "attributes")
				attributes.clear();
			else if(key == "link")
				links.clear();
			else if(key == "asteroids" || key == "minables")
				asteroids.clear();
			else if(key == "haze")
				haze = nullptr;
			else if(key == "trade")
				trade.clear();
			else if(key == "fleet")
				fleets.clear();
			else if(key == "hazard")
				hazards.clear();
			else if(key == "object")
			{
				// Make sure any planets that were linked to this system know
				// that they are no longer here.
				for(StellarObject &object : objects)
					if(object.GetPlanet())
						planets.Get(object.GetPlanet()->TrueName())->RemoveSystem(this);
				
				objects.clear();
			}
			else if(key == "hidden")
				hidden = false;
			
			// If not in "overwrite" mode, move on to the next node.
			if(overwriteAll)
				shouldOverwrite.erase(key == "minables" ? "asteroids" : key);
			else
				continue;
		}
		
		// Handle the attributes which can be "removed."
		if(key == "hidden")
			hidden = true;
		else if(!hasValue && key != "object")
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		else if(key == "attributes")
		{
			if(remove)
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.erase(child.Token(i));
			else
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.insert(child.Token(i));
		}
 		else if(key == "link")
		{
			if(remove)
				links.erase(GameData::Systems().Get(value));
			else
				links.insert(GameData::Systems().Get(value));
		}
		else if(key == "asteroids")
		{
			if(remove)
			{
				for(auto it = asteroids.begin(); it != asteroids.end(); ++it)
					if(it->Name() == value)
					{
						asteroids.erase(it);
						break;
					}
			}
			else if(child.Size() >= 4)
				asteroids.emplace_back(value, child.Value(valueIndex + 1), child.Value(valueIndex + 2));
		}
		else if(key == "minables")
		{
			const Minable *type = GameData::Minables().Get(value);
			if(remove)
			{
				for(auto it = asteroids.begin(); it != asteroids.end(); ++it)
					if(it->Type() == type)
					{
						asteroids.erase(it);
						break;
					}
			}
			else if(child.Size() >= 4)
				asteroids.emplace_back(type, child.Value(valueIndex + 1), child.Value(valueIndex + 2));
		}
		else if(key == "fleet")
		{
			const Fleet *fleet = GameData::Fleets().Get(value);
			if(remove)
			{
				for(auto it = fleets.begin(); it != fleets.end(); ++it)
					if(it->Get() == fleet)
					{
						fleets.erase(it);
						break;
					}
			}
			else
				fleets.emplace_back(fleet, child.Value(valueIndex + 1));
		}
		else if(key == "hazard")
		{
			const Hazard *hazard = GameData::Hazards().Get(value);
			if(remove)
			{
				for(auto it = hazards.begin(); it != hazards.end(); ++it)
					if(it->Get() == hazard)
					{
						hazards.erase(it);
						break;
					}
			}
			else
				hazards.emplace_back(hazard, child.Value(valueIndex + 1));
		}
		// Handle the attributes which cannot be "removed."
		else if(remove)
		{
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
			continue;
		}
		else if(key == "pos" && child.Size() >= 3)
		{
			position.Set(child.Value(valueIndex), child.Value(valueIndex + 1));
			hasPosition = true;
		}
		else if(key == "government")
			government = GameData::Governments().Get(value);
		else if(key == "music")
			music = value;
		else if(key == "habitable")
			habitable = child.Value(valueIndex);
		else if(key == "belt")
			asteroidBelt = child.Value(valueIndex);
		else if(key == "jump range")
			jumpRange = max(0., child.Value(valueIndex));
		else if(key == "haze")
			haze = SpriteSet::Get(value);
		else if(key == "trade" && child.Size() >= 3)
			trade[value].SetBase(child.Value(valueIndex + 1));
		else if(key == "object")
			LoadObject(child, planets);
		else if(key == "arrival")
		{
			if(child.Size() >= 2)
			{
				extraHyperArrivalDistance = child.Value(1);
				extraJumpArrivalDistance = fabs(child.Value(1));
			}
			for(const DataNode &grand : child)
			{
				const string &type = grand.Token(0);
				if(type == "link" && grand.Size() >= 2)
					extraHyperArrivalDistance = grand.Value(1);
				else if(type == "jump" && grand.Size() >= 2)
					extraJumpArrivalDistance = fabs(grand.Value(1));
				else
					grand.PrintTrace("Skipping unsupported arrival distance limitation:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	// Set planet messages based on what zone they are in.
	for(StellarObject &object : objects)
	{
		if(object.message || object.planet)
			continue;
		
		const StellarObject *root = &object;
		while(root->parent >= 0)
			root = &objects[root->parent];
		
		static const string STAR = "You cannot land on a star!";
		static const string HOTPLANET = "This planet is too hot to land on.";
		static const string COLDPLANET = "This planet is too cold to land on.";
		static const string UNINHABITEDPLANET = "This planet is uninhabited.";
		static const string HOTMOON = "This moon is too hot to land on.";
		static const string COLDMOON = "This moon is too cold to land on.";
		static const string UNINHABITEDMOON = "This moon is uninhabited.";
		static const string STATION = "This station cannot be docked with.";
		
		double fraction = root->distance / habitable;
		if(object.IsStar())
			object.message = &STAR;
		else if(object.IsStation())
			object.message = &STATION;
		else if(object.IsMoon())
		{
			if(fraction < .5)
				object.message = &HOTMOON;
			else if(fraction >= 2.)
				object.message = &COLDMOON;
			else
				object.message = &UNINHABITEDMOON;
		}
		else
		{
			if(fraction < .5)
				object.message = &HOTPLANET;
			else if(fraction >= 2.)
				object.message = &COLDPLANET;
			else
				object.message = &UNINHABITEDPLANET;
		}
	}
	// Print a warning if this system wasn't explicitly given a position.
	if(!hasPosition)
		node.PrintTrace("Warning: system will be ignored due to missing position:");
}



// Update any information about the system that may have changed due to events,
// or because the game was started, e.g. neighbors, solar wind and power, or
// if the system is inhabited.
void System::UpdateSystem(const Set<System> &systems, const set<double> &neighborDistances)
{
	neighbors.clear();
	// Neighbors are cached for each system for the purpose of quicker
	// pathfinding. If this system has a static jump range then that
	// is the only range that we need to create jump neighbors for, but
	// otherwise we must create a set of neighbors for every potential
	// jump range that can be encountered.
	if(jumpRange)
	{
		UpdateNeighbors(systems, jumpRange);
		// Systems with a static jump range must also create a set for
		// the DEFAULT_NEIGHBOR_DISTANCE to be returned for those systems
		// which are visible from it.
		UpdateNeighbors(systems, DEFAULT_NEIGHBOR_DISTANCE);
	}
	else
		for(const double distance : neighborDistances)
			UpdateNeighbors(systems, distance);
	
	// Calculate the solar power and solar wind.
	solarPower = 0.;
	solarWind = 0.;
	for(const StellarObject &object : objects)
	{
		solarPower += GameData::SolarPower(object.GetSprite());
		solarWind += GameData::SolarWind(object.GetSprite());
	}
	
	// Systems only have a single auto-attribute, "uninhabited." It is set if
	// the system has no inhabited planets that are accessible to all ships.
	if(IsInhabited(nullptr))
		attributes.erase("uninhabited");
	else
		attributes.insert("uninhabited");
}



// Modify a system's links.
void System::Link(System *other)
{
	links.insert(other);
	other->links.insert(this);
}



void System::Unlink(System *other)
{
	links.erase(other);
	other->links.erase(this);
}



// Check that this system has been loaded and given a position.
bool System::IsValid() const
{
	return isDefined && hasPosition;
}



// Get this system's name.
const string &System::Name() const
{
	return name;
}



void System::SetName(const std::string &name)
{
	this->name = name;
}



// Get this system's position in the star map.
const Point &System::Position() const
{
	return position;
}



// Get this system's government.
const Government *System::GetGovernment() const
{
	static const Government empty;
	return government ? government : &empty;
}




// Get the name of the ambient audio to play in this system.
const string &System::MusicName() const
{
	return music;
}



// Get the list of "attributes" of the planet.
const set<string> &System::Attributes() const
{
	return attributes;
}



// Get a list of systems you can travel to through hyperspace from here.
const set<const System *> &System::Links() const
{
	return links;
}



// Get a list of systems that can be jumped to from here with the given
// jump distance, whether or not there is a direct hyperspace link to them.
// If this system has its own jump range, then it will always return the
// systems within that jump range instead of the jump range given.
const set<const System *> &System::JumpNeighbors(double neighborDistance) const
{
	static const set<const System *> EMPTY;
	const auto it = neighbors.find(jumpRange ? jumpRange : neighborDistance);
	return it == neighbors.end() ? EMPTY : it->second;
}



// Whether this system can be seen when not linked.
bool System::Hidden() const
{
	return hidden;
}



// Additional travel distance to target for ships entering through hyperspace.
double System::ExtraHyperArrivalDistance() const
{
	return extraHyperArrivalDistance;
}



// Additional travel distance to target for ships entering using a jumpdrive.
double System::ExtraJumpArrivalDistance() const
{
	return extraJumpArrivalDistance;
}



// Get a list of systems you can "see" from here, whether or not there is a
// direct hyperspace link to them.
const set<const System *> &System::VisibleNeighbors() const
{
	static const set<const System *> EMPTY;
	const auto it = neighbors.find(DEFAULT_NEIGHBOR_DISTANCE);
	return it == neighbors.end() ? EMPTY : it->second;
}



// Move the stellar objects to their positions on the given date.
void System::SetDate(const Date &date)
{
	double now = date.DaysSinceEpoch();
	
	for(StellarObject &object : objects)
	{
		// "offset" is used to allow binary orbits; the second object is offset
		// by 180 degrees.
		object.angle = Angle(now * object.speed + object.offset);
		object.position = object.angle.Unit() * object.distance;
		
		// Because of the order of the vector, the parent's position has always
		// been updated before this loop reaches any of its children, so:
		if(object.parent >= 0)
			object.position += objects[object.parent].position;
		
		if(object.position)
			object.angle = Angle(object.position);
		
		if(object.planet)
			object.planet->ResetDefense();
	}
}



// Get the stellar object locations on the most recently set date.
const vector<StellarObject> &System::Objects() const
{
	return objects;
}



// Get the stellar object (if any) for the given planet.
const StellarObject *System::FindStellar(const Planet *planet) const
{
	if(planet)
		for(const StellarObject &object : objects)
			if(object.GetPlanet() == planet)
				return &object;
	
	return nullptr;
}



// Get the habitable zone's center.
double System::HabitableZone() const
{
	return habitable;
}



// Get the radius of the asteroid belt.
double System::AsteroidBelt() const
{
	return asteroidBelt;
}



// Get how far ships can jump from this system.
double System::JumpRange() const
{
	return jumpRange;
}



// Get the rate of solar collection and ramscoop refueling.
double System::SolarPower() const
{
	return solarPower;
}



double System::SolarWind() const
{
	return solarWind;
}



// Check if this system is inhabited.
bool System::IsInhabited(const Ship *ship) const
{
	for(const StellarObject &object : objects)
		if(object.GetPlanet())
		{
			const Planet &planet = *object.GetPlanet();
			if(!planet.IsWormhole() && planet.IsInhabited() && planet.IsAccessible(ship))
				return true;
		}
	
	return false;
}



// Check if ships of the given government can refuel in this system.
bool System::HasFuelFor(const Ship &ship) const
{
	for(const StellarObject &object : objects)
		if(object.GetPlanet() && object.GetPlanet()->HasFuelFor(ship))
			return true;
	
	return false;
}



// Check whether you can buy or sell ships in this system.
bool System::HasShipyard() const
{
	for(const StellarObject &object : objects)
		if(object.GetPlanet() && object.GetPlanet()->HasShipyard())
			return true;
	
	return false;
}



// Check whether you can buy or sell ship outfits in this system.
bool System::HasOutfitter() const
{
	for(const StellarObject &object : objects)
		if(object.GetPlanet() && object.GetPlanet()->HasOutfitter())
			return true;
	
	return false;
}



// Get the specification of how many asteroids of each type there are.
const vector<System::Asteroid> &System::Asteroids() const
{
	return asteroids;
}



// Get the background haze sprite for this system.
const Sprite *System::Haze() const
{
	return haze;
}



// Get the price of the given commodity in this system.
int System::Trade(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.price;
}



bool System::HasTrade() const
{
	return !trade.empty();
}



// Update the economy.
void System::StepEconomy()
{
	for(auto &it : trade)
	{
		it.second.exports = EXPORT * it.second.supply;
		it.second.supply *= KEEP;
		it.second.supply += Random::Normal() * VOLUME;
		it.second.Update();
	}
}



void System::SetSupply(const string &commodity, double tons)
{
	auto it = trade.find(commodity);
	if(it == trade.end())
		return;
	
	it->second.supply = tons;
	it->second.Update();
}



double System::Supply(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.supply;
}



double System::Exports(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.exports;
}



// Get the probabilities of various fleets entering this system.
const vector<System::FleetProbability> &System::Fleets() const
{
	return fleets;
}



// Get the probabilities of various hazards in this system.
const vector<System::HazardProbability> &System::Hazards() const
{
	return hazards;
}



// Check how dangerous this system is (credits worth of enemy ships jumping
// in per frame).
double System::Danger() const
{
	double danger = 0.;
	for(const auto &fleet : fleets)
		if(fleet.Get()->GetGovernment()->IsEnemy())
			danger += static_cast<double>(fleet.Get()->Strength()) / fleet.Period();
	return danger;
}



void System::LoadObject(const DataNode &node, Set<Planet> &planets, int parent)
{
	int index = objects.size();
	objects.push_back(StellarObject());
	StellarObject &object = objects.back();
	object.parent = parent;
	
	bool isAdded = (node.Token(0) == "add");
	if(node.Size() >= 2 + isAdded)
	{
		Planet *planet = planets.Get(node.Token(1 + isAdded));
		object.planet = planet;
		planet->SetSystem(this);
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite" && child.Size() >= 2)
		{
			object.LoadSprite(child);
			object.isStar = !child.Token(1).compare(0, 5, "star/");
			if(!object.isStar)
			{
				object.isStation = !child.Token(1).compare(0, 14, "planet/station");
				object.isMoon = (!object.isStation && parent >= 0 && !objects[parent].IsStar());
			}
		}
		else if(child.Token(0) == "distance" && child.Size() >= 2)
			object.distance = child.Value(1);
		else if(child.Token(0) == "period" && child.Size() >= 2)
			object.speed = 360. / child.Value(1);
		else if(child.Token(0) == "offset" && child.Size() >= 2)
			object.offset = child.Value(1);
		else if(child.Token(0) == "object")
			LoadObject(child, planets, index);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Once the star map is fully loaded or an event has changed systems
// or links, figure out which stars are "neighbors" of this one, i.e.
// close enough to see or to reach via jump drive.
void System::UpdateNeighbors(const Set<System> &systems, double distance)
{
	set<const System *> &neighborSet = neighbors[distance];
	
	// Every star system that is linked to this one is automatically a neighbor,
	// even if it is farther away than the maximum distance.
	for(const System *system : links)
		neighborSet.insert(system);
	
	// Any other star system that is within the neighbor distance is also a
	// neighbor.
	for(const auto &it : systems)
	{
		// Skip systems that have no name.
		if(it.first.empty() || it.second.Name().empty())
			continue;

		if(&it.second != this && it.second.Position().Distance(position) <= distance)
			neighborSet.insert(&it.second);
	}
}



void System::Price::SetBase(int base)
{
	this->base = base;
	this->price = base;
}



void System::Price::Update()
{
	price = base + static_cast<int>(-100. * erf(supply / LIMIT));
}
