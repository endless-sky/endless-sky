/* Mask.h
Michael Zahniser, 16 Jan 2014

Class representing the outline of an object, with functions for checking if a
line segment intersetcs that object or if a point is within a certain distance.
*/

#ifndef MASK_H_INCLUDED
#define MASK_H_INCLUDED

#include "Angle.h"
#include "Point.h"

#include <vector>

class SDL_Surface;



class Mask {
public:
	// Default constructor.
	Mask();
	
	// Construct a mask from the alpha channel of an SDL surface. (The surface
	// must therefore be a 4-byte RGBA format.)
	void Create(SDL_Surface *surface);
	
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
	std::vector<Point> outline;
	double radius;
};



#endif
