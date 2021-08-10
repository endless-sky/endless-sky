/* Wormhole.cpp
Copyright (c) 2021 quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Wormhole.h"

#include "DataNode.h"
#include "GameData.h"
#include "System.h"

#include <algorithm>

using namespace std;



Wormhole::Wormhole(const vector<const System *> &systems) noexcept : systems(&systems)
{
}



// Load a planet's description from a file.
void Wormhole::Load(const DataNode &node)
{
	if(node.Size() < 1)
		return;
	isDefined = true;
	
	for(const DataNode &child : node)
	{
		// Check for the "add" or "remove" keyword.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}
		
		// Get the key and value (if any).
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);
		
		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		bool removeAll = (remove && !hasValue);
		// "<key> clear" is the deprecated way of writing "remove <key>."
		removeAll |= (!add && !remove && hasValue && value == "clear");
		// Clear the links.
		if(removeAll && key == "link")
		{
			links.clear();
			continue;
		}
		
		// Handle the attributes which can be "removed."
		if(key == "link" && child.Size() > valueIndex + 1)
		{
			const auto *from = GameData::Systems().Get(child.Token(valueIndex));
			const auto *to = GameData::Systems().Get(child.Token(valueIndex + 1));
			if(remove)
			{
				// Only erase if the link is an exact match.
				if(links[from] == to)
					links.erase(from);
			}
			else
				links[from] = to;
		}
		else if(key == "linked")
			linked = !remove;
		else if(key == "display name")
		{
			if(remove)
				name.clear();
			else if(hasValue)
				name = node.Token(1);
		}
		else if(remove)
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool Wormhole::IsValid() const
{
	return isDefined;
}



// Get the name of the wormhole.
const string &Wormhole::Name() const
{
	return name;
}



bool Wormhole::IsLinked() const
{
	return linked;
}



const System *Wormhole::WormholeSource(const System *to) const
{
	using value_type = decltype(links)::value_type;
	auto it = find_if(links.begin(), links.end(),
			[&to](const value_type &val)
			{
				return val.second == to;
			});
	return it == links.end() ? to : it->first;
}




const System *Wormhole::WormholeDestination(const System *from) const
{
	auto it = links.find(from);
	return it == links.end() ? from : it->second;
}



const vector<const System *> &Wormhole::Systems() const
{
	return *systems;
}



void Wormhole::RemoveLinks(const System *from)
{
	links.erase(from);

	auto it = links.begin();
	using value_type = decltype(links)::value_type;
	while(find_if(it, links.end(), [&from](const value_type &pair)
				{
					return pair.second == from;
				}) != links.end())
			it = links.erase(it);
}
