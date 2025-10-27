/* Projectile.cpp
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

#include "Projectile.h"

#include "Effect.h"
#include "FighterHitHelper.h"
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
		return (Random::Real() < base * probability);
	}

	// Returns if the missile is confused or not.
	bool ConfusedTracking(double tracking, double weaponRange, double jamming, double distance)
	{
		if(!jamming)
			return Random::Real() > tracking;
		else
			return Random::Real() > (tracking * distance) / (sqrt(jamming) * weaponRange);
	}
}



Projectile::Projectile(const Ship &parent, Point position, Angle angle, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), position, parent.Velocity(), angle),
	weapon(weapon), targetShip(parent.GetTargetShip()), lifetime(weapon->Lifetime())
{
	government = parent.GetGovernment();
	hitsRemaining = weapon->PenetrationCount();

	// If you are boarding your target, do not fire on it.
	if(parent.IsBoarding() || parent.Commands().Has(Command::BOARD))
		targetShip.reset();

	cachedTarget = TargetPtr().get();
	if(cachedTarget)
	{
		targetGovernment = cachedTarget->GetGovernment();
		targetDisabled = cachedTarget->IsDisabled();
	}

	dV = this->angle.Unit() * (weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	velocity += dV;

	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);

	// Set an initial confusion turn direction.
	if(weapon->Homing())
		confusionDirection = Random::Int(2) ? -1 : 1;
}



Projectile::Projectile(const Projectile &parent, const Point &offset, const Angle &angle, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), parent.position + parent.velocity + parent.angle.Rotate(offset),
	parent.velocity, parent.angle + angle),
	weapon(weapon), targetShip(parent.targetShip), lifetime(weapon->Lifetime())
{
	government = parent.government;
	targetGovernment = parent.targetGovernment;
	targetDisabled = parent.targetDisabled;
	hitsRemaining = weapon->PenetrationCount();

	cachedTarget = TargetPtr().get();

	// Given that submunitions inherit the velocity of the parent projectile,
	// it is often the case that submunitions don't add any additional velocity.
	// But we still want inaccuracy to have an effect on submunitions. Because of
	// this, we tilt the velocity of submunitions in the direction of the inaccuracy.
	dV = this->angle.Unit() * (parent.dV.Length() + weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	velocity += dV - parent.dV;

	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);

	// Set an initial confusion turn direction.
	if(weapon->Homing())
		confusionDirection = Random::Int(2) ? -1 : 1;
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
		if(lifetime > -1000)
		{
			// This projectile didn't die in a collision. Create any death effects.
			// Place effects ahead of the projectile by 1.5x velocity. 1x comes from
			// the anticipated movement of the projectile on its frame of death, and
			// 0.5x comes from the behavior of BatchDrawList::Add drawing the projectile sprite
			// half way between its current position and its next position.
			Point effectPosition = position + 1.5 * velocity;
			for(const auto &it : weapon->DieEffects())
				for(int i = 0; i < it.second; ++i)
					visuals.emplace_back(*it.first, effectPosition, velocity, angle);

			for(const auto &it : weapon->Submunitions())
				if(lifetime > -100 ? it.spawnOnNaturalDeath : it.spawnOnAntiMissileDeath)
					for(size_t i = 0; i < it.count; ++i)
					{
						const Weapon *const subWeapon = it.weapon.get();
						Angle inaccuracy = Distribution::GenerateInaccuracy(subWeapon->Inaccuracy(),
								subWeapon->InaccuracyDistribution());
						projectiles.emplace_back(*this, it.offset, it.facing + inaccuracy, subWeapon);
					}
		}
		MarkForRemoval();
		return;
	}
	// Spawn live effects. By using the current position of the projectile and not
	// adding any offset from the projectile's velocity, effects will appear to spawn
	// from behind the projectile, as by the time the effect is visible, the projectile
	// will have moved one frame forward from this position.
	for(const auto &it : weapon->LiveEffects())
		if(!Random::Int(it.second))
			visuals.emplace_back(*it.first, position, velocity, angle);

	// If the target has left the system, stop following it. Also stop if the
	// target has been captured by a different government.
	// Also stop targeting fighters that have become disabled after this projectile was fired.
	const Ship *target = cachedTarget;
	if(target)
	{
		target = TargetPtr().get();
		if(!target || !target->IsTargetable() || target->GetGovernment() != targetGovernment ||
				(!targetDisabled && !FighterHitHelper::IsValidTarget(target)))
		{
			BreakTarget();
			target = nullptr;
		}
	}

	double turn = weapon->Turn();
	double accel = weapon->Acceleration();
	bool homing = weapon->Homing();
	if(target && homing && !Random::Int(30))
	{
		CheckLock(*target);
		CheckConfused(*target);
	}
	// Update the confusion direction after the projectile turns about
	// 180 degrees away from its target.
	if(!Random::Int(ceil(180 / turn)))
		confusionDirection = Random::Int(2) ? -1 : 1;
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
		if(weapon->Leading())
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
		if(isFacingAway && weapon->HasBlindspot())
			targetShip.reset();
		else
		{
			double desiredTurn = TO_DEG * asin(cross);
			if(fabs(desiredTurn) > turn)
				turn = copysign(turn, desiredTurn);
			else
				turn = desiredTurn;

			// Levels 3 and 4 stop accelerating when facing away.
			if(weapon->ThrottleControl())
			{
				double stepsToFace = desiredTurn / turn;

				// If you are facing away from the target, stop accelerating.
				if(stepsToFace * 1.5 > stepsToReach)
					accel = 0.;
			}
		}
	}
	// Turn in a random direction if this weapon is confused.
	else if(target && homing && isConfused)
		turn *= confusionDirection;
	// If a weapon is homing but has no target, do not turn it.
	else if(homing)
		turn = 0.;

	if(turn)
		angle += Angle(turn);

	if(accel)
	{
		double d = 1. - weapon->Drag();
		Point a = accel * angle.Unit();
		velocity *= d;
		velocity += a;
		dV *= d;
		dV += a;
	}

	position += velocity;
	// Only measure the distance that this projectile traveled under its own
	// power, as opposed to including any velocity that came from the firing
	// ship.
	distanceTraveled += dV.Length();

	// If this projectile is now within its "split range," it should split into
	// sub-munitions next turn.
	if(target && (position - target->Position()).Length() < weapon->SplitRange())
		lifetime = 0;

	// A projectile will begin to fade out when the remaining lifetime is smaller
	// than the specified "fade out" time.
	if(lifetime < weapon->FadeOut())
		alpha = static_cast<double>(lifetime) / weapon->FadeOut();
}



// This projectile hit something. Create the explosion, if any. This also
// marks the projectile as needing deletion if it has run out of hits.
void Projectile::Explode(vector<Visual> &visuals, double intersection, Point hitVelocity)
{
	// Offset the placement position of effects by the projectile's velocity while
	// also accounting for the intersection clipping. Hit effects should appear from
	// the front of the projectile, and so are shifted forward by the full velocity
	// of the projectile.
	Point effectPosition = position + velocity * intersection;
	for(const auto &it : weapon->HitEffects())
		for(int i = 0; i < it.second; ++i)
			visuals.emplace_back(*it.first, effectPosition, velocity, angle, hitVelocity);
	// The projectile dies if it has no hits remaining.
	if(--hitsRemaining == 0)
	{
		clip = intersection;
		lifetime = -1000;
	}
}



// Get the amount of clipping that should be applied when drawing this projectile.
double Projectile::Clip() const
{
	return clip;
}



// Get whether the lifetime on this projectile has run out.
bool Projectile::IsDead() const
{
	return lifetime <= 0;
}



// This projectile was killed, e.g. by an anti-missile system.
void Projectile::Kill()
{
	lifetime = -100;
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



// Get information on how this projectile impacted a ship.
Projectile::ImpactInfo Projectile::GetInfo(double intersection) const
{
	// Account for the distance that this projectile traveled before intersecting
	// with the target.
	return ImpactInfo(*weapon, position, distanceTraveled + dV.Length() * intersection);
}



// Find out which ship this projectile is targeting.
const Ship *Projectile::Target() const
{
	return cachedTarget;
}



const Government *Projectile::TargetGovernment() const
{
	return targetGovernment;
}



shared_ptr<Ship> Projectile::TargetPtr() const
{
	return targetShip.lock();
}



// Clear the targeting information on this projectile.
void Projectile::BreakTarget()
{
	targetShip.reset();
	cachedTarget = nullptr;
	targetGovernment = nullptr;
	targetDisabled = false;
}



// TODO: add more conditions in the future. For example maybe proximity to stars
// and their brightness could could cause IR missiles to lose their locks more
// often, and dense asteroid fields could do the same for radar and optically
// guided missiles.
void Projectile::CheckLock(const Ship &target)
{
	static const double RELOCK_RATE = .3;
	double base = hasLock ? 1. : RELOCK_RATE;
	hasLock = false;

	// For each tracking type, calculate the probability twice every second that a
	// lock will be lost.
	if(weapon->Tracking())
	{
		double lockChance = (weapon ->Tracking());
		double probability = lockChance / (RELOCK_RATE - (lockChance * RELOCK_RATE) + lockChance);
		hasLock |= Check(probability, base);
	}

	// Optical tracking is about 1.5% for an average interceptor (250 mass),
	// about 50% for an average medium warship (1000 mass),
	// and about 95% for an average heavy warship (2500 mass),
	// but can be affected by jamming.
	if(weapon->OpticalTracking())
	{
		double opticalJamming = target.IsDisabled() ? 0. : target.Attributes().Get("optical jamming");
		if(opticalJamming)
		{
			double distance = position.Distance(target.Position());
			double jammingRange = 500. + sqrt(opticalJamming) * 500.;
			double rangeFraction = min(1., distance / jammingRange);
			opticalJamming = (1. - rangeFraction) * opticalJamming;
		}
		double targetMass = target.Mass();
		double weight = targetMass * targetMass * targetMass / 1e9;
		double lockChance = weapon->OpticalTracking() * weight / ((1. + weight) * (1. + opticalJamming));
		double probability = lockChance / (RELOCK_RATE - (lockChance * RELOCK_RATE) + lockChance);
		hasLock |= Check(probability, base);
	}

	// Infrared tracking is zero when heat is zero and 100% when heat is full.
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
		double lockChance = weapon->InfraredTracking() * min(1., target.Heat() * multiplier);
		double probability = lockChance / (RELOCK_RATE - (lockChance * RELOCK_RATE) + lockChance);
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
		double lockChance = weapon->RadarTracking() / (1. + radarJamming);
		double probability = lockChance / (RELOCK_RATE - (lockChance * RELOCK_RATE) + lockChance);
		hasLock |= Check(probability, base);
	}
}



// Homing weapons that have lost their lock have a chance to get confused
// and turn in a random direction ("go haywire"). Each tracking method has
// a different haywire condition. Weapons with multiple tracking methods
// only go haywire if all of the tracking methods have gotten confused.
void Projectile::CheckConfused(const Ship &target)
{
	if(hasLock)
	{
		isConfused = false;
		return;
	}

	bool trackingConfused = true;
	bool infraredConfused = true;
	bool opticalConfused = true;
	bool radarConfused = true;

	// Tracking and Infrared: proportional to tracking quality.
	if(weapon->Tracking())
		trackingConfused = Random::Real() > weapon->Tracking();

	if(weapon->InfraredTracking())
		infraredConfused = Random::Real() > weapon->InfraredTracking();

	// Optical and Radar: If the target has no jamming, then proportional to tracking
	// quality. If the target does have jamming, then it's proportional to
	// tracking quality, the strength of target's jamming, and the distance
	// to the target (jamming power attenuates with distance).
	double distance = position.Distance(target.Position());
	if(weapon->OpticalTracking())
	{
		double opticalTracking = weapon->OpticalTracking();
		double opticalJamming = target.Attributes().Get("optical jamming");
		opticalConfused = ConfusedTracking(opticalTracking, weapon->Range(),
			opticalJamming, distance);
	}

	if(weapon->RadarTracking())
	{
		double radarTracking = weapon->RadarTracking();
		double radarJamming = target.Attributes().Get("radar jamming");
		radarConfused = ConfusedTracking(radarTracking, weapon->Range(),
			radarJamming, distance);
	}

	isConfused = trackingConfused && infraredConfused && opticalConfused && radarConfused;
}



double Projectile::DistanceTraveled() const
{
	return distanceTraveled;
}



bool Projectile::Phases(const Ship &ship) const
{
	return phasedShip == &ship;
}



uint16_t Projectile::HitsRemaining() const
{
	return hitsRemaining;
}



void Projectile::SetPhases(const Ship *ship)
{
	phasedShip = ship;
}



bool Projectile::ShouldExplode() const
{
	return !government || (weapon->IsFused() && lifetime == 1);
}
