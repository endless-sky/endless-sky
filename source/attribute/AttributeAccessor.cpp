/* AttributeAccessor.cpp
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

#include "AttributeAccessor.h"

#include <limits>

using namespace std;



AttributeAccessor::AttributeAccessor(const AttributeCategory category, const AttributeEffectType effect)
		: category(IsAlwaysComposite(category) ? WithCategoryEffect(category, effect) : category), effect(effect)
{
}



AttributeAccessor::AttributeAccessor(const AttributeCategory category, const AttributeEffectType categoryEffect,
		const AttributeEffectType effect) : category(AttributeAccessor::WithCategoryEffect(category, categoryEffect)),
		effect(effect)
{
}



// Accessors.
AttributeCategory AttributeAccessor::Category() const
{
	return category;
}



AttributeEffectType AttributeAccessor::Effect() const
{
	return effect;
}



// Checks whether this attribute is a multiplier.
bool AttributeAccessor::IsMultiplier() const
{
	return IsMultiplier(effect);
}



bool AttributeAccessor::IsMultiplier(const AttributeEffectType effect)
{
	return effect >= ATTRIBUTE_EFFECT_COUNT && (effect < ATTRIBUTE_EFFECT_COUNT * 2
		|| effect >= ATTRIBUTE_EFFECT_COUNT * 3);
}



// Creates a multiplier for this attribute, if not already a multiplier.
AttributeAccessor AttributeAccessor::Multiplier() const
{
	if(IsMultiplier())
		return *this;
	return {category, Multiplier(effect)};
}



AttributeEffectType AttributeAccessor::Multiplier(AttributeEffectType effect)
{
	return static_cast<AttributeEffectType>(effect + ATTRIBUTE_EFFECT_COUNT);
}



// Checks whether this attribute is relative.
bool AttributeAccessor::IsRelative() const
{
	return IsRelative(effect);
}



bool AttributeAccessor::IsRelative(const AttributeEffectType effect)
{
	return effect >= ATTRIBUTE_EFFECT_COUNT * 2;
}


// Creates a relative version of this attribute, if not already relative.
AttributeAccessor AttributeAccessor::Relative() const
{
	if(IsRelative())
		return *this;
	return AttributeAccessor(category, Relative(effect));
}



AttributeEffectType AttributeAccessor::Relative(const AttributeEffectType effect)
{
	return static_cast<AttributeEffectType>(effect + 2 * ATTRIBUTE_EFFECT_COUNT);
}



// Gets the attribute's category effect (variant), or -1 if none.
AttributeEffectType AttributeAccessor::GetCategoryEffect() const
{
	return GetCategoryEffect(category);
}



AttributeEffectType AttributeAccessor::GetCategoryEffect(const AttributeCategory category)
{
	return static_cast<AttributeEffectType>((category / ATTRIBUTE_CATEGORY_COUNT) - 1);
}



// Creates a version of this attribute that has the specified effect in its category.
AttributeAccessor AttributeAccessor::WithCategoryEffect(const AttributeEffectType type) const
{
	return AttributeAccessor(WithCategoryEffect(category, type), effect);
}



AttributeCategory AttributeAccessor::WithCategoryEffect(const AttributeCategory category,
		const AttributeEffectType effect)
{
	return static_cast<AttributeCategory>((category % ATTRIBUTE_CATEGORY_COUNT) +
			ATTRIBUTE_CATEGORY_COUNT * (effect + 1));
}



// Checks whether this effect is a requirement for its category.
// Required effects mark resource consumption when an action is taken.
bool AttributeAccessor::IsRequirement() const
{
	return IsRequirement(category, effect);
}



bool AttributeAccessor::IsRequirement(const AttributeCategory category, const AttributeEffectType effect)
{
	if(category == PASSIVE || category == DAMAGE || category == PROTECTION)
		return false;
	if(static_cast<int>(category) == static_cast<int>(effect) && category <= CLOAKING)
		return false;
	if(effect <= HULL || effect == ENERGY || effect == FUEL)
		return true;
	return false;
}



// Checks if this effect, when used with the PASSIVE category, denotes a capacity or a
// passively applied effect.
bool AttributeAccessor::IsCapacity(const AttributeEffectType effect)
{
	return effect != COOLING;
}



// Gets the basic effect of an attribute category. This is the effect used when
// the category is used in a node with a value directly applied to it.
optional<AttributeEffectType> AttributeAccessor::GetBaseEffect(const AttributeCategory category)
{
	// Categories until CLOAKING correspond to their effects.
	if(category <= CLOAKING)
		return static_cast<AttributeEffectType>(category);
	// Composite categories always have their own composite effect as the default.
	AttributeEffectType effect = GetCategoryEffect(category);
	if(effect >= 0)
		return effect;
	return std::nullopt;
}



// Gets the default minimum value for this attribute.
double AttributeAccessor::GetDefaultMinimum() const
{
	if(IsMultiplier())
		return -1.;
	if(category % ATTRIBUTE_CATEGORY_COUNT == PROTECTION && GetCategoryEffect() == effect)
		return -0.99;
	return numeric_limits<double>::lowest();
}



bool AttributeAccessor::operator<(const AttributeAccessor other) const
{
	if(category == other.category)
		return effect < other.effect;
	return category < other.category;
}

bool AttributeAccessor::IsAlwaysComposite(const AttributeCategory category)
{
	return category == RESISTANCE || category == PROTECTION;
}
