/* AttributeEffect.h
Copyright (c) 2023 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ATTRIBUTE_EFFECT_H_
#define ATTRIBUTE_EFFECT_H_

#include "AttributeCategory.h"
#include "AttributeEffectType.h"

#include <cmath>
#include <limits>

class AttributeEffect {
public:
	// Creates a new effect of a specified type, value and minimum.
	AttributeEffect(const AttributeEffectType type, const double value, const double minimum =
			std::numeric_limits<double>::lowest());

	// Gets the type of the effect, its value, and its sub-effects.
	AttributeEffectType Type() const;
	double Value() const;
	double Minimum() const;


	// Checks whether this effect is a multiplier.
	bool IsMultiplier() const;
	// Checks whether this effect is relative.
	bool IsRelative() const;

	// Adds the specified amount to this effect's value.
	void Add(const double amount);
	// Sets the effect's value to the specified amount.
	void Set(const double amount);

	// Checks whether this effect is a requirement for its category.
	// Required effects mark resource consumption when an action is taken.
	bool IsRequirement(const AttributeCategory category) const;

	// Checks if this effect, when used with the PASSIVE category, denotes a capacity or a
	// passively applied effect.
	bool IsCapacity() const;

	const static double EPS;
private:
	// The type of this effect,
	const AttributeEffectType type;
	// its value,
	double value;
	// and its minimum value.
	const double min;
};

#endif
