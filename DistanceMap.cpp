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
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"

#include <vector>

using namespace std;



// If a player is given, the map will only use hyperspace paths known to the
// player; that is, one end of the path has been visited. Also, if the
// player's flagship has a jump drive, the jumps will be make use of it.
DistanceMap::DistanceMap(const System *center, int maxCount, int maxDistance)
	: maxCount(maxCount), maxDistance(maxDistance)
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
	
	// Check what travel capabilities this ship has. If no ship is given, assume
	// hyperdrive capability and no jump drive.
	bool hasHyper = ship ? ship->Attributes().Get("hyperdrive") : true;
	bool hasJump = ship ? ship->Attributes().Get("jump drive") : false;
	// If the ship has no jump capability, do pathfinding as if it has a
	// hyperdrive. The Ship class still won't let it jump, though.
	hasHyper |= !(hasHyper | hasJump);
	
	edge.emplace(0, center);
	distance[center] = 0;
	while(maxCount && !edge.empty())
	{
		pair<int, const System *> top = edge.top();
		edge.pop();
		
		int steps = -top.first;
		const System *system = top.second;
		if(system == source)
			break;
		
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
		// Find out whether we already have a better path to this system.
		auto it = distance.find(link);
		if(it != distance.end() && it->second <= steps)
			continue;
		
		// Check whether this link can be traveled. If this route is being
		// selected by the player, they are constrained to known routes.
		if(!CheckLink(system, link))
			continue;
		
		// This is the best path we have found so far to this system, but it is
		// conceivable that a better one will be found.
		distance[link] = steps;
		route[link] = system;
		if(maxDistance < 0 || steps < maxDistance)
			edge.emplace(-steps, link);
		if(!--maxCount)
			return false;
	}
	return true;
}



// Check whether the given link is mappable. If no player was given in the
// constructor then this is always true; otherwise, the player must know
// that the given link exists.
bool DistanceMap::CheckLink(const System *from, const System *to) const
{
	if(!player)
		return true;
	
	if(!player->HasSeen(to))
		return false;
	
	return (player->HasVisited(from) || player->HasVisited(to));
}
