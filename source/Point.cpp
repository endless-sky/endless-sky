/* Point.cpp
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

#include "Point.h"

#ifndef __SSE3__
#include <algorithm>
#include <cmath>
using namespace std;
#endif



Point::Point() noexcept
#ifdef __SSE3__
	: v(_mm_setzero_pd())
#else
	: x(0.), y(0.)
#endif
{
}



Point::Point(double x, double y) noexcept
#ifdef __SSE3__
	: v(_mm_set_pd(y, x))
#else
	: x(x), y(y)
#endif
{
}



// Check if the point is anything but (0, 0).
Point::operator bool() const noexcept
{
	return !!*this;
}



bool Point::operator!() const noexcept
{
#ifdef __SSE3__
	return (!val.x & !val.y);
#else
	return (!x & !y);
#endif
}



Point Point::operator+(const Point &point) const
{
#ifdef __SSE3__
	return Point(v + point.v);
#else
	return Point(x + point.x, y + point.y);
#endif
}



Point &Point::operator+=(const Point &point)
{
#ifdef __SSE3__
	v += point.v;
#else
	x += point.x;
	y += point.y;
#endif
	return *this;
}



Point Point::operator-(const Point &point) const
{
#ifdef __SSE3__
	return Point(v - point.v);
#else
	return Point(x - point.x, y - point.y);
#endif
}



Point &Point::operator-=(const Point &point)
{
#ifdef __SSE3__
	v -= point.v;
#else
	x -= point.x;
	y -= point.y;
#endif
	return *this;
}



Point Point::operator-() const
{
	return Point() - *this;
}



Point Point::operator*(double scalar) const
{
#ifdef __SSE3__
	return Point(v * _mm_loaddup_pd(&scalar));
#else
	return Point(x * scalar, y * scalar);
#endif
}



Point operator*(double scalar, const Point &point)
{
#ifdef __SSE3__
	return Point(point.v * _mm_loaddup_pd(&scalar));
#else
	return Point(point.x * scalar, point.y * scalar);
#endif
}



Point &Point::operator*=(double scalar)
{
#ifdef __SSE3__
	v *= _mm_loaddup_pd(&scalar);
#else
	x *= scalar;
	y *= scalar;
#endif
	return *this;
}



Point Point::operator*(const Point &other) const
{
#ifdef __SSE3__
	Point result;
	result.v = v * other.v;
	return result;
#else
	return Point(x * other.x, y * other.y);
#endif
}



Point &Point::operator*=(const Point &other)
{
#ifdef __SSE3__
	v *= other.v;
#else
	x *= other.x;
	y *= other.y;
#endif
	return *this;
}



Point Point::operator/(double scalar) const
{
#ifdef __SSE3__
	return Point(v / _mm_loaddup_pd(&scalar));
#else
	return Point(x / scalar, y / scalar);
#endif
}



Point &Point::operator/=(double scalar)
{
#ifdef __SSE3__
	v /= _mm_loaddup_pd(&scalar);
#else
	x /= scalar;
	y /= scalar;
#endif
	return *this;
}



void Point::Set(double x, double y)
{
#ifdef __SSE3__
	v = _mm_set_pd(y, x);
#else
	this->x = x;
	this->y = y;
#endif
}



// Operations that treat this point as a vector from (0, 0):
double Point::Dot(const Point &point) const
{
#ifdef __SSE3__
	__m128d b = v * point.v;
	b = _mm_hadd_pd(b, b);
	return reinterpret_cast<double &>(b);
#else
	return x * point.x + y * point.y;
#endif
}



double Point::Cross(const Point &point) const
{
#ifdef __SSE3__
	__m128d b = _mm_shuffle_pd(point.v, point.v, 0x01);
	b *= v;
	b = _mm_hsub_pd(b, b);
	return reinterpret_cast<double &>(b);
#else
	return x * point.y - y * point.x;
#endif
}



double Point::Length() const
{
#ifdef __SSE3__
	__m128d b = v * v;
	b = _mm_hadd_pd(b, b);
	b = _mm_sqrt_pd(b);
	return reinterpret_cast<double &>(b);
#else
	return sqrt(x * x + y * y);
#endif
}



double Point::LengthSquared() const
{
	return Dot(*this);
}



Point Point::Unit() const
{
#ifdef __SSE3__
	__m128d b = v * v;
	b = _mm_hadd_pd(b, b);
	if(!_mm_cvtsd_f64(b))
		return Point(1., 0.);
	b = _mm_sqrt_pd(b);
	return Point(v / b);
#else
	double b = x * x + y * y;
	if(!b)
		return Point(1., 0.);
	b = 1. / sqrt(b);
	return Point(x * b, y * b);
#endif
}



double Point::Distance(const Point &point) const
{
	return (*this - point).Length();
}



double Point::DistanceSquared(const Point &point) const
{
	return (*this - point).LengthSquared();
}



// Absolute value of both coordinates.
Point abs(const Point &p)
{
#ifdef __SSE3__
	// Absolute value for doubles just involves clearing the sign bit.
	static const __m128d sign_mask = _mm_set1_pd(-0.);
	return Point(_mm_andnot_pd(sign_mask, p.v));
#else
	return Point(abs(p.x), abs(p.y));
#endif
}



// Take the min of the x and y coordinates.
Point min(const Point &p, const Point &q)
{
#ifdef __SSE3__
	return Point(_mm_min_pd(p.v, q.v));
#else
	return Point(min(p.x, q.x), min(p.y, q.y));
#endif
}



// Take the max of the x and y coordinates.
Point max(const Point &p, const Point &q)
{
#ifdef __SSE3__
	return Point(_mm_max_pd(p.v, q.v));
#else
	return Point(max(p.x, q.x), max(p.y, q.y));
#endif
}



#ifdef __SSE3__
// Private constructor, using a vector.
inline Point::Point(const __m128d &v)
	: v(v)
{
}
#endif
