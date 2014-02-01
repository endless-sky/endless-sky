/* Point.cpp
Michael Zahniser, 27 Jun 2011

Function definitions for the vectorized Point class.
*/

#include "Point.h"



Point::Point()
	: v(_mm_setzero_pd())
{
}



Point::Point(double x, double y)
	: v(_mm_set_pd(y, x))
{
}



Point::Point(const Point &point)
	: v(point.v)
{
}



Point &Point::operator=(const Point &point)
{
	v = point.v;
	return *this;
}



// Check if the point is anything but (0, 0).
Point::operator bool() const
{
	return !!*this;
}



bool Point::operator!() const
{
	return (!x & !y);
}



Point Point::operator+(const Point &point) const
{
	return Point(v + point.v);
}



Point &Point::operator+=(const Point &point)
{
	v += point.v;
	return *this;
}



Point Point::operator-(const Point &point) const
{
	return Point(v - point.v);
}



Point &Point::operator-=(const Point &point)
{
	v -= point.v;
	return *this;
}



Point Point::operator-() const
{
	return Point() - *this;
}



Point Point::operator*(double scalar) const
{
	return Point(v * _mm_loaddup_pd(&scalar));
}



Point operator*(double scalar, const Point &point)
{
	return Point(point.v * _mm_loaddup_pd(&scalar));
}



Point &Point::operator*=(double scalar)
{
	v *= _mm_loaddup_pd(&scalar);
	return *this;
}



Point Point::operator*(const Point &other) const
{
	Point result;
	result.v = v * other.v;
	return result;
}



Point &Point::operator*=(const Point &other)
{
	v *= other.v;
	return *this;
}



Point Point::operator/(double scalar) const
{
	return Point(v / _mm_loaddup_pd(&scalar));
}



Point &Point::operator/=(double scalar)
{
	v /= _mm_loaddup_pd(&scalar);
	return *this;
}



double &Point::X()
{
	return x;
}



const double &Point::X() const
{
	return x;
}



double &Point::Y()
{
	return y;
}



const double &Point::Y() const
{
	return y;
}



void Point::Set(double x, double y)
{
	v = _mm_set_pd(y, x);
}



// Operations that treat this point as a vector from (0, 0):
double Point::Dot(const Point &point) const
{
	__m128d b = v * point.v;
	b = _mm_hadd_pd(b, b);
	return reinterpret_cast<double &>(b);
}



double Point::Cross(const Point &point) const
{
	__m128d b = _mm_shuffle_pd(point.v, point.v, 0x01);
	b *= v;
	b = _mm_hsub_pd(b, b);
	return reinterpret_cast<double &>(b);
}



double Point::Length() const
{
	__m128d b = v * v;
	b = _mm_hadd_pd(b, b);
	b = _mm_sqrt_pd(b);
	return reinterpret_cast<double &>(b);
}



double Point::LengthSquared() const
{
	return Dot(*this);
}



Point Point::Unit() const
{
	__m128d b = v * v;
	b = _mm_hadd_pd(b, b);
	b = _mm_sqrt_pd(b);
	return Point(v / b);
}



double Point::Distance(const Point &point) const
{
	return (*this - point).Length();
}



double Point::DistanceSquared(const Point &point) const
{
	return (*this - point).LengthSquared();
}



// Private constructor, using a vector.
Point::Point(__m128d v)
	: v(v)
{
}

