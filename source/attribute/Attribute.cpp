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
#include <map>

using namespace std;

const string Attribute::effectNames[] = {"shields", "hull", "thrust", "reverse thrust", "turn", "active cooling",
										"ramscoop", "cloak", "cooling", "force", "energy", "fuel", "heat", "discharge", "corrosion", "leak", "burn",
										"ion", "scramble", "slowing", "disruption", "disabled", "minable", "piercing"};

// Some category names are always the same as the corresponding effect names.
const string Attribute::categoryNames[] = {"shield generation", "hull repair",
										GetEffectName(THRUST), GetEffectName(REVERSE_THRUST), GetEffectName(TURN), GetEffectName(ACTIVE_COOLING),
										GetEffectName(RAMSCOOP), GetEffectName(CLOAK), "afterburner thrust", "firing", "protection",
										"resistance", "damage", "capacity"};




namespace {
	// Cached mappings between the two formats.
	// Any attribute without an effect will not be present in newToOld.
	map<string, Attribute> oldToNew = {
			{"capacity", Attribute(PASSIVE)},
			{"energy capacity", AttributeAccess(PASSIVE, ENERGY)},
			{"shields", AttributeAccess(PASSIVE, SHIELDS)},
			{"shield multiplier", AttributeAccess(PASSIVE, SHIELDS).Multiplier()},
			{"shield generation", AttributeAccess(SHIELD_GENERATION, SHIELDS)},
			{"shield energy", AttributeAccess(SHIELD_GENERATION, ENERGY)},
			{"shield heat", AttributeAccess(SHIELD_GENERATION, HEAT)},
			{"shield fuel", AttributeAccess(SHIELD_GENERATION, FUEL)},
			{"hull", AttributeAccess(PASSIVE, HULL)},
			{"hull multiplier", AttributeAccess(PASSIVE, HULL).Multiplier()},
			{"hull repair rate", AttributeAccess(HULL_REPAIR, HULL)},
			{"hull energy", AttributeAccess(HULL_REPAIR, ENERGY)},
			{"hull heat", AttributeAccess(HULL_REPAIR, HEAT)},
			{"hull fuel", AttributeAccess(HULL_REPAIR, FUEL)},
			{"shield generation multiplier", AttributeAccess(SHIELD_GENERATION, SHIELDS).Multiplier()},
			{"shield energy multiplier", AttributeAccess(SHIELD_GENERATION, ENERGY).Multiplier()},
			{"shield heat multiplier", AttributeAccess(SHIELD_GENERATION, HEAT).Multiplier()},
			{"shield fuel multiplier", AttributeAccess(SHIELD_GENERATION, FUEL).Multiplier()},
			{"hull repair multiplier", AttributeAccess(HULL_REPAIR, HULL).Multiplier()},
			{"hull energy multiplier", AttributeAccess(HULL_REPAIR, ENERGY).Multiplier()},
			{"hull heat multiplier", AttributeAccess(HULL_REPAIR, HEAT).Multiplier()},
			{"hull fuel multiplier", AttributeAccess(HULL_REPAIR, FUEL).Multiplier()},
			{"ramscoop", AttributeAccess(RAMSCOOPING, RAMSCOOP)},
			{"fuel capacity", AttributeAccess(PASSIVE, FUEL)},
			{"thrust", AttributeAccess(THRUSTING, THRUST)},
			{"thrusting energy", AttributeAccess(THRUSTING, ENERGY)},
			{"thrusting heat", AttributeAccess(THRUSTING, HEAT)},
			{"thrusting shields", AttributeAccess(THRUSTING, SHIELDS)},
			{"thrusting hull", AttributeAccess(THRUSTING, HULL)},
			{"thrusting fuel", AttributeAccess(THRUSTING, FUEL)},
			{"thrusting discharge", AttributeAccess(THRUSTING, DISCHARGE)},
			{"thrusting corrosion", AttributeAccess(THRUSTING, CORROSION)},
			{"thrusting ion", AttributeAccess(THRUSTING, ION)},
			{"thrusting scramble", AttributeAccess(THRUSTING, SCRAMBLE)},
			{"thrusting leakage", AttributeAccess(THRUSTING, LEAK)},
			{"thrusting burn", AttributeAccess(THRUSTING, BURN)},
			{"thrusting slowing", AttributeAccess(THRUSTING, SLOWING)},
			{"thrusting disruption", AttributeAccess(THRUSTING, DISRUPTION)},
			{"turn", AttributeAccess(TURNING, TURN)},
			{"turning energy", AttributeAccess(TURNING, ENERGY)},
			{"turning heat", AttributeAccess(TURNING, HEAT)},
			{"turning shields", AttributeAccess(TURNING, SHIELDS)},
			{"turning hull", AttributeAccess(TURNING, HULL)},
			{"turning fuel", AttributeAccess(TURNING, FUEL)},
			{"turning discharge", AttributeAccess(TURNING, DISCHARGE)},
			{"turning corrosion", AttributeAccess(TURNING, CORROSION)},
			{"turning ion", AttributeAccess(TURNING, ION)},
			{"turning scramble", AttributeAccess(TURNING, SCRAMBLE)},
			{"turning leakage", AttributeAccess(TURNING, LEAK)},
			{"turning burn", AttributeAccess(TURNING, BURN)},
			{"turning slowing", AttributeAccess(TURNING, SLOWING)},
			{"turning disruption", AttributeAccess(TURNING, DISRUPTION)},
			{"reverse thrust", AttributeAccess(REVERSE_THRUSTING, REVERSE_THRUST)},
			{"reverse thrusting energy", AttributeAccess(REVERSE_THRUSTING, ENERGY)},
			{"reverse thrusting heat", AttributeAccess(REVERSE_THRUSTING, HEAT)},
			{"reverse thrusting shields", AttributeAccess(REVERSE_THRUSTING, SHIELDS)},
			{"reverse thrusting hull", AttributeAccess(REVERSE_THRUSTING, HULL)},
			{"reverse thrusting fuel", AttributeAccess(REVERSE_THRUSTING, FUEL)},
			{"reverse thrusting discharge", AttributeAccess(REVERSE_THRUSTING, DISCHARGE)},
			{"reverse thrusting corrosion", AttributeAccess(REVERSE_THRUSTING, CORROSION)},
			{"reverse thrusting ion", AttributeAccess(REVERSE_THRUSTING, ION)},
			{"reverse thrusting scramble", AttributeAccess(REVERSE_THRUSTING, SCRAMBLE)},
			{"reverse thrusting leakage", AttributeAccess(REVERSE_THRUSTING, LEAK)},
			{"reverse thrusting burn", AttributeAccess(REVERSE_THRUSTING, BURN)},
			{"reverse thrusting slowing", AttributeAccess(REVERSE_THRUSTING, SLOWING)},
			{"reverse thrusting disruption", AttributeAccess(REVERSE_THRUSTING, DISRUPTION)},
			{"afterburner thrust", AttributeAccess(AFTERBURNING, THRUST)},
			{"afterburner energy", AttributeAccess(AFTERBURNING, ENERGY)},
			{"afterburner heat", AttributeAccess(AFTERBURNING, HEAT)},
			{"afterburner shields", AttributeAccess(AFTERBURNING, SHIELDS)},
			{"afterburner hull", AttributeAccess(AFTERBURNING, HULL)},
			{"afterburner fuel", AttributeAccess(AFTERBURNING, FUEL)},
			{"afterburner discharge", AttributeAccess(AFTERBURNING, DISCHARGE)},
			{"afterburner corrosion", AttributeAccess(AFTERBURNING, CORROSION)},
			{"afterburner ion", AttributeAccess(AFTERBURNING, ION)},
			{"afterburner scramble", AttributeAccess(AFTERBURNING, SCRAMBLE)},
			{"afterburner leakage", AttributeAccess(AFTERBURNING, LEAK)},
			{"afterburner burn", AttributeAccess(AFTERBURNING, BURN)},
			{"afterburner slowing", AttributeAccess(AFTERBURNING, SLOWING)},
			{"afterburner disruption", AttributeAccess(AFTERBURNING, DISRUPTION)},
			{"cooling", AttributeAccess(PASSIVE, COOLING)},
			{"active cooling", AttributeAccess(ACTIVE_COOL, ACTIVE_COOLING)},
			{"cooling energy", AttributeAccess(ACTIVE_COOL, ENERGY)},
			{"heat capacity", AttributeAccess(PASSIVE, HEAT)},
			{"cloak", AttributeAccess(CLOAKING, CLOAK)},
			{"cloaking energy", AttributeAccess(CLOAKING, ENERGY)},
			{"cloaking fuel", AttributeAccess(CLOAKING, FUEL)},
			{"cloaking heat", AttributeAccess(CLOAKING, HEAT)},
			{"cloak shield protection", AttributeAccess(PROTECTION, CLOAK, SHIELDS)},
			{"cloak hull protection", AttributeAccess(PROTECTION, CLOAK, HULL)},
			{"disruption resistance", AttributeAccess(RESISTANCE, DISRUPTION)},
			{"disruption resistance energy", AttributeAccess(RESISTANCE, DISRUPTION, ENERGY)},
			{"disruption resistance heat", AttributeAccess(RESISTANCE, DISRUPTION, HEAT)},
			{"disruption resistance fuel", AttributeAccess(RESISTANCE, DISRUPTION, FUEL)},
			{"ion resistance", AttributeAccess(RESISTANCE, ION)},
			{"ion resistance energy", AttributeAccess(RESISTANCE, ION, ENERGY)},
			{"ion resistance heat", AttributeAccess(RESISTANCE, ION, HEAT)},
			{"ion resistance fuel", AttributeAccess(RESISTANCE, ION, FUEL)},
			{"scramble resistance", AttributeAccess(RESISTANCE, SCRAMBLE)},
			{"scramble resistance energy", AttributeAccess(RESISTANCE, SCRAMBLE, ENERGY)},
			{"scramble resistance heat", AttributeAccess(RESISTANCE, SCRAMBLE, HEAT)},
			{"scramble resistance fuel", AttributeAccess(RESISTANCE, SCRAMBLE, FUEL)},
			{"slowing resistance", AttributeAccess(RESISTANCE, SLOWING)},
			{"slowing resistance energy", AttributeAccess(RESISTANCE, SLOWING, ENERGY)},
			{"slowing resistance heat", AttributeAccess(RESISTANCE, SLOWING, HEAT)},
			{"slowing resistance fuel", AttributeAccess(RESISTANCE, SLOWING, FUEL)},
			{"discharge resistance", AttributeAccess(RESISTANCE, DISCHARGE)},
			{"discharge resistance energy", AttributeAccess(RESISTANCE, DISCHARGE, ENERGY)},
			{"discharge resistance heat", AttributeAccess(RESISTANCE, DISCHARGE, HEAT)},
			{"discharge resistance fuel", AttributeAccess(RESISTANCE, DISCHARGE, FUEL)},
			{"corrosion resistance", AttributeAccess(RESISTANCE, CORROSION)},
			{"corrosion resistance energy", AttributeAccess(RESISTANCE, CORROSION, ENERGY)},
			{"corrosion resistance heat", AttributeAccess(RESISTANCE, CORROSION, HEAT)},
			{"corrosion resistance fuel", AttributeAccess(RESISTANCE, CORROSION, FUEL)},
			{"leak resistance", AttributeAccess(RESISTANCE, LEAK)},
			{"leak resistance energy", AttributeAccess(RESISTANCE, LEAK, ENERGY)},
			{"leak resistance heat", AttributeAccess(RESISTANCE, LEAK, HEAT)},
			{"leak resistance fuel", AttributeAccess(RESISTANCE, LEAK, FUEL)},
			{"burn resistance", AttributeAccess(RESISTANCE, BURN)},
			{"burn resistance energy", AttributeAccess(RESISTANCE, BURN, ENERGY)},
			{"burn resistance heat", AttributeAccess(RESISTANCE, BURN, HEAT)},
			{"burn resistance fuel", AttributeAccess(RESISTANCE, BURN, FUEL)},
			{"piercing resistance", AttributeAccess(RESISTANCE, PIERCING)},
			{"disruption protection", AttributeAccess(PROTECTION, DISRUPTION)},
			{"energy protection", AttributeAccess(PROTECTION, ENERGY)},
			{"force protection", AttributeAccess(PROTECTION, FORCE)},
			{"fuel protection", AttributeAccess(PROTECTION, FUEL)},
			{"heat protection", AttributeAccess(PROTECTION, HEAT)},
			{"hull protection", AttributeAccess(PROTECTION, HULL)},
			{"ion protection", AttributeAccess(PROTECTION, ION)},
			{"scramble protection", AttributeAccess(PROTECTION, SCRAMBLE)},
			{"piercing protection", AttributeAccess(PROTECTION, PIERCING)},
			{"shield protection", AttributeAccess(PROTECTION, SHIELDS)},
			{"slowing protection", AttributeAccess(PROTECTION, SLOWING)},
			{"discharge protection", AttributeAccess(PROTECTION, DISCHARGE)},
			{"corrosion protection", AttributeAccess(PROTECTION, CORROSION)},
			{"leak protection", AttributeAccess(PROTECTION, LEAK)},
			{"burn protection", AttributeAccess(PROTECTION, BURN)},
			{"firing energy", AttributeAccess(FIRING, ENERGY)},
			{"firing force", AttributeAccess(FIRING, FORCE)},
			{"firing fuel", AttributeAccess(FIRING, FUEL)},
			{"firing heat", AttributeAccess(FIRING, HEAT)},
			{"firing hull", AttributeAccess(FIRING, HULL)},
			{"firing shields", AttributeAccess(FIRING, SHIELDS)},
			{"firing ion", AttributeAccess(FIRING, ION)},
			{"firing scramble", AttributeAccess(FIRING, SCRAMBLE)},
			{"firing slowing", AttributeAccess(FIRING, SLOWING)},
			{"firing disruption", AttributeAccess(FIRING, DISRUPTION)},
			{"firing discharge", AttributeAccess(FIRING, DISCHARGE)},
			{"firing corrosion", AttributeAccess(FIRING, CORROSION)},
			{"firing leak", AttributeAccess(FIRING, LEAK)},
			{"firing burn", AttributeAccess(FIRING, BURN)},
			{"relative firing energy", AttributeAccess(FIRING, ENERGY).Relative()},
			{"relative firing fuel", AttributeAccess(FIRING, FUEL).Relative()},
			{"relative firing heat", AttributeAccess(FIRING, HEAT).Relative()},
			{"relative firing hull", AttributeAccess(FIRING, HULL).Relative()},
			{"relative firing shields", AttributeAccess(FIRING, SHIELDS).Relative()},
			{"hit force", AttributeAccess(DAMAGE, FORCE)},
			{"piercing", AttributeAccess(DAMAGE, PIERCING)},
			{"shield damage", AttributeAccess(DAMAGE, SHIELDS)},
			{"hull damage", AttributeAccess(DAMAGE, HULL)},
			{"disabled damage", AttributeAccess(DAMAGE, DISABLED)},
			{"minable damage", AttributeAccess(DAMAGE, MINABLE)},
			{"heat damage", AttributeAccess(DAMAGE, HEAT)},
			{"fuel damage", AttributeAccess(DAMAGE, FUEL)},
			{"energy damage", AttributeAccess(DAMAGE, ENERGY)},
			{"relative shield damage", AttributeAccess(DAMAGE, SHIELDS).Relative()},
			{"relative hull damage", AttributeAccess(DAMAGE, HULL).Relative()},
			{"relative disabled damage", AttributeAccess(DAMAGE, DISABLED).Relative()},
			{"relative minable damage", AttributeAccess(DAMAGE, MINABLE).Relative()},
			{"relative heat damage", AttributeAccess(DAMAGE, HEAT).Relative()},
			{"relative fuel damage", AttributeAccess(DAMAGE, FUEL).Relative()},
			{"relative energy damage", AttributeAccess(DAMAGE, ENERGY).Relative()},
			{"ion damage", AttributeAccess(DAMAGE, ION)},
			{"scrambling damage", AttributeAccess(DAMAGE, SCRAMBLE)},
			{"disruption damage", AttributeAccess(DAMAGE, DISRUPTION)},
			{"slowing damage", AttributeAccess(DAMAGE, SLOWING)},
			{"discharge damage", AttributeAccess(DAMAGE, DISCHARGE)},
			{"corrosion damage", AttributeAccess(DAMAGE, CORROSION)},
			{"leak damage", AttributeAccess(DAMAGE, LEAK)},
			{"burn damage", AttributeAccess(DAMAGE, BURN)}
	};

