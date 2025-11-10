/* Hardpoint.h
Copyright (c) 2016 by Michael Zahniser

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
#include "Point.h"

#include <vector>

class Body;
class Flotsam;
class Outfit;
class Projectile;
class Ship;
class Visual;
class Weapon;



// A single weapon hardpoint on the ship (i.e. a gun port or turret mount),
// which may or may not have a weapon installed.
class Hardpoint {
public:
	enum class Side {
		OVER,
		INSIDE,
		UNDER
	};

	// The base attributes of a hardpoint, without considering additional limitations of the installed outfit.
	struct BaseAttributes {
		// The angle that this weapon is aimed at (without harmonization/convergence), relative to the ship.
		// The turret should point to this angle when idling.
		Angle baseAngle;
		// Indicates if this hardpoint disallows converging (guns and the idle position of turrets).
		bool isParallel;
		// An omnidirectional turret can rotate infinitely.
		bool isOmnidirectional;
		// Whether the hardpoint should be drawn over the ship, under it, or not at all.
		Side side = Side::OVER;
		// Range over which the turret can turn, from leftmost position to rightmost position.
		// (directional turret only)
		Angle minArc;
		Angle maxArc;
		std::vector<std::pair<Angle, Angle>> blindspots;
		// This attribute is added to the turret turn multiplier of the ship.
		double turnMultiplier;
	};


public:
	// Constructor. Hardpoints may or may not specify what weapon is in them.
	Hardpoint(const Point &point, const BaseAttributes &attributes,
		bool isTurret, const Outfit *outfit = nullptr);

	// Get the weapon installed in this hardpoint (or null if there is none).
	// The Outfit is guaranteed to have a Weapon after GameData::FinishLoading.
	const Outfit *GetOutfit() const;
	const Weapon *GetWeapon() const;
	// Get the location, relative to the center of the ship, from which
	// projectiles of this weapon should originate. This point must be
	// rotated to take the ship's current facing direction into account.
	const Point &GetPoint() const;
	// Get the angle that this weapon is aimed at, relative to the ship.
	const Angle &GetAngle() const;
	// Get the angle of a turret when idling, relative to the ship.
	// For guns, this function is equal to GetAngle().
	const Angle &GetIdleAngle() const;
	// Get the arc of fire if this is a directional turret,
	// otherwise a pair of 180 degrees + baseAngle.
	const Angle &GetMinArc() const;
	const Angle &GetMaxArc() const;
	// Get the angle this weapon ought to point at for ideal gun harmonization.
	Angle HarmonizedAngle() const;
	// Get the turret turn rate of this hardpoint, considering all applicable multipliers.
	double TurnRate(const Ship &ship) const;
	// Shortcuts for querying weapon characteristics.
	bool IsTurret() const;
	bool IsParallel() const;
	bool IsOmnidirectional() const;
	Side GetSide() const;
	bool IsHoming() const;
	bool IsSpecial() const;
	bool CanAim(const Ship &ship) const;

	// Check if this weapon is ready to fire.
	bool IsReady() const;
	// Check if this weapon can't fire because of its blindspots.
	bool IsBlind() const;
	// Check if this weapon was firing in the previous step.
	bool WasFiring() const;
	// If this is a burst weapon, get the number of shots left in the burst.
	int BurstRemaining() const;
	// Perform one step (i.e. decrement the reload count).
	void Step();

	// Adjust this weapon's aim by the given amount, relative to its maximum
	// "turret turn" rate.
	void Aim(const Ship &ship, double amount);
	// Fire this weapon. If it is a turret, it automatically points toward
	// the given ship's target. If the weapon requires ammunition, it will
	// be subtracted from the given ship.
	void Fire(Ship &ship, std::vector<Projectile> &projectiles, std::vector<Visual> &visuals);
	// Fire an anti-missile. Returns true if the missile should be killed.
	bool FireAntiMissile(Ship &ship, const Projectile &projectile, std::vector<Visual> &visuals);
	// Fire a tractor beam. Returns true if the flotsam was hit.
	bool FireTractorBeam(Ship &ship, const Flotsam &flotsam, std::vector<Visual> &visuals);
	// This weapon jammed. Increase its reload counters, but don't fire.
	void Jam();

	// Install a weapon here (assuming it is empty). This is only for
	// Armament to call internally.
	void Install(const Outfit *outfit);
	// Reload this weapon.
	void Reload();
	// Uninstall the outfit from this port (if it has one).
	void Uninstall();

	// Get the attributes that can be used as a parameter of the constructor when cloning this.
	const BaseAttributes &GetBaseAttributes() const;


private:
	// Check whether a projectile or flotsam is within the range of the anti-missile
	// or tractor beam system and create visuals if it is.
	bool FireSpecialSystem(Ship &ship, const Body &body, std::vector<Visual> &visuals);
	// Reset the reload counters and expend ammunition, if any.
	void Fire(Ship &ship, const Point &start, const Angle &aim);

	// The arc depends on both the base hardpoint and the installed outfit.
	void UpdateArc(bool isNewlyConstructed = false);


private:
	// The Outfit installed in this hardpoint is guaranteed to have a Weapon after GameData::FinishLoading.
	const Outfit *outfit = nullptr;
	// Hardpoint location, in world coordinates relative to the ship's center.
	Point point;
	// Angle of firing direction (guns) or idle position (turret).
	Angle baseAngle;
	// Range over which the turret can turn, from leftmost position to rightmost position if this is a directional turret,
	// otherwise a pair of 180 degrees + baseAngle.
	Angle minArc;
	Angle maxArc;
	// The base attributes of a hardpoint, without considering additional limitations of the installed outfit.
	BaseAttributes baseAttributes;
	// This hardpoint is for a turret or a gun.
	bool isTurret = false;
	// Indicates if this hardpoint disallows converging (guns only).
	bool isParallel = false;
	// Indicates if this hardpoint is omnidirectional (turret only).
	bool isOmnidirectional = true;

	// Angle adjustment for convergence.
	Angle angle;
	// Reload timers and other attributes.
	double reload = 0.;
	double burstReload = 0.;
	int burstCount = 0;
	bool isFiring = false;
	bool wasFiring = false;
};
