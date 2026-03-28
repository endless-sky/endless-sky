/* Hardpoint.cpp
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

#include "Hardpoint.h"

#include "audio/Audio.h"
#include "Body.h"
#include "Effect.h"
#include "Flotsam.h"
#include "Outfit.h"
#include "pi.h"
#include "Projectile.h"
#include "Random.h"
#include "Ship.h"
#include "Visual.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <map>

using namespace std;

namespace {
	// Create all the effects in the given list, at the given location, velocity, and angle.
	void CreateEffects(const map<const Effect *, int> &m, Point pos, Point vel, Angle angle, vector<Visual> &visuals)
	{
		for(const auto &it : m)
			for(int i = 0; i < it.second; ++i)
				visuals.emplace_back(*it.first, pos, vel, angle);
	}
}



// Constructor.
Hardpoint::Hardpoint(const Point &point, const BaseAttributes &attributes,
	bool isTurret, const Outfit *outfit)
	: outfit(outfit), point(point * .5),
	baseAngle(attributes.baseAngle), baseAttributes(attributes),
	isTurret(isTurret), isParallel(baseAttributes.isParallel)
{
	UpdateArc(true);
}



// Get the weapon installed in this hardpoint (or null if there is none).
// The Outfit is guaranteed to have a Weapon.
const Outfit *Hardpoint::GetOutfit() const
{
	return outfit;
}



const Weapon *Hardpoint::GetWeapon() const
{
	return outfit ? outfit->GetWeapon().get() : nullptr;
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



// Get the angle of a turret when idling, relative to the ship.
// For guns, this function is equal to GetAngle().
const Angle &Hardpoint::GetIdleAngle() const
{
	return baseAngle;
}



// Get the arc of fire if this is a directional turret,
// otherwise a pair of 180 degree + baseAngle.
const Angle &Hardpoint::GetMinArc() const
{
	return minArc;
}



const Angle &Hardpoint::GetMaxArc() const
{
	return maxArc;
}



// Get the angle this weapon ought to point at for ideal gun harmonization.
Angle Hardpoint::HarmonizedAngle() const
{
	if(!outfit)
		return Angle();

	// Calculate reference point for non-forward facing guns.
	Angle rotateAngle = Angle() - baseAngle;
	Point refPoint = rotateAngle.Rotate(point);

	// Find the point of convergence of shots fired from this gun. That is,
	// find the angle where the projectile's X offset will be zero when it
	// reaches the very end of its range.
	double d = outfit->GetWeapon()->Range();
	// Projectiles with a range of zero should fire straight forward. A
	// special check is needed to avoid divide by zero errors.
	return Angle(d <= 0. ? 0. : -asin(refPoint.X() / d) * TO_DEG);
}



double Hardpoint::TurnRate(const Ship &ship) const
{
	if(!outfit)
		return 0.;
	return outfit->GetWeapon()->TurretTurn()
		* (1. + ship.Attributes().Get("turret turn multiplier") + baseAttributes.turnMultiplier);
}



// Find out if this is a turret hardpoint (whether or not it has a turret installed).
bool Hardpoint::IsTurret() const
{
	return isTurret;
}



bool Hardpoint::IsParallel() const
{
	return isParallel;
}



bool Hardpoint::IsOmnidirectional() const
{
	return isOmnidirectional;
}



Hardpoint::Side Hardpoint::GetSide() const
{
	return baseAttributes.side;
}



// Find out if this hardpoint has a homing weapon installed.
bool Hardpoint::IsHoming() const
{
	return outfit && outfit->GetWeapon()->Homing();
}



// Find out if this hardpoint has a special weapon installed
// (e.g. anti-missile, tractor beam).
bool Hardpoint::IsSpecial() const
{
	if(!outfit)
		return false;
	const Weapon *weapon = outfit->GetWeapon().get();
	return weapon->AntiMissile() || weapon->TractorBeam();
}



bool Hardpoint::CanAim(const Ship &ship) const
{
	return TurnRate(ship);
}



// Check if this weapon is ready to fire.
bool Hardpoint::IsReady() const
{
	return outfit && burstReload <= 0. && burstCount && (!IsBlind() || IsSpecial());
}



// Check if this weapon can't fire because of its blindspots.
bool Hardpoint::IsBlind() const
{
	return any_of(baseAttributes.blindspots.begin(), baseAttributes.blindspots.end(),
		[this](pair<Angle, Angle> blindspot)
		{
			return angle.IsInRange(blindspot.first + baseAngle, blindspot.second + baseAngle);
		});
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
		burstCount = outfit->GetWeapon()->BurstCount();
	if(burstReload > 0.)
		--burstReload;
	// If the burst reload time has elapsed, this weapon will not count as firing
	// continuously if it is not fired this frame.
	if(burstReload <= 0.)
		isFiring = false;
}



// Adjust this weapon's aim by the given amount, relative to its maximum
// "turret turn" rate. Up to its angle limit.
void Hardpoint::Aim(const Ship &ship, double amount)
{
	if(!outfit)
		return;

	const double add = TurnRate(ship) * amount;
	if(isOmnidirectional)
		angle += add;
	else
	{
		const Angle newAngle = angle + add;
		if(add < 0. && minArc.IsInRange(newAngle, angle))
			angle = minArc;
		else if(add > 0. && maxArc.IsInRange(angle, newAngle))
			angle = maxArc;
		else
			angle += add;
	}
}



// Fire this weapon. If it is a turret, it automatically points toward
// the given ship's target. If the weapon requires ammunition, it will
// be subtracted from the given ship.
void Hardpoint::Fire(Ship &ship, vector<Projectile> &projectiles, vector<Visual> &visuals)
{
	// Since this is only called internally by Armament (no one else has non-
	// const access), assume Armament checked that this is a valid call.

	const Weapon *weapon = outfit->GetWeapon().get();

	Angle aim = ship.Facing();
	Point start = ship.Position() + aim.Rotate(point);

	// Apply the aim and hardpoint offset.
	aim += angle;
	start += aim.Rotate(weapon->HardpointOffset());

	// Apply the weapon's inaccuracy to the aim. This allows firing effects
	// to share the same inaccuracy as the projectile.
	aim += Distribution::GenerateInaccuracy(weapon->Inaccuracy(), weapon->InaccuracyDistribution());

	// Create a new projectile, originating from this hardpoint.
	// In order to get projectiles to start at the right position they are drawn
	// at an offset of (.5 * velocity). See BatchDrawList.cpp for more details.
	projectiles.emplace_back(ship, start - .5 * ship.Velocity(), aim, weapon);

	// Create any effects this weapon creates when it is fired.
	CreateEffects(weapon->FireEffects(), start, ship.Velocity(), aim, visuals);

	// Update the reload and burst counters, and expend ammunition if applicable.
	Fire(ship, start, aim);
}



// Fire an anti-missile. Returns true if the missile should be killed.
bool Hardpoint::FireAntiMissile(Ship &ship, const Projectile &projectile, vector<Visual> &visuals)
{
	// Make sure this hardpoint really is an anti-missile.
	int strength = outfit->GetWeapon()->AntiMissile();
	if(!strength)
		return false;

	// Check whether the projectile is within range and create any visuals.
	if(!FireSpecialSystem(ship, projectile, visuals))
		return false;

	// Check whether the missile was destroyed.
	return (Random::Int(strength) > Random::Int(projectile.MissileStrength()));
}



// Fire a tractor beam. Returns true if the flotsam was hit.
bool Hardpoint::FireTractorBeam(Ship &ship, const Flotsam &flotsam, vector<Visual> &visuals)
{
	// Make sure this hardpoint really is a tractor beam.
	double strength = outfit->GetWeapon()->TractorBeam();
	if(!strength)
		return false;

	// Check whether the flotsam is within range and create any visuals.
	if(!FireSpecialSystem(ship, flotsam, visuals))
		return false;

	return true;
}



// This weapon jammed. Increase its reload counters, but don't fire.
void Hardpoint::Jam()
{
	// Since this is only called internally by Armament (no one else has non-
	// const access), assume Armament checked that this is a valid call.

	const Weapon *weapon = outfit->GetWeapon().get();

	// Reset the reload count.
	reload += weapon->Reload();
	burstReload += weapon->BurstReload();
	--burstCount;
}



// Install a weapon here (assuming it is empty). This is only for
// Armament to call internally.
void Hardpoint::Install(const Outfit *outfit)
{
	// If the given outfit is not a valid weapon, this hardpoint becomes empty.
	// Also check that the type of the weapon (gun or turret) is right.
	if(!outfit || !outfit->GetWeapon() || (isTurret == !outfit->Get("turret mounts")))
		Uninstall();
	else
	{
		// Reset all the reload counters.
		this->outfit = outfit;
		Reload();

		// Update the arc of fire because of changing an outfit.
		UpdateArc();

		// For fixed weapons and idling turrets, apply "gun harmonization,"
		// pointing them slightly inward so the projectiles will converge.
		// Weapons that fire parallel beams don't get a harmonized angle.
		// And some hardpoints/gunslots are configured not to get harmonized.
		// So only harmonize when both the port and the outfit supports it.
		if(!isParallel && !outfit->GetWeapon()->IsParallel())
		{
			const Angle harmonized = baseAngle + HarmonizedAngle();
			// The harmonized angle might be out of the arc of a turret.
			// If so, this turret is forced "parallel."
			if(!isTurret || isOmnidirectional || harmonized.IsInRange(GetMinArc(), GetMaxArc()))
				baseAngle = harmonized;
		}
		angle = baseAngle;
	}
}



// Reload this weapon.
void Hardpoint::Reload()
{
	reload = 0.;
	burstReload = 0.;
	burstCount = outfit ? outfit->GetWeapon()->BurstCount() : 0;
}



// Uninstall the outfit from this port (if it has one).
void Hardpoint::Uninstall()
{
	outfit = nullptr;

	// Update the arc of fire because of changing an outfit.
	UpdateArc();
}



// Get the attributes that can be used as a parameter of the constructor when cloning this.
const Hardpoint::BaseAttributes &Hardpoint::GetBaseAttributes() const
{
	return baseAttributes;
}



// Check whether a projectile or flotsam is within the range of the anti-missile
// or tractor beam system and create visuals if it is.
bool Hardpoint::FireSpecialSystem(Ship &ship, const Body &body, vector<Visual> &visuals)
{
	const Weapon *weapon = outfit->GetWeapon().get();

	// Get the weapon range. Anti-missile and tractor beam shots always last a
	// single frame, so their range is equal to their velocity.
	double range = weapon->Velocity();
	Angle facing = ship.Facing();

	// Check if the body is within range of this hardpoint.
	Point start = ship.Position() + facing.Rotate(point);
	Point offset = body.Position() - start;
	if(offset.Length() > range)
		return false;

	// Check if the target is within the arc of fire and isn't blocked by a blindspot.
	Angle aim(offset);
	if(!isOmnidirectional)
	{
		const Angle minArc = GetMinArc() + facing;
		const Angle maxArc = GetMaxArc() + facing;
		if(!aim.IsInRange(minArc, maxArc))
			return false;
	}
	angle = aim - facing;
	if(IsBlind())
		return false;

	// Precompute the number of visuals that will be added.
	visuals.reserve(visuals.size() + weapon->FireEffects().size()
		+ weapon->HitEffects().size() + weapon->DieEffects().size());

	start += aim.Rotate(weapon->HardpointOffset());
	CreateEffects(weapon->FireEffects(), start, ship.Velocity(), aim, visuals);

	// Figure out where the hit effect should be placed. Anti-missile and tractor
	// beam systems do not create projectiles; they just create a blast animation.
	CreateEffects(weapon->HitEffects(), start + (.5 * range) * aim.Unit(), ship.Velocity(), aim, visuals);

	// Die effects are displayed at the body, whether or not it actually "dies."
	CreateEffects(weapon->DieEffects(), body.Position(), body.Velocity(), aim, visuals);

	// Update the reload and burst counters, and expend ammunition if applicable.
	Fire(ship, start, aim);

	return true;
}



// Update any counters that change when this projectile fires.
void Hardpoint::Fire(Ship &ship, const Point &start, const Angle &aim)
{
	// Since this is only called internally, it is safe to assume that the
	// outfit pointer is not null.

	const Weapon *weapon = outfit->GetWeapon().get();

	// Reset the reload count.
	reload += weapon->Reload();
	burstReload += weapon->BurstReload();
	--burstCount;
	isFiring = true;

	// Anti-missile sounds can be specified either in the outfit itself or in
	// the effect they create.
	const Sound *weaponSound = weapon->WeaponSound();
	if(weaponSound)
		Audio::Play(weaponSound, start, IsSpecial() ? SoundCategory::ANTI_MISSILE : SoundCategory::WEAPON);
	// Apply any "kick" from firing this weapon.
	double force = weapon->FiringForce();
	if(force)
		ship.ApplyForce(aim.Unit() * -force);

	// Expend any ammo that this weapon uses. Do this as the very last thing, in
	// case the outfit is its own ammunition.
	ship.ExpendAmmo(*weapon);
}



// The arc depends on both the base hardpoint and the installed outfit.
void Hardpoint::UpdateArc(bool isNewlyConstructed)
{
	if(!outfit)
		return;

	// Restore the initial value (from baseAttributes).
	isOmnidirectional = baseAttributes.isOmnidirectional;
	baseAngle = baseAttributes.baseAngle;
	if(isOmnidirectional)
	{
		const Angle opposite = baseAngle + Angle(180.);
		minArc = opposite;
		maxArc = opposite;
	}
	else
	{
		minArc = baseAttributes.minArc;
		maxArc = baseAttributes.maxArc;
	}

	// The installed weapon restricts the arc of fire.
	const double hardpointsArc = (maxArc - minArc).AbsDegrees();
	const double weaponsArc = isNewlyConstructed ? 360. : outfit->GetWeapon()->Arc();
	if(weaponsArc < 360. && (isOmnidirectional || weaponsArc < hardpointsArc))
	{
		isOmnidirectional = false;
		const double weaponsHalf = weaponsArc / 2.;

		// The base angle is placed as close to center as possible.
		const Angle &firstAngle = minArc;
		const Angle &secondAngle = maxArc;
		double hardpointsMinArc = (baseAngle - firstAngle).AbsDegrees();
		double hardpointsMaxArc = (secondAngle - baseAngle).AbsDegrees();
		if(hardpointsMinArc < weaponsHalf)
			hardpointsMaxArc = weaponsArc - hardpointsMinArc;
		else if(hardpointsMaxArc < weaponsHalf)
			hardpointsMinArc = weaponsArc - hardpointsMaxArc;
		else
		{
			hardpointsMinArc = weaponsHalf;
			hardpointsMaxArc = weaponsHalf;
		}
		minArc = baseAngle - hardpointsMinArc;
		maxArc = baseAngle + hardpointsMaxArc;
	}
}
