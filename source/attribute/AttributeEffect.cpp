/* AttributeEffect.cpp
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

#include "AttributeEffect.h"

#include "AttributeAccessor.h"



const double AttributeEffect::EPS = 0.0000000001;



// Creates a new effect of a specified type and value, with optional sub-effects.
AttributeEffect::AttributeEffect(const AttributeEffectType type, const double value, const double minimum)
		: type(type), value(std::fmax(value, minimum)), min(minimum)
{
}



// Checks whether this effect is a multiplier.
bool AttributeEffect::IsMultiplier() const
{
	return AttributeAccessor::IsMultiplier(type);
}



// Checks whether this effect is relative.
bool AttributeEffect::IsRelative() const
{
	return AttributeAccessor::IsRelative(type);
}



// Checks whether this effect is a requirement for its category.
// Required effects mark resource consumption when an action is taken.
bool AttributeEffect::IsRequirement(const AttributeCategory category) const
{
	return AttributeAccessor::IsRequirement(category, type);
}



// Checks if this effect, when used with the PASSIVE category, denotes a capacity or a
// passively applied effect.
bool AttributeEffect::IsCapacity() const
{
	return AttributeAccessor::IsCapacity(type);
}



// Gets the type of the effect, its value, and minimum value.
AttributeEffectType AttributeEffect::Type() const
{
	return type;
}



double AttributeEffect::Value() const
{
	return value;
}



double AttributeEffect::Minimum() const
{
	return min;
}



// Adds the specified amount to this effect's value.
void AttributeEffect::Add(const double amount)
{
	Set(value + amount);
}



// Sets the effect's value to the specified amount.
void AttributeEffect::Set(const double amount)
{
	value = std::fmax(min, amount);
	if(value && fabs(value) < EPS)
		value = 0;
}
