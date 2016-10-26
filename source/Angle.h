/* Angle.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ANGLE_H_
#define ANGLE_H_

#include "Point.h"

#include <cstdint>



// Represents an angle, in degrees. Angles are in "clock" orientation rather
// than usual mathematical orientation. That is, 0 degrees is up, and angles
// increase in a clockwise direction. Angles can be efficiently mapped to unit
// vectors, which also makes rotating a vector an efficient operation.
class Angle {
public:
	// Return a random angle up to the given amount (between 0 and 360).
	static Angle Random();
	static Angle Random(double range);
	
	
public:
	Angle();
	Angle(double degrees);
	Angle(const Point &point);
	
	Angle operator+(const Angle &other) const;
	Angle &operator+=(const Angle &other);
	Angle operator-(const Angle &other) const;
	Angle &operator-=(const Angle &other);
	Angle operator-() const;
	
	Point Unit() const;
	// Convert an Angle object to degrees, in the range -180 to 180.
	double Degrees() const;
	
	// Return a point rotated by this angle around (0, 0).
	Point Rotate(const Point &point) const;
	
	
private:
	Angle(int32_t angle);
	
	
private:
	int32_t angle;
};

#endif
