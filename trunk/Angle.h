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



// Represents an angle that can be converted to a unit vector. Internally the angle
// is stored as an integer index into a lookup array, because that is about 7-8
// times faster than calling sin() and cos() whenever we want a unit vector.
class Angle {
public:
	// Return a random angle up to the given amount (between 0 and 360).
	static Angle Random();
	static Angle Random(double range);
	
	
public:
	Angle();
	Angle(double degrees);
	
	Angle operator+(const Angle &other) const;
	Angle &operator+=(const Angle &other);
	Angle operator-(const Angle &other) const;
	Angle &operator-=(const Angle &other);
	Angle operator-() const;
	
	Point Unit() const;
	
	// Return a point rotated by this angle around (0, 0).
	Point Rotate(const Point &point) const;
	
	
private:
	Angle(int32_t angle);
	
	
private:
	int32_t angle;
};

#endif
