/* Angle.cpp
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
	constexpr int32_t STEPS = 0x10000;
	constexpr int32_t MASK = STEPS - 1;
	constexpr double DEG_TO_STEP = STEPS / 360.;
	constexpr double STEP_TO_RAD = PI / (STEPS / 2);

	vector<Point> InitUnitCache()
	{
		vector<Point> cache;
		cache.reserve(STEPS);
		for(int i = 0; i < STEPS; ++i)
		{
			const double radians = i * STEP_TO_RAD;
			// The graphics use the usual screen coordinate system, meaning that
			// positive Y is down rather than up. Angles are clock angles, i.e.
			// 0 is 12:00 and angles increase in the clockwise direction. So, an
			// angle of 0 degrees is pointing in the direction (0, -1).
			cache.emplace_back(sin(radians), -cos(radians));
		}
		return cache;
	}

	const vector<Point> unitCache = InitUnitCache();
}



// Get a random angle.
Angle Angle::Random()
{
	return Angle(static_cast<int32_t>(Random::Int(STEPS)));
}



// Get a random angle between 0 and the given number of degrees.
Angle Angle::Random(const double range)
{
	// The given range would have to be about 22.6 million degrees to overflow
	// the size of a 32-bit int, which should never happen in normal usage.
	const uint32_t mod = static_cast<uint32_t>(fabs(range) * DEG_TO_STEP) + 1;
	return Angle(mod ? static_cast<int32_t>(Random::Int(mod)) & MASK : 0);
}



// Construct an Angle from the given angle in degrees.
Angle::Angle(const double degrees) noexcept
	: angle(llround(degrees * DEG_TO_STEP) & MASK)
{
	// Make sure llround does not overflow with the values of System::SetDate.
	// "now" has 32 bit integer precision. "speed" and "offset" have floating
	// point precision and should be in the range from -360 to 360.
	static_assert(sizeof(long long) >= 8, "llround can overflow");
}



// Construct an angle pointing in the direction of the given vector.
Angle::Angle(const Point &point) noexcept
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



bool Angle::operator==(const Angle &other) const
{
	return angle == other.angle;
}



bool Angle::operator!=(const Angle &other) const
{
	return angle != other.angle;
}



// Get a unit vector in the direction of this angle.
Point Angle::Unit() const
{
	return unitCache[angle];
}



// Convert an angle back to a value in degrees.
double Angle::Degrees() const
{
	// Most often when this function is used, it's in settings where it makes
	// sense to return an angle in the range [-180, 180) rather than in the
	// Angle's native range of [0, 360).
	return angle / DEG_TO_STEP - 360. * (angle >= STEPS / 2);
}



// Convert an Angle object to degrees, in the range 0 to 360.
double Angle::AbsDegrees() const
{
	return angle / DEG_TO_STEP;
}



// Return a point rotated by this angle around (0, 0).
Point Angle::Rotate(const Point &point) const
{
	// If using the normal mathematical coordinate system, this would be easier.
	// Since we're not, the math is a tiny bit less elegant:
	const Point unit = Unit();
	return Point(-unit.Y() * point.X() - unit.X() * point.Y(),
		-unit.Y() * point.Y() + unit.X() * point.X());
}



// Judge whether this is inside from "base" to "limit."
// The range from "base" to "limit" is expressed by "clock" orientation.
bool Angle::IsInRange(const Angle &base, const Angle &limit) const
{
	// Choose an edge of the arc as the reference angle (base) and
	// compare relative angles to decide whether this is in the range.
	Angle normalizedLimit = limit - base;
	Angle normalizedTarget = *this - base;
	return normalizedTarget.angle <= normalizedLimit.angle;
}



// Constructor using Angle's internal representation.
Angle::Angle(const int32_t angle)
	: angle(angle)
{
}
