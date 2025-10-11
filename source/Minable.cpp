/* Minable.cpp
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

#include "Minable.h"

#include "DataNode.h"
#include "Effect.h"
#include "Flotsam.h"
#include "text/Format.h"
#include "GameData.h"
#include "MinableDamageDealt.h"
#include "Outfit.h"
#include "pi.h"
#include "Projectile.h"
#include "Random.h"
#include "image/SpriteSet.h"
#include "Visual.h"

#include <algorithm>
#include <cmath>

using namespace std;



Minable::Payload::Payload(const DataNode &node)
{
	outfit = GameData::Outfits().Get(node.Token(1));
	maxDrops = (node.Size() == 2 ? 1 : max<int>(1, node.Value(2)));

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(!hasValue)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "max drops")
			maxDrops = max<int>(1, child.Value(1));
		else if(key == "drop rate")
			dropRate = max(0., min(child.Value(1), 1.));
		else if(key == "toughness")
			toughness = max(1., child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Load a definition of a minable object.
void Minable::Load(const DataNode &node)
{
	// Set the name of this minable, so we know it has been loaded.
	if(node.Size() >= 2)
		name = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(!hasValue)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "display name")
			displayName = child.Token(1);
		else if(key == "noun")
			noun = child.Token(1);
		else if(key == "sprite")
		{
			LoadSprite(child);
			for(const DataNode &grand : child)
				if(grand.Token(0) == "frame rate" || grand.Token(0) == "frame time")
					useRandomFrameRate = false;
		}
		else if(key == "hull")
			hull = child.Value(1);
		else if(key == "random hull")
			randomHull = max(0., child.Value(1));
		else if(key == "payload")
			payload.emplace_back(child);
		else if(key == "live effect")
			liveEffects.emplace_back(child);
		else if(key == "explode")
		{
			int count = (child.Size() == 2 ? 1 : child.Value(2));
			explosions[GameData::Effects().Get(child.Token(1))] += count;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(displayName.empty())
		displayName = Format::Capitalize(name);
	if(noun.empty())
		noun = "Asteroid";
}



// Calculate the expected payload value of this Minable after all outfits have been fully loaded.
void Minable::FinishLoading()
{
	for(const auto &it : payload)
		value += it.outfit->Cost() * it.maxDrops * it.dropRate;
}



const string &Minable::TrueName() const
{
	return name;
}



const string &Minable::DisplayName() const
{
	return displayName;
}



const string &Minable::Noun() const
{
	return noun;
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
	orbitScale = (sMin + Random::Real() * (sMax - sMin)) * beltRadius;

	// At periapsis, the object should have this velocity:
	double maximumVelocity = (Random::Real() + 2. * eccentricity) * .5 * energy;
	// That means that its angular momentum is equal to:
	angularMomentum = (maximumVelocity * orbitScale) / (1. + eccentricity);

	// Start the object off with a random facing angle and spin rate.
	angle = Angle::Random();
	spin = Angle::Random(energy) - Angle::Random(energy);
	if(useRandomFrameRate)
		SetFrameRate(Random::Real() * 4. * energy + 5.);
	// Choose a random direction for the angle of periapsis.
	rotation = Random::Real() * 2. * PI;

	// Calculate the object's initial position.
	radius = orbitScale / (1. + eccentricity * cos(theta));
	position = radius * Point(cos(theta + rotation), sin(theta + rotation));

	// Add a random amount of hull value to the object.
	hull += Random::Real() * randomHull;
	maxHull = hull;
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
		for(const Payload &it : payload)
		{
			// Each payload has a default 25% chance of surviving. This
			// creates a distribution with occasional very good payoffs.
			double dropRate = it.dropRate;
			// Special weapons are capable of increasing this drop rate through
			// prospecting.
			if(prospecting > 0. && dropRate < 1.)
				dropRate += (1. - dropRate) / (1. + it.toughness / prospecting);
			if(dropRate <= 0.)
				continue;
			for(int amount = Random::Binomial(it.maxDrops, dropRate); amount > 0; amount -= Flotsam::TONS_PER_BOX)
			{
				flotsam.emplace_back(new Flotsam(it.outfit, min(amount, Flotsam::TONS_PER_BOX)));
				flotsam.back()->Place(*this);
			}
		}
		return false;
	}

	for(const auto &it : liveEffects)
		if(!Random::Int(it.interval))
			visuals.emplace_back(*it.effect, position, velocity, it.relativeToSystem ? Angle{position} : angle);

	// Spin the object.
	angle += spin;

	// Advance the object forward one step.
	theta += angularMomentum / (radius * radius);
	radius = orbitScale / (1. + eccentricity * cos(theta));

	// Calculate the new position.
	Point newPosition(radius * cos(theta + rotation), radius * sin(theta + rotation));
	// Calculate the velocity this object is moving at, so that its motion blur
	// will be rendered correctly.
	velocity = newPosition - position;
	position = newPosition;

	return true;
}



// Damage this object (because a projectile collided with it).
void Minable::TakeDamage(const MinableDamageDealt &damage)
{
	hull -= damage.hullDamage;
	prospecting += damage.prospecting;
}



double Minable::Hull() const
{
	return min(1., hull / maxHull);
}



double Minable::MaxHull() const
{
	return maxHull;
}



// Determine what flotsam this asteroid will create.
const vector<Minable::Payload> &Minable::GetPayload() const
{
	return payload;
}



// Get the expected value of the flotsams this minable will create when destroyed.
const int64_t &Minable::GetValue() const
{
	return value;
}



Minable::LiveEffect::LiveEffect(const DataNode &node)
{
	interval = (node.Size() == 2 ? 1 : node.Value(2));
	effect = GameData::Effects().Get(node.Token(1));
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "relative to system center")
			relativeToSystem = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}
