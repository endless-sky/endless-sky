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

#pragma once

#include "AttributeCategory.h"
#include "AttributeEffectType.h"
#include "Modifier.h"

#include <optional>
#include <string>
#include <variant>

// An attribute accessor describes a categorized attribute. It contains the category
// and effect type of the attribute, and provides conversion functions for various modifiers.
class AttributeAccessor {
public:
	AttributeAccessor(AttributeCategory category, AttributeEffectType effect);
	AttributeAccessor(AttributeCategory category, AttributeEffectType effect, Modifier modifier);
	AttributeAccessor(AttributeCategory category, AttributeEffectType categoryEffect,
			AttributeEffectType effect, Modifier modifier);
	AttributeAccessor(AttributeCategory category, AttributeEffectType categoryEffect,
			AttributeEffectType effect);

	// Accessors
	AttributeCategory Category() const;
	AttributeEffectType Effect() const;

	// Checks whether this attribute has a specific modifier. Attribute effects have exactly one modifier.
	bool HasModifier(Modifier modifier) const;
	static bool HasModifier(AttributeEffectType effect, Modifier modifier);
	// Creates a new accessor with the effect's modifier set to the given value.
	AttributeAccessor WithModifier(Modifier modifier) const;
	static AttributeEffectType WithModifier(AttributeEffectType effect, Modifier modifier);

	// Gets the attribute's category effect (variant), or -1 if none.
	AttributeEffectType GetCategoryEffect() const;
	static AttributeEffectType GetCategoryEffect(AttributeCategory category);
	// Creates a version of this attribute that has the specified effect in its category.
	AttributeAccessor WithCategoryEffect(AttributeEffectType type) const;
	static AttributeCategory WithCategoryEffect(AttributeCategory category, AttributeEffectType effect);

	// Checks whether this effect is a requirement for its category.
	// Required effects mark resource consumption when an action is taken.
	bool IsRequirement() const;
	static bool IsRequirement(AttributeCategory category, AttributeEffectType effect);
	// Checks if this effect, when used with the PASSIVE category, denotes a capacity or a
	// passively applied effect.
	static bool IsCapacity(AttributeEffectType type);
	// Gets the basic effect of an attribute category. This is the effect used when
	// the category is used in a node with a value directly applied to it.
	static std::optional<AttributeEffectType> GetBaseEffect(AttributeCategory category);

	// Gets the default minimum value for this attribute.
	double GetDefaultMinimum() const;

	bool operator==(AttributeAccessor other) const;

	bool operator<(AttributeAccessor other) const;

private:
	// Checks if the given attribute category is always composite. These categories are always merged with their effect
	// in the constructor.
	static bool IsAlwaysComposite(AttributeCategory category);

private:
	// The category and effect type of the described attribute.
	AttributeCategory category;
	AttributeEffectType effect;
};

// Helper class for using string and AttributeAccessor
class AnyAttribute : public std::variant<std::string, AttributeAccessor>
{
public:
	// Allow easy conversion from any related type. These constructors are not marked 'explicit' on purpose.
	AnyAttribute(const char *str) : std::variant<std::string, AttributeAccessor>(str) {}
	AnyAttribute(const std::string &str) : std::variant<std::string, AttributeAccessor>(str) {}
	AnyAttribute(const std::string &&str) : std::variant<std::string, AttributeAccessor>(str) {}
	AnyAttribute(const AttributeAccessor &str) : std::variant<std::string, AttributeAccessor>(str) {}
	AnyAttribute(const AttributeAccessor &&str) : std::variant<std::string, AttributeAccessor>(str) {}
	AnyAttribute(AttributeCategory category, AttributeEffectType effect)
			: AnyAttribute(AttributeAccessor{category, effect}) {}
	AnyAttribute(AttributeCategory category, AttributeEffectType categoryEffect, AttributeEffectType effect,
			Modifier modifier) : AnyAttribute(AttributeAccessor{category, categoryEffect, effect, modifier}) {}
	AnyAttribute(AttributeCategory category, AttributeEffectType effect, Modifier modifier)
			: AnyAttribute(AttributeAccessor{category, effect, modifier}) {}
	AnyAttribute(AttributeCategory category, AttributeEffectType categoryEffect, AttributeEffectType effect)
			: AnyAttribute(AttributeAccessor{category, categoryEffect, effect}) {}

	bool IsCategorized() const;
	bool IsString() const;
	const AttributeAccessor &Categorized() const;
	const std::string &String() const;
};
