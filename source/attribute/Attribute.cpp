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
			{"energy capacity", AttributeAccessor(PASSIVE, ENERGY)},
			{"shields", AttributeAccessor(PASSIVE, SHIELDS)},
			{"shield multiplier", AttributeAccessor(PASSIVE, SHIELDS).Multiplier()},
			{"shield generation", AttributeAccessor(SHIELD_GENERATION, SHIELDS)},
			{"shield energy", AttributeAccessor(SHIELD_GENERATION, ENERGY)},
			{"shield heat", AttributeAccessor(SHIELD_GENERATION, HEAT)},
			{"shield fuel", AttributeAccessor(SHIELD_GENERATION, FUEL)},
			{"hull", AttributeAccessor(PASSIVE, HULL)},
			{"hull multiplier", AttributeAccessor(PASSIVE, HULL).Multiplier()},
			{"hull repair rate", AttributeAccessor(HULL_REPAIR, HULL)},
			{"hull energy", AttributeAccessor(HULL_REPAIR, ENERGY)},
			{"hull heat", AttributeAccessor(HULL_REPAIR, HEAT)},
			{"hull fuel", AttributeAccessor(HULL_REPAIR, FUEL)},
			{"shield generation multiplier", AttributeAccessor(SHIELD_GENERATION, SHIELDS).Multiplier()},
			{"shield energy multiplier", AttributeAccessor(SHIELD_GENERATION, ENERGY).Multiplier()},
			{"shield heat multiplier", AttributeAccessor(SHIELD_GENERATION, HEAT).Multiplier()},
			{"shield fuel multiplier", AttributeAccessor(SHIELD_GENERATION, FUEL).Multiplier()},
			{"hull repair multiplier", AttributeAccessor(HULL_REPAIR, HULL).Multiplier()},
			{"hull energy multiplier", AttributeAccessor(HULL_REPAIR, ENERGY).Multiplier()},
			{"hull heat multiplier", AttributeAccessor(HULL_REPAIR, HEAT).Multiplier()},
			{"hull fuel multiplier", AttributeAccessor(HULL_REPAIR, FUEL).Multiplier()},
			{"ramscoop", AttributeAccessor(RAMSCOOPING, RAMSCOOP)},
			{"fuel capacity", AttributeAccessor(PASSIVE, FUEL)},
			{"thrust", AttributeAccessor(THRUSTING, THRUST)},
			{"thrusting energy", AttributeAccessor(THRUSTING, ENERGY)},
			{"thrusting heat", AttributeAccessor(THRUSTING, HEAT)},
			{"thrusting shields", AttributeAccessor(THRUSTING, SHIELDS)},
			{"thrusting hull", AttributeAccessor(THRUSTING, HULL)},
			{"thrusting fuel", AttributeAccessor(THRUSTING, FUEL)},
			{"thrusting discharge", AttributeAccessor(THRUSTING, DISCHARGE)},
			{"thrusting corrosion", AttributeAccessor(THRUSTING, CORROSION)},
			{"thrusting ion", AttributeAccessor(THRUSTING, ION)},
			{"thrusting scramble", AttributeAccessor(THRUSTING, SCRAMBLE)},
			{"thrusting leakage", AttributeAccessor(THRUSTING, LEAK)},
			{"thrusting burn", AttributeAccessor(THRUSTING, BURN)},
			{"thrusting slowing", AttributeAccessor(THRUSTING, SLOWING)},
			{"thrusting disruption", AttributeAccessor(THRUSTING, DISRUPTION)},
			{"turn", AttributeAccessor(TURNING, TURN)},
			{"turning energy", AttributeAccessor(TURNING, ENERGY)},
			{"turning heat", AttributeAccessor(TURNING, HEAT)},
			{"turning shields", AttributeAccessor(TURNING, SHIELDS)},
			{"turning hull", AttributeAccessor(TURNING, HULL)},
			{"turning fuel", AttributeAccessor(TURNING, FUEL)},
			{"turning discharge", AttributeAccessor(TURNING, DISCHARGE)},
			{"turning corrosion", AttributeAccessor(TURNING, CORROSION)},
			{"turning ion", AttributeAccessor(TURNING, ION)},
			{"turning scramble", AttributeAccessor(TURNING, SCRAMBLE)},
			{"turning leakage", AttributeAccessor(TURNING, LEAK)},
			{"turning burn", AttributeAccessor(TURNING, BURN)},
			{"turning slowing", AttributeAccessor(TURNING, SLOWING)},
			{"turning disruption", AttributeAccessor(TURNING, DISRUPTION)},
			{"reverse thrust", AttributeAccessor(REVERSE_THRUSTING, REVERSE_THRUST)},
			{"reverse thrusting energy", AttributeAccessor(REVERSE_THRUSTING, ENERGY)},
			{"reverse thrusting heat", AttributeAccessor(REVERSE_THRUSTING, HEAT)},
			{"reverse thrusting shields", AttributeAccessor(REVERSE_THRUSTING, SHIELDS)},
			{"reverse thrusting hull", AttributeAccessor(REVERSE_THRUSTING, HULL)},
			{"reverse thrusting fuel", AttributeAccessor(REVERSE_THRUSTING, FUEL)},
			{"reverse thrusting discharge", AttributeAccessor(REVERSE_THRUSTING, DISCHARGE)},
			{"reverse thrusting corrosion", AttributeAccessor(REVERSE_THRUSTING, CORROSION)},
			{"reverse thrusting ion", AttributeAccessor(REVERSE_THRUSTING, ION)},
			{"reverse thrusting scramble", AttributeAccessor(REVERSE_THRUSTING, SCRAMBLE)},
			{"reverse thrusting leakage", AttributeAccessor(REVERSE_THRUSTING, LEAK)},
			{"reverse thrusting burn", AttributeAccessor(REVERSE_THRUSTING, BURN)},
			{"reverse thrusting slowing", AttributeAccessor(REVERSE_THRUSTING, SLOWING)},
			{"reverse thrusting disruption", AttributeAccessor(REVERSE_THRUSTING, DISRUPTION)},
			{"afterburner thrust", AttributeAccessor(AFTERBURNING, THRUST)},
			{"afterburner energy", AttributeAccessor(AFTERBURNING, ENERGY)},
			{"afterburner heat", AttributeAccessor(AFTERBURNING, HEAT)},
			{"afterburner shields", AttributeAccessor(AFTERBURNING, SHIELDS)},
			{"afterburner hull", AttributeAccessor(AFTERBURNING, HULL)},
			{"afterburner fuel", AttributeAccessor(AFTERBURNING, FUEL)},
			{"afterburner discharge", AttributeAccessor(AFTERBURNING, DISCHARGE)},
			{"afterburner corrosion", AttributeAccessor(AFTERBURNING, CORROSION)},
			{"afterburner ion", AttributeAccessor(AFTERBURNING, ION)},
			{"afterburner scramble", AttributeAccessor(AFTERBURNING, SCRAMBLE)},
			{"afterburner leakage", AttributeAccessor(AFTERBURNING, LEAK)},
			{"afterburner burn", AttributeAccessor(AFTERBURNING, BURN)},
			{"afterburner slowing", AttributeAccessor(AFTERBURNING, SLOWING)},
			{"afterburner disruption", AttributeAccessor(AFTERBURNING, DISRUPTION)},
			{"cooling", AttributeAccessor(PASSIVE, COOLING)},
			{"active cooling", AttributeAccessor(ACTIVE_COOL, ACTIVE_COOLING)},
			{"cooling energy", AttributeAccessor(ACTIVE_COOL, ENERGY)},
			{"heat capacity", AttributeAccessor(PASSIVE, HEAT)},
			{"cloak", AttributeAccessor(CLOAKING, CLOAK)},
			{"cloaking energy", AttributeAccessor(CLOAKING, ENERGY)},
			{"cloaking fuel", AttributeAccessor(CLOAKING, FUEL)},
			{"cloaking heat", AttributeAccessor(CLOAKING, HEAT)},
			{"cloak shield protection", AttributeAccessor(PROTECTION, CLOAK, SHIELDS)},
			{"cloak hull protection", AttributeAccessor(PROTECTION, CLOAK, HULL)},
			{"disruption resistance", AttributeAccessor(RESISTANCE, DISRUPTION)},
			{"disruption resistance energy", AttributeAccessor(RESISTANCE, DISRUPTION, ENERGY)},
			{"disruption resistance heat", AttributeAccessor(RESISTANCE, DISRUPTION, HEAT)},
			{"disruption resistance fuel", AttributeAccessor(RESISTANCE, DISRUPTION, FUEL)},
			{"ion resistance", AttributeAccessor(RESISTANCE, ION)},
			{"ion resistance energy", AttributeAccessor(RESISTANCE, ION, ENERGY)},
			{"ion resistance heat", AttributeAccessor(RESISTANCE, ION, HEAT)},
			{"ion resistance fuel", AttributeAccessor(RESISTANCE, ION, FUEL)},
			{"scramble resistance", AttributeAccessor(RESISTANCE, SCRAMBLE)},
			{"scramble resistance energy", AttributeAccessor(RESISTANCE, SCRAMBLE, ENERGY)},
			{"scramble resistance heat", AttributeAccessor(RESISTANCE, SCRAMBLE, HEAT)},
			{"scramble resistance fuel", AttributeAccessor(RESISTANCE, SCRAMBLE, FUEL)},
			{"slowing resistance", AttributeAccessor(RESISTANCE, SLOWING)},
			{"slowing resistance energy", AttributeAccessor(RESISTANCE, SLOWING, ENERGY)},
			{"slowing resistance heat", AttributeAccessor(RESISTANCE, SLOWING, HEAT)},
			{"slowing resistance fuel", AttributeAccessor(RESISTANCE, SLOWING, FUEL)},
			{"discharge resistance", AttributeAccessor(RESISTANCE, DISCHARGE)},
			{"discharge resistance energy", AttributeAccessor(RESISTANCE, DISCHARGE, ENERGY)},
			{"discharge resistance heat", AttributeAccessor(RESISTANCE, DISCHARGE, HEAT)},
			{"discharge resistance fuel", AttributeAccessor(RESISTANCE, DISCHARGE, FUEL)},
			{"corrosion resistance", AttributeAccessor(RESISTANCE, CORROSION)},
			{"corrosion resistance energy", AttributeAccessor(RESISTANCE, CORROSION, ENERGY)},
			{"corrosion resistance heat", AttributeAccessor(RESISTANCE, CORROSION, HEAT)},
			{"corrosion resistance fuel", AttributeAccessor(RESISTANCE, CORROSION, FUEL)},
			{"leak resistance", AttributeAccessor(RESISTANCE, LEAK)},
			{"leak resistance energy", AttributeAccessor(RESISTANCE, LEAK, ENERGY)},
			{"leak resistance heat", AttributeAccessor(RESISTANCE, LEAK, HEAT)},
			{"leak resistance fuel", AttributeAccessor(RESISTANCE, LEAK, FUEL)},
			{"burn resistance", AttributeAccessor(RESISTANCE, BURN)},
			{"burn resistance energy", AttributeAccessor(RESISTANCE, BURN, ENERGY)},
			{"burn resistance heat", AttributeAccessor(RESISTANCE, BURN, HEAT)},
			{"burn resistance fuel", AttributeAccessor(RESISTANCE, BURN, FUEL)},
			{"piercing resistance", AttributeAccessor(RESISTANCE, PIERCING)},
			{"disruption protection", AttributeAccessor(PROTECTION, DISRUPTION)},
			{"energy protection", AttributeAccessor(PROTECTION, ENERGY)},
			{"force protection", AttributeAccessor(PROTECTION, FORCE)},
			{"fuel protection", AttributeAccessor(PROTECTION, FUEL)},
			{"heat protection", AttributeAccessor(PROTECTION, HEAT)},
			{"hull protection", AttributeAccessor(PROTECTION, HULL)},
			{"ion protection", AttributeAccessor(PROTECTION, ION)},
			{"scramble protection", AttributeAccessor(PROTECTION, SCRAMBLE)},
			{"piercing protection", AttributeAccessor(PROTECTION, PIERCING)},
			{"shield protection", AttributeAccessor(PROTECTION, SHIELDS)},
			{"slowing protection", AttributeAccessor(PROTECTION, SLOWING)},
			{"discharge protection", AttributeAccessor(PROTECTION, DISCHARGE)},
			{"corrosion protection", AttributeAccessor(PROTECTION, CORROSION)},
			{"leak protection", AttributeAccessor(PROTECTION, LEAK)},
			{"burn protection", AttributeAccessor(PROTECTION, BURN)},
			{"firing energy", AttributeAccessor(FIRING, ENERGY)},
			{"firing force", AttributeAccessor(FIRING, FORCE)},
			{"firing fuel", AttributeAccessor(FIRING, FUEL)},
			{"firing heat", AttributeAccessor(FIRING, HEAT)},
			{"firing hull", AttributeAccessor(FIRING, HULL)},
			{"firing shields", AttributeAccessor(FIRING, SHIELDS)},
			{"firing ion", AttributeAccessor(FIRING, ION)},
			{"firing scramble", AttributeAccessor(FIRING, SCRAMBLE)},
			{"firing slowing", AttributeAccessor(FIRING, SLOWING)},
			{"firing disruption", AttributeAccessor(FIRING, DISRUPTION)},
			{"firing discharge", AttributeAccessor(FIRING, DISCHARGE)},
			{"firing corrosion", AttributeAccessor(FIRING, CORROSION)},
			{"firing leak", AttributeAccessor(FIRING, LEAK)},
			{"firing burn", AttributeAccessor(FIRING, BURN)},
			{"relative firing energy", AttributeAccessor(FIRING, ENERGY).Relative()},
			{"relative firing fuel", AttributeAccessor(FIRING, FUEL).Relative()},
			{"relative firing heat", AttributeAccessor(FIRING, HEAT).Relative()},
			{"relative firing hull", AttributeAccessor(FIRING, HULL).Relative()},
			{"relative firing shields", AttributeAccessor(FIRING, SHIELDS).Relative()},
			{"hit force", AttributeAccessor(DAMAGE, FORCE)},
			{"piercing", AttributeAccessor(DAMAGE, PIERCING)},
			{"shield damage", AttributeAccessor(DAMAGE, SHIELDS)},
			{"hull damage", AttributeAccessor(DAMAGE, HULL)},
			{"disabled damage", AttributeAccessor(DAMAGE, DISABLED)},
			{"minable damage", AttributeAccessor(DAMAGE, MINABLE)},
			{"heat damage", AttributeAccessor(DAMAGE, HEAT)},
			{"fuel damage", AttributeAccessor(DAMAGE, FUEL)},
			{"energy damage", AttributeAccessor(DAMAGE, ENERGY)},
			{"relative shield damage", AttributeAccessor(DAMAGE, SHIELDS).Relative()},
			{"relative hull damage", AttributeAccessor(DAMAGE, HULL).Relative()},
			{"relative disabled damage", AttributeAccessor(DAMAGE, DISABLED).Relative()},
			{"relative minable damage", AttributeAccessor(DAMAGE, MINABLE).Relative()},
			{"relative heat damage", AttributeAccessor(DAMAGE, HEAT).Relative()},
			{"relative fuel damage", AttributeAccessor(DAMAGE, FUEL).Relative()},
			{"relative energy damage", AttributeAccessor(DAMAGE, ENERGY).Relative()},
			{"ion damage", AttributeAccessor(DAMAGE, ION)},
			{"scrambling damage", AttributeAccessor(DAMAGE, SCRAMBLE)},
			{"disruption damage", AttributeAccessor(DAMAGE, DISRUPTION)},
			{"slowing damage", AttributeAccessor(DAMAGE, SLOWING)},
			{"discharge damage", AttributeAccessor(DAMAGE, DISCHARGE)},
			{"corrosion damage", AttributeAccessor(DAMAGE, CORROSION)},
			{"leak damage", AttributeAccessor(DAMAGE, LEAK)},
			{"burn damage", AttributeAccessor(DAMAGE, BURN)}
	};

	// Mapping of new-style AttributeAccessor to legacy names.
	map<AttributeAccessor, string> newToOld = [](){
		map<AttributeAccessor, string> m;
		for(const auto &it : oldToNew)
			for(const auto &effect : it.second.Effects()) // There should only be a single effect.
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
			optional<AttributeEffectType> effect = AttributeAccessor::GetBaseEffect(category);
			if(effect.has_value())
				names.emplace(Attribute::GetCategoryName(category), AttributeAccessor(category, effect.value()));
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



// Creates an attribute with a single initial effect.
Attribute::Attribute(const AttributeAccessor access, double value) : category(access.Category())
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
string Attribute::GetLegacyName(const AttributeAccessor access)
{
	auto it = newToOld.find(access);
	if(it == newToOld.end())
	{
		// TODO: find a better solution, or resort to manually mapping all supported attributes.
		// This is just a stopgap measure to avoid hard crashes during testing; not to be relied upon.
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
			AttributeAccessor access{category, it->second};
			AddEffect({access.Effect(), node.Value(1), access.GetDefaultMinimum()});
		}
	}
	else
		node.PrintTrace("Skipping attribute effect without value:");
}



// Parses an attribute into an AttributeAccessor or the original string.
AnyAttribute Attribute::ParseAny(const std::string &attribute)
{
	auto it = oldToNew.find(attribute);
	if(it == oldToNew.end())
		return attribute;
	for(const auto &item : it->second.Effects())
		if(item.second.Value() != 0.)
			return AttributeAccessor(it->second.Category(), item.first);
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