	// Mapping of new-style AttributeAccess to legacy names.
	map<AttributeAccess, string> newToOld = [](){
		map<AttributeAccess, string> m;
		for(const auto &it : oldToNew)
			for(const auto &effect : it.second.Effects()) // There should only be a single effect
				m[{it.second.Category(), effect.second.Type()}] = it.first;
		return m;
	}();

	// The name of every individual effect, as used within category nodes.
	map<string, AttributeEffectType> allEffects = [](){
		map<string, AttributeEffectType> names;
		for(int i = 0; i < 4 * ATTRIBUTE_EFFECT_COUNT; i++)
		{
			AttributeEffectType type = static_cast<AttributeEffectType>(i);
			names.emplace(Attribute::GetEffectName(type), type);
		}
		return names;
	}();

	// The name of every category that has a base attribute. These categories can have a value defined in their node,
	// while others have to define all their effects in their children.
	map<string, Attribute> allBaseAttributes = [](){
		map<string, Attribute> names;
		for(int i = 0; i < 4 * ATTRIBUTE_CATEGORY_COUNT; i++)
		{
			AttributeCategory category = static_cast<AttributeCategory>(i);
			optional<AttributeEffectType> effect = AttributeAccess::GetBaseEffect(category);
			if(effect.has_value())
				names.emplace(Attribute::GetCategoryName(category), AttributeAccess(category, effect.value()));
		}
		return names;
	}();
}



