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



AttributeAccessor::AttributeAccessor(AttributeCategory category, AttributeEffectType effect)
		: category(IsAlwaysComposite(category) ? WithCategoryEffect(category, effect) : category), effect(effect)
{
}



AttributeAccessor::AttributeAccessor(AttributeCategory category, AttributeEffectType effect, Modifier modifier)
		: category(IsAlwaysComposite(category) ? WithCategoryEffect(category, WithModifier(effect, modifier)) : category),
		effect(WithModifier(effect, modifier))
{
}



AttributeAccessor::AttributeAccessor(AttributeCategory category, AttributeEffectType categoryEffect,
		AttributeEffectType effect)
		: category(AttributeAccessor::WithCategoryEffect(category, categoryEffect)), effect(effect)
{
}



AttributeAccessor::AttributeAccessor(AttributeCategory category, AttributeEffectType categoryEffect,
		AttributeEffectType effect, Modifier modifier)
		: category(AttributeAccessor::WithCategoryEffect(category, categoryEffect)), effect(WithModifier(effect, modifier))
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



// Checks whether this attribute has a specific modifier. Attribute effects have exactly one modifier.
bool AttributeAccessor::HasModifier(Modifier modifier) const
{
	return HasModifier(effect, modifier);
}



bool AttributeAccessor::HasModifier(AttributeEffectType effect, Modifier modifier)
{
	return effect / ATTRIBUTE_EFFECT_COUNT == static_cast<int>(modifier);
}



// Creates a new accessor with the effect's modifier set to the given value.
AttributeAccessor AttributeAccessor::WithModifier(Modifier modifier) const
{
	return {category, effect, modifier};
}



AttributeEffectType AttributeAccessor::WithModifier(AttributeEffectType effect, Modifier modifier)
{
	int newEffect = effect % ATTRIBUTE_EFFECT_COUNT + static_cast<int>(modifier) * ATTRIBUTE_EFFECT_COUNT;
	return static_cast<AttributeEffectType>(newEffect);
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
	if(HasModifier(effect, Modifier::OVER_TIME))
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
	if(HasModifier(Modifier::MULTIPLIER))
		return -1.;
	if(category % ATTRIBUTE_CATEGORY_COUNT == PROTECTION && GetCategoryEffect() == effect)
		return -0.99;
	return numeric_limits<double>::lowest();
}



bool AttributeAccessor::operator==(AttributeAccessor other) const
{
	return category == other.category && effect == other.effect;
}



bool AttributeAccessor::operator<(AttributeAccessor other) const
{
	if(category == other.category)
		return effect < other.effect;
	return category < other.category;
}



bool AttributeAccessor::IsAlwaysComposite(AttributeCategory category)
{
	return category == RESISTANCE || category == PROTECTION;
}



bool AnyAttribute::IsCategorized() const
{
	return std::variant<string, AttributeAccessor>::index();
}



bool AnyAttribute::IsString() const
{
	return !std::variant<string, AttributeAccessor>::index();
}



const AttributeAccessor &AnyAttribute::Categorized() const
{
	return std::get<1>(*this);
}



const string &AnyAttribute::String() const
{
	return std::get<0>(*this);
}
