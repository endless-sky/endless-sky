/* CapacityList.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "CapacityList.h"

#include "DataNode.h"

#include <algorithm>
#include <ranges>

using namespace std;



CapacityList::Capacity::Capacity(const DataNode &node)
{
	Load(node);
}



void CapacityList::Capacity::Load(const DataNode &node)
{
	backingAttribute = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "display" && hasValue)
			displayName = child.Token(1);
		else if(key == "required")
			required = true;
		else if(key == "optional")
			required = false;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool CapacityList::Capacity::Required() const
{
	return required;
}



const std::string &CapacityList::Capacity::Name() const
{
	return displayName;
}



const std::string &CapacityList::Capacity::Attribute() const
{
	return backingAttribute;
}



bool CapacityList::Capacity::operator<(const Capacity &other) const
{
	return SortHelper(*this, other);
}



bool CapacityList::Capacity::operator()(const Capacity &a, const Capacity &b) const
{
	return SortHelper(a, b);
}



bool CapacityList::Capacity::SortHelper(const Capacity &a, const Capacity &b)
{
	if(a.precedence == b.precedence)
		return a.displayName < b.displayName;
	return a.precedence < b.precedence;
}



void CapacityList::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) != "attribute")
		{
			child.PrintTrace("Skipping unrecognized attribute:");
			continue;
		}
		if(child.Size() < 2)
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		const string &attribute = child.Token(1);
		if(child.Size() >= 3)
			currentPrecedence = child.Value(2);

		// If a given capacity attribute already exists, its details will be updated.
		auto it = ranges::find_if(list, [&attribute](const Capacity &c) noexcept -> bool {
			return attribute == c.backingAttribute;
		});
		if(it != list.end())
		{
			it->Load(child);
			// Increment the precedence for the next capacity that gets loaded.
			it->precedence = currentPrecedence++;
		}
		else
		{
			list.emplace_back(child);
			list.back().precedence = currentPrecedence++;
		}
	}
}



void CapacityList::Clear()
{
	list.clear();
	currentPrecedence = 0;
}



void CapacityList::Sort()
{
	sort(list.begin(), list.end());
}