// Creates a new categorized attribute. Use -1 if there is no category in the definition.
Attribute::Attribute(AttributeCategory category) : category(category)
{
}



// Copies an attribute and multiplies all of its values.
Attribute::Attribute(const Attribute &other, double multiplier) : category(other.category)
{
	for(const auto &item : other.effects)
		effects.insert({item.first, AttributeEffect(item.second.Type(),
		item.second.Value() * multiplier, item.second.Minimum())});
}



// Creates an attribute with a single initial effect
Attribute::Attribute(const AttributeAccess access, double value) : category(access.Category())
{
	effects.emplace(access.Effect(), AttributeEffect(access.Effect(), value, access.GetDefaultMinimum()));
}



// Gets the data format name of the category, as used in the new syntax.
string Attribute::GetCategoryName(const AttributeCategory category)
{
	if(category >= ATTRIBUTE_CATEGORY_COUNT)
		return GetEffectName(static_cast<AttributeEffectType>(category / ATTRIBUTE_CATEGORY_COUNT - 1)) + " " +
				GetCategoryName(static_cast<AttributeCategory>(category % ATTRIBUTE_CATEGORY_COUNT));
	return category >= 0 ? categoryNames[category] : "";
}



// Gets the data format name of the effect, as used in the new syntax. This also supports
// multipliers, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
// multiplier effect.
string Attribute::GetEffectName(const AttributeEffectType effect)
{
	if(effect >= ATTRIBUTE_EFFECT_COUNT * 2)
		return "relative " + GetEffectName(static_cast<AttributeEffectType>(effect - 2 * ATTRIBUTE_EFFECT_COUNT));
	if(effect >= ATTRIBUTE_EFFECT_COUNT)
		return GetEffectName(static_cast<AttributeEffectType>(effect - ATTRIBUTE_EFFECT_COUNT)) + " multiplier";
	if(effect < 0)
		return "";
	return effectNames[effect];
}



