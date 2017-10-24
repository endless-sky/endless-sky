/* CollisionSet.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CollisionSet.h"

#include "Body.h"
#include "Government.h"
#include "Mask.h"
#include "Point.h"
#include "Projectile.h"
#include "Ship.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <set>

using namespace std;

namespace {
	// Generate offsets for the line test of infinite collision sets.
	void InfiniteLineOffsets(vector<Point> &offsets, const Projectile &projectile, const Body *body, double wrapSize)
	{
		// Find offset closest to the center of the path.
		const Point &velocity = projectile.Velocity();
		Point halfVelocity = .5 * velocity;
		Point offset = (projectile.Position() + halfVelocity) - body->Position();
		offset = Point(remainder(offset.X(), wrapSize), remainder(offset.Y(), wrapSize)) - halfVelocity;
		
		// Record relevant offsets.
		offsets.assign(1, offset);
		if(abs(velocity.X()) >= wrapSize)
		{
			// Extra offsets in the positive X direction.
			//   offset + N * wrapSize + velocity <= 0.5 * width, N > 0
			//   N * wrapSize <= 0.5 * width - offset - velocity, N > 0
			//   N <= (0.5 * width - offset - velocity) / wrapSize, N > 0
			for(int n = floor((.5 * body->Width() - offset.X() - velocity.X()) / wrapSize); n > 0; --n)
				offsets.emplace_back(offset.X() + n * wrapSize, offset.Y());
			// Extra offsets in the negative X direction.
			//   offset - N * wrapSize + velocity >= -0.5 * width, N > 0
			//   N * -wrapSize >= -0.5 * width - offset - velocity, N > 0
			//   N <= (0.5 * width + offset + velocity) / wrapSize, N > 0
			for(int n = floor((.5 * body->Width() + offset.X() + velocity.X()) / wrapSize); n > 0; --n)
				offsets.emplace_back(offset.X() - n * wrapSize, offset.Y());
		}
		if(abs(velocity.Y()) >= wrapSize)
		{
			size_t M = offsets.size();
			// Extra offsets in the positive Y direction.
			for(int n = floor((.5 * body->Height() - offset.Y() - velocity.Y()) / wrapSize); n > 0; --n)
				for(size_t i = 0; i < M; ++i)
					offsets.emplace_back(offsets[i].X(), offset.Y() + n * wrapSize);
			// Extra offsets in the negative Y direction.
			for(int n = floor((.5 * body->Height() + offset.Y() + velocity.Y()) / wrapSize); n > 0; --n)
				for(size_t i = 0; i < M; ++i)
					offsets.emplace_back(offsets[i].X(), offset.Y() - n * wrapSize);
		}
	}
}



// Initialize a collision set. The cell size and cell count should both be
// powers of two; otherwise, they are rounded down to a power of two.
// Infinite sets repeat bodies every wrap size (cells * cell size).
CollisionSet::CollisionSet(int cellSize, int cellCount, bool isInfinite)
	: isInfinite(isInfinite)
{
	// Right shift amount to convert from (x, y) location to grid (x, y).
	SHIFT = 0;
	while(cellSize >>= 1)
		++SHIFT;
	CELL_SIZE = (1 << SHIFT);
	CELL_MASK = CELL_SIZE - 1;
	
	// Number of grid rows and columns.
	CELLS = 1;
	while(cellCount >>= 1)
		CELLS <<= 1;
	WRAP_MASK = CELLS - 1;

	WRAP_SIZE = CELLS * CELL_SIZE;
	
	// Just in case Clear() isn't called before objects are added:
	Clear(0);
}



// Clear all objects in the set.
void CollisionSet::Clear(int step)
{
	this->step = step;
	
	added.clear();
	sorted.clear();
	counts.clear();
	// The counts vector starts with two sentinel slots that will be used in the
	// course of performing the radix sort.
	counts.resize(CELLS * CELLS + 2, 0);
}



// Add an object to the set.
void CollisionSet::Add(Body &body)
{
	// Calculate the range of (x, y) grid coordinates this object covers.
	int minX = static_cast<int>(body.Position().X() - body.Radius()) >> SHIFT;
	int minY = static_cast<int>(body.Position().Y() - body.Radius()) >> SHIFT;
	int maxX = static_cast<int>(body.Position().X() + body.Radius()) >> SHIFT;
	int maxY = static_cast<int>(body.Position().Y() + body.Radius()) >> SHIFT;
	
	// Add a pointer to this object in every grid cell it occupies.
	for(int y = minY; y <= maxY; ++y)
	{
		int gy = y & WRAP_MASK;
		for(int x = minX; x <= maxX; ++x)
		{
			int gx = x & WRAP_MASK;
			added.emplace_back(&body, x, y);
			++counts[gy * CELLS + gx + 2];
		}
	}
}



// Finish adding objects (and organize them into the final lookup table).
void CollisionSet::Finish()
{
	// Perform a partial sum to convert the counts of items in each bin into the
	// index of the output element where that bin begins.
	partial_sum(counts.begin(), counts.end(), counts.begin());
	
	// Allocate space for a sorted copy of the vector.
	sorted.resize(added.size());
	
	// Now, perform a radix sort.
	for(const Entry &entry : added)
	{
		int gx = entry.x & WRAP_MASK;
		int gy = entry.y & WRAP_MASK;
		int index = gy * CELLS + gx + 1;
		
		sorted[counts[index]++] = entry;
	}
	
	// Now, counts[index] is where a certain bin begins.
}



// Get the first object that collides with the given projectile. If a
// "closest hit" value is given, update that value.
Body *CollisionSet::Line(const Projectile &projectile, double *closestHit) const
{
	// What objects the projectile hits depends on its government.
	const Government *pGov = projectile.GetGovernment();
	
	// Convert the start and end coordinates to integers.
	Point from = projectile.Position();
	Point to = from + projectile.Velocity();
	int x = from.X();
	int y = from.Y();
	int endX = to.X();
	int endY = to.Y();
	
	// Figure out which grid cell the line starts and ends in.
	int gx = x >> SHIFT;
	int gy = y >> SHIFT;
	int endGX = endX >> SHIFT;
	int endGY = endY >> SHIFT;
	
	// Keep track of the closest collision found so far.
	double closest = 1.;
	Body *result = nullptr;
	
	// Special case, very common: the projectile is contained in one grid cell.
	// In this case, all the complicated code below can be skipped.
	if(gx == endGX && gy == endGY)
	{
		// Examine all objects in the current grid cell.
		int i = (gy & WRAP_MASK) * CELLS + (gx & WRAP_MASK);
		vector<Entry>::const_iterator it = sorted.begin() + counts[i];
		vector<Entry>::const_iterator end = sorted.begin() + counts[i + 1];
		for( ; it != end; ++it)
		{
			// Skip objects that were put in this same grid cell only because
			// of the cell coordinates wrapping around.
			if(!isInfinite && (it->x != gx || it->y != gy))
				continue;
			
			// Check if this projectile can hit this object. If either the
			// projectile or the object has no government, it will always hit.
			const Government *iGov = it->body->GetGovernment();
			if(it->body != projectile.Target() && iGov && pGov && !iGov->IsEnemy(pGov))
				continue;
			
			const Mask &mask = it->body->GetMask(step);
			if(isInfinite)
			{
				vector<Point> offsets;
				InfiniteLineOffsets(offsets, projectile, it->body, WRAP_SIZE);
				for(Point &offset : offsets)
				{
					double range = mask.Collide(offset, projectile.Velocity(), it->body->Facing());
			
					if(range < closest)
					{
						closest = range;
						result = it->body;
					}
				}
 			}
			else
			{
				Point offset = projectile.Position() - it->body->Position();
				double range = mask.Collide(offset, projectile.Velocity(), it->body->Facing());
			
				if(range < closest)
				{
					closest = range;
					result = it->body;
				}
			}
		}
		if(closest < 1. && closestHit)
			*closestHit = closest;
		return result;
	}
	
	// When stepping from one grid cell to the next, we'll go in this direction.
	int stepX = (x <= endX ? 1 : -1);
	int stepY = (y <= endY ? 1 : -1);
	// Calculate the slope of the line, shifted so it is positive in both axes.
	int mx = abs(endX - x);
	int my = abs(endY - y);
	// Behave as if each grid cell has this width and height. This guarantees
	// that we only need to work with integer coordinates.
	int scale = max(mx, 1) * max(my, 1);
	int full = CELL_SIZE * scale;
	
	// Get the "remainder" distance that we must travel in x and y in order to
	// reach the next grid cell.
	int64_t rx = scale * (x & CELL_MASK);
	int64_t ry = scale * (y & CELL_MASK);
	if(stepX > 0)
		rx = full - rx;
	if(stepY > 0)
		ry = full - ry;
	
	// Keep track of which objects we've already considered.
	set<const Body *> seen;
	while(true)
	{
		// Examine all objects in the current grid cell.
		int i = (gy & WRAP_MASK) * CELLS + (gx & WRAP_MASK);
		vector<Entry>::const_iterator it = sorted.begin() + counts[i];
		vector<Entry>::const_iterator end = sorted.begin() + counts[i + 1];
		for( ; it != end; ++it)
		{
			// Skip objects that were put in this same grid cell only because
			// of the cell coordinates wrapping around.
			if(!isInfinite && (it->x != gx || it->y != gy))
				continue;
			
			if(seen.count(it->body))
				continue;
			seen.insert(it->body);
			
			// Check if this projectile can hit this object. If either the
			// projectile or the object has no government, it will always hit.
			const Government *iGov = it->body->GetGovernment();
			if(it->body != projectile.Target() && iGov && pGov && !iGov->IsEnemy(pGov))
				continue;
			
			const Mask &mask = it->body->GetMask(step);
			if(isInfinite)
			{
				vector<Point> offsets;
				InfiniteLineOffsets(offsets, projectile, it->body, WRAP_SIZE);
				for(Point &offset : offsets)
				{
					double range = mask.Collide(offset, projectile.Velocity(), it->body->Facing());
			
					if(range < closest)
					{
						closest = range;
						result = it->body;
					}
				}
 			}
			else
			{
				Point offset = projectile.Position() - it->body->Position();
				double range = mask.Collide(offset, projectile.Velocity(), it->body->Facing());
			
				if(range < closest)
				{
					closest = range;
					result = it->body;
				}
			}
		}
		
		// Check if we've found a collision or reached the final grid cell.
		if(result || (gx == endGX && gy == endGY))
			break;
		// If not, move to the next one. Check whether rx / mx < ry / my.
		int64_t diff = rx * my - ry * mx;
		if(!diff)
		{
			// The line is exactly intersecting a corner.
			rx = full;
			ry = full;
			// Make sure we don't step past the end grid.
			if(gx == endGX && gy + stepY == endGY)
				break;
			if(gy == endGY && gx + stepX == endGX)
				break;
			gx += stepX;
			gy += stepY;
		}
		else if(diff < 0)
		{
			// Because of the scale used, the rx coordinate is always divisble
			// by mx, so this will always come out even. The mx will always be
			// nonzero because otherwise, the comparison would be false.
			ry -= my * (rx / mx);
			rx = full;
			gx += stepX;
		}
		else
		{
			// Calculate how much x distance remains until the edge of the cell
			// after moving forward to the edge in the y direction.
			rx -= mx * (ry / my);
			ry = full;
			gy += stepY;
		}
	}
	
	if(closest < 1. && closestHit)
		*closestHit = closest;
	return result;
}



// Get all objects within the given range of the given point.
const vector<Body *> &CollisionSet::Circle(const Point &center, double radius) const
{
	// Calculate the range of (x, y) grid coordinates this circle covers.
	int minX = static_cast<int>(center.X() - radius) >> SHIFT;
	int minY = static_cast<int>(center.Y() - radius) >> SHIFT;
	int maxX = static_cast<int>(center.X() + radius) >> SHIFT;
	int maxY = static_cast<int>(center.Y() + radius) >> SHIFT;
	
	// Keep track of which objects we've already considered.
	set<const Body *> seen;
	result.clear();
	for(int y = minY; y <= maxY; ++y)
	{
		int gy = y & WRAP_MASK;
		for(int x = minX; x <= maxX; ++x)
		{
			int gx = x & WRAP_MASK;
			int i = gy * CELLS + gx;
			vector<Entry>::const_iterator it = sorted.begin() + counts[i];
			vector<Entry>::const_iterator end = sorted.begin() + counts[i + 1];
			
			for( ; it != end; ++it)
			{
				// Skip objects that were put in this same grid cell only because
				// of the cell coordinates wrapping around.
				if(!isInfinite && (it->x != x || it->y != y))
					continue;
				
				if(seen.count(it->body))
					continue;
				seen.insert(it->body);
				
				Point offset = center - it->body->Position();
				if(isInfinite)
					offset = Point(remainder(offset.X(), WRAP_SIZE), remainder(offset.Y(), WRAP_SIZE));
				
				const Mask &mask = it->body->GetMask(step);
				if(offset.Length() <= radius || mask.WithinRange(offset, it->body->Facing(), radius))
					result.push_back(it->body);
			}
		}
	}
	return result;
}
