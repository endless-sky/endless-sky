/* Angle.h
Michael Zahniser, 21 Oct 2013

Represents an angle that can be converted to a unit vector. Internally the angle
is stored as an integer index into a lookup array, because that is about 7-8
times faster than calling sin() and cos() whenever we want a unit vector.
*/

#ifndef ANGLE_H_INCLUDED
#define ANGLE_H_INCLUDED

#include "Point.h"

#include <cstdint>



class Angle {
public:
	// Return a random angle up to the given amount (between 0 and 360).
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
