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
#include "text/Format.h"
#include "GameData.h"
#include "Logger.h"
#include "Planet.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "System.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {
	// A default string to return if somehow the current state does not exist in the state map.
	static const string ILLEGAL = "(ILLEGAL STATE)";
}



StartConditions::StartConditions(const DataNode &node, const ConditionsStore *globalConditions,
		const ConditionsStore *playerConditions)
{
	Load(node, globalConditions, playerConditions);
}



void StartConditions::Load(const DataNode &node, const ConditionsStore *globalConditions,
		const ConditionsStore *playerConditions)
{
	// When a plugin modifies an existing starting condition, default to
	// clearing the previously-defined description text. The plugin may
	// amend it by using "add description"
	StartInfo &unlocked = infoByState[StartState::UNLOCKED];
	bool clearDescription = !unlocked.description.empty();

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

		// Determine if the child is a "core" or "UNLOCKED state" attribute. Removal of such attributes
		// is handled below.
		if(!remove && (CoreStartData::LoadChild(child, add)
				|| LoadStateChild(child, unlocked, clearDescription, add)))
			continue;

		// Otherwise, we should try to parse it.
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);

		if(remove)
		{
			if(key == "name")
				unlocked.displayName.clear();
			else if(key == "description")
				unlocked.description.clear();
			else if(key == "thumbnail")
				unlocked.thumbnail = nullptr;
			else if(key == "ships")
				ships.clear();
			else if(key == "ship" && hasValue)
				ships.erase(remove_if(ships.begin(), ships.end(),
					[&value](const Ship &s) noexcept -> bool { return s.TrueModelName() == value; }),
					ships.end());
			else if(key == "conversation")
				conversation = ExclusiveItem<Conversation>();
			else if(key == "to" && hasValue)
			{
				if(value == "display")
					toDisplay = ConditionSet();
				else if(value == "reveal")
					toReveal = ConditionSet();
				else if(value == "unlock")
					toUnlock = ConditionSet();
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
			else if(key == "conditions")
				conditions = ConditionAssignments();
			else
				child.PrintTrace("Skipping unsupported use of \"remove\":");
		}
		else if(key == "ship" && hasValue)
		{
			// TODO: support named stock ships.
			// Assume that child nodes introduce a full ship definition. Even without child nodes,
			// Ship::Load + Ship::FinishLoading will create the expected ship instance if there is
			// a 3rd token (i.e. this will be treated as though it were a ship variant definition,
			// without making the variant available to the rest of GameData).
			if(child.HasChildren() || child.Size() >= add + 3)
				ships.emplace_back(child, playerConditions);
			// If there's only 2 tokens & there's no child nodes, the created instance would be ill-formed.
			else
				child.PrintTrace("Skipping unsupported use of a \"stock\" ship (a full definition is required):");
		}
		else if(key == "conversation" && child.HasChildren() && !add)
			conversation = ExclusiveItem<Conversation>(Conversation(child, playerConditions));
		else if(key == "conversation" && hasValue && !child.HasChildren())
			conversation = ExclusiveItem<Conversation>(GameData::Conversations().Get(value));
		else if(add)
			child.PrintTrace("Skipping unsupported use of \"add\":");
		// Only global conditions are supported in these ConditionSets. The global conditions are accessed directly,
		// and therefore do not need the "global: " prefix.
		else if(key == "to" && hasValue)
		{
			if(value == "display")
				toDisplay.Load(child, globalConditions);
			else if(value == "reveal")
				toReveal.Load(child, globalConditions);
			else if(value == "unlock")
				toUnlock.Load(child, globalConditions);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "on" && hasValue)
		{
			// The HIDDEN state contains no information. The UNLOCKED StateInfo is a child of the root node
			// instead of an "on" node.
			if(value == "display")
				LoadState(child, StartState::DISPLAYED);
			else if(value == "reveal")
				LoadState(child, StartState::REVEALED);
		}
		else
			conditions.Add(child, playerConditions);
	}

	// The unlocked state must have at least some information.
	if(unlocked.description.empty())
		unlocked.description = "(No description provided.)";
	if(unlocked.displayName.empty())
		unlocked.displayName = "(Unnamed start)";

	// If the REVEALED or DISPLAYED states are missing information, fill them in with "???".
	// Also use the UNLOCKED state thumbnail if the other states are missing one.
	FillState(StartState::REVEALED, unlocked.thumbnail);
	FillState(StartState::DISPLAYED, unlocked.thumbnail);

	// If no identifier is supplied, the creator would like this starting scenario to be isolated from
	// other plugins. Thus, use an unguessable, non-reproducible identifier, this item's memory address.
	if(identifier.empty() && node.Size() >= 2)
		identifier = node.Token(1);
	else if(identifier.empty())
	{
		stringstream addr;
		addr << unlocked.displayName << " " << this;
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
	StartInfo &unlocked = infoByState[StartState::UNLOCKED];
	unlocked.planet = GetPlanet().DisplayName();
	unlocked.system = GetSystem().DisplayName();
	unlocked.date = GetDate();
	unlocked.credits = Format::Credits(GetAccounts().Credits());
	unlocked.debt = Format::Credits(GetAccounts().TotalDebt());

	string reason = GetConversation().Validate();
	if(!GetConversation().IsValidIntro() || !reason.empty())
		Logger::Log("The start scenario \"" + Identifier() + "\" (named \""
			+ unlocked.displayName + "\") has an invalid starting conversation."
			+ (reason.empty() ? "" : "\n\t" + std::move(reason)), Logger::Level::WARNING);
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



const ConditionAssignments &StartConditions::GetConditions() const noexcept
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
	return it == infoByState.end() ? ILLEGAL : it->second.displayName;
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



const string &StartConditions::GetDateString() const noexcept
{
	auto it = infoByState.find(state);
	if(it == infoByState.end())
		return ILLEGAL;
	if(it->second.date)
		return it->second.date.ToString();
	return it->second.dateString;
}



const string &StartConditions::GetCredits() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.credits;
}



