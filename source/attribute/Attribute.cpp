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

#include <map>

using namespace std;

namespace {
	// The names of effects and categories, as used in the new data format.
	const map<AttributeEffectType, string> effectNames{{SHIELDS, "shields"}, {HULL, "hull"}, {THRUST, "thrust"},
			{REVERSE_THRUST, "reverse thrust"}, {TURN, "turn"}, {ACTIVE_COOLING, "active cooling"},
			{RAMSCOOP, "ramscoop"}, {CLOAK, "cloak"}, {COOLING, "cooling"}, {FORCE, "force"}, {ENERGY, "energy"},
			{FUEL, "fuel"}, {HEAT, "heat"}, {JAM, "jam"}, {DISABLED, "disabled"}, {MINABLE, "minable"}, {PIERCING, "piercing"}};

	const map<AttributeCategory, string> categoryNames{{SHIELD_GENERATION,"shield generation"},
			{HULL_REPAIR, "hull repair"}, {THRUSTING, "thrust"}, {REVERSE_THRUSTING, "reverse thrust"}, {TURNING, "turn"},
			{ACTIVE_COOL, "active cooling"}, {RAMSCOOPING, "ramscoop"}, {CLOAKING, "cloak"},
			{AFTERBURNING, "afterburner thrust"}, {FIRING, "firing"}, {PROTECTION, "protection"}, {RESISTANCE, "resistance"},
			{DAMAGE, "damage"}, {PASSIVE, "capacity"}};

