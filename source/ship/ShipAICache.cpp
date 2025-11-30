/* ShipAICache.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipAICache.h"

#include "../Armament.h"
#include "../Outfit.h"
#include "../pi.h"
#include "../Ship.h"
#include "../Weapon.h"

#include <algorithm>
#include <cmath>

using namespace std;



void ShipAICache::Calibrate(const Ship &ship)
{
	mass = ship.Mass();
	hasWeapons = false;
	canFight = false;
	double totalDPS = 0.;
	double splashDPS = 0.;
	double artilleryDPS = 0.;
	turretRange = 0.;
	gunRange = 0.;

	shortestRange = 4000.;
	shortestArtillery = 4000.;
	minSafeDistance = 0.;

	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(!weapon || hardpoint.IsSpecial())
			continue;

		hasWeapons = true;
		bool lackingAmmo = (weapon->Ammo() && weapon->AmmoUsage() && !ship.OutfitCount(weapon->Ammo()));
		// Weapons without ammo might as well not exist, so don't even consider them
		if(lackingAmmo)
			continue;
		canFight = true;

		// Calculate the damage per second,
		// ignoring any special effects. (could be improved to account for those, maybe be based on cost instead)
		double DPS = (weapon->ShieldDamage() + weapon->HullDamage()
			+ (weapon->RelativeShieldDamage() * ship.MaxShields())
			+ (weapon->RelativeHullDamage() * ship.MaxHull()))
			/ weapon->Reload();
		totalDPS += DPS;

		// Exploding weaponry that can damage this ship requires special consideration.
		if(weapon->SafeRange())
		{
			minSafeDistance = max(weapon->SafeRange(), minSafeDistance);
			splashDPS += DPS;
		}

		// The artillery AI should be applied at 1000 pixels range, or 500 if the weapon is homing.
		double range = weapon->Range();
		shortestRange = min(range, shortestRange);
		if(range >= 1000. || (weapon->Homing() && range >= 500.))
		{
			shortestArtillery = min(range, shortestArtillery);
			artilleryDPS += DPS;
		}
	}

	// Calculate this ship's "turning radius"; that is, the smallest circle it
	// can make while at full speed.
	double stepsInHalfTurn = 180. / ship.TurnRate();
	double circumference = stepsInHalfTurn * ship.MaxVelocity();
	maxTurningRadius = circumference / PI;

	// If this ship was using the artillery AI to run away and bombard its
	// target from a distance, have it stop running once it is out of ammo. This
	// is not realistic, but it's less annoying for the player.
	if(hasWeapons && !canFight && !ship.IsYours())
	{
		shortestRange = 0.;
		shortestArtillery = 0.;
	}
	else if(hasWeapons)
	{
		// Artillery AI is the AI responsible for handling the behavior of missile boats
		// and other ships with exceptionally long range weapons such as detainers
		// The AI shouldn't use the artillery AI if it has no reverse and it's turning
		// capabilities are very bad. Otherwise it spends most of it's time flying around.
		useArtilleryAI = (artilleryDPS > totalDPS * .75
			&& (ship.MaxReverseVelocity() || maxTurningRadius < 0.2 * shortestArtillery));

		// Don't try to avoid your own splash damage if it means you would be losing out
		// on a lot of DPS. Helps with ships with very slow turning and not a lot of splash
		// weapons being overly afraid of dying.
		if(minSafeDistance && !(useArtilleryAI || shortestRange * (splashDPS / totalDPS) > maxTurningRadius))
			minSafeDistance = 0.;
	}
	// Get the weapon ranges for this ship, so the AI can call it.
	for(const auto &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(!weapon || hardpoint.IsSpecial())
			continue;
		if((weapon->Ammo() && !ship.OutfitCount(weapon->Ammo())) || !weapon->DoesDamage())
			continue;
		double weaponRange = weapon->Range() + hardpoint.GetPoint().Length();
		if(hardpoint.IsTurret())
			turretRange = max(turretRange, weaponRange);
		else
			gunRange = max(gunRange, weaponRange);
	}
}



void ShipAICache::Recalibrate(const Ship &ship)
{
	if(mass != ship.Mass())
		Calibrate(ship);
}