// Gets the old-style name of the attribute. Returns an empty string if the attribute is not supported
// in the old format.
string Attribute::GetLegacyName(const AttributeAccess access)
{
	auto it = newToOld.find(access);
	if(it == newToOld.end())
	{
		// TODO: find a better solution, or resort to manually mapping all supported attributes.
		// This is just a stopgap measure to avoid hard crashes during testing; not to be relied upon
		auto baseType = static_cast<AttributeEffectType>(access.Effect() % static_cast<int>(ATTRIBUTE_EFFECT_COUNT));
		string prefix = access.Effect() >= 2 * ATTRIBUTE_EFFECT_COUNT ? "relative " : "";
		string suffix = (access.Effect() >= ATTRIBUTE_EFFECT_COUNT && prefix.empty()) ? " multiplier" : "";
		string category = GetCategoryName(access.Category()) + " ";
		string effect = GetEffectName(baseType);
		string final = prefix + category + effect + suffix;
		newToOld.insert(it, {access, final});
		return final;
	}
	return it->second;
}



string Attribute::GetLegacyName(const AnyAttribute &attribute)
{
	return attribute.index() ? Attribute::GetLegacyName(get<1>(attribute)) : get<0>(attribute);
}



// Gets the attribute for the specified token, if any.
Attribute *Attribute::Parse(const string &token)
{
	// Check if it's a legacy attribute name.
	auto it = oldToNew.find(token);
	if(it == oldToNew.end())
	{
		// Check if it's a category name.
		auto it = allBaseAttributes.find(token);
		if(it == allBaseAttributes.end())
			return nullptr;
		return &(it->second);
	}
	return &(it->second);
}



