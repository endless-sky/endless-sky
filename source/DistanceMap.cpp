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
#include "StellarObject.h"
#include "System.h"

using namespace std;



// Find paths branching out from the given system. The optional arguments put
// a limit on how many systems will be returned (e.g. buying a local map) and
// a limit on how many jumps away they can be (e.g. a valid mission location).
DistanceMap::DistanceMap(const System *center, int maxSystems, int maxDays)
	: center(center), maxSystems(maxSystems), maxDays(maxDays)
{
	Init();
}



// If a player is given, the map will start from the player's system.
// Pathfinding will only use hyperspace paths known to the player; that is,
// one end of the path has been visited. Also, if the ship has a jump drive
// or wormhole access, the route will make use of it.
DistanceMap::DistanceMap(const PlayerInfo &player)
	: player(&player),  useWormholes(true)
{
	if(!player.Flagship())
		return;

	if(player.Flagship()->IsEnteringHyperspace())
		center = player.Flagship()->GetTargetSystem();
	else
		center = player.Flagship()->GetSystem();

	Init(player.Flagship());
}


// Private constructors with a destination for use by RoutePlan
DistanceMap::DistanceMap(const System &center, const System &destination)
	: center(&center), destination(&destination)
{
	Init();
}



DistanceMap::DistanceMap(const PlayerInfo &player, const System &center, const System &destination)
	: center(&center), destination(&destination), useWormholes(true)
{
	if(!player.Flagship())
		return;

	Init(player.Flagship());
}



DistanceMap::DistanceMap(const Ship &ship, const System &destination)
	: center(ship.GetSystem()), destination(&destination), useWormholes(true)
{
	Init(&ship);
}



// Find out if the given system is reachable
bool DistanceMap::HasRoute(const System &system) const
{
	return route.count(&system);
}



// Find out how many days away the given system is.
int DistanceMap::Days(const System &system) const
{
	auto it = route.find(&system);
	return (it == route.end() ? -1 : it->second.days);
}



// Get a set containing all the systems.
set<const System *> DistanceMap::Systems() const
{
	set<const System *> systems;
	for(const auto &it : route)
		systems.insert(it.first);
	return systems;
}



// Get the planned route from center to this system.
vector<const System *> DistanceMap::Plan(const System &system) const
{
	auto plan = vector<const System *> {};
	if(!HasRoute(system))
		return plan;

	const System *nextStep = &system;
	while(nextStep != center)
	{
		plan.push_back(nextStep);
		nextStep = route.at(nextStep).prev;
	}
	return plan;
}



DistanceMap::Edge::Edge(const System *system)
	: prev(system)
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
void DistanceMap::Init(const Ship *ship)
{
	if(!center || center == destination)
		return;

	// To get to the starting point, there is no previous system,
	// and it takes no fuel or days.
	route[center] = Edge();
	if(!maxDays)
		return;

	// Check what travel capabilities this ship has. If no ship is given, the
	// DistanceMap class defaults assume hyperdrive capability only.
	if(ship)
	{
		hyperspaceFuel = ship->HyperdriveFuel();
		// Todo: consider outfit "jump distance" at each link to find if more fuel is needed
		// by a second jump drive outfit with more range and cost via JumpDriveFuel(to).
		jumpFuel = ship->JumpDriveFuel();
		jumpRange = ship->JumpRange();
		// If hyperjumps and non-hyper jumps cost the same amount, there is no
		// need to check hyperjump paths at all.
		if(hyperspaceFuel == jumpFuel)
			hyperspaceFuel = 0.;

		// If this ship has no mode of hyperspace travel, and no local
		// wormhole to use, bail out.
		if(!jumpFuel && !hyperspaceFuel)
		{
			bool hasWormhole = false;
			for(const StellarObject &object : ship->GetSystem()->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsWormhole())
				{
					hasWormhole = true;
					break;
				}

			if(!hasWormhole)
				return;
		}
	}

	// Find the route with lowest fuel use. If multiple routes use the same fuel,
	// choose the one with the fewest jumps (i.e. using jump drive rather than
	// hyperdrive). If multiple routes have the same fuel and the same number of
	// jumps, break the tie by using how "dangerous" the route is.

	// Find all edges from that route, add better routes to the map, and continue.

	// Add this fake edge "from center" so it's the first popped value.
	edgesTodo.emplace(center);
	while(maxSystems && !edgesTodo.empty())
	{
		// Use the best known route to build upon and create the next set
		// of candidate edges. Edges are inserted with 'prev' assigned, but the
		// other values are a step behind. Here we copy and update them:
		// For each system 'X' reachable from 'prev', the edge's fuel/days/danger
		// are built upon to determine if this new edge from 'prev' to 'X' is
		// the best. If so, it's added as route[X], and a copy is added to
		// edgesTodo to process later.
		Edge nextEdge = edgesTodo.top();
		edgesTodo.pop();

		const System *currentSystem = nextEdge.prev;

		// If a destination is given, stop searching once we have the best route.
		if(currentSystem == destination)
			break;

		// Increment the danger to include this system.
		// Don't need to worry about the danger for the next system because
		// if you're going there, all routes would include that same danger.
		// (It is slightly redundant that this includes the danger of the
		//  starting system instead, but the code is simpler this way.)
		nextEdge.danger += currentSystem->Danger();

		// Increment the travel time to include the next system. The fuel cost will be
		// incremented later, because it depends on what type of travel is being done.
		++nextEdge.days;

		// Check for wormholes (which cost zero fuel). Wormhole travel should
		// not be included in Local Maps or mission itineraries.
		if(useWormholes)
			for(const StellarObject &object : currentSystem->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsWormhole())
				{
					const System &link = *object.GetPlanet()->WormholeDestination(currentSystem);
					if(HasBetter(link, nextEdge))
						continue;

					// In order to plan travel through a wormhole, it must be
					// "accessible" to the ship, and you must have visited
					// the wormhole and both endpoint systems. (If this is a
					// multi-stop wormhole, you may know about some paths that
					// it takes but not others.)
					if(ship && !object.GetPlanet()->IsAccessible(ship))
						continue;
					if(player && !player->HasVisited(*object.GetPlanet()))
						continue;
					if(player && !(player->HasVisited(*currentSystem) && player->HasVisited(link)))
						continue;

					Add(link, nextEdge);
					// Bail out if the maximum number of systems is reached.
					if(!--maxSystems)
						break;
				}

		// Bail out if the maximum number of systems is reached.
		if(hyperspaceFuel && !Propagate(nextEdge, false))
			break;
		if(jumpFuel && !Propagate(nextEdge, true))
			break;
	}
}



