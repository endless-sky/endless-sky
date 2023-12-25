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

#include "AttributeAccess.h"
#include "AttributeCategory.h"
#include "AttributeEffect.h"
#include "../DataNode.h"

#include <map>
#include <string>
#include <variant>

using AnyAttribute = std::variant<std::string, AttributeAccess>;

class Attribute {
public:
	// Creates a new categorized attribute. Use -1 if there is no category in the definition.
	explicit Attribute(AttributeCategory category);
	// Copies an attribute and multiplies all of its values.
	Attribute(const Attribute &other, double multiplier = 1.);
	// Creates an attribute with a single initial effect
	Attribute(const AttributeAccess access, double value = 1.);
	// Gets the attribute for the specified token, if any.
	static Attribute *Parse(const std::string &token);

	// Applies the effect from the token to this attribute.
	void Parse(const DataNode &node);

	// Parses an attribute into an AttributeAccess or the original string.
	static AnyAttribute ParseAny(const std::string &attribute);

	// Gets the data format name of the effect, as used in the new syntax. This also supports
	// variants, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
	// multiplier effect.
	static std::string GetEffectName(const AttributeEffectType effect);
	// Gets the data format name of the category, as used in the new syntax. This also supports
	// variants.
	static std::string GetCategoryName(const AttributeCategory category);
	// Gets the old-style name of the attribute.
	static std::string GetLegacyName(const AttributeAccess access);
	static std::string GetLegacyName(const AnyAttribute &attribute);

	// Gets the category of this attribute.
	const AttributeCategory Category() const;
	// Gets the effect of this attribute.
	const std::map<AttributeEffectType, AttributeEffect> &Effects() const;
	// Adds a new effect to this attribute.
	void AddEffect(const AttributeEffect effect);
	// Gets an existing effect, or nullptr.
	const AttributeEffect *GetEffect(const AttributeEffectType type) const;
	AttributeEffect *GetEffect(const AttributeEffectType type);

	Attribute &operator=(Attribute &other) = delete;

	// Category-based comparators
	template <class A>
	bool operator==(const A &other) const;
	bool operator<(const Attribute &other) const;
	bool operator<(const AttributeCategory &other) const;

private:
	// The name of each effect as used in the data files
	static const std::string effectNames[];
	// The name of each category as used in the data files. Some are linked to the name of the effect which they expand on.
	static const std::string categoryNames[];

	// The category and effects of this attribute
	const AttributeCategory category;
	std::map<AttributeEffectType, AttributeEffect> effects;
};



// Compare two variants
inline bool operator<(const AnyAttribute &first, const AnyAttribute &second)
{
	if(first.index() == second.index())
	{
		if(first.index())
			return std::get<1>(first) < std::get<1>(second);
		return std::get<0>(first) < std::get<0>(second);
	}
	return first.index() < second.index();
}



template <>
inline bool Attribute::operator==(const Attribute &other) const
{
	return category == other.category;
}



template <>
inline bool Attribute::operator==(const AttributeCategory &other) const
{
	return category == other;
}



template <class A>
inline bool Attribute::operator==(const A &other) const
{
	return false;
}



#endif
