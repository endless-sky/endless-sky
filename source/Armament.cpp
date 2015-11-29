/* Armament.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Armament.h"

#include "Audio.h"
#include "Effect.h"
#include "Outfit.h"
#include "pi.h"
#include "Projectile.h"
#include "Random.h"
#include "Ship.h"

#include <cmath>
#include <limits>

using namespace std;



// Members of Armament::Weapon:

// Constructor.
Armament::Weapon::Weapon(const Point &point, bool isTurret, const Outfit *outfit)
	: outfit(outfit), point(point * .5), reload(0), isTurret(isTurret)
{
}



// Don't call this without checking HasOutfit()!
const Outfit *Armament::Weapon::GetOutfit() const
{
	return outfit;
}



// Get the point, in ship image coordinates, from which projectiles of
// this weapon should originate.
const Point &Armament::Weapon::GetPoint() const
{
	return point;
}



// Get the convergence angle adjustment of this weapon.
const Angle &Armament::Weapon::GetAngle() const
{
	return angle;
}



// Shortcuts for querying weapon characteristics.
bool Armament::Weapon::IsTurret() const
{
	return isTurret;
}



bool Armament::Weapon::IsHoming() const
{
	return outfit && outfit->Homing();
}



bool Armament::Weapon::IsAntiMissile() const
{
	return outfit && outfit->AntiMissile() > 0;
}



// Check if this weapon is ready to fire.
bool Armament::Weapon::IsReady() const
{
	return outfit && reload <= 0;
}



// Perform one step (i.e. decrement the reload count).
void Armament::Weapon::Step()
{
	if(reload)
		--reload;
}



// Fire this weapon. If it is a turret, it automatically points toward
// the given ship's target. If the weapon requires ammunition, it will
// be subtracted from the given ship.
void Armament::Weapon::Fire(Ship &ship, list<Projectile> &projectiles, std::list<Effect> &effects)
{
	// Since this is only called internally by Armament (no one else has non-
	// const access), assume Armament checked that this is a valid call.
	Angle aim = ship.Facing();
	
	// Get projectiles to start at the right position. They are drawn at an
	// offset of (.5 * velocity) and that velocity includes the velocity of the
	// ship that fired them.
	Point start = ship.Position() + aim.Rotate(point) - .5 * ship.Velocity();
	
	shared_ptr<const Ship> target = ship.GetTargetShip();
	// If you are boarding your target, do not fire on it.
	if(ship.IsBoarding() || ship.Commands().Has(Command::BOARD))
		target.reset();
	
	if(!isTurret || !target || target->GetSystem() != ship.GetSystem())
		aim += angle;
	else
	{
		Point p = target->Position() - start + ship.GetPersonality().Confusion();
		Point v = target->Velocity() - ship.Velocity();
		double steps = RendezvousTime(p, v, outfit->Velocity());
		
		// Special case: RendezvousTime() may return NaN. But in that case, this
		// comparison will return false.
		if(!(steps < outfit->TotalLifetime()))
			steps = outfit->TotalLifetime();
		
		p += steps * v;
		
		aim = Angle(TO_DEG * atan2(p.X(), -p.Y()));
	}
	
	projectiles.emplace_back(ship, start, aim, outfit);
	if(outfit->WeaponSound())
		Audio::Play(outfit->WeaponSound(), start);
	double force = outfit->FiringForce();
	if(force)
		ship.ApplyForce(aim.Unit() * -force);
	
	for(const auto &eit : outfit->FireEffects())
		for(int i = 0; i < eit.second; ++i)
		{
			effects.push_back(*eit.first);
			effects.back().Place(start, ship.Velocity(), aim);
		}
	
	// Reset the reload count.
	reload += outfit->Reload();
	ship.ExpendAmmo(outfit);
}



// Fire an anti-missile. Returns true if the missile should be killed.
bool Armament::Weapon::FireAntiMissile(Ship &ship, const Projectile &projectile, list<Effect> &effects)
{
	int strength = outfit->AntiMissile();
	if(!strength)
		return false;
	
	double range = outfit->Velocity();
	
	// Check if the missile is in range.
	Point start = ship.Position() + ship.Facing().Rotate(point);
	Point offset = projectile.Position() - start;
	if(offset.Length() > range)
		return false;
	
	// Figure out where the effect should be placed. Anti-missiles do not create
	// projectiles; they just create a blast animation.
	start += (.5 * range) * offset.Unit();
	Angle aim = TO_DEG * atan2(offset.X(), -offset.Y());
	for(const auto &eit : outfit->HitEffects())
		for(int i = 0; i < eit.second; ++i)
		{
			effects.push_back(*eit.first);
			effects.back().Place(start, ship.Velocity(), aim);
		}
	
	// Reset the reload count.
	reload += outfit->Reload();
	ship.ExpendAmmo(outfit);
	
	return (Random::Int(strength) > Random::Int(projectile.MissileStrength()));
}



// Install a weapon here (assuming it is empty). This is only for
// Armament to call internally.
void Armament::Weapon::Install(const Outfit *outfit)
{
	if(!outfit || !outfit->IsWeapon())
		this->outfit = nullptr;
	else if(!outfit->Get("turret mounts") || isTurret)
	{
		this->outfit = outfit;
		
		// Find the point of convergence of shots fired from this gun.
		double d = outfit->Range();
		// The angle is therefore:
		angle = Angle(-asin(point.X() / d) * TO_DEG);
	}
}



// Uninstall the outfit from this port (if it has one).
void Armament::Weapon::Uninstall()
{
	outfit = nullptr;
}



// Members of Armament:

// Add a gun or turret hard-point.
void Armament::AddGunPort(const Point &point, const Outfit *outfit)
{
	weapons.emplace_back(point, false, outfit);
}



void Armament::AddTurret(const Point &point, const Outfit *outfit)
{
	weapons.emplace_back(point, true, outfit);
}



// This must be called after all the outfit data is loaded. If you add more
// of a given weapon than there are slots for it, the extras will not fire.
// But, the "gun ports" attribute should keep that from happening.
void Armament::Add(const Outfit *outfit, int count)
{
	if(!count || !outfit || !outfit->IsWeapon())
		return;
	
	int installed = 0;
	bool isTurret = outfit->Get("turret mounts");
	
	if(count < 0)
	{
		// Look for slots where this weapon is installed.
		for(Weapon &weapon : weapons)
			if(weapon.GetOutfit() == outfit)
			{
				weapon.Uninstall();
				if(--installed == count)
					break;
			}
	}
	else
	{
		// Look for empty, compatible slots.
		for(Weapon &weapon : weapons)
			if(!weapon.GetOutfit() && weapon.IsTurret() == isTurret)
			{
				weapon.Install(outfit);
				if(++installed == count)
					break;
			}
	}
	
	// If this weapon is streamed, create a stream counter. Missiles and anti-
	// missiles do not stream.
	if(!outfit->MissileStrength() && !outfit->AntiMissile())
	{
		auto it = streamReload.find(outfit);
		if(it == streamReload.end())
			streamReload[outfit] = count;
		else
		{
			it->second += count;
			if(!it->second)
				streamReload.erase(it);
		}
	}
}



// Call this once all the outfits have been loaded to make sure they are all
// set up properly (even the ones that were pre-assigned to a hardpoint).
void Armament::FinishLoading()
{
	streamReload.clear();
	for(Weapon &weapon : weapons)
		if(weapon.GetOutfit())
		{
			const Outfit *outfit = weapon.GetOutfit();
			
			// Make sure the firing angle is set properly.
			weapon.Install(outfit);
			// If this weapon is streamed, create a stream counter.
			// Missiles and anti-missiles do not stream.
			if(!outfit->MissileStrength() && !outfit->AntiMissile())
				++streamReload[outfit];
		}
}



// Swap the weapons in the given two hardpoints.
void Armament::Swap(int first, int second)
{
	if(static_cast<unsigned>(first) >= weapons.size())
		return;
	if(static_cast<unsigned>(second) >= weapons.size())
		return;
	if(weapons[first].IsTurret() != weapons[second].IsTurret())
		return;
	
	const Outfit *outfit = weapons[first].GetOutfit();
	weapons[first].Install(weapons[second].GetOutfit());
	weapons[second].Install(outfit);
}



// Access the array of weapons.
const vector<Armament::Weapon> &Armament::Get() const
{
	return weapons;
}



int Armament::GunCount() const
{
	return weapons.size() - TurretCount();
}



int Armament::TurretCount() const
{
	int count = 0;
	for(const Weapon &weapon : weapons)
		count += weapon.IsTurret();
	return count;
}



// Fire the given weapon, if it is ready. If it did not fire because it is
// not ready, return false.
void Armament::Fire(int index, Ship &ship, list<Projectile> &projectiles, std::list<Effect> &effects)
{
	if(static_cast<unsigned>(index) >= weapons.size() || !weapons[index].IsReady())
		return;
	
	auto it = streamReload.find(weapons[index].GetOutfit());
	if(it != streamReload.end() && it->second > 0)
		return;
	
	weapons[index].Fire(ship, projectiles, effects);
	if(it != streamReload.end())
		it->second += it->first->Reload();
}



bool Armament::FireAntiMissile(int index, Ship &ship, const Projectile &projectile, list<Effect> &effects)
{
	if(static_cast<unsigned>(index) >= weapons.size() || !weapons[index].IsReady())
		return false;
	
	return weapons[index].FireAntiMissile(ship, projectile, effects);
}



// Update the reload counters.
void Armament::Step(const Ship &ship)
{
	for(Weapon &weapon : weapons)
		weapon.Step();
	
	for(auto &it : streamReload)
	{
		int count = ship.OutfitCount(it.first);
		it.second -= count;
		// Always reload to the quickest firing interval.
		it.second = max(it.second, 1 - count);
	}
}



// Get the amount of time it would take the given weapon to reach the given
// target, assuming it can be fired in any direction (i.e. turreted). For
// non-turreted weapons this can be used to calculate the ideal direction to
// point the ship in.
double Armament::RendezvousTime(const Point &p, const Point &v, double vp)
{
	// How many steps will it take this projectile
	// to intersect the target?
	// (p.x + v.x*t)^2 + (p.y + v.y*t)^2 = vp^2*t^2
	// p.x^2 + 2*p.x*v.x*t + v.x^2*t^2
	//    + p.y^2 + 2*p.y*v.y*t + v.y^2t^2
	//    - vp^2*t^2 = 0
	// (v.x^2 + v.y^2 - vp^2) * t^2
	//    + (2 * (p.x * v.x + p.y * v.y)) * t
	//    + (p.x^2 + p.y^2) = 0
	double a = v.Dot(v) - vp * vp;
	double b = 2. * p.Dot(v);
	double c = p.Dot(p);
	double discriminant = b * b - 4 * a * c;
	if(discriminant < 0.)
		return numeric_limits<double>::quiet_NaN();

	discriminant = sqrt(discriminant);

	// The solutions are b +- discriminant.
	// But it's not a solution if it's negative.
	double r1 = (-b + discriminant) / (2. * a);
	double r2 = (-b - discriminant) / (2. * a);
	if(r1 >= 0. && r2 >= 0.)
		return min(r1, r2);
	else if(r1 >= 0. || r2 >= 0.)
		return max(r1, r2);

	return numeric_limits<double>::quiet_NaN();
}
