/* News.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "News.h"

#include "DataNode.h"
#include "Random.h"
#include "SpriteSet.h"

using namespace std;



void News::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &tag = child.Token(0);
		if(tag == "location")
			location.Load(child);
		else if(tag == "name")
			names.Load(child);
		else if(tag == "portrait")
		{
			for(int i = 1; i < child.Size(); ++i)
				portraits.push_back(SpriteSet::Get(child.Token(i)));
			for(const DataNode &grand : child)
				portraits.push_back(SpriteSet::Get(grand.Token(0)));
		}
		else if(tag == "message")
			messages.Load(child);
		else
			child.PrintTrace("Unrecognized news attribute:");
	}
}



// Check if this news item is available on the given planet.
bool News::Matches(const Planet *planet) const
{
	// If no location filter is specified, it should never match. This can be
	// used to create news items that are never shown until an event "activates"
	// them by specifying their location.
	return location.IsEmpty() ? false : location.Matches(planet);
}



// Get the speaker's name.
string News::Name() const
{
	return names.Get();
}



// Pick a portrait at random out of the possible options.
const Sprite *News::Portrait() const
{
	return portraits.empty() ? nullptr : portraits[Random::Int(portraits.size())];
}



// Get the speaker's message, chosen randomly.
string News::Message() const
{
	return messages.Get();
}
