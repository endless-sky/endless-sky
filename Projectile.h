/* Projectile.h
Michael Zahniser, 15 Jan 2014

Class representing a projectile (a moving object which can hit ships or
asteroids and can potentially be hit by anti-missile systems).
*/

#ifndef PROJECTILE_H_INCLUDED
#define PROJECTILE_H_INCLUDED

#include "Angle.h"
#include "Animation.h"
#include "Effect.h"
#include "Point.h"

class Government;
class Outfit;
class Ship;
class System;

#include <list>
#include <memory>



class Projectile {
public:
	Projectile(const Ship &parent, Point position, Angle angle, const Outfit *weapon);
	Projectile(const Projectile &parent, const Outfit *weapon);
	
	// This returns false if it is time to delete this projectile.
	bool Move(std::list<Effect> &effects);
	// This is called when a projectile "dies," either of natural causes or
	// because it hit its target.
	void MakeSubmunitions(std::list<Projectile> &projectiles) const;
	// Check if this projectile collides with the given step, with the animation
	// frame for the given step.
	double CheckCollision(const Ship &ship, int step) const;
	// Check if this projectile has a blast radius.
	bool HasBlastRadius() const;
	// Check if the given ship is within this projectile's blast radius. (The
	// projectile will not explode unless it is also within the trigger radius.)
	bool InBlastRadius(const Ship &ship, int step) const;
	// This projectile hit something. Create the explosion, if any. This also
	// marks the projectile as needing deletion.
	void Explode(std::list<Effect> &effects, double intersection);
	// This projectile was killed, e.g. by an anti-missile system.
	void Kill();
	
	// Find out if this is a missile, and if so, how strong it is (i.e. what
	// chance an anti-missile shot has of destroying it).
	int MissileStrength() const;
	// Get information on the weapon that fired this projectile.
	const Outfit &GetWeapon() const;
	
	// Get the projectiles characteristics, for drawing.
	const Animation &GetSprite() const;
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	// Get the facing unit vector times the scale factor.
	Point Unit() const;
	
	// Find out which ship this projectile is targeting.
	const Ship *Target() const;
	// Find out which government this projectile belongs to.
	const Government *GetGovernment() const;
	
	
private:
	const Outfit *weapon;
	Animation animation;
	
	Point position;
	Point velocity;
	Angle angle;
	
	std::weak_ptr<const Ship> targetShip;
	const Government *government;
	
	int lifetime;
};



#endif
