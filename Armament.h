/* Armament.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ARMAMENT_H_
#define ARMAMENT_H_

#include "Angle.h"
#include "Point.h"

#include <map>
#include <list>
#include <vector>

class Effect;
class Outfit;
class Projectile;
class Ship;



// Class representing the collection of weapons that a given ship has, along with
// tracking reload counts, source points, etc.
class Armament {
public:
	class Weapon {
	public:
		Weapon(const Point &point, bool isTurret, const Outfit *outfit = nullptr);
		
		// Be sure to check if this is nullptr!
		const Outfit *GetOutfit() const;
		// Get the point, in ship image coordinates, from which projectiles of
		// this weapon should originate.
		const Point &GetPoint() const;
		// Get the convergence angle adjustment of this weapon.
		const Angle &GetAngle() const;
		// Shortcuts for querying weapon characteristics.
		bool IsTurret() const;
		bool IsHoming() const;
		bool IsAntiMissile() const;
		
		// Check if this weapon is ready to fire.
		bool IsReady() const;
		// Perform one step (i.e. decrement the reload count).
		void Step();
		
		// Fire this weapon. If it is a turret, it automatically points toward
		// the given ship's target. If the weapon requires ammunition, it will
		// be subtracted from the given ship.
		void Fire(Ship &ship, std::list<Projectile> &projectiles);
		// Fire an anti-missile. Returns true if the missile should be killed.
		bool FireAntiMissile(Ship &ship, const Projectile &projectile, std::list<Effect> &effects);
		
		// Install a weapon here (assuming it is empty). This is only for
		// Armament to call internally.
		void Install(const Outfit *outfit);
		// Uninstall the outfit from this port (if it has one).
		void Uninstall();
		
	private:
		const Outfit *outfit;
		Point point;
		// Angle adjustment for convergence.
		Angle angle;
		int reload;
		bool isTurret;
	};
	
	
public:
	// Add a gun or turret hard-point.
	void AddGunPort(const Point &point, const Outfit *outfit = nullptr);
	void AddTurret(const Point &point, const Outfit *outfit = nullptr);
	// This must be called after all the outfit data is loaded. If you add more
	// of a given weapon than there are slots for it, the extras will not fire.
	// But, the "gun ports" attribute should keep that from happening. To
	// remove a weapon, just pass a negative value here.
	void Add(const Outfit *outfit, int count = 1);
	// Call this once all the outfits have been loaded to make sure they are all
	// set up properly (even the ones that were pre-assigned to a hardpoint).
	void FinishLoading();
	
	// Access the array of weapons.
	const std::vector<Weapon> &Get() const;
	int GunCount() const;
	int TurretCount() const;
	
	// Fire the given weapon, if it is ready. If it did not fire because it is
	// not ready, return false.
	void Fire(int index, Ship &ship, std::list<Projectile> &projectiles);
	// Fire the given anti-missile system.
	bool FireAntiMissile(int index, Ship &ship, const Projectile &projectile, std::list<Effect> &effects);
	
	// Update the reload counters.
	void Step(const Ship &ship);
	
	// Calculate how long it will take a projectile to reach a target given the
	// target's relative position and velocity and the velocity of the
	// projectile. If it cannot hit the target, this returns NaN.
	static double RendevousTime(const Point &p, const Point &v, double vp);
	
	
private:
	// Note: the Armament must be copied when an instance of a Ship is made, so
	// it should not hold any pointers specific to one ship (including to
	// elements of this Armament itself).
	std::map<const Outfit *, int> streamReload;
	std::vector<Weapon> weapons;
};



#endif
