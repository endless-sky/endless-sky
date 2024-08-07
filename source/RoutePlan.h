/* RoutePlan.h
Copyright (c) 2022 by alextd

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ROUTE_PLAN_H_
#define ROUTE_PLAN_H_

#include "RouteEdge.h"

#include <vector>

class DistanceMap;
class Planet;
class PlayerInfo;
class Ship;
class System;



// RoutePlan is a wrapper for DistanceMap that uses a destination
// and keeps only the route to that system.
class RoutePlan {
public:
	explicit RoutePlan(const System &center, const System &destination, const PlayerInfo *player = nullptr);
	explicit RoutePlan(const Ship &ship, const System &destination, const PlayerInfo *player = nullptr);

	// Find out if the destination is reachable.
	bool HasRoute() const;
	// Get the first step on the route from center to the destination.
	const System *FirstStep() const;
	// Find out how many days away the destination is.
	int Days() const;
	// How much fuel is needed to travel to this system along the route.
	int RequiredFuel() const;

	// Get the list of jumps to take to get to the destination.
	std::vector<const System *> Plan() const;
	// Get the list of jumps + fuel to take to get to the destination.
	std::vector<std::pair<const System *, int>> FuelCosts() const;

private:
	void Init(const DistanceMap &distance);

private:
	// The final planned route. plan.front() is the destination.
	std::vector<std::pair<const System *, RouteEdge>> plan;
	bool hasRoute = false;
};



#endif
