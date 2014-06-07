/* Mask.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Mask.h"

#include "SDL/SDL.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Trace out a pixmap.
	void Trace(SDL_Surface *surface, vector<Point> *raw)
	{
		SDL_LockSurface(surface);
		
		uint32_t on = surface->format->Amask;
		const uint32_t *begin = reinterpret_cast<uint32_t *>(surface->pixels);
		
		// Convert the pitch to uint32_ts instead of bytes.
		int pitch = surface->pitch / 4;
		// Skip this many pixels (of padding) in between rows.
		int skip = pitch - surface->w;
		
		// First, find a non-empty pixel.
		// This points tot he current pixel. We will be added the inter-row
		// pitch first thing, so subtract it here:
		const uint32_t *it = begin - skip;
		// This is where we will store the current point:
		Point point;
		
		for(int y = 0; y < surface->h; ++y)
		{
			it += skip;
			for(int x = 0; x < surface->w; ++x)
			{
				// If this pixel is occupied, bail out of both loops.
				if(*it & on)
				{
					point.Set(x, y);
					y = surface->h;
					break;
				}
				++it;
			}
		}
		
		// Now "it" points to the first pixel, whose coordinates are in "point".
		// We will step around the outline in these 8 basic directions:
		static const Point step[8] = {
			{0., -1.}, {1., -1.}, {1., 0.}, {1., 1.},
			{0., 1.}, {-1., 1.}, {-1., 0.}, {-1., -1.}};
		const int off[8] = {
			-pitch, -pitch + 1, 1, pitch + 1,
			pitch, pitch - 1, -1, -pitch - 1};
		int d = 0;
		// All points must be less than this,
		const double maxX = surface->w - .5;
		const double maxY = surface->h - .5;
		
		// Loop until we come back here.
		begin = it;
		do {
			raw->push_back(point);
			
			Point next;
			int firstD = d;
			while(true)
			{
				next = point + step[d];
				// Use padded comparisons in case errors somehow accumulate and
				// the doubles are no longer canceling out to 0.
				if((next.X() >= -.5) & (next.Y() >= -.5) & (next.X() < maxX) & (next.Y() < maxY))
					if(it[off[d]] & on)
						break;
				
				// Advance to the next direction.
				d = (d + 1) & 7;
				// If this point is alone, bail out.
				if(d == firstD)
					return;
			}
			
			point = next;
			it += off[d];
			// Rotate the direction backward ninety degrees.
			d = (d + 6) & 7;
			
			// Loop until we are back where we started.
		} while(it != begin);
		
		SDL_UnlockSurface(surface);
	}
	
	
	void SmoothAndCenter(vector<Point> *raw, Point size)
	{
		// Smooth out the outline by averaging neighboring points.
		Point prev = raw->back();
		for(Point &p : *raw)
		{
			prev += p;
			prev -= size;
			// Since we'll always be using these sprites at 50% scale, do that
			// scaling here. Also rotate the mask 180 degrees.
			prev *= -.25;
			swap(prev, p);
		}
	}
	
	
	// Distance from a point to a line, squared.
	double Distance(Point p, Point a, Point b)
	{
		// Convert to a coordinate system where a is the origin.
		p -= a;
		b -= a;
		double length = b.LengthSquared();
		if(length)
		{
			// Find out how far along the line the tangent to p intersects.
			double u = b.Dot(p) / length;
			// If it is beyond one of the endpoints, use that endpoint.
			p -= max(0., min(1., u)) * b;
		}
		return p.LengthSquared();
	}
	
	
	void Simplify(const vector<Point> &p, int first, int last, vector<Point> *result)
	{
		// Find the most divergent point.
		double dmax = 0.;
		int imax = 0;
		
		for(int i = first + 1; true; ++i)
		{
			if(static_cast<unsigned>(i) == p.size())
				i = 0;
			if(i == last)
				break;
			
			double d = Distance(p[i], p[first], p[last]);
			// Enforce symmetry by using y position as a tiebreaker rather than
			// just the order in the list.
			if(d > dmax || (d == dmax && p[i].Y() > p[imax].Y()))
			{
				dmax = d;
				imax = i;
			}
		}
		
		// If the most divergent point is close enough to the outline, stop.
		if(dmax < 1.)
			return;
		
		// Recursively simplify the lines to both sides of that point.
		Simplify(p, first, imax, result);
	
		result->push_back(p[imax]);
	
		Simplify(p, imax, last, result);
	}
	
	
	// Simplify the given outline using the Ramer-Douglas-Peucker algorithm.
	void Simplify(const vector<Point> &raw, vector<Point> *result)
	{
		result->clear();
		
		// The image has been scaled to 50% size, so the raw outline must have
		// vertices every half-pixel. Find all vertices with X coordinates
		// within a quarter-pixel of 0, and of those, select the top-most and
		// bottom-most ones.
		int top = -1;
		int bottom = -1;
		for(int i = 0; static_cast<unsigned>(i) < raw.size(); ++i)
			if(raw[i].X() >= -.25 && raw[i].X() < .25)
			{
				if(top == -1)
					top = bottom = i;
				else if(raw[i].Y() > raw[bottom].Y())
					bottom = i;
				else
					top = i;
			}
		
		// Bail out if we couldn't find top and bottom vertices.
		if(top == bottom)
			return;
		
		result->push_back(raw[top]);
		Simplify(raw, top, bottom, result);
		result->push_back(raw[bottom]);
		Simplify(raw, bottom, top, result);
	}
	
	
	// Find the radius of the object.
	double Radius(const vector<Point> &outline)
	{
		double radius = 0.;
		for(const Point &p : outline)
			radius = max(radius, p.LengthSquared());
		return sqrt(radius);
	}
}



// Default constructor.
Mask::Mask()
	: radius(0.)
{
}



// Construct a mask from the alpha channel of an SDL surface. (The surface
// must therefore be a 4-byte RGBA format.)
void Mask::Create(SDL_Surface *surface)
{
	if(surface->format->BytesPerPixel != 4)
		return;
	
	vector<Point> raw;
	Trace(surface, &raw);
	
	SmoothAndCenter(&raw, Point(surface->w, surface->h));
	
	Simplify(raw, &outline);
	
	radius = Radius(outline);
}



// Check whether a mask was successfully loaded.
bool Mask::IsLoaded() const
{
	return !outline.empty();
}



// Check if this mask intersects the given line segment (from sA to vA). If
// it does, return the fraction of the way along the segment where the
// intersection occurs. The sA should be relative to this object's center.
// If this object contains the given point, the return value is 0. If there
// is no collision, the return value is 1.
double Mask::Collide(Point sA, Point vA, Angle facing) const
{
	// Bail out if we're too far away to possibly be touching.
	double distance = sA.Length();
	if(outline.empty() || distance > radius + vA.Length())
		return 1.;
	
	// Rotate into the mask's frame of reference.
	sA = (-facing).Rotate(sA);
	vA = (-facing).Rotate(vA);
	
	// If this point is contained within the mask, a ray drawn out from it will
	// intersect the mask an even number of times. If that ray coincides with an
	// edge, ignore that edge, and count all segments as closed at the start and
	// open at the end to avoid double-counting.
	
	// For simplicity, use a ray pointing straight downwards. A segment then
	// intersects only if its x coordinates span the point's coordinates.
	if(distance <= radius && Contains(sA))
		return 0.;
	
	return Intersection(sA, vA);
}



// Check whether the given vector intersects this object, and if it does,
// find the closest point of intersection. The mask is assumed to be
// rotated and scaled according to the given unit vector. The vector start
// should be translated so that this object's center is the origin.
bool Mask::Intersects(Point sA, Point vA, Angle facing, Point *result) const
{
	// Bail out if we're too far away to possibly be touching.
	if(outline.empty() || sA.Length() > radius + vA.Length())
		return false;
	
	// Rotate into the mask's frame of reference.
	sA = (-facing).Rotate(sA);
	vA = (-facing).Rotate(vA);
	
	// Keep track of the closest intersection point found.
	double closest = Intersection(sA, vA);
	if(closest == 1.)
		return false;
	
	if(result)
		*result = facing.Rotate(sA + closest * vA);
	return true;
}



// Check whether the mask contains the given point.
bool Mask::Contains(Point point, Angle facing) const
{
	if(outline.empty() || point.Length() > radius)
		return false;
	
	// Rotate into the mask's frame of reference.
	return Contains((-facing).Rotate(point));
}



// Find out how close this mask is to the given point. Again, the mask is
// assumed to be rotated and scaled according to the given unit vector.
bool Mask::WithinRange(Point point, Angle facing, double range) const
{
	// Bail out if the object is too far away to possible be touched.
	if(outline.empty() || range < point.Length() - radius)
		return false;
	
	// Rotate into the mask's frame of reference.
	point = (-facing).Rotate(point);
	// For efficiency, compare to range^2 instead of range.
	range *= range;
	
	for(const Point &p : outline)
		if(p.DistanceSquared(point) < range)
			return true;
	
	return false;
}



double Mask::Intersection(Point sA, Point vA) const
{
	// Keep track of the closest intersection point found.
	double closest = 1.;
	
	Point prev = outline.back();
	for(const Point &next : outline)
	{
		// Check if there is an intersection. (If not, the cross would be 0.) If
		// there is, handle it only if it is a point where the segment is
		// entering the polygon rather than exiting it (i.e. cross > 0).
		Point vB = next - prev;
		double cross = vB.Cross(vA);
		if(cross > 0.)
		{
			Point vS = prev - sA;
			double uB = vA.Cross(vS);
			double uA = vB.Cross(vS);
			// If the intersection occurs somewhere within this segment of the
			// outline, find out how far along the query vector it occurs and
			// remember it if it is the closest so far.
			if((uB >= 0.) & (uB < cross) & (uA >= 0.))
				closest = min(closest, uA / cross);
		}
		
		prev = next;
	}
	return closest;
}



bool Mask::Contains(Point point) const
{
	// If this point is contained within the mask, a ray drawn out from it will
	// intersect the mask an even number of times. If that ray coincides with an
	// edge, ignore that edge, and count all segments as closed at the start and
	// open at the end to avoid double-counting.
	
	// For simplicity, use a ray pointing straight downwards. A segment then
	// intersects only if its x coordinates span the point's coordinates.
	int intersections = 0;
	Point prev = outline.back();
	for(const Point &next : outline)
	{
		if(prev.X() != next.X())
			if((prev.X() <= point.X()) == (point.X() < next.X()))
			{
				double y = prev.Y() + (next.Y() - prev.Y()) *
					(point.X() - prev.X()) / (next.X() - prev.X());
				intersections += (y >= point.Y());
			}
		prev = next;
	}
	// If the number of intersections is odd, the point is within the mask.
	return (intersections & 1);
}
