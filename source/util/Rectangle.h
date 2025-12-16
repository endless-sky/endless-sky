/* Rectangle.h
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

#pragma once

#include "Point.h"



// Class representing a rectangle.
class Rectangle {
public:
	// Construct a rectangle by specifying the two corners rather than the
	// center and the dimensions. The two corners need not be in any order.
	static Rectangle WithCorners(const Point &from, const Point &to);
	// Construct a rectangle beginning at the given point and having the given
	// dimensions (which are allowed to be negative).
	static Rectangle FromCorner(const Point &corner, const Point &dimensions);

	// Default constructor.
	Rectangle() = default;
	// Constructor, specifying the center and the dimensions.
	Rectangle(const Point &center, const Point &dimensions);
	// Copy constructor.
	Rectangle(const Rectangle &other) = default;

	// Assignment operator.
	Rectangle &operator=(const Rectangle &other) = default;

	// Translation operators.
	Rectangle operator+(const Point &offset) const;
	Rectangle &operator+=(const Point &offset);
	Rectangle operator-(const Point &offset) const;
	Rectangle &operator-=(const Point &offset);

	// Query the attributes of the rectangle.
	Point Center() const;
	Point Dimensions() const;
	double Width() const;
	double Height() const;
	double Left() const;
	double Top() const;
	double Right() const;
	double Bottom() const;
	Point TopLeft() const;
	Point TopRight() const;
	Point BottomLeft() const;
	Point BottomRight() const;

	// Check if a point is inside this rectangle.
	bool Contains(const Point &point) const;
	// Check if the given rectangle is inside this one. If one of its edges is
	// touching the edge of this one, that still counts.
	bool Contains(const Rectangle &other) const;
	// Check if the given rectangle overlaps with this one.
	bool Overlaps(const Rectangle &other) const;
	// Check if the given circle overlaps with this rectangle.
	bool Overlaps(const Point &center, double radius) const;


private:
	Point center;
	Point dimensions;
};
