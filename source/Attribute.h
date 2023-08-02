/* Attribute.h
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

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

#include "AttributeCategory.h"
#include "AttributeEffect.h"
#include "DataNode.h"

#include <string>

class Attribute {
public:
	// Creates a new categorized attribute. Use -1 if there is no category or effect in the definition.
	// The created attribute may report a different category or effect
	// if the same attribute can be described in multiple ways.
	Attribute(AttributeCategory category, AttributeEffect effect,
			AttributeEffect secondary = static_cast<AttributeEffect>(-1),
			bool usePreferred = true);
	// Gets the attribute for the specified token, if any.
	static Attribute *Parse(const std::string &token);

	// Gets the data format name of the effect, as used in the new syntax. This also supports
	// variants, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
	// multiplier effect.
	static std::string GetEffectName(const AttributeEffect effect);
	// Gets the data format name of the category, as used in the new syntax.
	static std::string GetCategoryName(const AttributeCategory category);

	// Maps the attribute to the legacy single string format.
	std::string GetLegacyName() const;

	// Creates a node describing this attribute within the provided parent node.
	DataNode Save(const DataNode &node) const;

	// Checks whether this attribute is a multiplier.
	bool IsMultiplier() const;
	// Creates a multiplier for this attribute, if not already a multiplier.
	Attribute Multiplier() const;
	// Checks whether this attribute is relative.
	bool IsRelative() const;
	// Creates a relative version of this attribute, if not already relative.
	Attribute Relative() const;

	// Checks whether this attribute is a requirement for its category.
	// Required attributes mark resource consumption when an action is taken.
	bool IsRequirement() const;

	// Gets the category of this attribute.
	AttributeCategory Category() const;
	// Gets the effect of this attribute.
	AttributeEffect Effect() const;
	// Gets the secondary effect of this attribute.
	AttributeEffect Secondary() const;

	// Gets the minimum value of this attribute.
	double GetMinimumValue() const;

	// Checks whether this attribute is supported in the engine.
	bool IsSupported() const;

	template <class A>
	bool operator==(const A &other) const;
	bool operator<(const Attribute &other) const;

private:
	// Creates a cache for faster data loading.
	static void PrepareCache();
	// Calculates the legacy name of this attribute. For queries, use the cached GetLegacyName() function instead.
	std::string CalculateLegacyName() const;


private:
	// Stores whether the cache has been populated.
	static bool cached;

	// The name of each effect as used in the data files
	static const std::string effectNames[];
	// The name of each category as used in the data files. Some are linked to the name of the effect which they expand on.
	static const std::string categoryNames[];

	// The category and effects of this attribute
	AttributeCategory category;
	AttributeEffect effect;
	AttributeEffect secondary;
};



#endif
