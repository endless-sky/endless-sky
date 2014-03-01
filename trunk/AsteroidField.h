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
#include "Animation.h"
#include "Point.h"

class DrawList;
class GameData;
class Projectile;

#include <string>
#include <vector>



// Class representing a field of asteroids, which repeats regularly in order to
// fill all of space.
class AsteroidField {
public:
	AsteroidField(const GameData &gameData);
	
	void Clear();
	void Add(const std::string &name, int count, double energy = 1.);
	
	void Step();
	void Draw(DrawList &draw, const Point &center) const;
	double Collide(const Projectile &projectile, int step) const;
	
	
private:
	class Asteroid {
	public:
		Asteroid(const Sprite *sprite, double energy);
		
		void Step();
		void Draw(DrawList &draw, const Point &center) const;
		double Collide(const Projectile &projectile, int step) const;
		
	private:
		Point location;
		Point velocity;
		
		Angle angle;
		Angle spin;
		
		Animation animation;
	};
	
	
private:
	const GameData &gameData;
	
	std::vector<Asteroid> asteroids;
};



#endif
