/* Armament.h
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

#ifndef ARMAMENT_H_
#define ARMAMENT_H_

#include "Hardpoint.h"

#include <map>
#include <set>
#include <vector>

class FireCommand;
class Outfit;
class Point;
class Projectile;
class Ship;
class Visual;



// This class handles the logic for a ship's set of weapons. All weapons of the
// same type coordinate their fire with each other, either firing in clusters
// (if the projectiles are vulnerable to anti-missile) or in a "stream" where
// the guns take turns firing. Instead of firing straight, guns (that is, non-
// turreted weapons) fire aimed slightly inward in a convergence pattern so
// that even if the guns are spaced out horizontally on the ship, their
// projectiles will nearly meet at the end of their range. This class also
// handles turrets, which aim automatically and take into account the target's
// distance away and velocity relative to the ship that is firing.
class Armament {
public:
	Armament() = default;
	Armament(const Armament &other);
	Armament &operator=(const Armament &other);
	Armament(Armament &&other) = default;
	Armament &operator=(Armament &&other) = default;

	// Add a gun or turret hard-point.
	void AddGunPort(const Point &point, const Angle &angle, bool isParallel, bool isUnder, const Outfit *outfit = nullptr);
	void AddTurret(const Point &point, bool isUnder, const Outfit *outfit = nullptr);
	// This must be called after all the outfit data is loaded. If you add more
	// of a given weapon than there are slots for it, the extras will not fire.
	// But, the "gun ports" attribute should keep that from happening. To
	// remove a weapon, just pass a negative value here.
	int Add(const Outfit *outfit, int count = 1);
	// Call this once all the outfits have been loaded to make sure they are all
	// set up properly (even the ones that were pre-assigned to a hardpoint).
	void FinishLoading();
	// Reload all weapons (because a day passed in-game).
	void ReloadAll();
	// Uninstall all weapons (because the weapon outfits have potentially changed).
	void UninstallAll();

	// Swap the weapons in the given two hardpoints.
	void Swap(int first, int second);

	// Access the array of weapon hardpoints.
	const std::vector<Hardpoint> &Get() const;
	const std::vector<Hardpoint *> &NonAMWeapons() const;
	const std::vector<Hardpoint *> &TurrettedWeapons() const;
	const std::vector<Hardpoint *> &FixedWeapons() const;
	const std::vector<Hardpoint *> &AntiMissileWeapons() const;
	// Get the index of the given hardpoint.
	int WeaponIndex(const Hardpoint &hardpoint) const;

	// Get actual min/max range of the mounted weapons
	std::pair<double, double> GetMinMaxRange() const;
	// Get max range of turrets only weapons
	double GetTurretsMaxRange() const;

	int GunCount() const;
	int TurretCount() const;
	// Determine the ammunition used by this armament that can be resupplied (i.e. is not self-uninstalling).
	std::set<const Outfit *> RestockableAmmo() const;

	// Adjust the aim of the turrets.
	void Aim(const FireCommand &command);
	// Fire the given weapon, if it is ready. If it did not fire because it is
	// not ready, return false.
	void Fire(int index, Ship &ship, std::vector<Projectile> &projectiles, std::vector<Visual> &visuals, bool jammed);
	// Fire the given anti-missile system.
	bool FireAntiMissile(int index, Ship &ship, const Projectile &projectile, std::vector<Visual> &visuals, bool jammed);

	// Update the reload counters.
	void Step(const Ship &ship);


private:
	void RecreateViewsAndRanges();


private:
	// Notes:
	// * the Armament must be copied when an instance of a Ship is made, so
	//   it should not hold any pointers specific to one ship (including to
	//   elements of this Armament itself)
	// * the above holds for views because their elements must refer to
	//   those on the newly created copy
	std::map<const Outfit *, int> streamReload;
	std::vector<Hardpoint> hardpoints;
	// 'Views' useful to iterate over specific categories, skipping empty hardpoints
	// Note: turrettedWeapons and fixedWeapons are mutually exclusive
	std::vector<Hardpoint *> nonAMWeapons;
	std::vector<Hardpoint *> turrettedWeapons;
	std::vector<Hardpoint *> fixedWeapons;
	std::vector<Hardpoint *> antiMissileWeapons;

	// Global ranges of actual configuration (excluding AntiMissiles)
	double minRange = 0.;
	double maxRange = 0.;
	// Max ranges of turrets (excluding AntiMissiles)
	double maxTurretsRange = 0.;
};



#endif
