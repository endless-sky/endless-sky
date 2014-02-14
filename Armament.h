/* Armament.h
Michael Zahniser, 11 Feb 2014

Class representing the collection of weapons that a given ship has, along with
tracking reload counts, source points, etc.
*/

#ifndef ARMAMENT_H_INCLUDED
#define ARMAMENT_H_INCLUDED

#include "Angle.h"
#include "Point.h"

#include <map>
#include <list>
#include <vector>

class Effect;
class Outfit;
class Projectile;
class Ship;



class Armament {
public:
	class Weapon {
	public:
		Weapon(const Point &point, bool isTurret);
		
		// Check if anything is installed in this gun port.
		bool HasOutfit() const;
		// Don't call this without checking HasOutfit()!
		const Outfit *GetOutfit() const;
		// Get the point, in ship image coordinates, from which projectiles of
		// this weapon should originate.
		const Point &GetPoint() const;
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
	void AddGunPort(const Point &point);
	void AddTurret(const Point &point);
	// This must be called after all the outfit data is loaded. If you add more
	// of a given weapon than there are slots for it, the extras will not fire.
	// But, the "gun ports" attribute should keep that from happening. To
	// remove a weapon, just pass  negative value here.
	void Add(const Outfit *outfit, int count = 1);
	
	// Access the array of weapons.
	const std::vector<Weapon> &Get() const;
	
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
	static double RendevousTime(const Point &p, const Point &v, double vp)
	
	
private:
	// Note: the Armament must be copied when an instance of a Ship is made, so
	// it should not hold any pointers specific to one ship (including to
	// elements of this Armement itself).
	std::map<const Outfit *, int> streamReload;
	std::vector<Weapon> weapons;
};



#endif
