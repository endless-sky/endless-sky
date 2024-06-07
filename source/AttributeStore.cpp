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



const map<string, double> AttributeStore::MINIMUM_OVERRIDES = {
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



// Checking if an attribute is present.
bool AttributeStore::IsPresent(const AttributeAccess attribute) const
{
	return Get(attribute) != 0.;
}



bool AttributeStore::IsPresent(const char *attribute) const
{
	return Get(attribute) != 0.;
}



// Getting the value of an attribute, or a default.
double AttributeStore::Get(const AttributeAccess attribute) const
{
	const AttributeEffect *e = GetEffect(attribute);
	if(e == nullptr)
		return 0.;
	return e->Value();
}



double AttributeStore::Get(const char *attribute) const
{
	return textAttributes.Get(attribute);
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
	// const_cast is fine because the object isn't const originally
	return const_cast<Attribute*>(const_cast<const AttributeStore*>(this)->GetAttribute(category));
}



const AttributeEffect *AttributeStore::GetEffect(const AttributeAccess access) const
{
	const Attribute *a = GetAttribute(access.Category());
	if(a == nullptr)
		return nullptr;
	return a->GetEffect(access.Effect());
}



AttributeEffect *AttributeStore::GetEffect(const AttributeAccess access)
{
	// const_cast is fine because the object isn't const originally
	return const_cast<AttributeEffect*>(const_cast<const AttributeStore*>(this)->GetEffect(access));
}



// Setting attribute values
void AttributeStore::Set(const char *attribute, double value)
{
	std::string s = std::string(attribute);
	Set(s, value);
}



void AttributeStore::Set(const AttributeAccess access, double value)
{
	Attribute *a = GetAttribute(access.Category());
	if(!a)
	{
		Attribute attr = Attribute(access.Category());
		categorizedAttributes.insert({access.Category(), attr});
		a = GetAttribute(access.Category());
	}
	AttributeEffect *e = a->GetEffect(access.Effect());
	if(!e)
		a->AddEffect(AttributeEffect(access.Effect(), value, access.GetDefaultMinimum()));
	else
		e->Set(value);
}



// Checks whether there are any attributes stored here.
bool AttributeStore::empty() const
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
double AttributeStore::GetMinimum(const AttributeAccess attribute) const
{
	const AttributeEffect *e = GetEffect(attribute);
	if(e == nullptr)
		return attribute.GetDefaultMinimum();
	return e->Minimum();
}



double AttributeStore::GetMinimum(const char *attribute) const
{
	return GetMinimum(std::string(attribute));
}



// Loads data from the data node. This function can be called multiple times on an instance.
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
		// The node containing every effect of this attribute category
		DataNode node;
		node.AddToken(Attribute::GetCategoryName(it.first));
		// The base effect of the attribute that is put on the category's line
		optional<AttributeEffectType> baseEffect = AttributeAccess::GetBaseEffect(it.first);
		for(auto &entry : it.second.Effects())
		{
			if(!entry.second.Value())
				continue;

			ostringstream temp;
			temp << entry.second.Value();

			// Add the value to the category of as a new effect.
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
			count = min(count, CanAdd(AttributeAccess(it.first, item.first), other, count));

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



void AttributeStore::ForEach(const std::function<void(const AnyAttribute &, double)> &function) const
{
	for(auto &it : textAttributes)
		function({it.first}, it.second);
	for(auto &it : categorizedAttributes)
		for(const auto &item : it.second.Effects())
			function({AttributeAccess(it.first, item.first)}, item.second.Value());
}