	// The names of various "over time" effects, that are modified variants of other effects but can be parsed individually.
	// Slowing is special, since that affects both thrust, reverse thrust and turn.
	const map<AttributeEffectType, string> overTimeEffectNames{{SHIELDS, "discharge"}, {HULL, "corrosion"}, {THRUST, "slowing"},
			{ENERGY, "ion"}, {FUEL, "leak"}, {HEAT, "burn"}, {JAM, "scramble"}, {PIERCING, "disruption"}};
	// Cached mappings between the old and new format.
	// Any attribute without an effect will not be present in newToOld, as those have no legacy names.
	map<string, Attribute> oldToNew = {
			{"capacity", Attribute(PASSIVE)},
			{"energy capacity", AttributeAccessor(PASSIVE, ENERGY)},
			{"shields", AttributeAccessor(PASSIVE, SHIELDS)},
			{"shield multiplier", AttributeAccessor(PASSIVE, SHIELDS, Modifier::MULTIPLIER)},
			{"shield generation", AttributeAccessor(SHIELD_GENERATION, SHIELDS)},
			{"shield energy", AttributeAccessor(SHIELD_GENERATION, ENERGY)},
			{"shield heat", AttributeAccessor(SHIELD_GENERATION, HEAT)},
			{"shield fuel", AttributeAccessor(SHIELD_GENERATION, FUEL)},
			{"hull", AttributeAccessor(PASSIVE, HULL)},
			{"hull multiplier", AttributeAccessor(PASSIVE, HULL, Modifier::MULTIPLIER)},
			{"hull repair rate", AttributeAccessor(HULL_REPAIR, HULL)},
			{"hull energy", AttributeAccessor(HULL_REPAIR, ENERGY)},
			{"hull heat", AttributeAccessor(HULL_REPAIR, HEAT)},
			{"hull fuel", AttributeAccessor(HULL_REPAIR, FUEL)},
			{"shield generation multiplier", AttributeAccessor(SHIELD_GENERATION, SHIELDS, Modifier::MULTIPLIER)},
			{"shield energy multiplier", AttributeAccessor(SHIELD_GENERATION, ENERGY, Modifier::MULTIPLIER)},
			{"shield heat multiplier", AttributeAccessor(SHIELD_GENERATION, HEAT, Modifier::MULTIPLIER)},
			{"shield fuel multiplier", AttributeAccessor(SHIELD_GENERATION, FUEL, Modifier::MULTIPLIER)},
			{"hull repair multiplier", AttributeAccessor(HULL_REPAIR, HULL, Modifier::MULTIPLIER)},
			{"hull energy multiplier", AttributeAccessor(HULL_REPAIR, ENERGY, Modifier::MULTIPLIER)},
			{"hull heat multiplier", AttributeAccessor(HULL_REPAIR, HEAT, Modifier::MULTIPLIER)},
			{"hull fuel multiplier", AttributeAccessor(HULL_REPAIR, FUEL, Modifier::MULTIPLIER)},
			{"ramscoop", AttributeAccessor(RAMSCOOPING, RAMSCOOP)},
			{"fuel capacity", AttributeAccessor(PASSIVE, FUEL)},
			{"thrust", AttributeAccessor(THRUSTING, THRUST)},
			{"thrusting energy", AttributeAccessor(THRUSTING, ENERGY)},
			{"thrusting heat", AttributeAccessor(THRUSTING, HEAT)},
			{"thrusting shields", AttributeAccessor(THRUSTING, SHIELDS)},
			{"thrusting hull", AttributeAccessor(THRUSTING, HULL)},
			{"thrusting fuel", AttributeAccessor(THRUSTING, FUEL)},
			{"thrusting discharge", AttributeAccessor(THRUSTING, SHIELDS, Modifier::OVER_TIME)},
			{"thrusting corrosion", AttributeAccessor(THRUSTING, HULL, Modifier::OVER_TIME)},
			{"thrusting ion", AttributeAccessor(THRUSTING, ENERGY, Modifier::OVER_TIME)},
			{"thrusting scramble", AttributeAccessor(THRUSTING, JAM, Modifier::OVER_TIME)},
			{"thrusting leakage", AttributeAccessor(THRUSTING, FUEL, Modifier::OVER_TIME)},
			{"thrusting burn", AttributeAccessor(THRUSTING, HEAT, Modifier::OVER_TIME)},
			{"thrusting slowing", AttributeAccessor(THRUSTING, THRUST, Modifier::OVER_TIME)},
			{"thrusting disruption", AttributeAccessor(THRUSTING, PIERCING, Modifier::OVER_TIME)},
			{"turn", AttributeAccessor(TURNING, TURN)},
			{"turning energy", AttributeAccessor(TURNING, ENERGY)},
			{"turning heat", AttributeAccessor(TURNING, HEAT)},
			{"turning shields", AttributeAccessor(TURNING, SHIELDS)},
			{"turning hull", AttributeAccessor(TURNING, HULL)},
			{"turning fuel", AttributeAccessor(TURNING, FUEL)},
			{"turning discharge", AttributeAccessor(TURNING, SHIELDS, Modifier::OVER_TIME)},
			{"turning corrosion", AttributeAccessor(TURNING, HULL, Modifier::OVER_TIME)},
			{"turning ion", AttributeAccessor(TURNING, ENERGY, Modifier::OVER_TIME)},
			{"turning scramble", AttributeAccessor(TURNING, JAM, Modifier::OVER_TIME)},
			{"turning leakage", AttributeAccessor(TURNING, FUEL, Modifier::OVER_TIME)},
			{"turning burn", AttributeAccessor(TURNING, HEAT, Modifier::OVER_TIME)},
			{"turning slowing", AttributeAccessor(TURNING, THRUST, Modifier::OVER_TIME)},
			{"turning disruption", AttributeAccessor(TURNING, PIERCING, Modifier::OVER_TIME)},
			{"reverse thrust", AttributeAccessor(REVERSE_THRUSTING, REVERSE_THRUST)},
			{"reverse thrusting energy", AttributeAccessor(REVERSE_THRUSTING, ENERGY)},
			{"reverse thrusting heat", AttributeAccessor(REVERSE_THRUSTING, HEAT)},
			{"reverse thrusting shields", AttributeAccessor(REVERSE_THRUSTING, SHIELDS)},
			{"reverse thrusting hull", AttributeAccessor(REVERSE_THRUSTING, HULL)},
			{"reverse thrusting fuel", AttributeAccessor(REVERSE_THRUSTING, FUEL)},
			{"reverse thrusting discharge", AttributeAccessor(REVERSE_THRUSTING, SHIELDS, Modifier::OVER_TIME)},
			{"reverse thrusting corrosion", AttributeAccessor(REVERSE_THRUSTING, HULL, Modifier::OVER_TIME)},
			{"reverse thrusting ion", AttributeAccessor(REVERSE_THRUSTING, ENERGY, Modifier::OVER_TIME)},
			{"reverse thrusting scramble", AttributeAccessor(REVERSE_THRUSTING, JAM, Modifier::OVER_TIME)},
			{"reverse thrusting leakage", AttributeAccessor(REVERSE_THRUSTING, FUEL, Modifier::OVER_TIME)},
			{"reverse thrusting burn", AttributeAccessor(REVERSE_THRUSTING, HEAT, Modifier::OVER_TIME)},
			{"reverse thrusting slowing", AttributeAccessor(REVERSE_THRUSTING, THRUST, Modifier::OVER_TIME)},
			{"reverse thrusting disruption", AttributeAccessor(REVERSE_THRUSTING, PIERCING, Modifier::OVER_TIME)},
			{"afterburner thrust", AttributeAccessor(AFTERBURNING, THRUST)},
			{"afterburner energy", AttributeAccessor(AFTERBURNING, ENERGY)},
			{"afterburner heat", AttributeAccessor(AFTERBURNING, HEAT)},
			{"afterburner shields", AttributeAccessor(AFTERBURNING, SHIELDS)},
			{"afterburner hull", AttributeAccessor(AFTERBURNING, HULL)},
			{"afterburner fuel", AttributeAccessor(AFTERBURNING, FUEL)},
			{"afterburner discharge", AttributeAccessor(AFTERBURNING, SHIELDS, Modifier::OVER_TIME)},
			{"afterburner corrosion", AttributeAccessor(AFTERBURNING, HULL, Modifier::OVER_TIME)},
			{"afterburner ion", AttributeAccessor(AFTERBURNING, ENERGY, Modifier::OVER_TIME)},
			{"afterburner scramble", AttributeAccessor(AFTERBURNING, JAM, Modifier::OVER_TIME)},
			{"afterburner leakage", AttributeAccessor(AFTERBURNING, FUEL, Modifier::OVER_TIME)},
			{"afterburner burn", AttributeAccessor(AFTERBURNING, HEAT, Modifier::OVER_TIME)},
			{"afterburner slowing", AttributeAccessor(AFTERBURNING, THRUST, Modifier::OVER_TIME)},
			{"afterburner disruption", AttributeAccessor(AFTERBURNING, PIERCING, Modifier::OVER_TIME)},
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
			{"disruption resistance", AttributeAccessor(RESISTANCE, PIERCING, Modifier::OVER_TIME)},
			{"disruption resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(PIERCING, Modifier::OVER_TIME), ENERGY)},
			{"disruption resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(PIERCING, Modifier::OVER_TIME), HEAT)},
			{"disruption resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(PIERCING, Modifier::OVER_TIME), FUEL)},
			{"ion resistance", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME))},
			{"ion resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME), ENERGY)},
			{"ion resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME), HEAT)},
			{"ion resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME), FUEL)},
			{"scramble resistance", AttributeAccessor(RESISTANCE, JAM, Modifier::OVER_TIME)},
			{"scramble resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(JAM, Modifier::OVER_TIME), ENERGY)},
			{"scramble resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(JAM, Modifier::OVER_TIME), HEAT)},
			{"scramble resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(JAM, Modifier::OVER_TIME), FUEL)},
			{"slowing resistance", AttributeAccessor(RESISTANCE, THRUST, Modifier::OVER_TIME)},
			{"slowing resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(THRUST, Modifier::OVER_TIME), ENERGY)},
			{"slowing resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(THRUST, Modifier::OVER_TIME), HEAT)},
			{"slowing resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(THRUST, Modifier::OVER_TIME), FUEL)},
			{"discharge resistance", AttributeAccessor(RESISTANCE, SHIELDS, Modifier::OVER_TIME)},
			{"discharge resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(SHIELDS, Modifier::OVER_TIME), ENERGY)},
			{"discharge resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(SHIELDS, Modifier::OVER_TIME), HEAT)},
			{"discharge resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(SHIELDS, Modifier::OVER_TIME), FUEL)},
			{"corrosion resistance", AttributeAccessor(RESISTANCE, HULL, Modifier::OVER_TIME)},
			{"corrosion resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HULL, Modifier::OVER_TIME), ENERGY)},
			{"corrosion resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HULL, Modifier::OVER_TIME), HEAT)},
			{"corrosion resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HULL, Modifier::OVER_TIME), FUEL)},
			{"leak resistance", AttributeAccessor(RESISTANCE, FUEL, Modifier::OVER_TIME)},
			{"leak resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(FUEL, Modifier::OVER_TIME), ENERGY)},
			{"leak resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(FUEL, Modifier::OVER_TIME), HEAT)},
			{"leak resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(FUEL, Modifier::OVER_TIME), FUEL)},
			{"burn resistance", AttributeAccessor(RESISTANCE, HEAT, Modifier::OVER_TIME)},
			{"burn resistance energy", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HEAT, Modifier::OVER_TIME), ENERGY)},
			{"burn resistance heat", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HEAT, Modifier::OVER_TIME), HEAT)},
			{"burn resistance fuel", AttributeAccessor(RESISTANCE,
					AttributeAccessor::WithModifier(HEAT, Modifier::OVER_TIME), FUEL)},
			{"piercing resistance", AttributeAccessor(RESISTANCE, PIERCING)},
			{"disruption protection", AttributeAccessor(PROTECTION, PIERCING, Modifier::OVER_TIME)},
			{"energy protection", AttributeAccessor(PROTECTION, ENERGY)},
			{"force protection", AttributeAccessor(PROTECTION, FORCE)},
			{"fuel protection", AttributeAccessor(PROTECTION, FUEL)},
			{"heat protection", AttributeAccessor(PROTECTION, HEAT)},
			{"hull protection", AttributeAccessor(PROTECTION, HULL)},
			{"ion protection", AttributeAccessor(PROTECTION, ENERGY, Modifier::OVER_TIME)},
			{"scramble protection", AttributeAccessor(PROTECTION, JAM, Modifier::OVER_TIME)},
			{"piercing protection", AttributeAccessor(PROTECTION, PIERCING)},
			{"shield protection", AttributeAccessor(PROTECTION, SHIELDS)},
			{"slowing protection", AttributeAccessor(PROTECTION, THRUST, Modifier::OVER_TIME)},
			{"discharge protection", AttributeAccessor(PROTECTION, SHIELDS, Modifier::OVER_TIME)},
			{"corrosion protection", AttributeAccessor(PROTECTION, HULL, Modifier::OVER_TIME)},
			{"leak protection", AttributeAccessor(PROTECTION, FUEL, Modifier::OVER_TIME)},
			{"burn protection", AttributeAccessor(PROTECTION, HEAT, Modifier::OVER_TIME)},
			{"firing energy", AttributeAccessor(FIRING, ENERGY)},
			{"firing force", AttributeAccessor(FIRING, FORCE)},
			{"firing fuel", AttributeAccessor(FIRING, FUEL)},
			{"firing heat", AttributeAccessor(FIRING, HEAT)},
			{"firing hull", AttributeAccessor(FIRING, HULL)},
			{"firing shields", AttributeAccessor(FIRING, SHIELDS)},
			{"firing ion", AttributeAccessor(FIRING, ENERGY, Modifier::OVER_TIME)},
			{"firing scramble", AttributeAccessor(FIRING, JAM, Modifier::OVER_TIME)},
			{"firing slowing", AttributeAccessor(FIRING, THRUST, Modifier::OVER_TIME)},
			{"firing disruption", AttributeAccessor(FIRING, PIERCING, Modifier::OVER_TIME)},
			{"firing discharge", AttributeAccessor(FIRING, SHIELDS, Modifier::OVER_TIME)},
			{"firing corrosion", AttributeAccessor(FIRING, HULL, Modifier::OVER_TIME)},
			{"firing leak", AttributeAccessor(FIRING, FUEL, Modifier::OVER_TIME)},
			{"firing burn", AttributeAccessor(FIRING, HEAT, Modifier::OVER_TIME)},
			{"relative firing energy", AttributeAccessor(FIRING, ENERGY, Modifier::RELATIVE)},
			{"relative firing fuel", AttributeAccessor(FIRING, FUEL, Modifier::RELATIVE)},
			{"relative firing heat", AttributeAccessor(FIRING, HEAT, Modifier::RELATIVE)},
			{"relative firing hull", AttributeAccessor(FIRING, HULL, Modifier::RELATIVE)},
			{"relative firing shields", AttributeAccessor(FIRING, SHIELDS, Modifier::RELATIVE)},
			{"hit force", AttributeAccessor(DAMAGE, FORCE)},
			{"piercing", AttributeAccessor(DAMAGE, PIERCING)},
			{"shield damage", AttributeAccessor(DAMAGE, SHIELDS)},
			{"hull damage", AttributeAccessor(DAMAGE, HULL)},
			{"disabled damage", AttributeAccessor(DAMAGE, DISABLED)},
			{"minable damage", AttributeAccessor(DAMAGE, MINABLE)},
			{"heat damage", AttributeAccessor(DAMAGE, HEAT)},
			{"fuel damage", AttributeAccessor(DAMAGE, FUEL)},
			{"energy damage", AttributeAccessor(DAMAGE, ENERGY)},
			{"relative shield damage", AttributeAccessor(DAMAGE, SHIELDS, Modifier::RELATIVE)},
			{"relative hull damage", AttributeAccessor(DAMAGE, HULL, Modifier::RELATIVE)},
			{"relative disabled damage", AttributeAccessor(DAMAGE, DISABLED, Modifier::RELATIVE)},
			{"relative minable damage", AttributeAccessor(DAMAGE, MINABLE, Modifier::RELATIVE)},
			{"relative heat damage", AttributeAccessor(DAMAGE, HEAT, Modifier::RELATIVE)},
			{"relative fuel damage", AttributeAccessor(DAMAGE, FUEL, Modifier::RELATIVE)},
			{"relative energy damage", AttributeAccessor(DAMAGE, ENERGY, Modifier::RELATIVE)},
			{"ion damage", AttributeAccessor(DAMAGE, ENERGY, Modifier::OVER_TIME)},
			{"scrambling damage", AttributeAccessor(DAMAGE, JAM, Modifier::OVER_TIME)},
			{"disruption damage", AttributeAccessor(DAMAGE, PIERCING, Modifier::OVER_TIME)},
			{"slowing damage", AttributeAccessor(DAMAGE, THRUST, Modifier::OVER_TIME)},
			{"discharge damage", AttributeAccessor(DAMAGE, SHIELDS, Modifier::OVER_TIME)},
			{"corrosion damage", AttributeAccessor(DAMAGE, HULL, Modifier::OVER_TIME)},
			{"leak damage", AttributeAccessor(DAMAGE, FUEL, Modifier::OVER_TIME)},
			{"burn damage", AttributeAccessor(DAMAGE, HEAT, Modifier::OVER_TIME)}
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
		for(int i = 0; i < static_cast<int>(Modifier::MODIFIER_COUNT) * ATTRIBUTE_EFFECT_COUNT; i++)
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
		for(int i = 0; i < static_cast<int>(Modifier::MODIFIER_COUNT) * ATTRIBUTE_CATEGORY_COUNT; i++)
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
	return category >= 0 ? categoryNames.at(category) : "";
}



// Gets the old-style name of the attribute. Returns an empty string if the attribute is not supported
// in the old format.
string Attribute::GetLegacyName(const AnyAttribute &attribute)
{
	if(attribute.IsString())
		return attribute.String();
	else
	{
		// newToOld contains all supported attributes.
		const AttributeAccessor &access = attribute.Categorized();
		auto it = newToOld.find(access);
		if(it == newToOld.end())
		{
			// TODO: find a better solution, or resort to manually mapping all supported attributes.
			// This is just a stopgap measure to avoid hard crashes during testing; not to be relied upon.
			auto baseType = static_cast<AttributeEffectType>(access.Effect() % static_cast<int>(ATTRIBUTE_EFFECT_COUNT));
			string category = GetCategoryName(access.Category());
			if(access.HasModifier(Modifier::RELATIVE))
				return "relative " + category + " " + GetEffectName(baseType);
			else
				return category + " " + GetEffectName(access.Effect());
		}
		return it->second;
	}
}



// Gets the data format name of the effect, as used in the new syntax. This also supports
// multipliers, so for any effect E, passing E + ATTRIBUTE_EFFECT_COUNT will produce the name of the
// multiplier effect.
string Attribute::GetEffectName(const AttributeEffectType effect)
{
	if(AttributeAccessor::HasModifier(effect, Modifier::RELATIVE))
		return "relative " + GetEffectName(static_cast<AttributeEffectType>(effect % ATTRIBUTE_EFFECT_COUNT));
	else if(AttributeAccessor::HasModifier(effect, Modifier::MULTIPLIER))
		return GetEffectName(static_cast<AttributeEffectType>(effect % ATTRIBUTE_EFFECT_COUNT)) + " multiplier";
	else if(AttributeAccessor::HasModifier(effect, Modifier::OVER_TIME))
	{
		const auto it = overTimeEffectNames.find(static_cast<AttributeEffectType>(effect % ATTRIBUTE_EFFECT_COUNT));
		if(it == overTimeEffectNames.end())
			return "";
		return it->second;
	}
	return effectNames.at(effect);
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
AttributeCategory Attribute::Category() const
{
	return category;
}



// Gets the effect of this attribute.
const std::map<AttributeEffectType, AttributeEffect> &Attribute::Effects() const
{
	return effects;
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



void Attribute::Add(const Attribute &other, double multiplier)
{
	if(other.effects.empty())
		return;

	auto otherIt = other.effects.begin();
	auto myIt = effects.begin();
	while(otherIt != other.effects.end() && myIt != effects.end())
	{
		if(otherIt->first > myIt->first)
			myIt++;
		else if(otherIt->first == myIt->first)
		{
			myIt->second.Add(otherIt->second.Value() * multiplier);
			++otherIt;
			++myIt;
		}
		else
		{
			effects.emplace_hint(myIt, otherIt->first, otherIt->second)->second.Set(otherIt->second.Value() * multiplier);
			otherIt++;
		}
	}
	effects.insert(otherIt, other.effects.end());
}



bool Attribute::operator==(const Attribute &other) const
{
	return category == other.category;
}



bool Attribute::operator<(const Attribute &other) const
{
	return category < other.category;
}