const string &StartConditions::GetDebt() const noexcept
{
	auto it = infoByState.find(state);
	return it == infoByState.end() ? ILLEGAL : it->second.debt;
}



bool StartConditions::Visible() const
{
	return toDisplay.Test();
}



void StartConditions::SetState()
{
	if(toDisplay.Test())
	{
		if(toReveal.Test())
		{
			if(toUnlock.Test())
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
		LoadStateChild(child, info, clearDescription, false);
}



bool StartConditions::LoadStateChild(const DataNode &child, StartInfo &info, bool &clearDescription, bool isAdd)
{
	const string &key = child.Token(isAdd ? 1 : 0);
	int valueIndex = (isAdd) ? 2 : 1;
	bool hasValue = (child.Size() > valueIndex);
	const string &value = child.Token(hasValue ? valueIndex : 0);

	if(key == "name" && hasValue)
		info.displayName = value;
	else if(key == "description" && hasValue)
	{
		if(clearDescription)
		{
			info.description.clear();
			clearDescription = false;
		}
		info.description += value + "\n";
	}
	else if(key == "thumbnail" && hasValue)
		info.thumbnail = SpriteSet::Get(value);
	else if(key == "system" && hasValue)
		info.system = value;
	else if(key == "planet" && hasValue)
		info.planet = value;
	else if(key == "date" && hasValue)
		if(child.Size() >= valueIndex + 3)
			info.date = Date(child.Value(valueIndex), child.Value(valueIndex + 1), child.Value(valueIndex + 2));
		else
			info.dateString = value;
	// Format credits and debt where applicable.
	else if(key == "credits" && hasValue)
		if(child.IsNumber(value))
			info.credits = Format::Credits(child.Value(value));
		else
			info.credits = value;
	else if(key == "debt" && hasValue)
		if(child.IsNumber(value))
			info.debt = Format::Credits(child.Value(value));
		else
			info.debt = value;
	else
		return false;
	return true;
}



void StartConditions::FillState(StartState fillState, const Sprite *thumbnail)
{
	StartInfo &fill = infoByState[fillState];
	if(!fill.thumbnail)
		fill.thumbnail = thumbnail;
	if(fill.displayName.empty())
		fill.displayName = "???";
	if(fill.description.empty())
		fill.description = "???";
	if(fill.system.empty())
		fill.system = "???";
	if(fill.planet.empty())
		fill.planet = "???";
	if(fill.dateString.empty() && !fill.date)
		fill.dateString = "???";
	if(fill.credits.empty())
		fill.credits = "???";
	if(fill.debt.empty())
		fill.debt = "???";
}
