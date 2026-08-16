/* Orbit.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Orbit.h"

#include "Angle.h"
#include "Point.h"


using namespace std;



Orbit::Orbit(double distance, double period, double offset)
	: distance(distance), speed(360. / period), offset(offset)
{
}



pair<Point, Angle> Orbit::Position(double now) const
{
	Angle angle = Angle(now * speed + offset);
	Point position = angle.Unit() * distance;
	return make_pair(position, angle);
}
