/* CollisionSet.cpp
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

#include "CollisionSet.h"

#include "Body.h"
#include "Collision.h"
#include "Government.h"
#include "Logger.h"
#include "image/Mask.h"
#include "Point.h"
#include "Projectile.h"
#include "Ship.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <set>
#include <string>

using namespace std;

namespace {
	// Maximum allowed projectile velocity.
	constexpr int MAX_VELOCITY = 450000;
	// Velocity used for any projectiles with v > MAX_VELOCITY
	constexpr int USED_MAX_VELOCITY = MAX_VELOCITY - 1;
	// Warn the user only once about too-large projectile velocities.
	bool warned = false;

	thread_local vector<bool> seen;
}


// Initialize a collision set. The cell size and cell count should both be
// powers of two; otherwise, they are rounded down to a power of two.
CollisionSet::CollisionSet(unsigned cellSize, unsigned cellCount, CollisionType collisionType)
	: collisionType(collisionType)
{
	// Right shift amount to convert from (x, y) location to grid (x, y).
	SHIFT = 0u;
	while(cellSize >>= 1u)
		++SHIFT;
	CELL_SIZE = (1u << SHIFT);
	CELL_MASK = CELL_SIZE - 1u;

	// Number of grid rows and columns.
	CELLS = 1u;
	while(cellCount >>= 1u)
		CELLS <<= 1;
	WRAP_MASK = CELLS - 1u;

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
	all.clear();
	// The counts vector starts with two sentinel slots that will be used in the
	// course of performing the radix sort.
	counts.resize(CELLS * CELLS + 2u, 0u);
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
		auto gy = y & WRAP_MASK;
		for(int x = minX; x <= maxX; ++x)
		{
			auto gx = x & WRAP_MASK;
			added.emplace_back(&body, all.size(), x, y);
			++counts[gy * CELLS + gx + 2];
		}
	}

	// Also save a pointer to this object irrespective of its grid location.
	all.emplace_back(&body);
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
		auto gx = entry.x & WRAP_MASK;
		auto gy = entry.y & WRAP_MASK;
		auto index = gy * CELLS + gx + 1;

		sorted[counts[index]++] = entry;
	}
	// Now, counts[index] is where a certain bin begins.
}



// Get all possible collisions for the given projectile. Collisions are not necessarily
// sorted by distance.
void CollisionSet::Line(const Projectile &projectile, vector<Collision> &result) const
{
	// What objects the projectile hits depends on its government.
	const Government *pGov = projectile.GetGovernment();

	// Convert the projectile to a line represented by its start and end points.
	Point from = projectile.Position();
	Point to = from + projectile.Velocity();
	Line(from, to, result, pGov, projectile.Target());
}



// Get all possible collisions along a line. Collisions are not necessarily sorted by
// distance.
void CollisionSet::Line(const Point &from, const Point &to, vector<Collision> &lineResult,
		const Government *pGov, const Body *target) const
{
	const int x = from.X();
	const int y = from.Y();
	const int endX = to.X();
	const int endY = to.Y();

	// Figure out which grid cell the line starts and ends in.
	int gx = x >> SHIFT;
	int gy = y >> SHIFT;
	const int endGX = endX >> SHIFT;
	const int endGY = endY >> SHIFT;

	// Special case, very common: the projectile is contained in one grid cell.
	// In this case, all the complicated code below can be skipped.
	if(gx == endGX && gy == endGY)
	{
		// Examine all objects in the current grid cell.
		const auto index = (gy & WRAP_MASK) * CELLS + (gx & WRAP_MASK);
		vector<Entry>::const_iterator it = sorted.begin() + counts[index];
		vector<Entry>::const_iterator end = sorted.begin() + counts[index + 1];
		for( ; it != end; ++it)
		{
			// Skip objects that were put in this same grid cell only because
			// of the cell coordinates wrapping around.
			if(it->x != gx || it->y != gy)
				continue;

			// Check if this projectile can hit this object. If either the
			// projectile or the object has no government, it will always hit.
			const Government *iGov = it->body->GetGovernment();
			if(it->body != target && iGov && pGov && !iGov->IsEnemy(pGov))
				continue;

			const Mask &mask = it->body->GetMask(step);
			Point offset = from - it->body->Position();
			const double range = mask.Collide(offset, to - from, it->body->Facing());

			if(range < 1.)
				lineResult.emplace_back(it->body, collisionType, range);
		}

		return;
	}

	const Point pVelocity = (to - from);
	if(pVelocity.Length() > MAX_VELOCITY)
	{
		// Cap projectile velocity to prevent integer overflows.
		if(!warned)
		{
			Logger::Log("A projectile exceeded the maximum allowed velocity (" + to_string(MAX_VELOCITY) + ").",
				Logger::Level::WARNING);
			warned = true;
		}
		Point newEnd = from + pVelocity.Unit() * USED_MAX_VELOCITY;

		Line(from, newEnd, lineResult, pGov, target);
		return;
	}

	// When stepping from one grid cell to the next, we'll go in this direction.
	const int stepX = (x <= endX ? 1 : -1);
	const int stepY = (y <= endY ? 1 : -1);
	// Calculate the slope of the line, shifted so it is positive in both axes.
	const uint64_t mx = abs(endX - x);
	const uint64_t my = abs(endY - y);
	// Behave as if each grid cell has this width and height. This guarantees
	// that we only need to work with integer coordinates.
	const uint64_t scale = max<uint64_t>(mx, 1) * max<uint64_t>(my, 1);
	const uint64_t fullScale = CELL_SIZE * scale;

	// Get the "remainder" distance that we must travel in x and y in order to
	// reach the next grid cell. These ensure we only check grid cells which the
	// line will pass through.
	uint64_t rx = scale * (x & CELL_MASK);
	uint64_t ry = scale * (y & CELL_MASK);
	if(stepX > 0)
		rx = fullScale - rx;
	if(stepY > 0)
		ry = fullScale - ry;

	seen.clear();
	seen.resize(all.size());

	while(true)
	{
		// Examine all objects in the current grid cell.
		auto i = (gy & WRAP_MASK) * CELLS + (gx & WRAP_MASK);
		vector<Entry>::const_iterator it = sorted.begin() + counts[i];
		vector<Entry>::const_iterator end = sorted.begin() + counts[i + 1];
		for( ; it != end; ++it)
		{
			// Skip objects that were put in this same grid cell only because
			// of the cell coordinates wrapping around.
			if(it->x != gx || it->y != gy)
				continue;

			if(seen[it->seenIndex])
				continue;
			seen[it->seenIndex] = true;

			// Check if this projectile can hit this object. If either the
			// projectile or the object has no government, it will always hit.
			const Government *iGov = it->body->GetGovernment();
			if(it->body != target && iGov && pGov && !iGov->IsEnemy(pGov))
				continue;

			const Mask &mask = it->body->GetMask(step);
			Point offset = from - it->body->Position();
			const double range = mask.Collide(offset, to - from, it->body->Facing());

			if(range < 1.)
				lineResult.emplace_back(it->body, collisionType, range);
		}

		// Check if we've reached the final grid cell.
		if(gx == endGX && gy == endGY)
			break;
		// If not, move to the next one. Check whether rx / mx < ry / my.
		const int64_t diff = rx * my - ry * mx;
		if(!diff)
		{
			// The line is exactly intersecting a corner.
			rx = fullScale;
			ry = fullScale;
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
			// Because of the scale used, the rx coordinate is always divisible
			// by mx, so this will always come out even. The mx will always be
			// nonzero because otherwise, the comparison would have been false.
			ry -= my * (rx / mx);
			rx = fullScale;
			gx += stepX;
		}
		else
		{
			// Calculate how much x distance remains until the edge of the cell
			// after moving forward to the edge in the y direction.
			rx -= mx * (ry / my);
			ry = fullScale;
			gy += stepY;
		}
	}
}



// Get all objects within the given range of the given point.
void CollisionSet::Circle(const Point &center, double radius, vector<Body *> &result) const
{
	Ring(center, 0., radius, result);
}



// Get all objects touching a ring with a given inner and outer range
// centered at the given point.
void CollisionSet::Ring(const Point &center, double inner, double outer, vector<Body *> &circleResult) const
{
	// Calculate the range of (x, y) grid coordinates this ring covers.
	const int minX = static_cast<int>(center.X() - outer) >> SHIFT;
	const int minY = static_cast<int>(center.Y() - outer) >> SHIFT;
	const int maxX = static_cast<int>(center.X() + outer) >> SHIFT;
	const int maxY = static_cast<int>(center.Y() + outer) >> SHIFT;

	seen.clear();
	seen.resize(all.size());

	for(int y = minY; y <= maxY; ++y)
	{
		const auto gy = y & WRAP_MASK;
		for(int x = minX; x <= maxX; ++x)
		{
			const auto gx = x & WRAP_MASK;
			const auto index = gy * CELLS + gx;
			vector<Entry>::const_iterator it = sorted.begin() + counts[index];
			vector<Entry>::const_iterator end = sorted.begin() + counts[index + 1];

			for( ; it != end; ++it)
			{
				// Skip objects that were put in this same grid cell only because
				// of the cell coordinates wrapping around.
				if(it->x != x || it->y != y)
					continue;

				if(seen[it->seenIndex])
					continue;
				seen[it->seenIndex] = true;

				const Mask &mask = it->body->GetMask(step);
				Point offset = center - it->body->Position();
				const double length = offset.Length();
				if((length <= outer && length >= inner)
					|| mask.WithinRing(offset, it->body->Facing(), inner, outer))
					circleResult.push_back(it->body);
			}
		}
	}
}



const vector<Body *> &CollisionSet::All() const
{
	return all;
}
