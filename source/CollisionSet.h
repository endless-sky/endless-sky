/* CollisionSet.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef COLLISION_SET_H_
#define COLLISION_SET_H_

#include <vector>

class Government;
class Point;
class Projectile;
class Body;



// A CollisionSet allows efficient collision detection by splitting space up
// into a grid and keeping track of which objects are in each grid cell. A check
// for collisions can then only examine objects in certain cells.
class CollisionSet {
public:
	// Initialize a collision set. The cell size and cell count should both be
	// powers of two; otherwise, they are rounded down to a power of two.
	CollisionSet(int cellSize, int cellCount);
	
	// Clear all objects in the set. Specify which engine step we are on, so we
	// know what animation frame each object is on.
	void Clear(int step);
	// Add an object to the set.
	void Add(Body &body);
	// Finish adding objects (and organize them into the final lookup table).
	void Finish();
	
	// Get the first object that collides with the given projectile. If a
	// "closest hit" value is given, update that value.
	Body *Line(const Projectile &projectile, double *closestHit = nullptr) const;
	// Check for collisions with a line, which may be a projectile's current
	// position or its entire expected trajectory (for the auto-firing AI).
	Body *Line(const Point &from, const Point &to, double *closestHit = nullptr,
		const Government *pGov = nullptr, const Body *target = nullptr) const;
	
	// Get all objects within the given range of the given point.
	const std::vector<Body *> &Circle(const Point &center, double radius) const;
	
	
private:
	class Entry {
	public:
		Entry() = default;
		Entry(Body *body, int x, int y) : body(body), x(x), y(y) {}
		
		Body *body;
		int x;
		int y;
	};
	
	
private:
	// The size of individual cells of the grid.
	int CELL_SIZE;
	int SHIFT;
	int CELL_MASK;
	
	// The number of grid cells.
	int CELLS;
	int WRAP_MASK;
	
	// The current game engine step.
	int step;
	
	// Vectors to store the objects in the collision set.
	std::vector<Entry> added;
	std::vector<Entry> sorted;
	std::vector<int> counts;
	
	// Vector for returning the result of a circle query.
	mutable std::vector<Body *> result;
};



#endif
