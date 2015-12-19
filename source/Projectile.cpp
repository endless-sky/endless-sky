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
#include "Mask.h"
#include "Outfit.h"
#include "pi.h"
#include "Random.h"
#include "Ship.h"
#include "Sprite.h"

#include <cmath>

using namespace std;



Projectile::Projectile(const Ship &parent, Point position, Angle angle, const Outfit *weapon)
	: weapon(weapon), animation(weapon->WeaponSprite()),
	position(position), velocity(parent.Velocity()), angle(angle),
	targetShip(parent.GetTargetShip()), government(parent.GetGovernment()),
	lifetime(weapon->Lifetime())
{
	// If you are boarding your target, do not fire on it.
	if(parent.IsBoarding() || parent.Commands().Has(Command::BOARD))
		targetShip.reset();
	
	cachedTarget = targetShip.lock().get();
	if(cachedTarget)
		targetGovernment = cachedTarget->GetGovernment();
	double inaccuracy = weapon->Inaccuracy();
	if(inaccuracy)
		this->angle += Angle::Random(inaccuracy) - Angle::Random(inaccuracy);
	
	velocity += this->angle.Unit() * weapon->Velocity();
}



Projectile::Projectile(const Projectile &parent, const Outfit *weapon)
	: weapon(weapon), animation(weapon->WeaponSprite()),
	position(parent.position + parent.velocity), velocity(parent.velocity), angle(parent.angle),
	targetShip(parent.targetShip), government(parent.government),
	targetGovernment(parent.targetGovernment), lifetime(weapon->Lifetime())
{
	cachedTarget = targetShip.lock().get();
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
	velocity += this->angle.Unit() * weapon->Velocity();
}



// Ship explosion.
Projectile::Projectile(Point position, const Outfit *weapon)
	: weapon(weapon), position(position)
{
}



// This returns false if it is time to delete this projectile.
bool Projectile::Move(list<Effect> &effects)
{
	if(--lifetime <= 0)
	{
		if(lifetime > -100)
			for(const auto &it : weapon->DieEffects())
				for(int i = 0; i < it.second; ++i)
				{
					effects.push_back(*it.first);
					effects.back().Place(position, velocity, angle);
				}
		
		return false;
	}
	
	// If the target has left the system, stop following it. Also stop if the
	// target has been captured by a different government.
	const Ship *target = cachedTarget;
	if(target)
	{
		target = targetShip.lock().get();
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
	if(target && homing)
	{
		Point d = position - target->Position();
		double drag = weapon->Drag();
		double trueVelocity = drag ? accel / drag : velocity.Length();
		double stepsToReach = d.Length() / trueVelocity;
		bool isFacingAway = d.Dot(angle.Unit()) > 0.;
		
		// At the highest homing level, compensate for target motion.
		if(homing >= 4)
		{
			// Adjust the target's position based on where it will be when we
			// reach it (assuming we're pointed right towards it).
			d -= stepsToReach * target->Velocity();
			stepsToReach = d.Length() / trueVelocity;
		}
		
		double cross = d.Unit().Cross(angle.Unit());
		
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
	// If a weapon is homing but has no target, do not turn it.
	else if(homing)
		turn = 0.;
	
	if(turn)
		angle += Angle(turn);
	
	if(accel)
	{
		velocity += accel * angle.Unit();
		velocity *= 1. - weapon->Drag();
	}
	
	position += velocity;
	
	if(target && (position - target->Position()).Length() < weapon->SplitRange() && !Random::Int(10))
		lifetime = 0;
	
	return true;
}



// This is called when a projectile "dies," either of natural causes or
// because it hit its target.
void Projectile::MakeSubmunitions(list<Projectile> &projectiles) const
{
	// Only make submunitions if you did *not* hit a target.
	if(lifetime <= -100)
		return;
	
	for(const auto &it : weapon->Submunitions())
		for(int i = 0; i < it.second; ++i)
			projectiles.emplace_back(*this, it.first);
}



// Check if this projectile collides with the given step, with the animation
// frame for the given step.
double Projectile::CheckCollision(const Ship &ship, int step) const
{
	const Mask &mask = ship.GetSprite().GetMask(step);
	Point offset = position - ship.Position();
	
	double radius = weapon->TriggerRadius();
	if(radius > 0. && mask.WithinRange(offset, angle, radius))
		return 0.;
	
	return mask.Collide(offset, velocity, ship.Facing());
}



// Check if this projectile has a blast radius.
bool Projectile::HasBlastRadius() const
{
	return (weapon->BlastRadius() > 0.);
}



// Check if the given ship is within this projectile's blast radius. (The
// projectile will not explode unless it is also within the trigger radius.)
bool Projectile::InBlastRadius(const Ship &ship, int step) const
{
	const Mask &mask = ship.GetSprite().GetMask(step);
	return mask.WithinRange(position - ship.Position(), ship.Facing(), weapon->BlastRadius());
}



// This projectile hit something. Create the explosion, if any. This also
// marks the projectile as needing deletion.
void Projectile::Explode(list<Effect> &effects, double intersection, Point hitVelocity)
{
	for(const auto &it : weapon->HitEffects())
		for(int i = 0; i < it.second; ++i)
		{
			effects.push_back(*it.first);
			effects.back().Place(
				position + velocity * intersection, velocity, angle, hitVelocity);
		}
	lifetime = -100;
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
const Outfit &Projectile::GetWeapon() const
{
	return *weapon;
}



// Get the projectiles characteristics, for drawing.
const Animation &Projectile::GetSprite() const
{
	return animation;
}



const Point &Projectile::Position() const
{
	return position;
}



const Point &Projectile::Velocity() const
{
	return velocity;
}



const Angle &Projectile::Facing() const
{
	return angle;
}



// Get the facing unit vector times the scale factor.
Point Projectile::Unit() const
{
	return angle.Unit() * .5;
}


	
// Find out which ship this projectile is targeting.
const Ship *Projectile::Target() const
{
	return cachedTarget;
}



// Find out which government this projectile belongs to.
const Government *Projectile::GetGovernment() const
{
	return government;
}
