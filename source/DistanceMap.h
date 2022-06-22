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
#include <vector>

class PlayerInfo;
class Ship;
class System;



// This is a map of shortest routes to get to other systems from the given "center"
// system. It also tracks how many days and fuel it takes to get there. Ships with
// a hyperdrive travel using the "links" between systems. Ships with jump drives
// can make use of those links, but can also travel to any system that's closeby.
// Wormholes can also be used by players or ships.
class DistanceMap {
public:
	// Find paths branching out from the given system. The optional arguments put
	// a limit on how many systems will be returned (e.g. buying a local map)
	// and how far away they are allowed to be (e.g. a valid mission location)
	explicit DistanceMap(const System *center, int maxSystems = -1, int maxDays = -1);
	// If a player is given, the map will only use hyperspace paths known to the
	// player; that is, one end of the path has been visited. Also, if the
	// player's flagship has a jump drive, the jumps will be make use of it.
	// Optionally, the pathfinding will stop once a path to the destination is found.
	explicit DistanceMap(const PlayerInfo &player, const System *center = nullptr, const System *destination = nullptr);
	// Calculate the path for the given ship to get to the given system. The
	// ship will use a jump drive or hyperdrive depending on what it has.
	DistanceMap(const Ship &ship, const System *destination);

	// Find out if the given system is reachable, defaulting to 'destination'
	bool HasRoute(const System *system = nullptr) const;
	// Find out how many days away the given system is.
	int Days(const System *system) const;
	// Get the first step on the route from center to this system (or 'destination')
	const System* FirstStep(const System* system = nullptr) const;
	// Get the planned route from center to this system (starting with .back())
	std::vector<const System*> Plan(const System* system = nullptr) const;
	// Get a set containing all the systems.
	std::set<const System *> Systems() const;
	// How much fuel is needed to travel to this system
	int RequiredFuel(const System *system) const;


private:
	// DistanceMap is built using branching paths from 'center' to all systems.
	// The final result, though, is Edges backtracking those paths:
	// Each system has one Edge which points to the previous step along
	// the route to get there, including how much fuel and how many days
	// the total route will take, and how much danger you will pass through.
	// While building the map, some systems have a non-optimal Edge that
	// gets replaced when a better route is found.
	class Edge {
	public:
		Edge(const System *system = nullptr);

		// Sorting operator to prioritize the "best" edges. The priority queue
		// returns the "largest" item, so this should return true if this item
		// is lower priority than the given item.
		bool operator<(const Edge &other) const;

		// There could be a System* thisSystem, but it would remained unused
		const System *prev = nullptr;
		// Fuel/days needed to get to this system using the route though 'prev'
		int fuel = 0;
		int days = 0;
		// Danger tracks up to the 'prev' system, not to the this system.
		// It's used for comparison purposes only. Anyone going to this system
		// is going to hit its danger anyway, so it doesn't change anything.
		double danger = 0.;
	};


private:
	// Depending on the capabilities of the given ship, use hyperspace paths,
	// jump drive paths, or both to find the shortest route. Bail out if the
	// destination system or the maximum count is reached.
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
	// Final route, each Edge pointing to the previous step along the route.
	std::map<const System *, Edge> route;

	// Variables only used during construction:
	// 'edgesTodo' holds unfinished candidate Edges - Only the 'prev' value
	// is up-to-date. Other values are one step behind, awaiting an update.
	// The top() value is the best route among uncertain systems. Once
	// popped, that's the best route to that system - and then adjacent links
	// from that system are processed, which will build upon the popped Edge
	std::priority_queue<Edge> edgesTodo;
	const PlayerInfo *player = nullptr;
	const System *destination = nullptr;
	const System *center = nullptr;
	int maxSystems = -1;
	int maxDays = -1;
	// How much fuel is used for travel. If either value is zero, it means that
	// the ship does not have that type of drive.
	// Defaults are set for hyperlane usage only. A given ships overrides these.
	int hyperspaceFuel = 100;
	int jumpFuel = 0;
	bool useWormholes = false;
	double jumpRange = 0.;
};



#endif
