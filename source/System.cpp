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
#include "Planet.h"

using namespace std;

namespace {
	static const double NEIGHBOR_DISTANCE = 100.;
}



System::Asteroid::Asteroid(const string &name, int count, double energy)
	: name(name), count(count), energy(energy)
{
}



const string &System::Asteroid::Name() const
{
	return name;
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



// Load a system's description.
void System::Load(const DataNode &node, Set<Planet> &planets)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	// If this is truly a change and not just being called by Load(), these sets
	// of objects will be entirely replaced by any new ones, rather than adding
	// the new ones on to the end of the list:
	bool resetLinks = !links.empty();
	bool resetAsteroids = !asteroids.empty();
	bool resetFleets = !fleets.empty();
	bool resetObjects = !objects.empty();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "pos" && child.Size() >= 3)
			position.Set(child.Value(1), child.Value(2));
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "link")
		{
			if(resetLinks)
			{
				resetLinks = false;
				links.clear();
			}
			if(child.Size() >= 2)
				links.push_back(GameData::Systems().Get(child.Token(1)));
		}
		else if(child.Token(0) == "habitable" && child.Size() >= 2)
			habitable = child.Value(1);
		else if(child.Token(0) == "asteroids")
		{
			if(resetAsteroids)
			{
				resetAsteroids = false;
				asteroids.clear();
			}
			if(child.Size() >= 4)
				asteroids.emplace_back(child.Token(1), child.Value(2), child.Value(3));
		}
		else if(child.Token(0) == "trade" && child.Size() >= 3)
			trade[child.Token(1)] = child.Value(2);
		else if(child.Token(0) == "fleet")
		{
			if(resetFleets)
			{
				resetFleets = false;
				fleets.clear();
			}
			if(child.Size() >= 3)
				fleets.emplace_back(GameData::Fleets().Get(child.Token(1)), child.Value(2));
		}
		else if(child.Token(0) == "object")
		{
			if(resetObjects)
			{
				for(StellarObject &object : objects)
					if(object.GetPlanet())
					{
						Planet *planet = planets.Get(object.GetPlanet()->Name());
						planet->RemoveSystem(this);
					}
				resetObjects = false;
				objects.clear();
			}
			LoadObject(child, planets);
		}
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
		else if (object.IsStation())
			object.message = &STATION;
		else if (object.IsMoon())
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
}



// Once the star map is fully loaded, figure out which stars are "neighbors"
// of this one, i.e. close enough to see or to reach via jump drive.
void System::UpdateNeighbors(const Set<System> &systems)
{
	neighbors.clear();
	
	// Every star system that is linked to this one is automatically a neighbor,
	// even if it is farther away than the maximum distance.
	for(const System *system : links)
		if(!(system->Position().Distance(position) <= NEIGHBOR_DISTANCE))
			neighbors.push_back(system);
	
	// Any other star system that is within the neighbor distance is also a
	// neighbor. This will include any nearby linked systems.
	for(const auto &it : systems)
		if(&it.second != this && it.second.Position().Distance(position) <= NEIGHBOR_DISTANCE)
			neighbors.push_back(&it.second);
}



// Modify a system's links.
void System::Link(System *other)
{
	if(find(links.begin(), links.end(), other) == links.end())
		links.push_back(other);
	if(find(other->links.begin(), other->links.end(), this) == other->links.end())
		other->links.push_back(this);
	
	if(find(neighbors.begin(), neighbors.end(), other) == neighbors.end())
		neighbors.push_back(other);
	if(find(other->neighbors.begin(), other->neighbors.end(), this) == other->neighbors.end())
		other->neighbors.push_back(this);
}



void System::Unlink(System *other)
{
	auto it = find(links.begin(), links.end(), other);
	if(it != links.end())
		links.erase(it);
	
	it = find(other->links.begin(), other->links.end(), this);
	if(it != other->links.end())
		other->links.erase(it);
	
	// If the only reason these systems are neighbors is because of a hyperspace
	// link, they are no longer neighbors.
	if(position.Distance(other->position) > NEIGHBOR_DISTANCE)
	{
		it = find(neighbors.begin(), neighbors.end(), other);
		if(it != neighbors.end())
			neighbors.erase(it);
		
		it = find(other->neighbors.begin(), other->neighbors.end(), this);
		if(it != other->neighbors.end())
			other->neighbors.erase(it);
	}
}



// Get this system's name and position (in the star map).
const string &System::Name() const
{
	return name;
}



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



// Get a list of systems you can travel to through hyperspace from here.
const vector<const System *> &System::Links() const
{
	return links;
}



// Get a list of systems you can "see" from here, whether or not there is a
// direct hyperspace link to them. This is also the set of systems that you
// can travel to from here via the jump drive.
const vector<const System *> &System::Neighbors() const
{
	return neighbors;
}



// Move the stellar objects to their positions on the given date.
void System::SetDate(const Date &date)
{
	double now = date.DaysSinceEpoch();
	
	for(StellarObject &object : objects)
	{
		// "offset" is used to allow binary orbits; the second object is offset
		// by 180 degrees.
		Angle angle(now * object.speed + object.offset);
		object.unit = angle.Unit();
		object.position = object.unit * object.distance;
		
		// Because of the order of the vector, the parent's position has always
		// been updated before this loop reaches any of its children, so:
		if(object.parent >= 0)
			object.position += objects[object.parent].position;
		
		if(object.position)
			object.unit = object.position.Unit();
		
		if(object.planet)
			object.planet->ResetDefense();
	}
}



// Get the stellar object locations on the most recently set date.
const vector<StellarObject> &System::Objects() const
{
	return objects;
}



// Get the habitable zone's center.
double System::HabitableZone() const
{
	return habitable;
}



// Check if this system is inhabited.
bool System::IsInhabited() const
{
	for(const StellarObject &object : objects)
		if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
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



// Get the price of the given commodity in this system.
int System::Trade(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second;
}



// Get the probabilities of various fleets entering this system.
const vector<System::FleetProbability> &System::Fleets() const
{
	return fleets;
}



void System::LoadObject(const DataNode &node, Set<Planet> &planets, int parent)
{
	int index = objects.size();
	objects.push_back(StellarObject());
	StellarObject &object = objects.back();
	object.parent = parent;
	
	if(node.Size() >= 2)
	{
		Planet *planet = planets.Get(node.Token(1));
		object.planet = planet;
		planet->SetSystem(this);
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite" && child.Size() >= 2)
		{
			object.animation.Load(child);
			object.isStar = !child.Token(1).compare(0, 5, "star/");
			if (!object.isStar)
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
	}
}
