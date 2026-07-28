/* Mask.cpp
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

#include "Mask.h"

#include "ImageBuffer.h"
#include "../Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace {
	// Trace out outlines from an image frame.
	void Trace(const ImageBuffer &image, int frame, vector<vector<Point>> &raw, const string &fileName)
	{
		const uint32_t on = 0xFF000000;
		const int width = image.Width();
		const int height = image.Height();
		const int numPixels = width * height;
		const uint32_t *begin = image.Pixels() + frame * numPixels;
		auto LogError = [width, height, fileName](string reason)
		{
			Logger::Log("Unable to create mask for " + to_string(width) + "x" + to_string(height)
				+ " px image " + fileName + ": " + std::move(reason), Logger::Level::WARNING);
		};
		raw.clear();

		auto hasOutline = vector<bool>(numPixels, false);
		vector<int> directions;
		vector<Point> points;
		int start = 0;
		while(start < numPixels)
		{
			directions.clear();
			points.clear();

			// Find a pixel with some renderable color data (i.e. a non-zero alpha component).
			for( ; start < numPixels; ++start)
			{
				if(begin[start] & on)
				{
					// If this pixel is not part of an existing outline, trace it.
					if(!hasOutline[start])
						break;
					// Otherwise, advance to the next transparent pixel.
					// (any non-transparent pixels will belong to the existing outline).
					for(++start; start < numPixels; ++start)
						if(!(begin[start] & on))
							break;
				}
			}
			if(start >= numPixels)
			{
				if(raw.empty())
					LogError("all pixels were transparent!");
				return;
			}

			// Direction kernel for obtaining the 8 nearest neighbors, beginning with "N" and
			// moving clockwise (since the frame data starts in the top-left and moves L->R).
			static const int step[][2] = {
				{0, -1}, { 1, -1}, { 1, 0}, { 1,  1},
				{0,  1}, {-1,  1}, {-1, 0}, {-1, -1},
			};
			// Convert from a direction index to the desired pixel.
			const int off[] = {
				-width, -width + 1,  1,  width + 1,
				 width,  width - 1, -1, -width - 1,
			};

			// Loop until we come back to the start, recording the directions
			// that outline each pixel (rather than the actual pixel itself).
			int d = 7;
			// The current image pixel, in index coordinates.
			int pos = start;
			// The current image pixel, in (X, Y) coordinates.
			int p[] = {pos % width, pos / width};
			do {
				hasOutline[pos] = true;
				int firstD = d;
				// The image pixel being inspected, in XY coords.
				int next[] = {p[0], p[1]};
				bool isAlone = false;
				while(true)
				{
					next[0] = p[0] + step[d][0];
					next[1] = p[1] + step[d][1];
					// First, ensure an offset in this direction would access a valid pixel index.
					if(next[0] >= 0 && next[0] < width && next[1] >= 0 && next[1] < height)
						// If that pixel has color data, then add it to the outline.
						if(begin[pos + off[d]] & on)
							break;

					// Otherwise, advance to the next direction.
					d = (d + 1) & 7;
					// If this point is alone, bail out.
					if(d == firstD)
					{
						isAlone = true;
						LogError("lone point found at (" + to_string(p[0]) + ", " + to_string(p[1]) + ")");
						break;
					}
				}
				if(isAlone)
					break;

				// Advance the pixels and store the direction traveled.
				p[0] = next[0];
				p[1] = next[1];
				pos += off[d];
				directions.push_back(d);

				// Rotate the direction backward ninety degrees.
				d = (d + 6) & 7;

				// Loop until we are back where we started.
			} while(pos != start);

			// At least 4 points are needed to outline a non-transparent pixel.
			if(directions.size() < 4)
				continue;


			// Interpolate outline points from directions and alpha values, rather than just the pixel's XY.
			points.reserve(directions.size());
			pos = start;
			p[0] = pos % width;
			p[1] = pos / width;
			int prev = directions.back();
			for(int next : directions)
			{
				// Face outside by rotating direction backward ninety degrees.
				int out0 = (prev + 6) & 7;
				int out1 = (next + 6) & 7;

				// Determine the subpixel shift, where higher alphas will shift the estimate outward.
				// (MAYBE: use an actual alpha gradient for dir & magnitude, or remove altogether.)
				static const double scale[] = { 1., 1. / sqrt(2.) };
				Point shift = Point(
					step[out0][0] * scale[out0 & 1] + step[out1][0] * scale[out1 & 1],
					step[out0][1] * scale[out0 & 1] + step[out1][1] * scale[out1 & 1]).Unit();
				shift *= ((begin[pos] & on) >> 24) * (1. / 255.) - .5;
				points.push_back(shift + Point(p[0], p[1]));

				p[0] += step[next][0];
				p[1] += step[next][1];
				pos += off[next];
				prev = next;
			}
			raw.push_back(points);
		}
	}


	void SmoothAndCenter(vector<Point> &raw, Point size)
	{
		// Smooth out the outline by averaging neighboring points.
		Point prev = raw.back();
		for(Point &p : raw)
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
	double DistanceSquared(Point p, Point a, Point b)
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


	void Simplify(const vector<Point> &p, int first, int last, vector<Point> &result)
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

			double d = DistanceSquared(p[i], p[first], p[last]);
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

		result.push_back(p[imax]);

		Simplify(p, imax, last, result);
	}


	// Simplify the given outline using the Ramer-Douglas-Peucker algorithm.
	vector<Point> Simplify(const vector<Point> &raw)
	{
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

		auto result = vector<Point>{};
		if(top != bottom)
		{
			result.push_back(raw[top]);
			Simplify(raw, top, bottom, result);
			result.push_back(raw[bottom]);
			Simplify(raw, bottom, top, result);
		}
		return result;
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



// Construct a mask from the alpha channel of an RGBA-formatted image.
void Mask::Create(const ImageBuffer &image, int frame, const string &fileName)
{
	outlines.clear();
	radius = 0.;

	vector<vector<Point>> raw;
	Trace(image, frame, raw, fileName);
	if(raw.empty())
		return;

	outlines.reserve(raw.size());
	for(auto &edge : raw)
	{
		SmoothAndCenter(edge, Point(image.Width(), image.Height()));

		auto outline = Simplify(edge);
		// Skip any outlines that have no area.
		if(outline.size() <= 2)
			continue;

		radius = max(radius, ComputeRadius(outline));
		outlines.push_back(std::move(outline));
		outlines.back().shrink_to_fit();
	}
	outlines.shrink_to_fit();
}



// Check whether a mask was successfully generated from the image.
bool Mask::IsLoaded() const
{
	return !outlines.empty();
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
	if(!IsLoaded() || distance > radius + vA.Length())
		return 1.;

	// Bail out even if the segment doesn't touch a circle of 'radius'.
	if(DistanceSquared(Point(), sA, sA + vA) > (radius * radius))
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
	if(!IsLoaded() || point.Length() > radius)
		return false;

	// Rotate into the mask's frame of reference.
	return Contains((-facing).Rotate(point));
}



// Find out whether this object is touching a ring defined by the given
// inner and outer ranges.
bool Mask::WithinRing(Point point, Angle facing, double inner, double outer) const
{
	// Bail out if the object is too far away to possibly be touched.
	if(!IsLoaded() || inner > point.Length() + radius || outer < point.Length() - radius)
		return false;

	// Rotate into the mask's frame of reference.
	point = (-facing).Rotate(point);
	// For efficiency, compare to range^2 instead of range.
	inner *= inner;
	outer *= outer;

	// Determine if the ring contains any of the outlines of the mask.
	for(auto &&outline : outlines)
		for(auto &&p : outline)
		{
			double pSquared = p.DistanceSquared(point);
			if(pSquared < outer && pSquared > inner)
				return true;
		}

	// While a ring might not contain any outlines of the mask, it may be
	// located entirely inside of the mask. This should still count as the
	// mask being within the ring. This can only be the case if the
	// entire ring is smaller than the radius of the mask and the center
	// of the ring is within the mask.
	return outer < radius && Contains(point);
}



// Find out how close the given point is to the mask.
double Mask::Range(Point point, Angle facing) const
{
	double range = numeric_limits<double>::infinity();
	if(!IsLoaded())
		return range;

	// Rotate into the mask's frame of reference.
	point = (-facing).Rotate(point);
	if(Contains(point))
		return 0.;

	for(auto &&outline : outlines)
		for(auto &&p : outline)
			range = min(range, p.Distance(point));

	return range;
}



double Mask::Radius() const
{
	return radius;
}



// Get the individual outlines that comprise this mask.
const vector<vector<Point>> &Mask::Outlines() const
{
	return outlines;
}



Mask Mask::operator*(Point scale) const
{
	Mask newMask = *this;
	newMask.radius = 0.;
	for(auto &outline : newMask.outlines)
	{
		for(Point &p : outline)
			p *= scale;
		double radius = ComputeRadius(outline);
		if(radius > newMask.radius)
			newMask.radius = radius;
	}
	return newMask;
}



Mask operator*(Point scale, const Mask &mask)
{
	return mask * scale;
}



double Mask::Intersection(Point sA, Point vA) const
{
	// Keep track of the closest intersection point found.
	double closest = 1.;

	for(auto &&outline : outlines)
	{
		Point prev = outline.back();
		for(auto &&next : outline)
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
	}
	return closest;
}



bool Mask::Contains(Point point) const
{
	if(!IsLoaded())
		return false;

	// If this point is contained within the mask, a ray drawn out from it will
	// intersect the mask an odd number of times. If that ray coincides with an
	// edge, ignore that edge, and count all segments as closed at the start and
	// open at the end to avoid double-counting.

	// For simplicity, use a ray pointing straight downwards. A segment then
	// intersects only if its x coordinates span the point's coordinates.
	// Compute the number of intersections across all outlines, not just one, as the
	// outlines may be nested (i.e. holes) or discontinuous (multiple separate shapes).
	int intersections = 0;
	for(auto &&outline : outlines)
	{
		Point prev = outline.back();
		for(auto &&next : outline)
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
	}
	// If the number of intersections is odd, the point is within the mask.
	return (intersections & 1);
}
