/* CollisionSet.h
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

#include "Collision.h"
#include "CollisionType.h"

#include <vector>

class Body;
class Government;
class Point;
class Projectile;



// A CollisionSet allows efficient collision detection by splitting space up
// into a grid and keeping track of which objects are in each grid cell. A check
// for collisions can then only examine objects in certain cells.
class CollisionSet {
public:
	// Initialize a collision set. The cell size and cell count should both be
	// powers of two; otherwise, they are rounded down to a power of two.
	CollisionSet(unsigned cellSize, unsigned cellCount, CollisionType collisionType);

	// Clear all objects in the set. Specify which engine step we are on, so we
	// know what animation frame each object is on.
	void Clear(int step);
	// Add an object to the set.
	void Add(Body &body);
	// Finish adding objects (and organize them into the final lookup table).
	void Finish();

	// Get all possible collisions for the given projectile. Collisions are not necessarily
	// sorted by distance.
	void Line(const Projectile &projectile, std::vector<Collision> &result) const;

	// Get all possible collisions along a line. Collisions are not necessarily sorted by
	// distance.
	void Line(const Point &from, const Point &to, std::vector<Collision> &result,
		const Government *pGov = nullptr, const Body *target = nullptr) const;

	// Get all objects within the given range of the given point.
	void Circle(const Point &center, double radius, std::vector<Body *> &result) const;
	// Get all objects touching a ring with a given inner and outer range
	// centered at the given point.
	void Ring(const Point &center, double inner, double outer, std::vector<Body *> &result) const;

	// Get all objects within this collision set.
	const std::vector<Body *> &All() const;


private:
	class Entry {
	public:
		Entry() = default;
		Entry(Body *body, unsigned seenIndex, int x, int y) : body(body), seenIndex(seenIndex), x(x), y(y) {}

		Body *body;
		unsigned seenIndex;
		int x;
		int y;
	};


private:
	// The type of collisions this CollisionSet is responsible for.
	CollisionType collisionType;

	// The size of individual cells of the grid.
	unsigned CELL_SIZE;
	unsigned SHIFT;
	unsigned CELL_MASK;

	// The number of grid cells.
	unsigned CELLS;
	unsigned WRAP_MASK;

	// The current game engine step.
	int step;

	// Vectors to store the objects in the collision set.
	std::vector<Body *> all;
	std::vector<Entry> added;
	std::vector<Entry> sorted;
	// After Finish(), counts[index] is where a certain bin begins.
	std::vector<unsigned> counts;
};
