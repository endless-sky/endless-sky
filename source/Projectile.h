/* Projectile.h
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

#ifndef PROJECTILE_H_
#define PROJECTILE_H_

#include "Body.h"

#include "Angle.h"
#include "Point.h"

#include <memory>
#include <vector>

class Government;
class Ship;
class Visual;
class Weapon;



// Class representing a projectile (a moving object which can hit ships or
// asteroids and can potentially be hit by anti-missile systems). A projectile
// may either move at a constant heading and velocity, or may accelerate or
// change course to track its target. Also, when they hit their target or reach
// the end of their lifetime, some projectiles split into "sub-munitions," new
// projectiles that may look different or travel in a new direction.
class Projectile : public Body {
public:
	class ImpactInfo {
	public:
		ImpactInfo(const Weapon &weapon, Point position, double distanceTraveled)
			: weapon(weapon), position(std::move(position)), distanceTraveled(distanceTraveled) {}

		const Weapon &weapon;
		Point position;
		double distanceTraveled;
	};


public:
	Projectile(const Ship &parent, Point position, Angle angle, const Weapon *weapon);
	Projectile(const Projectile &parent, const Point &offset, const Angle &angle, const Weapon *weapon);
	// Ship explosion.
	Projectile(Point position, const Weapon *weapon);

	// Functions provided by the Body base class:
	// Frame GetFrame(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;
	// const Government *GetGovernment() const;

	// Move the projectile. It may create effects or submunitions.
	void Move(std::vector<Visual> &visuals, std::vector<Projectile> &projectiles);
	// This projectile hit something. Create the explosion, if any. This also
	// marks the projectile as needing deletion.
	void Explode(std::vector<Visual> &visuals, double intersection, Point hitVelocity = Point());
	// Get the amount of clipping that should be applied when drawing this projectile.
	double Clip() const;
	// This projectile was killed, e.g. by an anti-missile system.
	void Kill();

	// Find out if this is a missile, and if so, how strong it is (i.e. what
	// chance an anti-missile shot has of destroying it).
	int MissileStrength() const;
	// Get information on the weapon that fired this projectile.
	const Weapon &GetWeapon() const;
	// Get information on how this projectile impacted a ship.
	ImpactInfo GetInfo() const;

	// Find out which ship or government this projectile is targeting. Note:
	// this pointer is not guaranteed to be dereferenceable, so only use it
	// for comparing.
	const Ship *Target() const;
	const Government *TargetGovernment() const;
	// This function is much more costly, so use it only if you need to get a
	// non-const shared pointer to the target ship.
	std::shared_ptr<Ship> TargetPtr() const;
	// Clear the targeting information on this projectile.
	void BreakTarget();

	// Get the distance that this projectile has traveled.
	double DistanceTraveled() const;


private:
	void CheckLock(const Ship &target);


private:
	const Weapon *weapon = nullptr;

	std::weak_ptr<Ship> targetShip;
	const Ship *cachedTarget = nullptr;
	const Government *targetGovernment = nullptr;

	// The change in velocity of all stages of this projectile
	// relative to the firing ship.
	Point dV;
	double clip = 1.;
	int lifetime = 0;
	double distanceTraveled = 0;
	bool hasLock = true;
};



#endif