// Applies the effect from the token to this attribute.
// The node is a single attribute effect within an attribute category node.
void Attribute::Parse(const DataNode &node)
{
	const string &text = node.Token(0);
	if(node.Size() >= 2)
	{
		auto it = allEffects.find(text);
		if(it == allEffects.end())
			node.PrintTrace("Skipping unrecognized attribute effect:");
		else
		{
			AttributeAccess access{category, it->second};
			AddEffect({access.Effect(), node.Value(1), access.GetDefaultMinimum()});
		}
	}
	else
		node.PrintTrace("Skipping attribute effect without value:");
}



// Parses an attribute into an AttributeAccess or the original string.
AnyAttribute Attribute::ParseAny(const std::string &attribute)
{
	auto it = oldToNew.find(attribute);
	if(it == oldToNew.end())
		return attribute;
	for(const auto &item : it->second.Effects())
		if(item.second.Value() != 0.)
			return AttributeAccess(it->second.Category(), item.first);
	return attribute;
}



// Gets the category of this attribute.
const AttributeCategory Attribute::Category() const
{
	return category;
}



// Gets the effect of this attribute.
const std::map<AttributeEffectType, AttributeEffect> &Attribute::Effects() const
{
	return effects;
}



// Gets an existing effect, or nullptr.
const AttributeEffect *Attribute::GetEffect(const AttributeEffectType type) const
{
	const auto &it = effects.find(type);
	if(it == effects.end())
		return nullptr;
	return &(it->second);
}



AttributeEffect *Attribute::GetEffect(const AttributeEffectType type)
{
	auto it = effects.find(type);
	if(it == effects.end())
		return nullptr;
	return &(it->second);
}



// Adds a new effect to this attribute.
void Attribute::AddEffect(const AttributeEffect effect)
{
	auto it = effects.find(effect.Type());
	if(it == effects.end())
		effects.insert({effect.Type(), effect});
	else
		it->second.Add(effect.Value());
}



bool Attribute::operator<(const Attribute &other) const
{
	return category < other.category;
}



bool Attribute::operator<(const AttributeCategory &other) const
{
	return category < other;
}
