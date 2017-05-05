/* Projectile.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PROJECTILE_H_
#define PROJECTILE_H_

#include "Angle.h"
#include "Body.h"
#include "Point.h"

#include <list>
#include <memory>

class Effect;
class Government;
class Outfit;
class Ship;



// Class representing a projectile (a moving object which can hit ships or
// asteroids and can potentially be hit by anti-missile systems). A projectile
// may either move at a constant heading and velocity, or may accelerate or
// change course to track its target. Also, when they hit their target or reach
// the end of their lifetime, some projectiles split into "sub-munitions," new
// projectiles that may look different or travel in a new direction.
class Projectile : public Body {
public:
	Projectile(const Ship &parent, Point position, Angle angle, const Outfit *weapon);
	Projectile(const Projectile &parent, const Outfit *weapon);
	// Ship explosion.
	Projectile(Point position, const Outfit *weapon);
	
	/* Functions provided by the Body base class:
	Frame GetFrame(int step = -1) const;
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	Point Unit() const;
	const Government *GetGovernment() const;
	*/
	
	// This returns false if it is time to delete this projectile.
	bool Move(std::list<Effect> &effects);
	// This is called when a projectile "dies," either of natural causes or
	// because it hit its target.
	void MakeSubmunitions(std::list<Projectile> &projectiles) const;
	// This projectile hit something. Create the explosion, if any. This also
	// marks the projectile as needing deletion.
	void Explode(std::list<Effect> &effects, double intersection, Point hitVelocity = Point());
	// This projectile was killed, e.g. by an anti-missile system.
	void Kill();
	
	// Find out if this is a missile, and if so, how strong it is (i.e. what
	// chance an anti-missile shot has of destroying it).
	int MissileStrength() const;
	// Get information on the weapon that fired this projectile.
	const Outfit &GetWeapon() const;
	
	// Find out which ship this projectile is targeting. Note: this pointer is
	// not guaranteed to be dereferenceable, so only use it for comparing.
	const Ship *Target() const;
	
	
private:
	void CheckLock(const Ship &target);
	
	
private:
	const Outfit *weapon = nullptr;
	
	std::weak_ptr<const Ship> targetShip;
	const Ship *cachedTarget = nullptr;
	const Government *targetGovernment = nullptr;
	
	int lifetime = 0;
	bool hasLock = true;
};



#endif
