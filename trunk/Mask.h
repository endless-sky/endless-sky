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
// line segment intersetcs that object or if a point is within a certain distance.
class Mask {
public:
	// Default constructor.
	Mask();
	
	// Construct a mask from the alpha channel of an SDL surface. (The surface
	// must therefore be a 4-byte RGBA format.)
	void Create(ImageBuffer *image);
	
	// Check whether a mask was successfully loaded.
	bool IsLoaded() const;
	
	// Check if this mask intersects the given line segment (from sA to vA). If
	// it does, return the fraction of the way along the segment where the
	// intersection occurs. The sA should be relative to this object's center.
	// If this object contains the given point, the return value is 0. If there
	// is no collision, the return value is 1.
	double Collide(Point sA, Point vA, Angle facing) const;
	
	// Check whether the given vector intersects this object, and if it does,
	// find the closest point of intersection. The mask is assumed to be
	// rotated and scaled according to the given unit vector. The vector start
	// should be translated so that this object's center is the origin.
	bool Intersects(Point sA, Point vA, Angle facing, Point *result = nullptr) const;
	
	// Check whether the mask contains the given point.
	bool Contains(Point point, Angle facing) const;
	
	// Find out whether this object (rotated and scaled as represented by the
	// given unit vector) 
	bool WithinRange(Point p, Angle facing, double range) const;
	
	
private:
	double Intersection(Point sA, Point vA) const;
	bool Contains(Point point) const;
	
	
private:
	std::vector<Point> outline;
	double radius;
};



#endif
