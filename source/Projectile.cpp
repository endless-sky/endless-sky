/* Projectile.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Projectile.h"

#include "Effect.h"
#include "pi.h"
#include "Random.h"
#include "Ship.h"
#include "Visual.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Given the probability of losing a lock in five tries, check randomly
	// whether it should be lost on this try.
	inline bool Check(double probability, double base)
	{
		return (Random::Real() < base * pow(probability, .2));
	}
}



Projectile::Projectile(const Ship &parent, Point position, Angle angle, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), position, parent.Velocity(), angle),
	weapon(weapon), targetShip(parent.GetTargetShip()), lifetime(weapon->Lifetime())
{
	government = parent.GetGovernment();
	
	// If you are boarding your target, do not fire on it.
	if(parent.IsBoarding() || parent.Commands().Has(Command::BOARD))
		targetShip.reset();
	
	cachedTarget = TargetPtr().get();
	if(cachedTarget)
		targetGovernment = cachedTarget->GetGovernment();
	double inaccuracy = weapon->Inaccuracy();
	if(inaccuracy)
		this->angle += Angle::Random(inaccuracy) - Angle::Random(inaccuracy);
	
	velocity += this->angle.Unit() * (weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	
	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);
}



Projectile::Projectile(const Projectile &parent, const Point &offset, const Angle &angle, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), parent.position + parent.velocity + parent.angle.Rotate(offset), parent.velocity, parent.angle + angle),
	weapon(weapon), targetShip(parent.targetShip), lifetime(weapon->Lifetime())
{
	government = parent.government;
	targetGovernment = parent.targetGovernment;
	
	cachedTarget = TargetPtr().get();
	double inaccuracy = weapon->Inaccuracy();
	if(inaccuracy)
	{
		this->angle += Angle::Random(inaccuracy) - Angle::Random(inaccuracy);
		if(!parent.weapon->Acceleration())
		{
			// Move in this new direction at the same velocity.
			double parentVelocity = parent.weapon->Velocity();
			velocity += (this->angle.Unit() - parent.angle.Unit()) * parentVelocity;
		}
	}
	velocity += this->angle.Unit() * (weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	
	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);
}



// Ship explosion.
Projectile::Projectile(Point position, const Weapon *weapon)
	: weapon(weapon)
{
	this->position = std::move(position);
}



// This returns false if it is time to delete this projectile.
void Projectile::Move(vector<Visual> &visuals, vector<Projectile> &projectiles)
{
	if(--lifetime <= 0)
	{
		if(lifetime > -100)
		{
			// This projectile died a "natural" death. Create any death effects
			// and submunitions.
			for(const auto &it : weapon->DieEffects())
				for(int i = 0; i < it.second; ++i)
					visuals.emplace_back(*it.first, position, velocity, angle);
			
			for(const auto &it : weapon->Submunitions())
				for(size_t i = 0; i < it.count; ++i)
					projectiles.emplace_back(*this, it.offset, it.facing, it.weapon);
		}
		MarkForRemoval();
		return;
	}
	for(const auto &it : weapon->LiveEffects())
		if(!Random::Int(it.second))
			visuals.emplace_back(*it.first, position, velocity, angle);
	
	// If the target has left the system, stop following it. Also stop if the
	// target has been captured by a different government.
	const Ship *target = cachedTarget;
	if(target)
	{
		target = TargetPtr().get();
		if(!target || !target->IsTargetable() || target->GetGovernment() != targetGovernment)
		{
			targetShip.reset();
			cachedTarget = nullptr;
			target = nullptr;
		}
	}
	
	double turn = weapon->Turn();
	double accel = weapon->Acceleration();
	int homing = weapon->Homing();
	if(target && homing && !Random::Int(30))
		CheckLock(*target);
	if(target && homing && hasLock)
	{
		// Vector d is the direction we want to turn towards.
		Point d = target->Position() - position;
		Point unit = d.Unit();
		double drag = weapon->Drag();
		double trueVelocity = drag ? accel / drag : velocity.Length();
		double stepsToReach = d.Length() / trueVelocity;
		bool isFacingAway = d.Dot(angle.Unit()) < 0.;
		// At the highest homing level, compensate for target motion.
		if(homing >= 4)
		{
			if(unit.Dot(target->Velocity()) < 0.)
			{
				// If the target is moving toward this projectile, the intercept
				// course is where the target and the projectile have the same
				// velocity normal to the distance between them.
				Point normal(unit.Y(), -unit.X());
				double vN = normal.Dot(target->Velocity());
				double vT = sqrt(max(0., trueVelocity * trueVelocity - vN * vN));
				d = vT * unit + vN * normal;
			}
			else
			{
				// Adjust the target's position based on where it will be when we
				// reach it (assuming we're pointed right towards it).
				d += stepsToReach * target->Velocity();
				stepsToReach = d.Length() / trueVelocity;
			}
			unit = d.Unit();
		}
		
		double cross = angle.Unit().Cross(unit);
		
		// The very dumbest of homing missiles lose their target if pointed
		// away from it.
		if(isFacingAway && homing == 1)
			targetShip.reset();
		else
		{
			double desiredTurn = TO_DEG * asin(cross);
			if(fabs(desiredTurn) > turn)
				turn = copysign(turn, desiredTurn);
			else
				turn = desiredTurn;
			
			// Levels 3 and 4 stop accelerating when facing away.
			if(homing >= 3)
			{
				double stepsToFace = desiredTurn / turn;
		
				// If you are facing away from the target, stop accelerating.
				if(stepsToFace * 1.5 > stepsToReach)
					accel = 0.;
			}
		}
	}
	// Homing weapons that have lost their lock have a chance to get confused
	// and turn in a random direction ("go haywire"). Each tracking method has
	// a different haywire condition. Weapons with multiple tracking methods
	// only go haywire if all of the tracking methods have gotten confused.
	else if(target && homing)
	{
		bool infraredConfused = true;
		bool opticalConfused = true;
		bool radarConfused = true;

		// Infrared: proportional to tracking quality.
		if(weapon->InfraredTracking())
			infraredConfused = Random::Real() > weapon->InfraredTracking();

		// Optical: proportional tracking quality.
		if(weapon->OpticalTracking())
			opticalConfused = Random::Real() > weapon->OpticalTracking();

		// Radar: If the target has no jamming, then proportional to tracking
		// quality. If the target does have jamming, then it's proportional to
		// tracking quality, the strength of target's jamming, and the distance
		// to the target (jamming power attenuates with distance).
		if(weapon->RadarTracking())
		{
			double radarTracking = weapon->RadarTracking();
			double radarJamming = target->Attributes().Get("radar jamming");
			if(!radarJamming)
				radarConfused = Random::Real() > radarTracking;
			else
				radarConfused = Random::Real() > (radarTracking * position.Distance(target->Position()))
					/ (sqrt(radarJamming) * weapon->Range());
		}
		if(infraredConfused && opticalConfused && radarConfused)
			turn = Random::Real() - min(.5, turn);
	}
	// If a weapon is homing but has no target, do not turn it.
	else if(homing)
		turn = 0.;
	
	if(turn)
		angle += Angle(turn);
	
	if(accel)
	{
		velocity *= 1. - weapon->Drag();
		velocity += accel * angle.Unit();
	}
	
	position += velocity;
	distanceTraveled += velocity.Length();
	
	// If this projectile is now within its "split range," it should split into
	// sub-munitions next turn.
	if(target && (position - target->Position()).Length() < weapon->SplitRange())
		lifetime = 0;
}



// This projectile hit something. Create the explosion, if any. This also
// marks the projectile as needing deletion.
void Projectile::Explode(vector<Visual> &visuals, double intersection, Point hitVelocity)
{
	clip = intersection;
	distanceTraveled += velocity.Length() * intersection;
	for(const auto &it : weapon->HitEffects())
		for(int i = 0; i < it.second; ++i)
		{
			visuals.emplace_back(*it.first, position + velocity * intersection, velocity, angle, hitVelocity);
		}
	lifetime = -100;
}



// Get the amount of clipping that should be applied when drawing this projectile.
double Projectile::Clip() const
{
	return clip;
}



// This projectile was killed, e.g. by an anti-missile system.
void Projectile::Kill()
{
	lifetime = 0;
}



// Find out if this is a missile, and if so, how strong it is (i.e. what
// chance an anti-missile shot has of destroying it).
int Projectile::MissileStrength() const
{
	return weapon->MissileStrength();
}



// Get information on the weapon that fired this projectile.
const Weapon &Projectile::GetWeapon() const
{
	return *weapon;
}


	
// Find out which ship this projectile is targeting.
const Ship *Projectile::Target() const
{
	return cachedTarget;
}



shared_ptr<Ship> Projectile::TargetPtr() const
{
	return targetShip.lock();
}


// TODO: add more conditions in the future. For example maybe proximity to stars
// and their brightness could could cause IR missiles to lose their locks more
// often, and dense asteroid fields could do the same for radar and optically
// guided missiles.
void Projectile::CheckLock(const Ship &target)
{
	double base = hasLock ? 1. : .15;
	hasLock = false;
	
	// For each tracking type, calculate the probability twice every second that a
	// lock will be lost.
	if(weapon->Tracking())
		hasLock |= Check(weapon->Tracking(), base);
	
	// Optical tracking is about 15% for interceptors and 75% for medium warships.
	if(weapon->OpticalTracking())
	{
		double weight = target.Mass() * target.Mass();
		double probability = weapon->OpticalTracking() * weight / (150000. + weight);
		hasLock |= Check(probability, base);
	}
	
	// Infrared tracking is 5% when heat is zero and 100% when heat is full.
	// When the missile is at under 1/3 of its maximum range, tracking is
	// linearly increased by up to a factor of 3, representing the fact that the
	// wavelengths of IR radiation are easier to distinguish at closer distances.
	if(weapon->InfraredTracking())
	{
		double distance = position.Distance(target.Position());
		double shortRange = weapon->Range() * 0.33;
		double multiplier = 1.;
		if(distance <= shortRange)
			multiplier = 2. - distance / shortRange;
		double probability = weapon->InfraredTracking() * min(1., target.Heat() * multiplier + .05);
		hasLock |= Check(probability, base);
	}
	
	// Radar tracking depends on whether the target ship has jamming capabilities.
	// The jamming effect attenuates with range, and that range is affected by
	// the power of the jamming. Jamming of 2 will cause a sidewinder fired at
	// least 1200 units away from a maneuvering target to miss about 25% of the
	// time. Jamming of 10 will increase that to about 60%.
	if(weapon->RadarTracking())
	{
		double radarJamming = target.IsDisabled() ? 0. : target.Attributes().Get("radar jamming");
		if(radarJamming)
		{
			double distance = position.Distance(target.Position());
			double jammingRange = 500. + sqrt(radarJamming) * 500.;
			double rangeFraction = min(1., distance / jammingRange);
			radarJamming = (1. - rangeFraction) * radarJamming;
		}
		double probability = weapon->RadarTracking() / (1. + radarJamming);
		hasLock |= Check(probability, base);
	}
}



double Projectile::DistanceTraveled() const
{
	return distanceTraveled;
}
