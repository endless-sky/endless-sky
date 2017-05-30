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

#include "Planet.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"

using namespace std;



// Find paths to the given system. If the given maximum count is above zero,
// it is a limit on how many systems should be returned. If it is below zero
// it specifies the maximum distance away that paths should be found.
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
	{
		if(player.Flagship()->IsEnteringHyperspace())
			center = player.Flagship()->GetTargetSystem();
		else
			center = player.Flagship()->GetSystem();
	}
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
	return route.count(system);
}



// Find out how many days away the given system is.
int DistanceMap::Days(const System *system) const
{
	auto it = route.find(system);
	return (it == route.end() ? -1 : it->second.days);
}



// Starting in the given system, what is the next system along the route?
const System *DistanceMap::Route(const System *system) const
{
	auto it = route.find(system);
	return (it == route.end() ? nullptr : it->second.next);
}
	
	
	
// Get a set containing all the systems.
set<const System *> DistanceMap::Systems() const
{
	set<const System *> systems;
	for(const auto &it : route)
		systems.insert(it.first);
	return systems;
}



DistanceMap::Edge::Edge(const System *system)
	: next(system)
{
}



// Sorting operator to prioritize the "best" edges. The priority queue
// returns the "largest" item, so this should return true if this item
// is lower priority than the given item.
bool DistanceMap::Edge::operator<(const Edge &other) const
{
	if(fuel != other.fuel)
		return (fuel > other.fuel);
	
	if(days != other.days)
		return (days > other.days);
	
	return (danger > other.danger);
}



// Depending on the capabilities of the given ship, use hyperspace paths,
// jump drive paths, or both to find the shortest route. Bail out if the
// source system or the maximum count is reached.
void DistanceMap::Init(const System *center, const Ship *ship)
{
	if(!center)
		return;
	
	route[center] = Edge();
	if(!maxDistance)
		return;
	
	// Check what travel capabilities this ship has. If no ship is given, assume
	// hyperdrive capability and no jump drive.
	if(ship)
	{
		hyperspaceFuel = ship->HyperdriveFuel();
		jumpFuel = ship->JumpDriveFuel();
		// If hyperjumps and non-hyper jumps cost the same amount, there is no
		// need to check hyperjump paths at all.
		if(hyperspaceFuel == jumpFuel)
			hyperspaceFuel = 0.;
		
		// If this ship has no mode of hyperspace travel, bail out.
		if(!hyperspaceFuel && !jumpFuel)
			return;
	}
	
	// Find the route with lowest fuel use. If multiple routes use the same fuel,
	// choose the one with the fewest jumps (i.e. using jump drive rather than
	// hyperdrive). If multiple routes have the same fuel and the same number of
	// jumps, break the tie by using how "dangerous" the route is.
	edges.emplace(center);
	while(maxCount && !edges.empty())
	{
		Edge top = edges.top();
		edges.pop();
		
		if(top.next == source)
			break;
		// Increment the danger and the travel time to include this system. The
		// fuel cost will be incremented later, because it depends on what type
		// of travel is being done.
		top.danger += top.next->Danger();
		++top.days;
		
		// Check for wormholes (which cost zero fuel). Wormhole travel should
		// not be included in maps or mission itineraries.
		if(useWormholes)
			for(const StellarObject &object : top.next->Objects())
				if(object.GetPlanet() && object.GetPlanet()->IsWormhole())
				{
					// If we're seeking a path toward a "source," travel through
					// wormholes in the reverse of the normal direction.
					const System *link = source ?
						object.GetPlanet()->WormholeSource(top.next) :
						object.GetPlanet()->WormholeDestination(top.next);
					if(HasBetter(link, top))
						continue;
					
					// In order to plan travel through a wormhole, it must be
					// "accessible" to your flagship, and you must have visited
					// the wormhole and both endpoint systems. (If this is a
					// multi-stop wormhole, you may know about some paths that
					// it takes but not others.)
					if(ship && !object.GetPlanet()->IsAccessible(ship))
						continue;
					if(player && !player->HasVisited(object.GetPlanet()))
						continue;
					if(player && !(player->HasVisited(top.next) && player->HasVisited(link)))
						continue;
					
					Add(link, top);
				}
		
		// Bail out if the maximum number of systems is reached.
		if(hyperspaceFuel && !Propagate(top, false))
			break;
		if(jumpFuel && !Propagate(top, true))
			break;
	}
}



// Add the given links to the map. Return false if an end condition is hit.
bool DistanceMap::Propagate(Edge edge, bool useJump)
{
	// The "length" of this link is 2 if using a jump drive.
	edge.fuel += (useJump ? jumpFuel : hyperspaceFuel);
	for(const System *link : (useJump ? edge.next->Neighbors() : edge.next->Links()))
	{
		// Find out whether we already have a better path to this system, and
		// check whether this link can be traveled. If this route is being
		// selected by the player, they are constrained to known routes.
		if(HasBetter(link, edge) || !CheckLink(edge.next, link, useJump))
			continue;
		
		Add(link, edge);
		if(!--maxCount)
			return false;
	}
	return true;
}



// Check if we already have a better path to the given system.
bool DistanceMap::HasBetter(const System *to, const Edge &edge)
{
	auto it = route.find(to);
	return (it != route.end() && !(it->second < edge));
}



// Add the given path to the record.
void DistanceMap::Add(const System *to, Edge edge)
{
	// This is the best path we have found so far to this system, but it is
	// conceivable that a better one will be found.
	route[to] = edge;
	edge.next = to;
	if(maxDistance < 0 || edge.days < maxDistance)
		edges.emplace(edge);
}



// Check whether the given link is travelable. If no player was given in the
// constructor then this is always true; otherwise, the player must know
// that the given link exists.
bool DistanceMap::CheckLink(const System *from, const System *to, bool useJump) const
{
	if(!player)
		return true;
	
	if(!player->HasSeen(to))
		return false;
	
	// If you are using a jump drive and you can see just from the positions of
	// the two systems that you can jump between them, you can plot a course
	// between them even if neither system is explored. Otherwise, you need to
	// know if a link exists, so you must have explored at least one of them.
	if(useJump && from->Position().Distance(to->Position()) <= System::NEIGHBOR_DISTANCE)
		return true;
	
	return (player->HasVisited(from) || player->HasVisited(to));
}
