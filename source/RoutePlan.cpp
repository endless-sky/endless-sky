/* RoutePlan.cpp
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

#include "RoutePlan.h"

#include "DistanceMap.h"

using namespace std;



// RoutePlan is a wrapper on DistanceMap that uses destination
RoutePlan::RoutePlan(const System &center, const System &destination, const PlayerInfo *player)
{
	Init(DistanceMap(center, destination, player));
}



RoutePlan::RoutePlan(const Ship &ship, const System &destination, const PlayerInfo *player)
{
	Init(DistanceMap(ship, destination, player));
}



RoutePlan::RoutePlan(const RoutePlan &other)
{
	plan = other.plan;
	hasRoute = other.hasRoute;
}



void RoutePlan::Init(const DistanceMap &distance)
{
	auto it = distance.route.find(distance.destination);
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
	vector<const System *> steps;
	for(const auto &it : plan)
		steps.push_back(it.first);
	return steps;
}



// How much fuel is needed to travel to this system along the route.
vector<pair<const System *, int>> RoutePlan::FuelCosts() const
{
	vector<pair<const System *, int>> steps;
	for(const auto &it : plan)
		steps.emplace_back(it.first, it.second.fuel);
	return steps;
}
