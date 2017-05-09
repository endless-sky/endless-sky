/* Hardpoint.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Hardpoint.h"

#include "Armament.h"
#include "Audio.h"
#include "Effect.h"
#include "Outfit.h"
#include "pi.h"
#include "Projectile.h"
#include "Personality.h"
#include "Random.h"
#include "Ship.h"

#include <cmath>
#include <map>

using namespace std;

namespace {
	// Create all the effects in the given list, at the given location, velocity, and angle.
	void CreateEffects(const map<const Effect *, int> &m, Point pos, Point vel, Angle angle, list<Effect> &effects)
	{
		for(const auto &it : m)
			for(int i = 0; i < it.second; ++i)
			{
				effects.push_back(*it.first);
				effects.back().Place(pos, vel, angle);
			}
	}
}



// Constructor.
Hardpoint::Hardpoint(const Point &point, bool isTurret, const Outfit *outfit)
	: outfit(outfit), point(point * .5), isTurret(isTurret)
{
}



// Get the weapon in this hardpoint. This returns null if there is none.
const Outfit *Hardpoint::GetOutfit() const
{
	return outfit;
}



// Get the location, relative to the center of the ship, from which
// projectiles of this weapon should originate.
const Point &Hardpoint::GetPoint() const
{
	return point;
}



// Get the convergence angle adjustment of this weapon (guns only, not turrets).
const Angle &Hardpoint::GetAngle() const
{
	return angle;
}



// Find out if this is a turret hardpoint (whether or not it has a turret installed).
bool Hardpoint::IsTurret() const
{
	return isTurret;
}



// Find out if this hardpoint has a homing weapon installed.
bool Hardpoint::IsHoming() const
{
	return outfit && outfit->Homing();
}



// Find out if this hardpoint has an anti-missile installed.
bool Hardpoint::IsAntiMissile() const
{
	return outfit && outfit->AntiMissile() > 0;
}



// Check if this weapon is ready to fire.
bool Hardpoint::IsReady() const
{
	return outfit && burstReload <= 0. && burstCount;
}



// Check if this weapon fired the last time it was able to fire. This is to
// figure out if the stream spacing timer should be applied or not.
bool Hardpoint::WasFiring() const
{
	return wasFiring;
}



// Get the number of remaining burst shots before a full reload is required.
int Hardpoint::BurstRemaining() const
{
	return burstCount;
}



// Perform one step (i.e. decrement the reload count).
void Hardpoint::Step()
{
	if(!outfit)
		return;
	
	wasFiring = isFiring;
	if(reload > 0.)
		--reload;
	// If the full reload time is elapsed, reset the burst counter.
	if(reload <= 0.)
		burstCount = outfit->BurstCount();
	if(burstReload > 0.)
		--burstReload;
	// If the burst reload time has elapsed, this weapon will not count as firing
	// continuously if it is not fired this frame.
	if(burstReload <= 0.)
		isFiring = false;
}



// Fire this weapon. If it is a turret, it automatically points toward
// the given ship's target. If the weapon requires ammunition, it will
// be subtracted from the given ship.
void Hardpoint::Fire(Ship &ship, list<Projectile> &projectiles, list<Effect> &effects)
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
	
	// If this is a fixed gun, or it if is a turret but you have no target
	// selected, it should fire straight forward, angled in slightly to cause
	// the shots to converge (gun harmonization).
	if(!isTurret || !target || !target->IsTargetable())
	{
		// If this is a turret and it is not tracking a target, reset its angle
		// to the proper convergence angle.
		if(isTurret)
		{
			double d = outfit->Range();
			// Projectiles with a range of zero should fire straight forward. A
			// special check is needed to avoid divide by zero errors.
			angle = Angle(d <= 0. ? 0. : -asin(point.X() / d) * TO_DEG);
		}
	}
	else
	{
		// Take the ship's targeting confusion into account. This is mainly an
		// effect to make turrets look less unnaturally precise.
		Point p = target->Position() - start + ship.GetPersonality().Confusion();
		Point v = target->Velocity() - ship.Velocity();
		double steps = Armament::RendezvousTime(p, v, outfit->Velocity() + .5 * outfit->RandomVelocity());
		
		// Special case: RendezvousTime() may return NaN. But in that case, this
		// comparison will return false. Check to see if this turret can hit the
		// target, and if not fire straight toward where the target will be when
		// the projectile dies (to avoid over-correcting).
		if(!(steps < outfit->TotalLifetime()))
			steps = outfit->TotalLifetime();
		
		// Aim toward where the target will be at the calculated rendezvous time.
		angle = Angle(p + steps * v) - aim;
	}
	
	// Apply the aim and hardpoint offset.
	aim += angle;
	start += outfit->HardpointOffset() * aim.Unit();
	
	// Create a new projectile, originating from this hardpoint.
	projectiles.emplace_back(ship, start, aim, outfit);
	
	// Create any effects this weapon creates when it is fired.
	CreateEffects(outfit->FireEffects(), start, ship.Velocity(), aim, effects);
	
	// Update the reload and burst counters, and expend ammunition if applicable.
	Fire(ship, start, aim);
}



// Fire an anti-missile. Returns true if the missile should be killed.
bool Hardpoint::FireAntiMissile(Ship &ship, const Projectile &projectile, list<Effect> &effects)
{
	// Make sure this hardpoint really is an anti-missile.
	int strength = outfit->AntiMissile();
	if(!strength)
		return false;
	
	// Get the anti-missile range. Anti-missile shots always last a single frame,
	// so their range is equal to their velocity.
	double range = outfit->Velocity();
	
	// Check if the missile is within range of this hardpoint.
	Point start = ship.Position() + ship.Facing().Rotate(point);
	Point offset = projectile.Position() - start;
	if(offset.Length() > range)
		return false;
	
	// Firing effects are displayed at the anti-missile hardpoint that just fired.
	Angle aim(offset);
	angle = aim - ship.Facing();
	start += outfit->HardpointOffset() * aim.Unit();
	CreateEffects(outfit->FireEffects(), start, ship.Velocity(), aim, effects);
	
	// Figure out where the effect should be placed. Anti-missiles do not create
	// projectiles; they just create a blast animation.
	CreateEffects(outfit->HitEffects(), start + (.5 * range) * aim.Unit(), ship.Velocity(), aim, effects);
	
	// Die effects are displayed at the projectile, whether or not it actually "dies."
	CreateEffects(outfit->DieEffects(), projectile.Position(), projectile.Velocity(), aim, effects);
	
	// Update the reload and burst counters, and expend ammunition if applicable.
	Fire(ship, start, aim);
	
	// Check whether the missile was destroyed.
	return (Random::Int(strength) > Random::Int(projectile.MissileStrength()));
}



// Install a weapon here (assuming it is empty). This is only for
// Armament to call internally.
void Hardpoint::Install(const Outfit *outfit)
{
	// If the given outfit is not a valid weapon, this hardpoint becomes empty.
	// Otherwise, check that the type of the weapon (gun or turret) is right.
	if(!outfit || !outfit->IsWeapon())
		Uninstall();
	else if(!outfit->Get("turret mounts") || isTurret)
	{
		// Reset all the reload counters.
		this->outfit = outfit;
		reload = 0.;
		burstReload = 0.;
		burstCount = outfit->BurstCount();
		
		// Find the point of convergence of shots fired from this gun. That is,
		// find the angle where the projectile's X offset will be zero when it
		// reaches the very end of its range.
		double d = outfit->Range();
		// Projectiles with a range of zero should fire straight forward. A
		// special check is needed to avoid divide by zero errors.
		angle = Angle(d <= 0. ? 0. : -asin(point.X() / d) * TO_DEG);
	}
}



// Uninstall the outfit from this port (if it has one).
void Hardpoint::Uninstall()
{
	outfit = nullptr;
}



// Update any counters that change when this projectile fires.
void Hardpoint::Fire(Ship &ship, const Point &start, const Angle &aim)
{
	// Since this is only called internally, it is safe to assume that the
	// outfit pointer is not null.
	
	// Reset the reload count.
	reload += outfit->Reload();
	burstReload += outfit->BurstReload();
	--burstCount;
	isFiring = true;
	
	// Anti-missile sounds can be specified either in the outfit itself or in
	// the effect they create.
	if(outfit->WeaponSound())
		Audio::Play(outfit->WeaponSound(), start);
	// Apply any "kick" from firing this weapon.
	double force = outfit->FiringForce();
	if(force)
		ship.ApplyForce(aim.Unit() * -force);
	
	// Expend any ammo that this weapon uses. Do this as the very last thing, in
	// case the outfit is its own ammunition.
	ship.ExpendAmmo(outfit);
}
