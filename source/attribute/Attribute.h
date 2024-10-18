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

#pragma once

#include "AttributeAccessor.h"
#include "AttributeCategory.h"
#include "AttributeEffect.h"
#include "../DataNode.h"

#include <map>
#include <string>

class Attribute {
public:
	// Creates a new categorized attribute. Use -1 if there is no category in the definition.
	explicit Attribute(AttributeCategory category);
	// Copies an attribute and multiplies all of its values.
	Attribute(const Attribute &other, double multiplier = 1.);
	// Creates an attribute with a single initial effect.
	Attribute(AttributeAccessor access, double value = 1.);
	// Gets the attribute for the specified token, if any.
	static Attribute *Parse(const std::string &token);

	// Applies the effect from the token to this attribute.
	// The node is a single attribute effect within an attribute category node.
	void Parse(const DataNode &node, const Modifier *modifier = nullptr);

	// Parses an attribute into an AttributeAccessor or the original string.
	static AnyAttribute ParseAny(const std::string &attribute);

	// Gets the data format name of the effect, as used in the new syntax. This also supports
	// variants, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
	// multiplier effect.
	static std::string GetEffectName(AttributeEffectType effect);
	// Gets the data format name of the category, as used in the new syntax. This also supports
	// variants.
	static std::string GetCategoryName(AttributeCategory category);
	// Gets the old-style name of the attribute.
	static std::string GetLegacyName(const AnyAttribute &attribute);

	// Gets the category of this attribute.
	AttributeCategory Category() const;
	// Gets the effect of this attribute.
	const std::map<AttributeEffectType, AttributeEffect> &Effects() const;
	// Adds a new effect to this attribute.
	void AddEffect(AttributeEffect effect);
	// Gets an existing effect, or nullptr.
	const AttributeEffect *GetEffect(AttributeEffectType type) const;
	AttributeEffect *GetEffect(AttributeEffectType type);

	void Add(const Attribute &other, double multiplier = 1.);

	// Category-based comparators.
	bool operator==(const Attribute &other) const;
	bool operator<(const Attribute &other) const;

private:
	// The category and effects of this attribute.
	AttributeCategory category;
	std::map<AttributeEffectType, AttributeEffect> effects;
};
