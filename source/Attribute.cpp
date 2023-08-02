/* Attribute.cpp
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

#include "Attribute.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

using namespace std;

const string Attribute::effectNames[] = {"shields", "hull", "thrust", "reverse thrust", "turn",
		"cooling", "active cooling", "cloak", "force", "energy", "fuel", "heat", "discharge", "corrosion", "leak", "burn",
		"ion", "scramble", "slowing", "disruption", "disabled", "minable", "ramscoop", "piercing", "delay", "depleted delay"};

const string Attribute::categoryNames[] = {"shield generation", "hull repair rate",
		GetEffectName(THRUST), GetEffectName(REVERSE_THRUST), GetEffectName(TURN), GetEffectName(COOLING),
		GetEffectName(ACTIVE_COOLING), GetEffectName(CLOAK), "afterburner thrust", "firing", "protection",
		"resistance", "damage", "capacity"};

bool Attribute::cached = false;



// Creates a new categorized attribute. Use -1 if there is no category or effect in the definition.
Attribute::Attribute(AttributeCategory category, AttributeEffect effect, AttributeEffect secondary, bool usePreferred)
{
	if(usePreferred)
	{
		if(category == -1)
			category = PASSIVE;
		if(category == PASSIVE && effect <= CLOAK && effect > HULL)
			category = static_cast<AttributeCategory>(effect);
		else if(effect == -1 && category <= CLOAKING)
			effect = static_cast<AttributeEffect>(category);
		else if(effect == -1 && category == AFTERBURNING)
			effect = THRUST;
		if(category == PASSIVE && effect == PIERCING)
			category = DAMAGE;
		if(category == COOL && effect == ENERGY)
			category = ACTIVE_COOL;
	}
	this->category = category;
	this->effect = effect;
	this->secondary = secondary;
}



// Gets the data format name of the category, as used in the new syntax.
string Attribute::GetCategoryName(const AttributeCategory category)
{
	if(category >= ATTRIBUTE_CATEGORY_COUNT || category < 0)
		return "";
	return categoryNames[category];
}



// Gets the data format name of the effect, as used in the new syntax. This also supports
// multipliers, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
// multiplier effect.
string Attribute::GetEffectName(const AttributeEffect effect)
{
	if(effect >= ATTRIBUTE_EFFECT_COUNT * 2)
		return "relative " + GetEffectName(static_cast<AttributeEffect>(effect - 2 * ATTRIBUTE_EFFECT_COUNT));
	if(effect >= ATTRIBUTE_EFFECT_COUNT)
		return GetEffectName(static_cast<AttributeEffect>(effect - ATTRIBUTE_EFFECT_COUNT)) + " multiplier";
	if(effect < 0)
		return "";
	return effectNames[effect];
}



// Calculates what the legacy name of the category-effect pair is. These results are cached
// for faster access via GetLegacyName().
string Attribute::CalculateLegacyName() const
{
	if(effect == -1)
		return Attribute::GetCategoryName(category);
	if(category == PASSIVE || category == -1)
	{
		if(effect == HULL)
			return "hull";
		if(effect == SHIELDS)
			return "shields";
		if(effect == RAMSCOOP)
			return "ramscoop";
		return Attribute::GetEffectName(effect) + (secondary == -1 ? "" : " " + Attribute::GetEffectName(secondary))
				+ " capacity";
	}

	int effectType = effect % ATTRIBUTE_EFFECT_COUNT;
	string effectName = Attribute::GetEffectName(static_cast<AttributeEffect>(effectType));
	string secondaryName = Attribute::GetEffectName(static_cast<AttributeEffect>(secondary % ATTRIBUTE_EFFECT_COUNT));
	string categoryName = Attribute::GetCategoryName(category);

	if(effectType == HULL)
		effectName = "hull";
	if(category == PASSIVE)
		categoryName = "";
	else if((category == CLOAKING || category == THRUSTING || category == REVERSE_THRUSTING || category == TURNING)
			&& (effect != -1 && static_cast<int>(category) != static_cast<int>(effect)))
		categoryName += "ing";
	else if(category == SHIELD_GENERATION && effectType != SHIELDS)
		categoryName = "shield";
	else if(category == HULL_REPAIR && effectType != HULL)
		categoryName = "hull";
	else if(category == HULL_REPAIR && (effect == HULL || effect == HULL + 2 * ATTRIBUTE_EFFECT_COUNT))
		categoryName = "hull repair rate";
	else if(category == HULL_REPAIR && effectType == HULL)
		categoryName = "hull repair";
	else if(category == AFTERBURNING)
		categoryName = "afterburner";
	else if(category == RESISTANCE && secondaryName != "")
		categoryName += " " + secondaryName;
	else if(category == DAMAGE && effectType == PIERCING)
		categoryName = "";
	else if(category == DAMAGE && effectType == FORCE)
		categoryName = "hit";
	else if(category == ACTIVE_COOL && effectType != ACTIVE_COOLING)
		categoryName = "cooling";
	else if(category == DAMAGE && effectType == SCRAMBLE)
		effectName = "scrambling";
	if(effectType == LEAK && (category == THRUSTING || category == REVERSE_THRUSTING || category == TURNING
			|| category == AFTERBURNING))
		effectName = "leakage";
	if((category >= PROTECTION && category <= DAMAGE) && effectType == SHIELDS)
		effectName = "shield";
	if(category == HULL_REPAIR && effectType == DELAY)
		return "repair " + effectName;
	if(category == HULL_REPAIR && effectType == DEPLETED_DELAY)
		return "disabled repair delay";
	if(category == SHIELD_GENERATION && effectType == DELAY)
		return "shield " + effectName;
	if(category == SHIELD_GENERATION && effectType == DEPLETED_DELAY)
		return "depleted shield delay";
	if(category == SHIELD_GENERATION && effectType == HULL)
		effectName = "repair";

	string composite;
	// shortcuts: "energy generation energy" is "energy generation", etc.
	if(static_cast<int>(category) == effectType && category <= CLOAKING)
		composite = categoryName;
	else if(category == PROTECTION || category == RESISTANCE || (category == DAMAGE && effect != FORCE))
		composite = effectName + ((effectName == "" || categoryName == "") ? "" : " ") + categoryName;
	else
		composite = categoryName + ((effectName == "" || categoryName == "") ? "" : " ") + effectName;

	if(IsRelative())
		composite = "relative " + composite;
	if(IsMultiplier())
		composite += " multiplier";
	return composite;
}



namespace {
	// Cached mappings between the two formats. The indexing of newToOld is offset by one compared to the enum values.
	string newToOld[ATTRIBUTE_CATEGORY_COUNT + 1][ATTRIBUTE_EFFECT_COUNT * 4 + 1][ATTRIBUTE_EFFECT_COUNT * 4 + 1];
	unordered_map<string, Attribute*> oldToNew;
}



// Maps the attribute to the legacy single string format.
string Attribute::GetLegacyName() const
{
	if(!cached)
		PrepareCache();
	return newToOld[category + 1][effect + 1][secondary + 1];
}



// Gets the attribute for the specified token, if any.
Attribute *Attribute::Parse(const string &token)
{
	if(!cached)
		PrepareCache();
	auto it = oldToNew.find(token);
	if(it == oldToNew.end())
		return nullptr;
	return it->second;
}



// Creates a cache for faster data loading.
void Attribute::PrepareCache()
{
	for(int effect = -1; effect < ATTRIBUTE_EFFECT_COUNT * 4; effect++)
	{
		for(int category = -1; category < ATTRIBUTE_CATEGORY_COUNT; category++)
		{
			for(int secondary = -1; secondary < (ATTRIBUTE_EFFECT_COUNT * 4) * (category == RESISTANCE); secondary++)
			{
				Attribute *a = new Attribute(static_cast<AttributeCategory>(category), static_cast<AttributeEffect>(effect),
						static_cast<AttributeEffect>(secondary));
				string s = a->CalculateLegacyName();
				newToOld[category + 1][effect + 1][secondary + 1] = s;
				if(oldToNew.find(s) == oldToNew.end())
					oldToNew[s] = a;
				else
					delete a;
			}
		}
	}
	// ensure that effect names are always recognized
	for(int effect = 0; effect < ATTRIBUTE_EFFECT_COUNT * 4; effect++)
	{
		string s = GetEffectName(static_cast<AttributeEffect>(effect));
		if(oldToNew.find(s) == oldToNew.end())
			oldToNew[s] = new Attribute(static_cast<AttributeCategory>(-1), static_cast<AttributeEffect>(effect),
					static_cast<AttributeEffect>(-1));;
	}
	cached = true;
}



// Checks whether the specified combination is fully supported by the engine.
bool Attribute::IsSupported() const
{
	if(effect == -1)
		return false;
	if(secondary != -1)
	{
		if(category != RESISTANCE)
			return false;
		if(effect >= ATTRIBUTE_EFFECT_COUNT || effect == PIERCING)
			return false;
		if(!Attribute(category, effect).IsSupported())
			return false;
		return secondary == ENERGY || secondary == HEAT || secondary == FUEL;
	}
	if(IsRelative())
	{
		if(IsMultiplier())
			return false;
		int effectType = effect % ATTRIBUTE_EFFECT_COUNT;
		switch(category)
		{
			case DAMAGE:
				if(effectType == DISABLED || effectType == MINABLE)
					return true;
			case FIRING:
				if(effectType <= HULL || (effectType >= ENERGY && effectType <= HEAT))
					return true;
			default:
				return false;
		}
	}
	if(IsMultiplier())
	{
		if(category != SHIELD_GENERATION && category != HULL_REPAIR)
			return false;
		int effectType = effect % ATTRIBUTE_EFFECT_COUNT;
		return effectType == ENERGY || effectType == HEAT || effectType == FUEL || effectType == category;
	}
	if(effect == THRUST && category == AFTERBURNING)
		return true;
	if(effect >= THRUST && effect <= CLOAK)
		return static_cast<int>(effect) == static_cast<int>(category);
	if(effect >= ENERGY && effect <= HEAT)
		return (category != COOL && category != ACTIVE_COOL && category != RESISTANCE) ||
				(effect == ENERGY && category == ACTIVE_COOL);
	if(effect >= DISCHARGE && effect <= DISRUPTION)
		return (category >= AFTERBURNING && category <= DAMAGE) || (category >= THRUSTING && category <= TURNING);
	switch(effect)
	{
		case SHIELDS:
		case HULL:
			if(category <= HULL_REPAIR && static_cast<int>(effect) != static_cast<int>(category))
				return false;
			return (category < COOL || category > CLOAKING) && category != RESISTANCE;
		case FORCE:
			return category == FIRING || category == PROTECTION || category == DAMAGE;
		case DISABLED:
		case MINABLE:
			return category == DAMAGE;
		case PIERCING:
			return category == DAMAGE || category == PROTECTION || category == RESISTANCE;
		case DELAY:
		case DEPLETED_DELAY:
			return category == HULL_REPAIR || category == SHIELD_GENERATION;
		case RAMSCOOP:
			return category == PASSIVE;
		default:
			return false;
	}
}



// Checks whether this attribute is a requirement for its category.
// Required attributes mark resource consumption when an action is taken.
bool Attribute::IsRequirement() const
{
	if(category == -1 || category == PASSIVE || category == DAMAGE || category == PROTECTION
			|| category == COOL || effect == -1)
		return false;
	if(static_cast<int>(category) == static_cast<int>(effect) && category <= CLOAKING)
		return false;
	if(effect <= HULL || effect == ENERGY || effect == FUEL || effect == DELAY)
		return true;
	return false;
}



// Checks whether this attribute is a multiplier.
bool Attribute::IsMultiplier() const
{
	return effect >= ATTRIBUTE_EFFECT_COUNT && (effect < ATTRIBUTE_EFFECT_COUNT * 2
			|| effect >= ATTRIBUTE_EFFECT_COUNT * 3);
}



// Creates a multiplier for this attribute, if not already a multiplier.
Attribute Attribute::Multiplier() const
{
	if(IsMultiplier())
		return *this;
	return Attribute(category, static_cast<AttributeEffect>(effect + ATTRIBUTE_EFFECT_COUNT), secondary);
}



// Checks whether this attribute is relative.
bool Attribute::IsRelative() const
{
	return effect >= ATTRIBUTE_EFFECT_COUNT * 2;
}



// Creates a relative version of this attribute, if not already relative.
Attribute Attribute::Relative() const
{
	if(IsRelative())
		return *this;
	return Attribute(category, static_cast<AttributeEffect>(effect + 2 * ATTRIBUTE_EFFECT_COUNT), secondary);
}



// Gets the category of this attribute.
AttributeCategory Attribute::Category() const
{
	return category;
}



// Gets the effect of this attribute.
AttributeEffect Attribute::Effect() const
{
	return effect;
}



// Gets the secondary effect of this attribute.
AttributeEffect Attribute::Secondary() const
{
	return secondary;
}



// Gets the minimum value of this attribute.
double Attribute::GetMinimumValue() const
{
	if(IsMultiplier())
		return -1.;
	if(category == PROTECTION && secondary == -1)
		return -0.99;
	return numeric_limits<double>::lowest();
}



template <>
bool Attribute::operator==(const Attribute &other) const
{
	return category == other.category && effect == other.effect && secondary == other.secondary;
}



template <class A>
bool Attribute::operator==(const A &other) const
{
	return false;
}



bool Attribute::operator<(const Attribute &other) const
{
	if(category == other.category)
	{
		if(effect == other.effect)
			return secondary < other.secondary;
		return effect < other.effect;
	}
	return category < other.category;
}
