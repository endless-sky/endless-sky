/* RouteEdge.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

class System;



// DistanceMap is built using branching paths from 'center' to all systems.
// The final result, though, is Edges backtracking those paths:
// Each system has one Edge which points to the previous step along
// the route to get there, including how much fuel and how many days
// the total route will take, and how much danger you will pass through.
// While building the map, some systems have a non-optimal Edge that
// gets replaced when a better route is found.
class RouteEdge {
public:
	RouteEdge(const System *system = nullptr);

	// Sorting operator to prioritize the "best" edges. The priority queue
	// returns the "largest" item, so this should return true if this item
	// is lower priority than the given item.
	bool operator<(const RouteEdge &other) const;

	// There could be a System *thisSystem, but it would remained unused.
	const System *prev = nullptr;
	// Fuel/days needed to get to this system using the route through 'prev'.
	int fuel = 0;
	int days = 0;
	// Danger tracks up to the 'prev' system, not to the this system.
	// It's used for comparison purposes only. Anyone going to this system
	// is going to hit its danger anyway, so it doesn't change anything.
	double danger = 0.;
};