// Add the given links to the map, if better. Return false if max systems has been reached.
bool DistanceMap::Propagate(Edge nextEdge, bool useJump)
{
	const System *currentSystem = nextEdge.prev;

	// nextEdge is a copy to be used for this jump type, so build upon its fields.
	nextEdge.fuel += (useJump ? jumpFuel : hyperspaceFuel);
	for(const System *link : (useJump ? currentSystem->JumpNeighbors(jumpRange) : currentSystem->Links()))
	{
		// Find out whether we already have a better path to this system, and
		// check whether this link can be traveled. If this route is being
		// selected by the player, they are constrained to known routes.
		if(HasBetter(*link, nextEdge) || !CheckLink(*currentSystem, *link, useJump))
			continue;

		Add(*link, nextEdge);
		if(!--maxSystems)
			return false;
	}
	return true;
}



// Check if we already have a better path to the given system.
bool DistanceMap::HasBetter(const System &to, const Edge &edge)
{
	auto it = route.find(&to);
	return (it != route.end() && !(it->second < edge));
}



// Add the given path to the record.
void DistanceMap::Add(const System &to, Edge edge)
{
	// This is the best path we have found so far to this system, but it is
	// conceivable that a better one will be found.
	route[&to] = edge;

	// Start building upon this edge and enqueue - this copy of edge
	// is in an incomplete state and needs to be dequeued and worked on.
	edge.prev = &to;
	if(maxDays < 0 || edge.days < maxDays)
		edgesTodo.emplace(edge);
}



// Check whether the given link is travelable. If no player was given in the
// constructor then this is always true; otherwise, the player must know
// that the given link exists.
bool DistanceMap::CheckLink(const System &from, const System &to, bool useJump) const
{
	if(!player)
		return true;

	if(!player->HasSeen(to))
		return false;

	// If you are using a jump drive and you can see just from the positions of
	// the two systems that you can jump between them, you can plot a course
	// between them even if neither system is explored. Otherwise, you need to
	// know if a link exists, so you must have explored at least one of them.
	// The jump range of a system overrides the jump range of this ship.
	double distance = from.JumpRange() ? from.JumpRange() : jumpRange;
	if(useJump && from.Position().Distance(to.Position()) <= distance)
		return true;

	return (player->HasVisited(from) || player->HasVisited(to));
}


// RoutePlan is a wrapper on DistanceMap that uses destination
RoutePlan::RoutePlan(const System &center, const System &destination)
{
	Init(DistanceMap(center, destination));
}



RoutePlan::RoutePlan(const PlayerInfo &player, const System &center, const System &destination)
{
	Init(DistanceMap(player, center, destination));
}



RoutePlan::RoutePlan(const Ship &ship, const System &destination)
{
	Init(DistanceMap(ship, destination));
}



void RoutePlan::Init(const DistanceMap& distance)
{
	const System* destination = distance.destination;
	auto it = distance.route.find(destination);
	if(it == distance.route.end())
		return;

	hasRoute = true;

	while(it->first != distance.center)
	{
		plan.emplace_back(it->first, it->second);
		it = distance.route.find(it->second.prev);
	}
}



// Find out if the destination is reachable.
bool RoutePlan::HasRoute() const
{
	return hasRoute;
}



// Get the first step on the route from center to the destination.
const System *RoutePlan::FirstStep() const
{
	if(!hasRoute)
		return nullptr;

	return plan.back().first;
}



// Find out how many days away the destination is.
int RoutePlan::Days() const
{
	if(!hasRoute)
		return -1;

	return plan.front().second.days;
}



// How much fuel is needed to travel to this system along the route.
int RoutePlan::RequiredFuel() const
{
	if(!hasRoute)
		return -1;

	return plan.front().second.fuel;
}




// Get the list of jumps to take to get to the destination.
vector<const System *> RoutePlan::Plan() const
{
	auto steps = vector<const System *> {};
	for(const auto it : plan)
		steps.push_back(it.first);
	return steps;
}



// How much fuel is needed to travel to this system along the route.
vector<pair<const System *, int>> RoutePlan::FuelCosts() const
{
	auto steps = vector<pair<const System *, int>> {};
	for(const auto it : plan)
		steps.emplace_back(it.first, it.second.fuel);
	return steps;
}
