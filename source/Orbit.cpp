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
#include "DataNode.h"
#include "Point.h"

#include <cmath>

using namespace std;



Orbit::Orbit(double distance, double period, double offset)
	: distance(distance), speed(360. / period), offset(offset)
{
}



Orbit::Orbit(const DataNode &node)
{
	Load(node);
}



void Orbit::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		const string &key = child.Token(0);
		if(key == "distance")
			distance = child.Value(1);
		else if(key == "period")
		{
			double period = child.Value(1);
			if(!period)
			{
				node.PrintTrace("An orbit's period may not be equal to zero.");
				return;
			}
			explicitPeriodSet = true;
			speed = 360. / period;
		}
		else if(key == "offset")
			offset = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



double Orbit::Distance() const
{
	return distance;
}



double Orbit::Period() const
{
	return 360. / speed;
}



bool Orbit::HasExplicitPeriod() const
{
	return explicitPeriodSet;
}



double Orbit::Offset() const
{
	return offset;
}



void Orbit::CalculatePeriod(double mass, optional<double> dist)
{
	if(explicitPeriodSet)
		return;
	double period = 10.;
	double useDistance = dist.value_or(distance);
	if(useDistance)
		period = sqrt(pow(useDistance, 3) / mass);
	speed = 360. / period;
}



pair<Point, Angle> Orbit::Position(double now) const
{
	Angle angle = Angle(now * speed + offset);
	Point position = angle.Unit() * distance;
	return make_pair(position, angle);
}
