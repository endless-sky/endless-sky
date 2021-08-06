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
	const int32_t STEPS = 0x10000;
	const int32_t MASK = STEPS - 1;
	const double DEG_TO_STEP = STEPS / 360.;
	const double STEP_TO_RAD = PI / (STEPS / 2);
}



// Get a random angle.
Angle Angle::Random()
{
	return Angle(static_cast<int32_t>(Random::Int(STEPS)));
}



// Get a random angle between 0 and the given number of degrees.
Angle Angle::Random(double range)
{
	// The given range would have to be about 22.6 million degrees to overflow
	// the size of a 32-bit int, which should never happen in normal usage.
	uint32_t mod = static_cast<uint32_t>(fabs(range) * DEG_TO_STEP) + 1;
	return Angle(mod ? static_cast<int32_t>(Random::Int(mod)) : 0);
}



// Default constructor: generates an angle pointing straight up.
Angle::Angle()
	: angle(0)
{
}



// Construct an Angle from the given angle in degrees.
Angle::Angle(double degrees)
	: angle(llround(degrees * DEG_TO_STEP) & MASK)
{
	// Make sure llround does not overflow with the values of System::SetDate.
	// "now" has 32 bit integer precision. "speed" and "offset" have floating
	// point precision and should be in the range from -360 to 360.
	static_assert(sizeof(long long) >= 8, "llround can overflow");
}



// Construct an angle pointing in the direction of the given vector.
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



// Get a unit vector in the direction of this angle.
Point Angle::Unit() const
{
	// The very first time this is called, create a lookup table of unit vectors.
	static vector<Point> cache;
	if(cache.empty())
	{
		cache.reserve(STEPS);
		for(int i = 0; i < STEPS; ++i)
		{
			double radians = i * STEP_TO_RAD;
			// The graphics use the usual screen coordinate system, meaning that
			// positive Y is down rather than up. Angles are clock angles, i.e.
			// 0 is 12:00 and angles increase in the clockwise direction. So, an
			// angle of 0 degrees is pointing in the direction (0, -1).
			cache.emplace_back(sin(radians), -cos(radians));
		}
	}
	return cache[angle];
}



// Convert an angle back to a value in degrees.
double Angle::Degrees() const
{
	// Most often when this function is used, it's in settings where it makes
	// sense to return an angle in the range [-180, 180) rather than in the
	// Angle's native range of [0, 360).
	return angle / DEG_TO_STEP - 360. * (angle >= STEPS / 2);
}


	
// Return a point rotated by this angle around (0, 0).
Point Angle::Rotate(const Point &point) const
{
	// If using the normal mathematical coordinate system, this would be easier.
	// Since we're not, the math is a tiny bit less elegant:
	Point unit = Unit();
	return Point(-unit.Y() * point.X() - unit.X() * point.Y(),
		-unit.Y() * point.Y() + unit.X() * point.X());
}



// Constructor using Angle's internal representation.
Angle::Angle(int32_t angle)
	: angle(angle)
{
}
