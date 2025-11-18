/* Point.h
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

#ifdef __SSE3__
#include <pmmintrin.h>
#elif defined(__SSE2__)
#include <xmmintrin.h>
#endif



// Class representing a 2D point with functions for a variety of vector operations.
// A Point can represent either a location or a vector (e.g. a velocity, or a
// distance between two points, or a unit vector representing a direction). All
// basic mathematical operations that make sense for vectors are supported.
// Internally the coordinates are stored in a SSE vector and the processor's vector
// extensions are used to optimize all operations.
class Point {
public:
	Point() noexcept;
	Point(double x, double y) noexcept;

	// Check if the point is anything but (0, 0).
	explicit operator bool() const noexcept;
	bool operator!() const noexcept;

	bool operator==(const Point &other) const noexcept;
	bool operator!=(const Point &other) const noexcept;

	Point operator+(const Point &point) const;
	Point &operator+=(const Point &point);
	Point operator-(const Point &point) const;
	Point &operator-=(const Point &point);
	Point operator-() const;

	Point operator*(double scalar) const;
	friend Point operator*(double scalar, const Point &point);
	Point &operator*=(double scalar);
	Point operator/(double scalar) const;
	Point &operator/=(double scalar);

	// Multiply the respective components of each Point.
	Point operator*(const Point &other) const;
	Point &operator*=(const Point &other);

	double &X();
	const double &X() const noexcept;
	double &Y();
	const double &Y() const noexcept;

	void Set(double x, double y);

	// Operations that treat this point as a vector from (0, 0):
	double Dot(const Point &point) const;
	double Cross(const Point &point) const;

	double Length() const;
	double LengthSquared() const;
	Point Unit() const;

	double Distance(const Point &point) const;
	double DistanceSquared(const Point &point) const;

	Point Lerp(const Point &to, const double c) const;

	// Take the absolute value of both coordinates.
	friend Point abs(const Point &p);
	// Use the min of each x and each y coordinates.
	friend Point min(const Point &p, const Point &q);
	// Use the max of each x and each y coordinates.
	friend Point max(const Point &p, const Point &q);


private:
#ifdef __SSE2__
	// Private constructor, using a vector.
	explicit Point(const __m128d &v);


private:
	struct PointInternal {
		double x;
		double y;
	};
	union {
		__m128d v;
		PointInternal val;
	};
#else
	double x;
	double y;
#endif
};



// Inline accessor functions, for speed:
inline double &Point::X()
{
#ifdef __SSE2__
	return val.x;
#else
	return x;
#endif
}



inline const double &Point::X() const noexcept
{
#ifdef __SSE2__
	return val.x;
#else
	return x;
#endif
}



inline double &Point::Y()
{
#ifdef __SSE2__
	return val.y;
#else
	return y;
#endif
}



inline const double &Point::Y() const noexcept
{
#ifdef __SSE2__
	return val.y;
#else
	return y;
#endif
}
