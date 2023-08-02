/* AttributeStore.cpp
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

#include "AttributeStore.h"

#include "Attribute.h"

#include <limits>

using namespace std;

namespace {
	const double EPS = 0.0000000001;
	const auto MINIMUM_OVERRIDES = map<string, double>{
		{"hull threshold", numeric_limits<double>::lowest()},
		{"energy generation", numeric_limits<double>::lowest()},
		{"energy consumption", numeric_limits<double>::lowest()},
		{"fuel generation", numeric_limits<double>::lowest()},
		{"fuel consumption", numeric_limits<double>::lowest()},
		{"fuel energy", numeric_limits<double>::lowest()},
		{"fuel heat", numeric_limits<double>::lowest()},
		{"heat generation", numeric_limits<double>::lowest()},
		{"flotsam chance", numeric_limits<double>::lowest()},
		{"crew equivalent", numeric_limits<double>::lowest()}
	};
}

// Checking if an attribute is present.
template <class A>
bool AttributeStore::IsPresent(const A &attribute) const
{
	return Get(attribute) != 0.;
}



bool AttributeStore::IsPresent(const char *attribute) const
{
	return Get(attribute) != 0.;
}



// Getting the value of an attribute, or a default.
template <>
double AttributeStore::Get(const string &attribute) const
{
	return textAttributes.Get(attribute);
}



double AttributeStore::Get(const char *attribute) const
{
	return textAttributes.Get(attribute);
}



template <>
double AttributeStore::Get(const Attribute &attribute) const
{
	auto it = categorizedAttributes.find(attribute);
	if(it == categorizedAttributes.end())
		return 0.;
	return it->second;
}



// Setting attribute values
template <>
void AttributeStore::Set(const string &attribute, double value)
{
	auto it = MINIMUM_OVERRIDES.find(attribute);
	if(it != MINIMUM_OVERRIDES.end())
		value = max(value, it->second);
	if(value && abs(value) < EPS)
		value = 0.;
	textAttributes[attribute] = value;
}



void AttributeStore::Set(const char *attribute, double value)
{
	Set(string(attribute), value);
}



template <>
void AttributeStore::Set(const Attribute &attribute, double value)
{
	value = max(value, attribute.GetMinimumValue());
	if(value && abs(value) < EPS)
		value = 0.;
	categorizedAttributes[attribute] = value;
	textAttributes[attribute.GetLegacyName()] = value;
}



// Checks whether there are any attributes stored here.
bool AttributeStore::empty() const
{
	if(textAttributes.empty())
		return true;
	for(auto &it : textAttributes)
	{
		if(it.second != 0.)
			return false;
	}
	return true;
}



// Gets the minimum allowed value of the attribute.
template <>
double AttributeStore::GetMinimum(const Attribute &attribute) const
{
	return attribute.GetMinimumValue();
}



template <>
double AttributeStore::GetMinimum(const string &attribute) const
{
	auto it = MINIMUM_OVERRIDES.find(attribute);
	if(it != MINIMUM_OVERRIDES.end())
		return it->second;
	return 0.;
}



double AttributeStore::GetMinimum(const char *attribute) const
{
	return GetMinimum(string(attribute));
}



// Loads data from the data node. This function can be called multiple times on an instance.
void AttributeStore::Load(const DataNode &node, const bool isWeapon, const Attribute parent)
{
	const string &key = node.Token(0);
	Attribute *a = Attribute::Parse(key);
	if(key == "minable" && parent.Category() == PASSIVE) // handle name collision
		a = nullptr;
	if(a)
	{
		Attribute attribute = *a;
		if(attribute.Category() == PASSIVE)
		{
			if(parent.Effect() == -1 || parent.Effect() == attribute.Effect() ||
					(parent.Category() <= CLOAKING && parent.Effect() == static_cast<int>(parent.Category())))
				attribute = Attribute(parent.Category(), attribute.Effect(), attribute.Secondary());
			else
				attribute = Attribute(parent.Category(), parent.Effect(), attribute.Effect());
		}
		else if(parent.Category() != PASSIVE && parent.Category() != attribute.Category())
		{
			node.PrintTrace("Illegally nested categories: \"" + key + "\":");
			return;
		}
		if(attribute.IsSupported())
		{
			// Weapons only have firing effects and damage, the rest are generic outfit categories.
			if(isWeapon == (attribute.Category() == FIRING || attribute.Category() == DAMAGE))
				Set(attribute, node.Size() >= 2 ? node.Value(1) : 0.);
			else if(isWeapon)
				node.PrintTrace("Attribute should be outside weapon node: \"" + key + "\":");
			else
				node.PrintTrace("Attribute should be inside weapon node: \"" + key + "\":");
		}
		else if(node.Size() >= 2)
			node.PrintTrace("Unsupported attribute: \"" + key + "\":");
		for(const DataNode &child : node)
			Load(child, isWeapon, attribute);
	}
	else if(node.Size() >= 2)
		textAttributes[key] = node.Value(1);
	else
		node.PrintTrace("Skipping unrecognized attribute:");
}



// Writes the attributes into the data writer.
void AttributeStore::Save(DataWriter &writer) const
{
	for(auto &it : textAttributes)
	{
		if(Attribute::Parse(it.first) == nullptr)
			writer.Write(it.first, it.second);
	}
	set<Attribute> written;
	Attribute last(static_cast<AttributeCategory>(-1), static_cast<AttributeEffect>(-1));
	for(auto &it : categorizedAttributes)
	{
		if(it.second)
			Save(writer, it.first, written, last);
	}
	if(last.Secondary() != -1)
		writer.EndChild();
	if(last.Effect() != -1 && last.Category() != PASSIVE)
		writer.EndChild();
}



// Gets the direct parent of the attribute
Attribute GetParent(const Attribute &attribute)
{
	if(attribute.Secondary() != -1)
		return Attribute(attribute.Category(), attribute.Effect(), static_cast<AttributeEffect>(-1), false);
	if(attribute.Effect() != -1)
		return Attribute(attribute.Category(), static_cast<AttributeEffect>(-1), static_cast<AttributeEffect>(-1), false);
	return Attribute(static_cast<AttributeCategory>(-1), static_cast<AttributeEffect>(-1),
			static_cast<AttributeEffect>(-1), false);
}



// Checks if the attribute is a (direct or indirect) child of the other.
bool IsChild(const Attribute &parent, const Attribute &child)
{
	if(parent.Category() == -1)
		return true;
	else if(parent == child)
		return false;
	else if(parent.Secondary() != -1)
		return false;
	else if(parent.Category() == PASSIVE && parent.Effect() == -1)
		return true;
	else if(parent.Effect() == child.Effect() || (parent.Effect() != -1 && child.Effect() == -1))
		return true;
	return parent.Category() == child.Category();
}



// Gets the value that is saved for this attribute. Used here because attributes are not
// always in their preferred form when saving.
Attribute GetPreferredForm(const Attribute &attribute)
{
	return Attribute(attribute.Category(), attribute.Effect(), attribute.Secondary());
}



// Saves an attribute. Also gets the set of already saved attributes and the last saved attribute.
void AttributeStore::Save(DataWriter &writer, const Attribute &attribute, set<Attribute> &written,
		Attribute &previous) const
{
	if(attribute.Category() == -1 || (attribute.Category() == PASSIVE && attribute.Effect() == -1)
			|| written.count(attribute))
		return;

	if(!IsChild(GetParent(previous), attribute)) // wrong parent
	{
		writer.EndChild();
		previous = GetParent(previous);
	}

	Save(writer, GetParent(attribute), written, previous);// saving parent
	if(written.count(GetPreferredForm(attribute))) // don't duplicate attributes
		return;

	if(previous.Category() != -1 && previous.Category() != PASSIVE && GetParent(attribute) == previous)
		writer.BeginChild(); // first child after parent

	if(attribute.Effect() != -1)
		writer.WriteToken(Attribute::GetEffectName(attribute.Secondary() == -1 ? attribute.Effect() : attribute.Secondary()));
	else
		writer.WriteToken(Attribute::GetCategoryName(attribute.Category()));
	Attribute preferred = GetPreferredForm(attribute);
	if(IsPresent(preferred))
		writer.Write(Get(preferred));
	else
		writer.Write();
	previous = attribute;
	written.emplace(attribute);
	written.emplace(preferred);
}



// Determine whether the given number of instances of the given attributes can
// be added to this instance. If not, return the maximum number that can be added.
template <class A>
int AttributeStore::CanAdd(const A &attribute, const AttributeStore &other, int count) const
{
	if(count)
	{
		double minimum = GetMinimum(attribute);
		if(attribute == string("required crew"))
			minimum = !(IsPresent(string("automaton")) || other.IsPresent(string("automaton")));

		double value = Get(attribute);
		double amount = other.Get(attribute);
		// Allow for rounding errors:
		if(value + amount * count < minimum - EPS)
			return (value - minimum) / -amount + EPS;
	}
	return count;
}



int AttributeStore::CanAdd(const AttributeStore &other, int count) const
{
	for(const auto &it : textAttributes)
		count = min(count, CanAdd(string(it.first), other, count));
	return count;
}



// Adds attributes the specified number of times.
void AttributeStore::Add(const AttributeStore &other, const int count)
{
	for(auto &it : other.textAttributes)
		Add(it.first, other, count);
	for(auto &it : other.categorizedAttributes)
		Add(it.first, other, count);
}



template <class A>
void AttributeStore::Add(const A &attribute, const AttributeStore &other, const int count)
{
	Set(attribute, Get(attribute) + other.Get(attribute) * count);
}



template <class A>
void AttributeStore::Add(const A &attribute, const double amount)
{
	Set(attribute, Get(attribute) + amount);
}



void AttributeStore::Add(const char *attribute, const double amount)
{
	Add(string(attribute), amount);
}



// Calls the given function on all attributes.
void AttributeStore::ForEach(const std::function<void(std::tuple<string,Attribute*,double>)> &function) const
{
	for(auto &it : textAttributes)
	{
		Attribute *a = (it.second == numeric_limits<double>::infinity() ? Attribute::Parse(it.first) : nullptr);
		function(tuple<string, Attribute*, double>(it.first, a, it.second));
	}
}
