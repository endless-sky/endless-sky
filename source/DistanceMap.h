/* DistanceMap.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DISTANCE_MAP_H_
#define DISTANCE_MAP_H_

#include <map>
#include <queue>
#include <set>
#include <utility>

class PlayerInfo;
class Ship;
class System;



// This is a map of how many hyperspace jumps it takes to get to other systems
// from the given "center" system. Ships with a hyperdrive travel using the
// "links" between systems. Ships with jump drives can make use of those links,
// but can also travel to any of a system's "neighbors." A distance map can also
// be used to calculate the shortest route between two systems.
class DistanceMap {
public:
	// Find paths to the given system. The optional arguments put a limit on how
	// many systems will be returned and how far away they are allowed to be.
	explicit DistanceMap(const System *center, int maxCount = -1, int maxDistance = -1);
	// If a player is given, the map will only use hyperspace paths known to the
	// player; that is, one end of the path has been visited. Also, if the
	// player's flagship has a jump drive, the jumps will be make use of it.
	explicit DistanceMap(const PlayerInfo &player, const System *center = nullptr);
	// Calculate the path for the given ship to get to the given system. The
	// ship will use a jump drive or hyperdrive depending on what it has. The
	// pathfinding will stop once a path to the destination is found.
	DistanceMap(const Ship &ship, const System *destination);
	
	// Find out if the given system is reachable.
	bool HasRoute(const System *system) const;
	// Find out how many days away the given system is.
	int Days(const System *system) const;
	// Starting in the given system, what is the next system along the route?
	const System *Route(const System *system) const;
	
	// Get a set containing all the systems.
	std::set<const System *> Systems() const;
	// Get the end of the route.
	const System *End() const;
	
	// How much fuel is needed to travel between two systems.
	int RequiredFuel(const System *system1, const System *system2) const;
	
	
private:
	// For each system, track how much fuel it will take to get there, how many
	// days, how much danger you will pass through, and where you will go next.
	class Edge {
	public:
		Edge(const System *system = nullptr);
		
		// Sorting operator to prioritize the "best" edges. The priority queue
		// returns the "largest" item, so this should return true if this item
		// is lower priority than the given item.
		bool operator<(const Edge &other) const;
		
		const System *next = nullptr;
		int fuel = 0;
		int days = 0;
		double danger = 0.;
	};
	
	
private:
	// Depending on the capabilities of the given ship, use hyperspace paths,
	// jump drive paths, or both to find the shortest route. Bail out if the
	// source system or the maximum count is reached.
	void Init(const Ship *ship = nullptr);
	// Add the given links to the map. Return false if an end condition is hit.
	bool Propagate(Edge edge, bool useJump);
	// Check if we already have a better path to the given system.
	bool HasBetter(const System &to, const Edge &edge);
	// Add the given path to the record.
	void Add(const System &to, Edge edge);
	// Check whether the given link is travelable. If no player was given in the
	// constructor then this is always true; otherwise, the player must know
	// that the given link exists.
	bool CheckLink(const System &from, const System &to, bool useJump) const;
	
	
private:
	std::map<const System *, Edge> route;
	
	// Variables only used during construction:
	std::priority_queue<Edge> edges;
	const PlayerInfo *player = nullptr;
	const System *source = nullptr;
	const System *center = nullptr;
	int maxCount = -1;
	int maxDistance = -1;
	// How much fuel is used for travel. If either value is zero, it means that
	// the ship does not have that type of drive.
	int hyperspaceFuel = 100;
	int jumpFuel = 0;
	bool useWormholes = true;
	double jumpRange = 0.;
};



#endif
