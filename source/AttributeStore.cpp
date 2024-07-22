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

#include <sstream>

using namespace std;



namespace {
	const map<string, double> MINIMUM_OVERRIDES = {
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



// Getting the value of an attribute, or a default.
double AttributeStore::Get(const AnyAttribute &attribute) const
{
	if(attribute.IsCategorized())
	{
		const AttributeEffect *e = GetEffect(attribute.Categorized());
		if(e == nullptr)
			return 0.;
		return e->Value();
	}
	else
		return textAttributes.Get(attribute.String());
}



const Attribute *AttributeStore::GetAttribute(const AttributeCategory category) const
{
	auto it = categorizedAttributes.find(category);
	if(it == categorizedAttributes.end())
		return nullptr;
	return &(it->second);
}



Attribute *AttributeStore::GetAttribute(const AttributeCategory category)
{
	// Using const_cast is fine because the object isn't const originally.
	return const_cast<Attribute*>(const_cast<const AttributeStore*>(this)->GetAttribute(category));
}



const AttributeEffect *AttributeStore::GetEffect(const AttributeAccessor access) const
{
	const Attribute *a = GetAttribute(access.Category());
	if(a == nullptr)
		return nullptr;
	return a->GetEffect(access.Effect());
}



AttributeEffect *AttributeStore::GetEffect(const AttributeAccessor access)
{
	// Using const_cast is fine because the object isn't const originally.
	return const_cast<AttributeEffect*>(const_cast<const AttributeStore*>(this)->GetEffect(access));
}



// Setting attribute values.
void AttributeStore::Set(const AnyAttribute &attribute, double value)
{
	if(attribute.IsCategorized())
	{
		const AttributeAccessor &attr = attribute.Categorized();
		auto it = categorizedAttributes.find(attr.Category());
		if(it == categorizedAttributes.end())
			categorizedAttributes.emplace(attr.Category(), Attribute(attr, value));
		else
			it->second.AddEffect({attr.Effect(), value, attr.GetDefaultMinimum()});
	}
	else
	{
		const std::string &attr = attribute.String();
		auto it = MINIMUM_OVERRIDES.find(attr);
		if(it != MINIMUM_OVERRIDES.end())
			value = std::max(value, it->second);
		if(value && fabs(value) < AttributeEffect::EPS)
			value = 0.;
		textAttributes[attr] = value;
	}
}



// Checks whether there are any attributes stored here.
bool AttributeStore::Empty() const
{
	if(textAttributes.empty() && categorizedAttributes.empty())
		return true;
	for(auto &it : textAttributes)
		if(it.second != 0.)
			return false;
	for(const auto &it : categorizedAttributes)
		for(const auto &item : it.second.Effects())
			if(item.second.Value() != 0.)
				return false;
	return true;
}



// Gets the minimum allowed value of the attribute.
double AttributeStore::GetMinimum(const AnyAttribute &attribute) const
{
	if(attribute.IsCategorized())
	{
		const AttributeAccessor &attr = attribute.Categorized();
		const AttributeEffect *effect = GetEffect(attr);
		if(effect)
			return effect->Minimum();
		else
			return attr.GetDefaultMinimum();
	}
	else
	{
		auto it = MINIMUM_OVERRIDES.find(attribute.String());
		if(it != MINIMUM_OVERRIDES.end())
			return it->second;
		return 0.;
	}
}



// Loads data from the data node. This function can be called multiple times on the same instance.
void AttributeStore::Load(const DataNode &node) {
	const string &key = node.Token(0);
	Attribute *parsed = Attribute::Parse(key);
	if(parsed)
	{
		Attribute attribute = Attribute(*parsed, node.Size() >= 2 ? node.Value(1) : 1.);
		for(const DataNode &child : node)
			attribute.Parse(child);
		for(const auto &it : attribute.Effects())
			Set({attribute.Category(), it.first}, it.second.Value());
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
		if(it.second)
			writer.Write(it.first, it.second);
	for(auto &it : categorizedAttributes)
	{
		// The node containing every effect of this attribute category.
		DataNode node;
		node.AddToken(Attribute::GetCategoryName(it.first));
		// The base effect of the attribute that is put on the category's line.
		optional<AttributeEffectType> baseEffect = AttributeAccessor::GetBaseEffect(it.first);
		for(auto &entry : it.second.Effects())
		{
			if(!entry.second.Value())
				continue;

			ostringstream temp;
			temp << entry.second.Value();

			// Add the value to the category as a new effect.
			if(baseEffect.has_value() && baseEffect.value() == entry.first)
				node.AddToken(temp.str());
			else
			{
				DataNode child(&node);
				child.AddToken(Attribute::GetEffectName(entry.second.Type()));
				child.AddToken(temp.str());
				node.AddChild(child);
			}
		}
		if(node.Size() == 1 && baseEffect.has_value())
			node.AddToken("0");

		if(node.HasChildren())
			writer.Write(node);
	}
}



int AttributeStore::CanAdd(const AttributeStore &other, int count) const
{
	for(const auto &it : other.textAttributes)
		count = min(count, CanAdd(string(it.first), other, count));
	for(auto &it : other.categorizedAttributes)
		for(const auto &item : it.second.Effects())
			count = min(count, CanAdd(AttributeAccessor(it.first, item.first), other, count));

	return count;
}



// Adds attributes the specified number of times.
void AttributeStore::Add(const AttributeStore &other, const int count)
{
	for(auto &it : other.textAttributes)
		Add(it.first, other, count);
	for(auto &it : other.categorizedAttributes)
		Add(it.second, count);
}



void AttributeStore::Add(const AnyAttribute &attribute, const AttributeStore &other, int count)
{
	Set(attribute, Get(attribute) + other.Get(attribute) * count);
}



void AttributeStore::Add(const AnyAttribute &attribute, double amount)
{
	Set(attribute, Get(attribute) + amount);
}



void AttributeStore::ForEach(const std::function<void(const AnyAttribute &, double)> &function) const
{
	for(auto &it : textAttributes)
		function({it.first}, it.second);
	for(auto &it : categorizedAttributes)
		for(const auto &item : it.second.Effects())
			function({AttributeAccessor(it.first, item.first)}, item.second.Value());
}



// Determine whether the attribute can be added to this instance 'count' times.
// If not, return the maximum number that can be added.
int AttributeStore::CanAdd(const AnyAttribute &attribute, const AttributeStore &other, int count) const
{
	if(count)
	{
		double minimum = GetMinimum(attribute);
		if(attribute.IsString() && attribute.String() == "required crew")
			minimum = !(Get("automaton") || other.Get("automaton"));

		double value = Get(attribute);
		double amount = other.Get(attribute);
		// Allow for rounding errors:
		if(value + amount * count < minimum - AttributeEffect::EPS)
			return (value - minimum) / -amount + AttributeEffect::EPS;
	}
	return count;
}



inline void AttributeStore::Add(const Attribute &attribute, const double amount)
{
	Attribute *current = GetAttribute(attribute.Category());
	if(current == nullptr)
		categorizedAttributes.emplace(attribute.Category(), attribute);
	else
		current->Add(attribute, amount);
}
