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

#include "ImageBuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace {
	// Trace out a pixmap.
	void Trace(const ImageBuffer &image, int frame, vector<Point> *raw)
	{
		uint32_t on = 0xFF000000;
		const uint32_t *begin = image.Pixels() + frame * image.Width() * image.Height();
		
		// Convert the pitch to uint32_ts instead of bytes.
		int pitch = image.Width();
		
		// First, find a non-empty pixel.
		// This points to the current pixel.
		const uint32_t *it = begin;
		// This is where we will store the point:
		Point point;
		
		for(int y = 0; y < image.Height(); ++y)
			for(int x = 0; x < image.Width(); ++x)
			{
				// If this pixel is occupied, bail out of both loops.
				if(*it & on)
				{
					point.Set(x, y);
					// Break out of both loops.
					y = image.Height();
					break;
				}
				++it;
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
		const double maxX = image.Width() - .5;
		const double maxY = image.Height() - .5;
		
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
			// scaling here.
			prev *= .25;
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
		
		// Out of all the top-most and bottom-most pixels, find the ones that
		// are closest to the center of the image.
		int top = -1;
		int bottom = -1;
		for(int i = 0; static_cast<unsigned>(i) < raw.size(); ++i)
		{
			double ax = fabs(raw[i].X());
			double y = raw[i].Y();
			if(top == -1)
				top = bottom = i;
			else if(y > raw[bottom].Y() || (y == raw[bottom].Y() && ax < fabs(raw[bottom].X())))
				bottom = i;
			else if(y < raw[top].Y() || (y == raw[top].Y() && ax < fabs(raw[top].X())))
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
	double ComputeRadius(const vector<Point> &outline)
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
void Mask::Create(const ImageBuffer &image, int frame)
{
	vector<Point> raw;
	Trace(image, frame, &raw);
	
	SmoothAndCenter(&raw, Point(image.Width(), image.Height()));
	
	Simplify(raw, &outline);
	
	radius = ComputeRadius(outline);
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



// Check whether the mask contains the given point.
bool Mask::Contains(Point point, Angle facing) const
{
	if(outline.empty() || point.Length() > radius)
		return false;
	
	// Rotate into the mask's frame of reference.
	return Contains((-facing).Rotate(point));
}



// Find out whether this object is touching a ring defined by the given
// inner and outer ranges.
bool Mask::WithinRing(Point point, Angle facing, double inner, double outer) const
{
	// Bail out if the object is too far away to possibly be touched.
	if(outline.empty() || inner > point.Length() + radius || outer < point.Length() - radius)
		return false;
	
	// Rotate into the mask's frame of reference.
	point = (-facing).Rotate(point);
	// For efficiency, compare to range^2 instead of range.
	inner *= inner;
	outer *= outer;
	
	for(const Point &p : outline)
	{
		double pSquared = p.DistanceSquared(point);
		if(pSquared < outer && pSquared > inner)
			return true;
	}
	
	return false;
}



// Find out how close the given point is to the mask.
double Mask::Range(Point point, Angle facing) const
{
	double range = numeric_limits<double>::infinity();
	if(outline.empty())
		return range;
	
	// Rotate into the mask's frame of reference.
	point = (-facing).Rotate(point);
	if(Contains(point))
		return 0.;
	
	for(const Point &p : outline)
		range = min(range, p.Distance(point));
	
	return range;
}



double Mask::Radius() const
{
	return radius;
}



// Get the list of points in the outline.
const vector<Point> &Mask::Points() const
{
	return outline;
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
