/* StatusEffectHandler.h
Copyright (c) 2026 by Amazinite

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

#include "ResourceLevels.h"

class Entity;
class Outfit;



// A class that handles damage and decay of status effects on an Entity.
class StatusEffectHandler {
public:
	// Setup this attribute handler with a pointer to the Entity instance that it is handling status effects for.
	void Setup(Entity *parent);
	// Update the stored ResourceLevels for various status effect related actions.
	void Calibrate();

	// Clear all status effects.
	void Clear() const;
	// Step all status effects and damage the parent according to any currently accumulated damage over time.
	void Do(bool disabled = false) const;


private:
	Entity *parent = nullptr;
	const Outfit *attributes = nullptr;
	ResourceLevels *entityLevels = nullptr;

	double corrosionResistance = 0.;
	ResourceLevels corrosionResistCost;
	double dischargeResistance = 0.;
	ResourceLevels dischargeResistCost;
	double ionizationResistance = 0.;
	ResourceLevels ionizationResistCost;
	double scramblingResistance = 0.;
	ResourceLevels scramblingResistCost;
	double burnResistance = 0.;
	ResourceLevels burnResistCost;
	double leakResistance = 0.;
	ResourceLevels leakageResistCost;
	double disruptionResistance = 0.;
	ResourceLevels disruptionResistCost;
	double slowingResistance = 0.;
	ResourceLevels slownessResistCost;
};
