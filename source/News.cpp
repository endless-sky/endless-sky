/* News.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "News.h"

#include "DataNode.h"
#include "Random.h"
#include "image/SpriteSet.h"

#include <algorithm>

using namespace std;



void News::Load(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	for(const DataNode &child : node)
	{
		const bool add = (child.Token(0) == "add");
		const bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		// Get the key and value (if any).
		const string &tag = child.Token((add || remove) ? 1 : 0);
		const int valueIndex = (add || remove) ? 2 : 1;
		const bool hasValue = child.Size() > valueIndex;

		if(tag == "location")
		{
			if(add && !location.IsEmpty())
				child.PrintTrace("Cannot \"add\" to an existing location filter:");
			else if(remove)
			{
				location = LocationFilter{};
				if(child.HasChildren())
					child.PrintTrace("Removing full location filter; partial removal is not supported:");
			}
			else
				location.Load(child, visitedSystems, visitedPlanets);
		}
		else if(tag == "name")
		{
			if(remove)
			{
				speakerNames = Phrase{};
				if(child.HasChildren())
					child.PrintTrace("Removing all names; removal of individual names is not supported:");
			}
			else
				speakerNames.Load(child);
		}
		else if(tag == "portrait")
		{
			if(remove && !hasValue)
				portraits.clear();
			else if(remove)
			{
				// Collect all values to be removed.
				auto toRemove = set<const Sprite *>{};
				for(int i = valueIndex; i < child.Size(); ++i)
					toRemove.emplace(SpriteSet::Get(child.Token(i)));

				// Erase them in unison.
				portraits.erase(remove_if(portraits.begin(), portraits.end(),
						[&toRemove](const Sprite *sprite) { return toRemove.find(sprite) != toRemove.end(); }),
					portraits.end());
			}
			else
			{
				for(int i = valueIndex; i < child.Size(); ++i)
					portraits.push_back(SpriteSet::Get(child.Token(i)));
				for(const DataNode &grand : child)
					portraits.push_back(SpriteSet::Get(grand.Token(0)));
			}
		}
		else if(tag == "message")
		{
			if(remove)
			{
				messages = Phrase{};
				if(child.HasChildren())
					child.PrintTrace("Removing all messages; removal of single messages is not supported:");
			}
			else
				messages.Load(child);
		}
		else if(tag == "to" && hasValue && child.Token(valueIndex) == "show")
		{
			if(add && !toShow.IsEmpty())
				child.PrintTrace("Cannot \"add\" to an existing condition set:");
			else if(remove)
			{
				toShow = ConditionSet{};
				if(child.HasChildren())
					child.PrintTrace("Removing all conditions; removal of condition subsets is not supported:");

			}
			else
				toShow.Load(child, playerConditions);
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool News::IsEmpty() const
{
	return messages.IsEmpty() || speakerNames.IsEmpty();
}



// Check if this news item is available given the player's planet.
bool News::Matches(const Planet *planet) const
{
	// If no location filter is specified, it should never match. This can be
	// used to create news items that are never shown until an event "activates"
	// them by specifying their location.
	// Similarly, by updating a news item with "remove location", it can be deactivated.
	return location.IsEmpty() ? false : (location.Matches(planet) && toShow.Test());
}



// Get the speaker's name.
string News::SpeakerName() const
{
	return speakerNames.Get();
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
