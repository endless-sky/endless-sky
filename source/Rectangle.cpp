/* Rectangle.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Rectangle.h"

#include <algorithm>

using namespace std;



// Construct a rectangle by specifying the two corners rather than the
// center and the dimensions. The two corners need not be in any order.
Rectangle Rectangle::WithCorners(const Point &from, const Point &to)
{
	// Regardless of which corner is which, the center is always their average
	// and the dimensions are the difference between them (which will be
	// converted to an absolute value in the Rectangle constructor).
	return Rectangle(.5 * (from + to), from - to);
}



// Construct a rectangle beginning at the given point and having the given
// dimensions (which are allowed to be negative).
Rectangle Rectangle::FromCorner(const Point &corner, const Point &dimensions)
{
	// The center is always the corner plus half the dimensions, regardless of
	// which corner is given and what sign the dimensions have.
	return Rectangle(corner + .5 * dimensions, dimensions);
}



// Constructor, specifying the center and the dimensions. Internally, make sure
// that the dimensions are always positive values.
Rectangle::Rectangle(const Point &center, const Point &dimensions)
	: center(center), dimensions(abs(dimensions))
{
}



// Shift this rectangle by the given offset.
Rectangle Rectangle::operator+(const Point &offset) const
{
	return Rectangle(center + offset, dimensions);
}



// Shift this rectangle by the given offset.
Rectangle &Rectangle::operator+=(const Point &offset)
{
	center += offset;
	return *this;
}



// Shift this rectangle by the given offset.
Rectangle Rectangle::operator-(const Point &offset) const
{
	return Rectangle(center - offset, dimensions);
}



// Shift this rectangle by the given offset.
Rectangle &Rectangle::operator-=(const Point &offset)
{
	center -= offset;
	return *this;
}



// Get the center of this rectangle.
Point Rectangle::Center() const
{
	return center;
}



// Get the dimensions, i.e. the full width and height.
Point Rectangle::Dimensions() const
{
	return dimensions;
}



// Get the width of the rectangle.
double Rectangle::Width() const
{
	return dimensions.X();
}



// Get the height of the rectangle.
double Rectangle::Height() const
{
	return dimensions.Y();
}



// Get the minimum X value.
double Rectangle::Left() const
{
	return center.X() - .5 * dimensions.X();
}



// Get the minimum Y value.
double Rectangle::Top() const
{
	return center.Y() - .5 * dimensions.Y();
}



// Get the maximum X value.
double Rectangle::Right() const
{
	return center.X() + .5 * dimensions.X();
}



// Get the maximum Y value.
double Rectangle::Bottom() const
{
	return center.Y() + .5 * dimensions.Y();
}



// Get the top left corner - that is, the minimum x and y.
Point Rectangle::TopLeft() const
{
	return center - .5 * dimensions;
}



// Get the top right conrer - that is, the maximum x and minimum y.
Point Rectangle::TopRight() const
{
	return center + Point(.5, -.5) * dimensions;
}



// Get the bottom left corner - that is, the minimum x and maximum y.
Point Rectangle::BottomLeft() const
{
	return center + Point(-.5, .5) * dimensions;
}



// Get the bottom right corner - that is, the maximum x and y.
Point Rectangle::BottomRight() const
{
	return center + .5 * dimensions;
}



// Check if a point is inside this rectangle.
bool Rectangle::Contains(const Point &point) const
{
	// The point is within the rectangle if its distance to the center is less
	// than half the dimensions.
	Point d = 2. * abs(point - center);
	return (d.X() <= dimensions.X() && d.Y() <= dimensions.Y());
}



// Check if the given rectangle is inside this one. If one of its edges is
// touching the edge of this one, that still counts.
bool Rectangle::Contains(const Rectangle &other) const
{
	return Contains(other.TopLeft()) && Contains(other.BottomRight());
}



bool Rectangle::Overlaps(const Rectangle &other) const
{
	return !(other.Left() > Right() || other.Right() < Left() || other.Top() > Bottom() || other.Bottom() < Top());
}



bool Rectangle::Overlaps(const Point &circle, const double radius) const
{
	// Handle case where circle is entirely within rectangle.
	if(Contains(circle))
		return true;

	const Point closest = Point(max(Left(), min(Right(), circle.X())), max(Top(), min(Bottom(), circle.Y())));
	return (circle - closest).LengthSquared() < radius * radius;
}
