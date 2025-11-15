/* Entity.h
Copyright (c) 2025 by TomGoodIdea

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

#include "Outfit.h"



// A class containing common elements for objects like ships and minable asteroids.
class Entity : public Body {
public:
	virtual ~Entity() {};
	// Get the current attributes of this entity.
	const Outfit &Attributes() const;
	virtual double Mass() const = 0;
	// A value typically between 0 and 1, but can be higher in case of overheat.
	double Heat() const;
	// Get the maximum heat level, in heat units (not temperature).
	virtual double MaximumHeat() const = 0;


protected:
	static constexpr double MAXIMUM_TEMPERATURE = 100.;


protected:
	Outfit attributes;
	// The current heat value of this entity.
	double heat = 0.;
};
