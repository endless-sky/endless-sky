/* DistanceMap.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DistanceMap.h"

#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"

using namespace std;



// If a player is given, the map will only use hyperspace paths known to the
// player; that is, one end of the path has been visited. Also, if the
// player's flagship has a jump drive, the jumps will be make use of it.
DistanceMap::DistanceMap(const System *center, int maxCount, int maxDistance)
	: maxCount(maxCount), maxDistance(maxDistance), useWormholes(false)
{
	Init(center);
}



// If a player is given, the map will only use hyperspace paths known to the
// player; that is, one end of the path has been visited. Also, if the
// player's flagship has a jump drive, the jumps will be make use of it.
DistanceMap::DistanceMap(const PlayerInfo &player, const System *center)
	: player(&player)
{
	if(!player.Flagship())
		return;
	
	if(!center)
		center = player.Flagship()->GetSystem();
	if(!center)
		return;
	
	Init(center, player.Flagship());
}



// Calculate the path for the given ship to get to the given system. The
// ship will use a jump drive or hyperdrive depending on what it has. The
// pathfinding will stop once a path to the destination is found.
DistanceMap::DistanceMap(const Ship &ship, const System *destination)
	: source(ship.GetSystem())
{
	if(!source || !destination)
		return;
	
	Init(destination, &ship);
}



// Find out if the given system is reachable.
bool DistanceMap::HasRoute(const System *system) const
{
	auto it = distance.find(system);
	return (it != distance.end());
}



// Find out how many jumps away the given system is.
int DistanceMap::Distance(const System *system) const
{
	auto it = distance.find(system);
	if(it == distance.end())
		return -1;
	
	return it->second;
}



// Find out how much the jump from the given system will cost: 1 means it is
// a normal hyperjump, 2 means it is a jump drive jump, and 0 means it is a
// wormhole. If there is no path from the given system, this returns -1.
int DistanceMap::Cost(const System *system) const
{
	int distance = Distance(system);
	if(distance < 0)
		return -1;
	
	int nextDistance = Distance(Route(system));
	return (nextDistance < 0) ? -1 : distance - nextDistance;
}



// If I am in the given system, going to the player's system, what system
// should I jump to next?
const System *DistanceMap::Route(const System *system) const
{
	auto it = route.find(system);
	if(it == route.end())
		return nullptr;
	
	return it->second;
}



// Access the distance map directly.
const map<const System *, int> DistanceMap::Distances() const
{
	return distance;
}



// Depending on the capabilities of the given ship, use hyperspace paths,
// jump drive paths, or both to find the shortest route. Bail out if the
// source system or the maximum count is reached.
void DistanceMap::Init(const System *center, const Ship *ship)
{
	if(!center)
		return;
	
	distance[center] = 0;
	if(!maxDistance)
		return;
	
	// Check what travel capabilities this ship has. If no ship is given, assume
	// hyperdrive capability and no jump drive.
	bool hasHyper = ship ? ship->Attributes().Get("hyperdrive") : true;
	bool hasJump = ship ? ship->Attributes().Get("jump drive") : false;
	// If the ship has no jump capability, do pathfinding as if it has a
	// hyperdrive. The Ship class still won't let it jump, though.
	hasHyper |= !(hasHyper | hasJump);
	
	edge.emplace(0, center);
	while(maxCount && !edge.empty())
	{
		pair<int, const System *> top = edge.top();
		edge.pop();
		
		int steps = -top.first;
		const System *system = top.second;
		if(system == source)
			break;
		
		// Check for wormholes (which cost zero fuel). Wormhole travel should
		// not be included in maps or mission itineraries.
		if(useWormholes)
			for(const StellarObject &object : system->Objects())
				if(object.GetPlanet() && object.GetPlanet()->IsWormhole())
				{
					// If we're seeking a path toward a "source," travel through
					// wormholes in the reverse of the normal direction.
					const System *link = source ?
						object.GetPlanet()->WormholeSource(system) :
						object.GetPlanet()->WormholeDestination(system);
					if(HasBetter(link, steps))
						continue;
					
					if(player && !player->HasVisited(object.GetPlanet()))
						continue;
					
					Add(system, link, steps);
				}
		
		if(hasHyper && !Propagate(system, false, steps))
			break;
		if(hasJump && !Propagate(system, true, steps))
			break;
	}
}



// Add the given links to the map. Return false if an end condition is hit.
bool DistanceMap::Propagate(const System *system, bool useJump, int steps)
{
	// The "length" of this link is 2 if using a jump drive.
	steps += 1 + useJump;
	for(const System *link : (useJump ? system->Neighbors() : system->Links()))
	{
		// Find out whether we already have a better path to this system, and
		// check whether this link can be traveled. If this route is being
		// selected by the player, they are constrained to known routes.
		if(HasBetter(link, steps) || !CheckLink(system, link, useJump))
			continue;
		
		Add(system, link, steps);
		if(!--maxCount)
			return false;
	}
	return true;
}



// Check if we already have a better path to the given system.
bool DistanceMap::HasBetter(const System *to, int steps)
{
	auto it = distance.find(to);
	return (it != distance.end() && it->second <= steps);
}



// Add the given path to the record.
void DistanceMap::Add(const System *from, const System *to, int steps)
{
	// This is the best path we have found so far to this system, but it is
	// conceivable that a better one will be found.
	distance[to] = steps;
	route[to] = from;
	if(maxDistance < 0 || steps < maxDistance)
		edge.emplace(-steps, to);
}



// Check whether the given link is mappable. If no player was given in the
// constructor then this is always true; otherwise, the player must know
// that the given link exists.
bool DistanceMap::CheckLink(const System *from, const System *to, bool useJump) const
{
	if(!player)
		return true;
	
	if(!player->HasSeen(to))
		return false;
	
	return (useJump || player->HasVisited(from) || player->HasVisited(to));
}
