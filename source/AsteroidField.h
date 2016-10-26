/* AsteroidField.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ASTEROID_FIELD_H_
#define ASTEROID_FIELD_H_

#include "Angle.h"
#include "Body.h"
#include "Point.h"

#include <string>
#include <vector>

class DrawList;
class Projectile;
class Sprite;



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
	AsteroidField();
	
	void Clear();
	void Add(const std::string &name, int count, double energy = 1.);
	
	void Step();
	void Draw(DrawList &draw, const Point &center) const;
	double Collide(const Projectile &projectile, int step, Point *hitVelocity = nullptr) const;
	
	
private:
	class Asteroid : public Body {
	public:
		Asteroid(const Sprite *sprite, double energy);
		
		void Step();
		void Draw(DrawList &draw, const Point &center) const;
		double Collide(const Projectile &projectile, int step) const;
		
	private:
		Angle spin;
	};
	
	
private:
	std::vector<Asteroid> asteroids;
};



#endif
