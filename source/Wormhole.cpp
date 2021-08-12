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
#include "Planet.h"
#include "System.h"

#include <algorithm>

using namespace std;



// Load a planet's description from a file.
void Wormhole::Load(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	isDefined = true;

	planet = GameData::Planets().Get(node.Token(1));
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
		if((remove && !hasValue) && key == "link")
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
				auto it = links.find(from);
				if(it != links.end() && it->second == to)
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
				name = "???";
			else if(hasValue)
				name = value;
		}
		else if(remove)
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// If no links were specified, auto generate them.
	if(links.empty())
		GenerateLinks();
}



void Wormhole::LoadFromPlanet(const Planet *planet)
{
	this->planet = planet;
	linked = !planet->Description().empty();
	GenerateLinks();
}



bool Wormhole::IsValid() const
{
	if(!isDefined)
		return false;

	for(auto &&pair : links)
		if(!pair.first->IsValid() || !pair.second->IsValid())
			return false;

	return true;
}



const Planet *Wormhole::GetPlanet() const
{
	return planet;
}



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

	auto source = it == links.end() ? to : it->first;

	// If source refers to a system without this wormhole, go through the link backwards.
	if(it != links.end() && !planet->IsInSystem(source))
		source = WormholeSource(source);
	return source;
}




const System *Wormhole::WormholeDestination(const System *from) const
{
	auto it = links.find(from);
	auto dest = it == links.end() ? from : it->second;

	// If dest refers to a system without this wormhole, go through the link.
	if(it != links.end() && !planet->IsInSystem(dest))
		dest = WormholeDestination(dest);
	return dest;
}



const unordered_map<const System *, const System *> &Wormhole::Links() const
{
	return links;
}



void Wormhole::UpdateFromPlanet()
{
	linked = !planet->Description().empty();
}



void Wormhole::GenerateLinks()
{
	// Wormhole links form a closed loop through every system this wormhole is in.
	for(size_t i = 0; i < planet->Systems().size(); ++i)
	{
		int next = i == planet->Systems().size() - 1 ? 0 : i + 1;
		links[planet->Systems()[i]] = planet->Systems()[next];
	}
}
