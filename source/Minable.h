/* Minable.h
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

#pragma once

#include "Body.h"

#include "Angle.h"

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class DataNode;
class Effect;
class Flotsam;
class MinableDamageDealt;
class Outfit;
class Projectile;
class Visual;



// Class representing an asteroid or other minable object that orbits in an
// ellipse around the system center.
class Minable : public Body {
public:
	class Payload {
	public:
		Payload(const DataNode &node);

		const Outfit *outfit;
		// The maximum number of outfits that this payload can drop.
		int maxDrops = 1;
		// The average percentage of the maximum number that drop.
		double dropRate = 0.25;
		// How resistant this payload is to having its drop rate increased by prospecting.
		double toughness = 1.;
	};


public:
	// Inherited from Body:
	// Frame GetFrame(int step = -1) const;
	// const Mask &GetMask(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;

	// Load a definition of a minable object.
	void Load(const DataNode &node);
	// Calculate the expected payload value of this Minable after all outfits have been fully loaded.
	void FinishLoading();
	const std::string &TrueName() const;
	const std::string &DisplayName() const;
	const std::string &Noun() const;

	// Place a minable object with up to the given energy level, on a random
	// orbit and a random position along that orbit.
	void Place(double energy, double beltRadius);

	// Move the object forward one step. If it has been reduced to zero hull, it
	// will "explode" instead of moving, creating flotsam and explosion effects.
	// In that case it will return false, meaning it should be deleted.
	bool Move(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam);

	// Damage this object (because a projectile collided with it).
	void TakeDamage(const MinableDamageDealt &damage);

	// Determine what flotsam this asteroid will create.
	const std::vector<Payload> &GetPayload() const;

	// Get the expected value of the flotsams this minable will create when destroyed.
	const int64_t &GetValue() const;

	// Get hull remaining of this asteroid, as a fraction between 0 and 1.
	double Hull() const;
	// Get the maximum hull value of this asteroid.
	double MaxHull() const;


private:
	class LiveEffect {
	public:
		LiveEffect(const DataNode &node);

		const Effect *effect;
		// Average interval between instances of the effect, in frames.
		unsigned interval = 1;
		// If set to true, the effect behaves like a comet tail,
		// always facing away from the system center.
		bool relativeToSystem = false;
	};


private:
	std::string name;
	std::string displayName;
	std::string noun;
	// Current angular position relative to the focus of the elliptical orbit,
	// in radians. An angle of zero is the periapsis point.
	double theta;
	// Eccentricity of the orbit. 0 is circular and 1 is a parabola.
	double eccentricity;
	// Angular momentum (radius^2 * angular velocity) will always be conserved.
	// The object's mass can be ignored, because it is a constant.
	double angularMomentum;
	// Scale of the orbit. This is the orbital radius when theta is 90 degrees.
	// The periapsis and apoapsis radii are scale / (1 +- eccentricity).
	double orbitScale;
	// Rotation of the orbit - that is, the angle of periapsis - in radians.
	double rotation;
	// Rate of spin of the object.
	Angle spin;

	// Cache the current orbital radius. It can be calculated from theta and the
	// parameters above, but this avoids having to calculate every radius twice.
	double radius;

	// Remaining "hull" strength of the object, before it is destroyed.
	double hull = 1000.;
	// The hull value that this object starts at.
	double maxHull = 1000.;
	// A random amount of hull that gets added to the object.
	double randomHull = 0.;
	// How much prospecting has been done on this object. Used to increase the
	// payload drop rate.
	double prospecting = 0.;
	// Material released when this object is destroyed.
	std::vector<Payload> payload;
	std::vector<LiveEffect> liveEffects;
	// Explosion effects created when this object is destroyed.
	std::map<const Effect *, int> explosions;
	// The expected value of the payload of this minable.
	int64_t value = 0.;
	bool useRandomFrameRate = true;
};
