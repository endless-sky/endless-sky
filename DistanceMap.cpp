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
DistanceMap::DistanceMap(const System *center, int maxCount)
{
	distance[center] = 0;
	Init(maxCount);
}



DistanceMap::DistanceMap(const PlayerInfo &player)
{
	if(!player.GetShip() || !player.GetShip()->GetSystem())
		return;
	
	distance[player.GetShip()->GetSystem()] = 0;
	
	if(player.GetShip()->Attributes().Get("jump drive"))
		InitJump(player);
	else if(player.GetShip()->Attributes().Get("hyperdrive"))
		InitHyper(player);
	
	// If the player has a ship but no hyperdrive capability, all systems are
	// marked as unreachable except for this one.
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



void DistanceMap::Init(int maxCount)
{
	vector<const System *> edge = {distance.begin()->first};
	for(int steps = 1; !edge.empty(); ++steps)
	{
		vector<const System *> newEdge;
		for(const System *system : edge)
			for(const System *link : system->Links())
			{
				auto it = distance.find(link);
				if(it != distance.end())
					continue;
				
				distance[link] = steps;
				route[link] = system;
				newEdge.push_back(link);
				
				// Bail out if we've found the specified number of systems.
				if(maxCount > 0 && !--maxCount)
					return;
			}
		if(steps == -maxCount)
			return;
		newEdge.swap(edge);
	}
}



void DistanceMap::InitHyper(const PlayerInfo &player)
{
	vector<const System *> edge = {distance.begin()->first};
	for(int steps = 1; !edge.empty(); ++steps)
	{
		vector<const System *> newEdge;
		for(const System *system : edge)
			for(const System *link : system->Links())
			{
				if(!player.HasSeen(link))
					continue;
				if(!player.HasVisited(link) && !player.HasVisited(system))
					continue;
				
				auto it = distance.find(link);
				if(it != distance.end())
					continue;
				
				distance[link] = steps;
				route[link] = system;
				newEdge.push_back(link);
			}
		newEdge.swap(edge);
	}
}



void DistanceMap::InitJump(const PlayerInfo &player)
{
	hasJump = true;
	
	vector<const System *> edge = {distance.begin()->first};
	for(int steps = 1; !edge.empty(); ++steps)
	{
		vector<const System *> newEdge;
		for(const System *system : edge)
			for(const System *link : system->Neighbors())
			{
				if(!player.HasSeen(link))
					continue;
				
				auto it = distance.find(link);
				if(it != distance.end())
					continue;
				
				distance[link] = steps;
				route[link] = system;
				newEdge.push_back(link);
			}
		newEdge.swap(edge);
	}
}
