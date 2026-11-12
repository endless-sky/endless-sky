/* Armament.cpp
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

#include "Armament.h"

#include "FireCommand.h"
#include "Logger.h"
#include "Outfit.h"
#include "Ship.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;



// Add a gun hardpoint (fixed-direction weapon).
void Armament::AddGunPort(const Point &point, const Hardpoint::BaseAttributes &attributes, const Outfit *outfit)
{
	hardpoints.emplace_back(point, attributes, false, outfit);
}



// Add a turret hardpoint.
void Armament::AddTurret(const Point &point, const Hardpoint::BaseAttributes &attributes, const Outfit *outfit)
{
	hardpoints.emplace_back(point, attributes, true, outfit);
}



// This must be called after all the outfit data is loaded. If you add more
// of a given weapon than there are slots for it, the extras will not fire.
// But, the "gun ports" attribute should keep that from happening.
int Armament::Add(const Outfit *outfit, int count)
{
	// Make sure this really is a weapon.
	if(!count || !outfit)
		return 0;
	const Weapon *weapon = outfit->GetWeapon().get();
	if(!weapon)
		return 0;

	int existing = 0;
	int added = 0;
	bool isTurret = outfit->Get("turret mounts");
	// Do not equip weapons that do not define how they are mounted.
	if(!isTurret && !outfit->Get("gun ports"))
	{
		Logger::Log("Skipping unmountable outfit \"" + outfit->TrueName() + "\"."
			" Weapon outfits must specify either \"gun ports\" or \"turret mounts\".",
			Logger::Level::ERROR);
		return 0;
	}

	// To start out with, check how many instances of this weapon are already
	// installed. If "adding" a negative number of outfits, remove the installed
	// instances until the given number have been removed.
	for(Hardpoint &hardpoint : hardpoints)
	{
		if(hardpoint.GetOutfit() == outfit)
		{
			// If this slot has the given outfit in it and we need to uninstall
			// some of them, uninstall it. Otherwise, remember the fact that one
			// of these outfits is installed.
			if(count < 0)
			{
				hardpoint.Uninstall();
				++count;
				--added;
			}
			else
				++existing;
		}
		else if(!hardpoint.GetOutfit() && hardpoint.IsTurret() == isTurret)
		{
			// If this is an empty, compatible slot, and we're adding outfits,
			// install one of them here and decrease the count of how many we
			// have left to install.
			if(count > 0)
			{
				hardpoint.Install(outfit);
				--count;
				++added;
			}
		}
	}

	// If a stream counter already exists for this outfit (because we did not
	// just add the first one or remove the last one), do nothing.
	if(!existing)
	{
		// If this weapon is streamed, create a stream counter. If it is not
		// streamed, or if the last of this weapon has been uninstalled, erase the
		// stream counter (if there is one).
		if(added > 0 && weapon->IsStreamed())
			streamReload[outfit] = 0;
		else
			streamReload.erase(outfit);
	}
	return added;
}



// Call this once all the outfits have been loaded to make sure they are all
// set up properly (even the ones that were pre-assigned to a hardpoint).
void Armament::FinishLoading()
{
	for(Hardpoint &hardpoint : hardpoints)
		if(hardpoint.GetOutfit())
			hardpoint.Install(hardpoint.GetOutfit());

	ReloadAll();
}



// Reload all weapons (because a day passed in-game).
void Armament::ReloadAll()
{
	streamReload.clear();
	for(Hardpoint &hardpoint : hardpoints)
		if(hardpoint.GetOutfit())
		{
			hardpoint.Reload();

			// If this weapon is streamed, create a stream counter.
			const Outfit *outfit = hardpoint.GetOutfit();
			if(outfit->GetWeapon()->IsStreamed())
				streamReload[outfit] = 0;
		}
}



// Uninstall all weapons (because the weapon outfits have potentially changed).
void Armament::UninstallAll()
{
	for(Hardpoint &hardpoint : hardpoints)
		hardpoint.Uninstall();
}



// Swap the weapons in the given two hardpoints.
void Armament::Swap(unsigned first, unsigned second)
{
	// Make sure both of the given indices are in range, and that both slots are
	// the same type (gun vs. turret).
	if(first >= hardpoints.size())
		return;
	if(second >= hardpoints.size())
		return;
	if(hardpoints[first].IsTurret() != hardpoints[second].IsTurret())
		return;

	// Swap the weapons in the two hardpoints.
	const Outfit *outfit = hardpoints[first].GetOutfit();
	hardpoints[first].Install(hardpoints[second].GetOutfit());
	hardpoints[second].Install(outfit);
}



// Access the array of weapon hardpoints.
const vector<Hardpoint> &Armament::Get() const
{
	return hardpoints;
}



// Determine how many fixed gun hardpoints are on this ship.
int Armament::GunCount() const
{
	return hardpoints.size() - TurretCount();
}



// Determine how many turret hardpoints are on this ship.
int Armament::TurretCount() const
{
	int count = 0;
	for(const Hardpoint &hardpoint : hardpoints)
		count += hardpoint.IsTurret();
	return count;
}



// Determine the installed weaponry's reusable ammunition. That is, all ammo outfits that are not also
// weapons (as then they would be installed on hardpoints, like the "Nuclear Missile" and other one-shots).
set<const Outfit *> Armament::RestockableAmmo() const
{
	auto restockable = set<const Outfit *>{};
	for(const Hardpoint &hardpoint : hardpoints)
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(weapon)
		{
			const Outfit *ammo = weapon->Ammo();
			if(ammo && !ammo->GetWeapon())
				restockable.emplace(ammo);
		}
	}
	return restockable;
}



// Adjust the aim of the turrets.
void Armament::Aim(const Ship &ship, const FireCommand &command)
{
	for(unsigned i = 0; i < hardpoints.size(); ++i)
		hardpoints[i].Aim(ship, command.Aim(i));
}



// Fire the given weapon, if it is ready.
void Armament::Fire(unsigned index, Ship &ship, vector<Projectile> &projectiles, vector<Visual> &visuals, bool jammed)
{
	// Don't check if the hardpoint jammed here, as the weapon may not even
	// attempt to fire due to stream reloading.
	if(!CheckHardpoint(index))
		return;

	// A weapon that has already started a burst ignores stream timing.
	if(!hardpoints[index].WasFiring())
	{
		auto it = streamReload.find(hardpoints[index].GetOutfit());
		if(it != streamReload.end())
		{
			if(it->second > 0)
				return;
			it->second += it->first->GetWeapon()->Reload() * hardpoints[index].BurstRemaining();
		}
	}
	if(jammed)
		hardpoints[index].Jam();
	else
		hardpoints[index].Fire(ship, projectiles, visuals);
}



bool Armament::FireAntiMissile(unsigned index, Ship &ship, const Projectile &projectile,
	vector<Visual> &visuals, bool jammed)
{
	if(!CheckHardpoint(index, jammed))
		return false;

	return hardpoints[index].FireAntiMissile(ship, projectile, visuals);
}



bool Armament::FireTractorBeam(unsigned index, Ship &ship, const Flotsam &flotsam,
	vector<Visual> &visuals, bool jammed)
{
	if(!CheckHardpoint(index, jammed))
		return false;

	return hardpoints[index].FireTractorBeam(ship, flotsam, visuals);
}



// Update the reload counters.
void Armament::Step(const Ship &ship)
{
	for(Hardpoint &hardpoint : hardpoints)
		hardpoint.Step();

	for(auto &it : streamReload)
	{
		int count = ship.OutfitCount(it.first);
		it.second -= count;
		// Always reload to the quickest firing interval.
		it.second = max(it.second, 1 - count);
	}
}



bool Armament::CheckHardpoint(unsigned index, bool jammed)
{
	if(index >= hardpoints.size() || !hardpoints[index].IsReady())
		return false;

	if(jammed)
	{
		hardpoints[index].Jam();
		return false;
	}

	return true;
}
