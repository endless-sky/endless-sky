/* ShipAICache->cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipAICache.h"

#include "../Ship.h"
#include "../pi.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Constructor.
ShipAICache::ShipAICache(const Ship &ship)
{
	this->ship = &ship;
	UpdateWeaponCache();
}



void ShipAICache::UpdateWeaponCache()
{
	bool hasAmmo = false;
	bool isArmed = false;
	double totalSpace = 0.;
	double splashSpace = 0.;
	double rangedSpace = 0.;
	
	shortestRange = 1000.;
	shortestArtillery = 4000.;
	minSafeDistance = 0.;
	
	for(const Hardpoint &hardpoint : ship->Weapons())
	{
		const Outfit  *weapon = hardpoint.GetOutfit();
		if(weapon && !hardpoint.IsAntiMissile())
		{
			isArmed = true;
			bool hasThisAmmo = (!weapon->Ammo() || ship->OutfitCount(weapon->Ammo()));
			// Weapons without ammo might as well not exsit, so don't even consider them
			if(!hasThisAmmo)
				continue;
			hasAmmo |= true;
			
			// Account for weapons that may have different weapon capacity
			// usage compared to outfit space usage. Also account for any
			// "weapons" that might use engine capacity.
			double outfitSpace = (weapon->Get("outfit space") + weapon->Get("weapon capacity")
					+ weapon->Get("engine capacity")) / -2;
			totalSpace += outfitSpace;

			// Exploding weaponry that can damage this ship requires special
			// consideration (while we have the ammo to use the weapon).
			if(hasThisAmmo && weapon->SafeRange())
			{
				minSafeDistance = max(weapon->SafeRange(), minSafeDistance);
				splashSpace += outfitSpace;
			}

			// The artillery AI should be applied at 1000 pixels range.
			// regardless of wether the weapon is homing or not.
			// The AI works fine with non homing weapons
			double range = weapon->Range();
			shortestRange = min(range, shortestRange);
			if(range > 1000)
			{
				shortestArtillery = min(range, shortestArtillery);
				rangedSpace += outfitSpace;
			}
			
			
		}
	}
	
	// Calculate this ship's "turning radius"; that is, the smallest circle it
	// can make while at full speed.
	double stepsInFullTurn = 360. / ship->TurnRate();
	double circumference = stepsInFullTurn * ship->Velocity().Length();
	turningRadius = circumference / PI;
	
	// If this ship was using the missile boat AI to run away and bombard its
	// target from a distance, have it stop running once it is out of ammo. This
	// is not realistic, but it's a whole lot less annoying for the player when
	// they are trying to hunt down and kill the last missile boat in a fleet.
	if(isArmed && !hasAmmo)
	{
		shortestRange = 0.;
		shortestArtillery = 0.;
	}
	
	// ArtilleryAI is the AI responsible for handling the behavior of missile boats
	// and other ships with exceptionally long range weapons such as detainers
	// The AI shouldn't use artilleryAI if it has no reverse and it's turning
	// capabilities are very bad. Otherwise it spends most of it's time flying around
	if(rangedSpace > totalSpace * 0.5 && (ship->MaxReverseVelocity() ||
			turningRadius < 0.2 * shortestArtillery))
		artilleryAI = true;
	else
		artilleryAI = false;
	
	// Don't try to avoid your own splash damage if it means you whould be losing out
	// on a lot of DPS. Helps with ships with very slow turning and not a lot of splash
	// weapons being overly afraid of dying.
	if(minSafeDistance && !(artilleryAI || shortestRange * splashSpace / totalSpace
			> turningRadius))
		minSafeDistance = 0.;
}
