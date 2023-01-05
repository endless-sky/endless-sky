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

namespace {
	// A default string to return if somehow the current state does not exist in the state map.
	static const string ILLEGAL = "(ILLEGAL STATE)";
}



StartConditions::StartConditions(const DataNode &node)
{
	Load(node);
}



void StartConditions::Load(const DataNode &node)
{
	// When a plugin modifies an existing starting condition, default to
	// clearing the previously-defined description text. The plugin may
	// amend it by using "add description"
	bool clearDescription = !infoByState[StartState::UNLOCKED].description.empty();

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
				infoByState[StartState::UNLOCKED].name.clear();
			else if(key == "description")
				infoByState[StartState::UNLOCKED].description.clear();
			else if(key == "thumbnail")
				infoByState[StartState::UNLOCKED].thumbnail = nullptr;
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
			infoByState[StartState::UNLOCKED].name = value;
		else if(key == "description" && hasValue)
		{
			if(!add && clearDescription)
			{
				infoByState[StartState::UNLOCKED].description.clear();
				clearDescription = false;
			}
			infoByState[StartState::UNLOCKED].description += value + "\n";
		}
		else if(key == "thumbnail" && hasValue)
			infoByState[StartState::UNLOCKED].thumbnail = SpriteSet::Get(value);
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
		// Only global conditions are supported in these ConditionSets. The global conditions are accessed directly,
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
		else if(key == "on")
		{
			// The HIDDEN state contains no information. The UNLOCKED StateInfo is a child of the root node
			// instead of an "on" node.
			if(child.Token(1) == "display")
				LoadState(child, StartState::DISPLAYED);
			else if(child.Token(1) == "reveal")
				LoadState(child, StartState::REVEALED);
		}
		else
			conditions.Add(child);
	}

	// The unlocked state must have at least some information.
	if(infoByState[StartState::UNLOCKED].description.empty())
		infoByState[StartState::UNLOCKED].description = "(No description provided.)";
	if(infoByState[StartState::UNLOCKED].name.empty())
		infoByState[StartState::UNLOCKED].name = "(Unnamed start)";

	// If no identifier is supplied, the creator would like this starting scenario to be isolated from
	// other plugins. Thus, use an unguessable, non-reproducible identifier, this item's memory address.
	if(identifier.empty() && node.Size() >= 2)
		identifier = node.Token(1);
	else if(identifier.empty())
	{
		stringstream addr;
		addr << infoByState[StartState::UNLOCKED].name << " " << this;
		identifier = addr.str();
	}
}



// Finish loading the ship definitions.
void StartConditions::FinishLoading()
{
	for(Ship &ship : ships)
		ship.FinishLoading(true);

	// The UNLOCKED StartInfo should always display the correct information. Therefore, we get the
	// planet and system names now. If we had gotten these during Load, the planet and system provided
	// may now be invalid, meaning the CoreStartData would actually send the start to New Boston instead
	// of what was displayed.
	infoByState[StartState::UNLOCKED].planet = GetPlanet().Name();
	infoByState[StartState::UNLOCKED].system = GetSystem().Name();

	// If a "lower" state is missing information, copy from the state "above" it.
	FillState(StartState::UNLOCKED, StartState::REVEALED);
	FillState(StartState::REVEALED, StartState::DISPLAYED);

	string reason = GetConversation().Validate();
	if(!GetConversation().IsValidIntro() || !reason.empty())
		Logger::LogError("Warning: The start scenario \"" + Identifier() + "\" (named \""
			+ infoByState[StartState::UNLOCKED].name + "\") has an invalid starting conversation."
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
	// The state we ask for should always be present in the map, but just in case it isn't,
	// make sure we're not accessing out of bound information.
	auto it = infoByState.find(state);
	return it == infoByState.end() ? nullptr : it->second.thumbnail;
}



const string &StartConditions::GetDisplayName() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.name;
}



const string &StartConditions::GetDescription() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.description;
}



const string &StartConditions::GetPlanetName() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.planet;
}



const string &StartConditions::GetSystemName() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.system;
}



bool StartConditions::Visible(const ConditionsStore &conditionsStore) const
{
	return toDisplay.Test(conditionsStore);
}



void StartConditions::SetState(const ConditionsStore &conditionsStore)
{
	if(toDisplay.Test(conditionsStore))
	{
		if(toReveal.Test(conditionsStore))
		{
			if(toUnlock.Test(conditionsStore))
				state = StartState::UNLOCKED;
			else
				state = StartState::REVEALED;
		}
		else
			state = StartState::DISPLAYED;
	}
	else
		state = StartState::HIDDEN;
}



bool StartConditions::IsUnlocked() const
{
	return state == StartState::UNLOCKED;
}



void StartConditions::LoadState(const DataNode &node, StartState state)
{
	StartInfo &info = infoByState[state];
	bool clearDescription = !info.description.empty();
	for(const auto &child : node)
	{
		if(child.Token(0) == "name" && child.Size() >= 2)
			info.name = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			if(clearDescription)
			{
				info.description.clear();
				clearDescription = false;
			}
			info.description += child.Token(1) + "\n";
		}
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			info.thumbnail = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			info.system = child.Token(1);
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			info.planet = child.Token(1);
	}
}



void StartConditions::FillState(StartState fromState, StartState toState)
{
	StartInfo &from = infoByState[fromState];
	StartInfo &to = infoByState[toState];
	if(!to.thumbnail)
		to.thumbnail = from.thumbnail;
	if(to.name.empty())
		to.name = from.name;
	if(to.description.empty())
		to.description = from.description;
	if(to.system.empty())
		to.system = from.system;
	if(to.planet.empty())
		to.planet = from.planet;
}
