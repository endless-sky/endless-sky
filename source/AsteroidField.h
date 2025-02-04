/* AsteroidField.h
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

#pragma once

#include "Angle.h"
#include "Body.h"
#include "CollisionSet.h"
#include "Point.h"
#include "WeightedList.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

class Collision;
class DrawList;
class Flotsam;
class Minable;
class Projectile;
class Sprite;
class Visual;



// Class representing a field of asteroids. The field actually "repeats" every
// 4096 pixels. That is, an asteroid present at (100, 200) is also present at
// (4196, 200), (100, 4296), etc. Other games often just "wrap" the asteroids
// to the screen, meaning that there are no actual asteroids beyond what the
// player can see, but that means that missiles are not in danger of hitting an
// asteroid unless they are on screen, and also causes trouble if the screen is
// resized on the fly. Asteroids never change direction or speed, even if they
// are hit by a projectile.
class AsteroidField {
public:
	// Constructor, to set up the collision set parameters.
	AsteroidField();

	// Reset the asteroid field (typically because you entered a new system).
	void Clear();
	void Add(const std::string &name, int count, double energy = 1.);
	void Add(const Minable *minable, int count, double energy, const WeightedList<double> &belts);

	// Move all the asteroids forward one time step, and populate the asteroid and minable collision sets.
	void Step(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam, int step);
	// Draw the asteroid field, with the field of view centered on the given point.
	void Draw(DrawList &draw, const Point &center, double zoom) const;

	// Check if the given projectile collides with any asteroids. This excludes minables.
	void CollideAsteroids(const Projectile &projectile, std::vector<Collision> &result) const;
	// Check if the given projectile collides with any minables.
	void CollideMinables(const Projectile &projectile, std::vector<Collision> &result) const;
	// Get a list of minables affected by an explosion with blast radius.
	void MinablesCollisionsCircle(const Point &center, double radius, std::vector<Body *> &result) const;

	// Get the list of minable asteroids.
	const std::list<std::shared_ptr<Minable>> &Minables() const;


private:
	// This class represents an asteroid that cannot be destroyed or even
	// deflected from its trajectory, and that repeats every 4096 pixels.
	class Asteroid : public Body {
	public:
		Asteroid(const Sprite *sprite, double energy);

		void Step();
		void Draw(DrawList &draw, const Point &center, double zoom) const;

	private:
		Angle spin;
		Point size;
	};


private:
	std::vector<Asteroid> asteroids;
	std::list<std::shared_ptr<Minable>> minables;

	CollisionSet asteroidCollisions;
	CollisionSet minableCollisions;
};
