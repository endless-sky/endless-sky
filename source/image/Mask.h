/* Mask.h
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

#include "../Angle.h"
#include "../Point.h"

#include <string>
#include <vector>

class ImageBuffer;



// Class representing the outline of an object, with functions for checking if a
// line segment intersects that object or if a point is within a certain distance.
// The outline is represented in polygonal form, which allows intersection tests
// to be done much more efficiently than if we were testing individual pixels in
// the image itself.
class Mask {
public:
	// Construct a mask from the alpha channel of an RGBA-formatted image.
	void Create(const ImageBuffer &image, int frame, const std::string &fileName);

	// Check whether a mask was successfully generated from the image.
	bool IsLoaded() const;

	// Check if this mask intersects the given line segment (from sA to vA). If
	// it does, return the fraction of the way along the segment where the
	// intersection occurs. The sA should be relative to this object's center,
	// while vA should be relative to sA.
	// If this object contains the given point, the return value is 0. If there
	// is no collision, the return value is 1.
	double Collide(Point sA, Point vA, Angle facing) const;

	// Check whether the mask contains the given point.
	bool Contains(Point point, Angle facing) const;

	// Find out whether this object (rotated and scaled as represented by the given
	// unit vector) is touching a ring defined by the given inner and outer ranges.
	bool WithinRing(Point point, Angle facing, double inner, double outer) const;

	// Find out how close the given point is to the mask.
	double Range(Point point, Angle facing) const;
	// Get the maximum distance from the center of this mask.
	double Radius() const;

	// Get the individual outlines that comprise this mask.
	const std::vector<std::vector<Point>> &Outlines() const;

	// Scale all the points in the mask.
	Mask operator*(Point scale) const;
	friend Mask operator*(Point scale, const Mask &mask);


private:
	double Intersection(Point sA, Point vA) const;
	bool Contains(Point point) const;


private:
	std::vector<std::vector<Point>> outlines;
	double radius = 0.;
};
