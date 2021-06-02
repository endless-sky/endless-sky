/* Mask.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MASK_H_
#define MASK_H_

#include "Angle.h"
#include "Point.h"

#include <vector>

class ImageBuffer;



// Class representing the outline of an object, with functions for checking if a
// line segment intersects that object or if a point is within a certain distance.
// The outline is represented in polygonal form, which allows intersection tests
// to be done much more efficiently than if we were testing individual pixels in
// the image itself.
class Mask {
public:
	// Default constructor.
	Mask();
	
	// Construct a mask from the alpha channel of an image.
	void Create(const ImageBuffer &image, int frame = 0);
	
	// Check whether a mask was successfully loaded.
	bool IsLoaded() const;
	
	// Check if this mask intersects the given line segment (from sA to vA). If
	// it does, return the fraction of the way along the segment where the
	// intersection occurs. The sA should be relative to this object's center.
	// If this object contains the given point, the return value is 0. If there
	// is no collision, the return value is 1.
	double Collide(Point sA, Point vA, Angle facing) const;
	
	// Check whether the mask contains the given point.
	bool Contains(Point point, Angle facing) const;
	
	// Find out whether this object (rotated and scaled as represented by the given
	// unit vector) is touching a ring defined by the given inner and outer ranges.
	bool WithinRing(Point point, Angle facing, double inner, double outter) const;
	
	// Find out how close the given point is to the mask.
	double Range(Point point, Angle facing) const;
	// Get the maximum distance from the center of this mask.
	double Radius() const;
	
	// Get the list of points in the outline.
	const std::vector<Point> &Points() const;
	
	
private:
	double Intersection(Point sA, Point vA) const;
	bool Contains(Point point) const;
	
	
private:
	std::vector<Point> outline;
	double radius;
};



#endif
