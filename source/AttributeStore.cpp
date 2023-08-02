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

using namespace std;

const double AttributeStore::EPS = 0.0000000001;
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
