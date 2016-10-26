/* Angle.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Angle.h"

#include "pi.h"
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

using namespace std;

namespace {
	// Suppose you want to be able to turn 360 degrees in one second. Then you are
	// turning 6 degrees per time step. If the Angle lookup is 2^16 steps, then 6
	// degrees is 1092 steps, and your turn speed is accurate to +- 0.05%. That seems
	// plenty accurate to me. At that step size, the lookup table is exactly 1 MB.
	static const int32_t STEPS = 0x10000;
	static const int32_t MASK = STEPS - 1;
	double DEG_TO_STEP = STEPS / 360.;
	double STEP_TO_RAD = PI / (STEPS / 2);
}



Angle Angle::Random()
{
	return Angle(static_cast<int32_t>(Random::Int(STEPS)));
}




Angle Angle::Random(double range)
{
	int64_t steps = fabs(range) * DEG_TO_STEP + .5;
	int32_t mod = min(steps, int64_t(STEPS - 1)) + 1;
	
	return Angle(static_cast<int32_t>(Random::Int(mod)));
}



Angle::Angle()
	: angle(0)
{
}



Angle::Angle(double degrees)
	: angle(static_cast<int64_t>(degrees * DEG_TO_STEP + .5) & MASK)
{
}



Angle::Angle(const Point &point)
	: Angle(TO_DEG * atan2(point.X(), -point.Y()))
{
}



Angle Angle::operator+(const Angle &other) const
{
	Angle result = *this;
	result += other;
	return result;
}



Angle &Angle::operator+=(const Angle &other)
{
	angle += other.angle;
	angle &= MASK;
	return *this;
}



Angle Angle::operator-(const Angle &other) const
{
	Angle result = *this;
	result -= other;
	return result;
}



Angle &Angle::operator-=(const Angle &other)
{
	angle -= other.angle;
	angle &= MASK;
	return *this;
}



Angle Angle::operator-() const
{
	return Angle((-angle) & MASK);
}



Point Angle::Unit() const
{
	static vector<Point> cache;
	if(cache.empty())
	{
		cache.reserve(STEPS);
		for(int i = 0; i < STEPS; ++i)
		{
			double radians = i * STEP_TO_RAD;
			cache.emplace_back(sin(radians), -cos(radians));
		}
	}
	return cache[angle];
}



double Angle::Degrees() const
{
	return angle / DEG_TO_STEP - 360. * (angle >= STEPS / 2);
}


	
// Return a point rotated by this angle around (0, 0).
Point Angle::Rotate(const Point &point) const
{
	if(!point)
		return point;
	
	Point unit = Unit();
	unit.Set(-unit.Y() * point.X() - unit.X() * point.Y(),
		-unit.Y() * point.Y() + unit.X() * point.X());
	return unit;
}



Angle::Angle(int32_t angle)
	: angle(angle)
{
}
