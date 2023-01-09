/* StartConditions.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StartConditions.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Logger.h"
#include "Planet.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "System.h"

#include <algorithm>
#include <sstream>

using namespace std;



StartConditions::StartConditions(const DataNode &node)
{
	Load(node);
}



void StartConditions::Load(const DataNode &node)
{
	// When a plugin modifies an existing starting condition, default to
	// clearing the previously-defined description text. The plugin may
	// amend it by using "add description"
	bool clearDescription = !description.empty();

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

		// Determine if the child is a "core" attribute.
		if(CoreStartData::LoadChild(child, add, remove))
			continue;

		// Otherwise, we should try to parse it.
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);

		if(remove)
		{
			if(key == "name")
				name.clear();
			else if(key == "description")
				description.clear();
			else if(key == "hint")
				hint.clear();
			else if(key == "thumbnail")
				thumbnail = nullptr;
			else if(key == "ships")
				ships.clear();
			else if(key == "ship" && hasValue)
				ships.erase(remove_if(ships.begin(), ships.end(),
					[&value](const Ship &s) noexcept -> bool { return s.ModelName() == value; }),
					ships.end());
			else if(key == "conversation")
				conversation = ExclusiveItem<Conversation>();
			else if(key == "to" && child.Size() >= 2)
			{
				if(child.Token(1) == "display")
					toDisplay = ConditionSet();
				else if(child.Token(1) == "reveal")
					toReveal = ConditionSet();
				else if(child.Token(1) == "unlock")
					toUnlock = ConditionSet();
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
			else if(key == "conditions")
				conditions = ConditionSet();
			else
				child.PrintTrace("Skipping unsupported use of \"remove\":");
		}
		else if(key == "name" && hasValue)
			name = value;
		else if(key == "description" && hasValue)
		{
			if(!add && clearDescription)
			{
				description.clear();
				clearDescription = false;
			}
			description += value + "\n";
		}
		else if(key == "hint" && hasValue)
			hint = value;
		else if(key == "thumbnail" && hasValue)
			thumbnail = SpriteSet::Get(value);
		else if(child.Token(0) == "ship" && child.Size() >= 2)
		{
			// TODO: support named stock ships.
			// Assume that child nodes introduce a full ship definition. Even without child nodes,
			// Ship::Load + Ship::FinishLoading will create the expected ship instance if there is
			// a 3rd token (i.e. this will be treated as though it were a ship variant definition,
			// without making the variant available to the rest of GameData).
			if(child.HasChildren() || child.Size() >= 3)
				ships.emplace_back(child);
			// If there's only 2 tokens & there's no child nodes, the created instance would be ill-formed.
			else
				child.PrintTrace("Skipping unsupported use of a \"stock\" ship (a full definition is required):");
		}
		else if(key == "conversation" && child.HasChildren() && !add)
			conversation = ExclusiveItem<Conversation>(Conversation(child));
		else if(key == "conversation" && hasValue && !child.HasChildren())
			conversation = ExclusiveItem<Conversation>(GameData::Conversations().Get(value));
		else if(add)
			child.PrintTrace("Skipping unsupported use of \"add\":");
		// Only global conditions are supported in these condition sets. The global conditions are accessed directly,
		// and therefore do not need the "global: " prefix.
		else if(key == "to" && child.Size() >= 2)
		{
			if(child.Token(1) == "display")
				toDisplay.Load(child);
			else if(child.Token(1) == "reveal")
				toReveal.Load(child);
			else if(child.Token(1) == "unlock")
				toUnlock.Load(child);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else
			conditions.Add(child);
	}
	if(description.empty())
		description = "(No description provided.)";
	if(name.empty())
		name = "(Unnamed start)";

	// If no identifier is supplied, the creator would like this starting scenario to be isolated from
	// other plugins. Thus, use an unguessable, non-reproducible identifier, this item's memory address.
	if(identifier.empty() && node.Size() >= 2)
		identifier = node.Token(1);
	else if(identifier.empty())
	{
		stringstream addr;
		addr << name << " " << this;
		identifier = addr.str();
	}
}



// Finish loading the ship definitions.
void StartConditions::FinishLoading()
{
	for(Ship &ship : ships)
		ship.FinishLoading(true);

	string reason = GetConversation().Validate();
	if(!GetConversation().IsValidIntro() || !reason.empty())
		Logger::LogError("Warning: The start scenario \"" + Identifier() + "\" (named \""
			+ GetDisplayName() + "\") has an invalid starting conversation."
			+ (reason.empty() ? "" : "\n\t" + std::move(reason)));
}



bool StartConditions::IsValid() const
{
	// A start must specify a valid system.
	if(!system || !system->IsValid())
		return false;

	// A start must specify a valid planet in its specified system.
	if(!planet || !planet->IsValid() || planet->GetSystem() != system)
		return false;

	// A start must reference a valid "intro" conversation, either stock or custom.
	if(!GetConversation().IsValidIntro() || !GetConversation().Validate().empty())
		return false;

	// All ship models must be valid.
	if(any_of(ships.begin(), ships.end(), [](const Ship &it) noexcept -> bool { return !it.IsValid(); }))
		return false;

	return true;
}



const ConditionSet &StartConditions::GetConditions() const noexcept
{
	return conditions;
}



const vector<Ship> &StartConditions::Ships() const noexcept
{
	return ships;
}



const Conversation &StartConditions::GetConversation() const
{
	return *conversation;
}



const Sprite *StartConditions::GetThumbnail() const noexcept
{
	return thumbnail;
}



const std::string &StartConditions::GetDisplayName() const noexcept
{
	return name;
}



const std::string &StartConditions::GetDescription() const noexcept
{
	return description;
}



const std::string &StartConditions::GetHint() const noexcept
{
	return hint;
}



bool StartConditions::Visible(const ConditionsStore &conditionsStore) const
{
	return toDisplay.Test(conditionsStore);
}



bool StartConditions::Revealed(const ConditionsStore &conditionsStore) const
{
	return toReveal.Test(conditionsStore);
}



bool StartConditions::Unlocked(const ConditionsStore &conditionsStore) const
{
	return toUnlock.Test(conditionsStore);
}
