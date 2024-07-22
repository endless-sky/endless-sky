/* AttributeAccessor.h
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

#ifndef ATTRIBUTE_ACCESS_H_
#define ATTRIBUTE_ACCESS_H_

#include "AttributeCategory.h"
#include "AttributeEffectType.h"

#include <optional>

class AttributeAccessor {
public:
	AttributeAccessor(const AttributeCategory category, const AttributeEffectType effect);
	AttributeAccessor(const AttributeCategory category, const AttributeEffectType categoryEffect,
			const AttributeEffectType effect);

	// Accessors
	AttributeCategory Category() const;
	AttributeEffectType Effect() const;

	// Checks whether this attribute is a multiplier.
	bool IsMultiplier() const;
	static bool IsMultiplier(const AttributeEffectType effect);
	// Creates a multiplier for this attribute, if not already a multiplier.
	AttributeAccessor Multiplier() const;
	static AttributeEffectType Multiplier(const AttributeEffectType effect);
	// Checks whether this attribute is relative.
	bool IsRelative() const;
	static bool IsRelative(const AttributeEffectType effect);
	// Creates a relative version of this attribute, if not already relative.
	AttributeAccessor Relative() const;
	static AttributeEffectType Relative(const AttributeEffectType effect);

	// Gets the attribute's category effect (variant), or -1 if none.
	AttributeEffectType GetCategoryEffect() const;
	static AttributeEffectType GetCategoryEffect(const AttributeCategory category);
	// Creates a version of this attribute that has the specified effect in its category.
	AttributeAccessor WithCategoryEffect(const AttributeEffectType type) const;
	static AttributeCategory WithCategoryEffect(const AttributeCategory category, const AttributeEffectType effect);

	// Checks whether this effect is a requirement for its category.
	// Required effects mark resource consumption when an action is taken.
	bool IsRequirement() const;
	static bool IsRequirement(const AttributeCategory category, const AttributeEffectType effect);
	// Checks if this effect, when used with the PASSIVE category, denotes a capacity or a
	// passively applied effect.
	static bool IsCapacity(const AttributeEffectType type);
	// Gets the basic effect of an attribute category. This is the effect used when
	// the category is used in a node with a value directly applied to it.
	static std::optional<AttributeEffectType> GetBaseEffect(const AttributeCategory category);

	// Gets the default minimum value for this attribute.
	double GetDefaultMinimum() const;

	template <typename A>
	bool operator==(const A &other) const;

	bool operator<(const AttributeAccessor other) const;

private:
	// Checks if the given attribute category is always composite. These categories are always merged with their effect
	// in the constructor.
	static bool IsAlwaysComposite(const AttributeCategory category);

private:
	// The category and effect type of the described attribute.
	AttributeCategory category;
	AttributeEffectType effect;
};



template<class T>
inline bool AttributeAccessor::operator==(const T &other) const
{
	return false;
}



template<>
inline bool AttributeAccessor::operator==(const AttributeAccessor &other) const
{
	return category == other.category && effect == other.effect;
}

#endif
