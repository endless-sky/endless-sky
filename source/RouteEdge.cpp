/* RouteEdge.cpp
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

#include "RouteEdge.h"



RouteEdge::RouteEdge(const System *system)
	: prev(system)
{
}



// Sorting operator to prioritize the "best" edges. The priority queue
// returns the "largest" item, so this should return true if this item
// is lower priority than the given item.
bool RouteEdge::operator<(const RouteEdge &other) const
{
	if(fuel != other.fuel)
		return fuel > other.fuel;

	if(days != other.days)
		return days > other.days;

	return danger > other.danger;
}
