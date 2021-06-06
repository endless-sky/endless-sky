/* Minable.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Minable.h"

#include "DataNode.h"
#include "Effect.h"
#include "Flotsam.h"
#include "GameData.h"
#include "Mask.h"
#include "Outfit.h"
#include "pi.h"
#include "Projectile.h"
#include "Random.h"
#include "SpriteSet.h"
#include "Visual.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Load a definition of a minable object.
void Minable::Load(const DataNode &node)
{
	// Set the name of this minable, so we know it has been loaded.
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		// A full sprite definition (frame rate, etc.) is not needed, because
		// the frame rate will be set randomly and it will always be looping.
		if(child.Token(0) == "sprite" && child.Size() >= 2)
			SetSprite(SpriteSet::Get(child.Token(1)));
		else if(child.Token(0) == "hull" && child.Size() >= 2)
			hull = child.Value(1);
		else if((child.Token(0) == "payload" || child.Token(0) == "explode") && child.Size() >= 2)
		{
			int count = (child.Size() == 2 ? 1 : child.Value(2));
			if(child.Token(0) == "payload")
				payload[GameData::Outfits().Get(child.Token(1))] += count;
			else
				explosions[GameData::Effects().Get(child.Token(1))] += count;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const string &Minable::Name() const
{
	return name;
}



// Place a minable object with up to the given energy level, on a random
// orbit and a random position along that orbit.
void Minable::Place(double energy, double beltRadius)
{
	// Note: there's no closed-form equation for orbital position as a function
	// of time, so either I need to use Newton's method to get high precision
	// (which, for a game would be overkill) or something will drift over time.
	// If that drift caused the orbit to decay, that would be a problem, which
	// rules out just applying gravity as a force from the system center.
	
	// Instead, each orbit is defined by an ellipse equation:
	// 1 / radius = constant * (1 + eccentricity * cos(theta)).
	
	// The only thing that will change over time is theta, the "true anomaly."
	// That way, the orbital period will only be approximate (which does not
	// really matter) but the orbit itself will never decay.
	
	// Generate random orbital parameters. Limit eccentricity so that the
	// objects do not spend too much time far away and moving slowly.
	eccentricity = Random::Real() * .6;
	
	// Since an object is moving slower at apoapsis than at periapsis, it is
	// more likely to start out there. So, rather than a uniform distribution of
	// angles, favor ones near 180 degrees. (Note: this is not the "correct"
	// equation; it is just a reasonable approximation.)
	theta = Random::Real();
	double curved = (pow(asin(theta * 2. - 1.) / (.5 * PI), 3.) + 1.) * .5;
	theta = (eccentricity * curved + (1. - eccentricity) * theta) * 2. * PI;
	
	// Now, pick the orbital "scale" such that, relative to the "belt radius":
	// periapsis distance (scale / (1 + e)) is no closer than .4: scale >= .4 * (1 + e)
	// apoapsis distance (scale / (1 - e)) is no farther than 4.: scale <= 4. * (1 - e)
	// periapsis distance is no farther than 1.3: scale <= 1.3 * (1 + e)
	// apoapsis distance is no closer than .8: scale >= .8 * (1 - e)
	double sMin = max(.4 * (1. + eccentricity), .8 * (1. - eccentricity));
	double sMax = min(4. * (1. - eccentricity), 1.3 * (1. + eccentricity));
	scale = (sMin + Random::Real() * (sMax - sMin)) * beltRadius;
	
	// At periapsis, the object should have this velocity:
	double maximumVelocity = (Random::Real() + 2. * eccentricity) * .5 * energy;
	// That means that its angular momentum is equal to:
	angularMomentum = (maximumVelocity * scale) / (1. + eccentricity);
	
	// Start the object off with a random facing angle and spin rate.
	angle = Angle::Random();
	spin = Angle::Random(energy) - Angle::Random(energy);
	SetFrameRate(Random::Real() * 4. * energy + 5.);
	// Choose a random direction for the angle of periapsis.
	rotation = Random::Real() * 2. * PI;
	
	// Calculate the object's initial position.
	radius = scale / (1. + eccentricity * cos(theta));
	position = radius * Point(cos(theta + rotation), sin(theta + rotation));
}



// Move the object forward one step. If it has been reduced to zero hull, it
// will "explode" instead of moving, creating flotsam and explosion effects.
// In that case it will return false, meaning it should be deleted.
bool Minable::Move(vector<Visual> &visuals, list<shared_ptr<Flotsam>> &flotsam)
{
	if(hull < 0)
	{
		// This object has been destroyed. Create explosions and flotsam.
		double scale = .1 * Radius();
		for(const auto &it : explosions)
		{
			for(int i = 0; i < it.second; ++i)
			{
				// Add a random velocity.
				Point dp = (Random::Real() * scale) * Angle::Random().Unit();
				
				visuals.emplace_back(*it.first, position + 2. * dp, velocity + dp, angle);
			}
		}
		for(const auto &it : payload)
		{
			// Each payload object has a 25% chance of surviving. This creates
			// a distribution with occasional very good payoffs.
			for(int amount = Random::Binomial(it.second, .25); amount > 0; amount -= Flotsam::TONS_PER_BOX)
			{
				flotsam.emplace_back(new Flotsam(it.first, min(amount, Flotsam::TONS_PER_BOX)));
				flotsam.back()->Place(*this);
			}
		}
		return false;
	}
	
	// Spin the object.
	angle += spin;
	
	// Advance the object forward one step.
	theta += angularMomentum / (radius * radius);
	radius = scale / (1. + eccentricity * cos(theta));
	
	// Calculate the new position.
	Point newPosition(radius * cos(theta + rotation), radius * sin(theta + rotation));
	// Calculate the velocity this object is moving at, so that its motion blur
	// will be rendered correctly.
	velocity = newPosition - position;
	position = newPosition;
	
	return true;
}



// Damage this object (because a projectile collided with it).
void Minable::TakeDamage(const Projectile &projectile)
{
	hull -= projectile.GetWeapon().HullDamage();
}



// Determine what flotsam this asteroid will create.
const map<const Outfit *, int> &Minable::Payload() const
{
	return payload;
}
