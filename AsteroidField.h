/* AsteroidField.h
Michael Zahniser, 14 Dec 2013

Class representing a field of asteroids, which repeats regularly in order to
fill all of space.
*/

#ifndef ASTEROID_FIELD_H_INCLUDED
#define ASTEROID_FIELD_H_INCLUDED

#include "Angle.h"
#include "Animation.h"
#include "Point.h"

class DrawList;
class GameData;
class Projectile;

#include <string>
#include <vector>



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
