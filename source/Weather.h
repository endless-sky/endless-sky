/* Weather.h
Copyright (c) 2020 by Amazinite

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

#include "Point.h"

#include <vector>

class Hazard;
class Visual;
class Weapon;



// Weather is used to contain an active system hazard, keeping track of the hazard's
// lifetime, its strength, and if it should cause any damage.
class Weather {
public:
	class ImpactInfo {
	public:
		ImpactInfo(const Weapon &weapon, Point position, double scale)
			: weapon(weapon), position(std::move(position)), scale(scale) {}

		const Weapon &weapon;
		Point position;
		double scale;
	};

public:
	Weather() = default;
	explicit Weather(const Hazard *hazard, int totalLifetime, int lifetimeRemaining, double strength, Point origin);

	// The hazard that is associated with this weather event.
	const Hazard *GetHazard() const;
	// Whether the hazard of this weather deals damage or not.
	bool HasWeapon() const;
	// The period of this weather, dictating how often it deals damage while active.
	int Period() const;
	// The origin of the hazard.
	const Point &Origin() const;
	// Create any environmental effects and decrease the lifetime of this weather.
	void Step(std::vector<Visual> &newVisuals, const Point &center);
	// Calculate this weather's strength for the current frame, to be used to find
	// out what the current period and damage multipliers are.
	void CalculateStrength();

	// Get information on how this hazard impacted a ship.
	ImpactInfo GetInfo() const;

	// Check if this object is marked for removal from the game.
	bool ShouldBeRemoved() const;


private:
	// What the hazard's damage is multiplied by given the current weather strength.
	double DamageMultiplier() const;


private:
	const Hazard *hazard = nullptr;
	int totalLifetime = 0;
	int lifetimeRemaining = 0;
	double strength = 0.;
	Point origin;
	// The current strength and its square root are calculated at the beginning of
	// each frame for weather that deviates to avoid needing to calculate it
	// multiple times.
	double currentStrength = 0.;
	double sqrtStrength = 0.;
	double deviation = 0.;

	// Record when this object is marked for removal from the game.
	bool shouldBeRemoved = false;
};
