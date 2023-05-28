/* MissionAction.cpp
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

#include "MissionAction.h"

#include "CargoHold.h"
#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "text/Format.h"
#include "GameData.h"
#include "GameEvent.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "TextReplacements.h"
#include "UI.h"

using namespace std;

namespace {
	int CountInCargo(const Outfit *outfit, const PlayerInfo &player)
	{
		int available = 0;
		// If landed, all cargo from available ships is pooled together.
		if(player.GetPlanet())
			available += player.Cargo().Get(outfit);
		// Otherwise only count outfits in the cargo holds of in-system ships.
		else
		{
			const System *here = player.GetSystem();
			for(const auto &ship : player.Ships())
			{
				if(ship->IsDisabled() || ship->IsParked())
					continue;
				if(ship->GetSystem() == here || (ship->CanBeCarried()
						&& !ship->GetSystem() && ship->GetParent()->GetSystem() == here))
					available += ship->Cargo().Get(outfit);
			}
		}
		return available;
	}
}



// Construct and Load() at the same time.
MissionAction::MissionAction(const DataNode &node, const string &missionName)
{
	Load(node, missionName);
}



void MissionAction::Load(const DataNode &node, const string &missionName)
{
	if(node.Size() >= 2)
		trigger = node.Token(1);
	if(node.Size() >= 3)
		system = node.Token(2);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = (child.Size() >= 2);

		if(key == "dialog")
		{
			if(hasValue && child.Token(1) == "phrase")
			{
				if(!child.HasChildren() && child.Size() == 3)
					dialogPhrase = ExclusiveItem<Phrase>(GameData::Phrases().Get(child.Token(2)));
				else
					child.PrintTrace("Skipping unsupported dialog phrase syntax:");
			}
			else if(!hasValue && child.HasChildren() && (*child.begin()).Token(0) == "phrase")
			{
				const DataNode &firstGrand = (*child.begin());
				if(firstGrand.Size() == 1 && firstGrand.HasChildren())
					dialogPhrase = ExclusiveItem<Phrase>(Phrase(firstGrand));
				else
					firstGrand.PrintTrace("Skipping unsupported dialog phrase syntax:");
			}
			else
				Dialog::ParseTextNode(child, 1, dialogText);
		}
		else if(key == "conversation" && child.HasChildren())
			conversation = ExclusiveItem<Conversation>(Conversation(child, missionName));
		else if(key == "conversation" && hasValue)
			conversation = ExclusiveItem<Conversation>(GameData::Conversations().Get(child.Token(1)));
		else if(key == "require" && hasValue)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			if(count >= 0)
				requiredOutfits[GameData::Outfits().Get(child.Token(1))] = count;
			else
				child.PrintTrace("Error: Skipping invalid \"require\" amount:");
		}
		// The legacy syntax "outfit <outfit> 0" means "the player must have this outfit installed."
		else if(key == "outfit" && child.Size() >= 3 && child.Token(2) == "0")
		{
			child.PrintTrace("Warning: Deprecated use of \"outfit\" with count of 0. Use \"require <outfit>\" instead:");
			requiredOutfits[GameData::Outfits().Get(child.Token(1))] = 1;
		}
		else if(key == "system")
		{
			if(system.empty() && child.HasChildren())
				systemFilter.Load(child);
			else
				child.PrintTrace("Error: Unsupported use of \"system\" LocationFilter:");
		}
		else
			action.LoadSingle(child, missionName);
	}
}



// Note: the Save() function can assume this is an instantiated mission, not
// a template, so it only has to save a subset of the data.
void MissionAction::Save(DataWriter &out) const
{
	if(system.empty())
		out.Write("on", trigger);
	else
		out.Write("on", trigger, system);
	out.BeginChild();
	{
		if(!systemFilter.IsEmpty())
		{
			out.Write("system");
			// LocationFilter indentation is handled by its Save method.
			systemFilter.Save(out);
		}
		if(!dialogText.empty())
		{
			out.Write("dialog");
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(dialogText, "\n\t"))
					out.Write(line);
			}
			out.EndChild();
		}
		if(!conversation->IsEmpty())
			conversation->Save(out);
		for(const auto &it : requiredOutfits)
			out.Write("require", it.first->TrueName(), it.second);

		action.Save(out);
	}
	out.EndChild();
}



// Check this template or instantiated MissionAction to see if any used content
// is not fully defined (e.g. plugin removal, typos in names, etc.).
string MissionAction::Validate() const
{
	// Any filter used to control where this action triggers must be valid.
	if(!systemFilter.IsValid())
		return "system location filter";

	// Stock phrases that generate text must be defined.
	if(dialogPhrase.IsStock() && dialogPhrase->IsEmpty())
		return "stock phrase";

	// Stock conversations must be defined.
	if(conversation.IsStock() && conversation->IsEmpty())
		return "stock conversation";

	// Conversations must have valid actions.
	string reason = conversation->Validate();
	if(!reason.empty())
		return reason;

	// Required content must be defined & valid.
	for(auto &&outfit : requiredOutfits)
		if(!outfit.first->IsDefined())
			return "required outfit \"" + outfit.first->TrueName() + "\"";

	return action.Validate();
}



const string &MissionAction::DialogText() const
{
	return dialogText;
}



// Check if this action can be completed right now. It cannot be completed
// if it takes away money or outfits that the player does not have.
bool MissionAction::CanBeDone(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	if(player.Accounts().Credits() < -Payment())
		return false;

	const Ship *flagship = player.Flagship();
	for(auto &&it : action.Outfits())
	{
		// If this outfit is being given, the player doesn't need to have it.
		if(it.second > 0)
			continue;

		// Outfits may always be taken from the flagship. If landed, they may also be taken from
		// the collective cargohold of any in-system, non-disabled escorts (player.Cargo()). If
		// boarding, consider only the flagship's cargo hold. If in-flight, show mission status
		// by checking the cargo holds of ships that would contribute to player.Cargo if landed.
		int available = flagship ? flagship->OutfitCount(it.first) : 0;
		available += boardingShip ? flagship->Cargo().Get(it.first)
				: CountInCargo(it.first, player);

		if(available < -it.second)
			return false;
	}

	for(auto &&it : requiredOutfits)
	{
		// Maps are not normal outfits; they represent the player's spatial awareness.
		int mapSize = it.first->Get("map");
		if(mapSize > 0)
		{
			bool needsUnmapped = it.second == 0;
			// This action can't be done if it requires an unmapped region, but the region is
			// mapped, or if it requires a mapped region but the region is not mapped.
			if(needsUnmapped == player.HasMapped(mapSize))
				return false;
			continue;
		}

		// Requiring the player to have 0 of this outfit means all ships and all cargo holds
		// must be checked, even if the ship is disabled, parked, or out-of-system.
		if(!it.second)
		{
			// When landed, ships pool their cargo into the player's cargo.
			if(player.GetPlanet() && player.Cargo().Get(it.first))
				return false;

			for(const auto &ship : player.Ships())
				if(!ship->IsDestroyed())
					if(ship->OutfitCount(it.first) || ship->Cargo().Get(it.first))
						return false;
		}
		else
		{
			// Required outfits must be present on the player's flagship or
			// in the cargo holds of able ships at the player's location.
			int available = flagship ? flagship->OutfitCount(it.first) : 0;
			available += boardingShip ? flagship->Cargo().Get(it.first)
					: CountInCargo(it.first, player);

			if(available < it.second)
				return false;
		}
	}

	// An `on enter` MissionAction may have defined a LocationFilter that
	// specifies the systems in which it can occur.
	if(!systemFilter.IsEmpty() && !systemFilter.Matches(player.GetSystem()))
		return false;
	return true;
}



void MissionAction::Do(PlayerInfo &player, UI *ui, const System *destination,
	const shared_ptr<Ship> &ship, const bool isUnique) const
{
	bool isOffer = (trigger == "offer");
	if(!conversation->IsEmpty() && ui)
	{
		// Conversations offered while boarding or assisting reference a ship,
		// which may be destroyed depending on the player's choices.
		ConversationPanel *panel = new ConversationPanel(player, *conversation, destination, ship, isOffer);
		if(isOffer)
			panel->SetCallback(&player, &PlayerInfo::MissionCallback);
		// Use a basic callback to handle forced departure outside of `on offer`
		// conversations.
		else
			panel->SetCallback(&player, &PlayerInfo::BasicCallback);
		ui->Push(panel);
	}
	else if(!dialogText.empty() && ui)
	{
		map<string, string> subs;
		GameData::GetTextReplacements().Substitutions(subs, player.Conditions());
		subs["<first>"] = player.FirstName();
		subs["<last>"] = player.LastName();
		if(player.Flagship())
			subs["<ship>"] = player.Flagship()->Name();
		string text = Format::Replace(dialogText, subs);

		// Don't push the dialog text if this is a visit action on a nonunique
		// mission; on visit, nonunique dialogs are handled by PlayerInfo as to
		// avoid the player being spammed by dialogs if they have multiple
		// missions active with the same destination (e.g. in the case of
		// stacking bounty jobs).
		if(isOffer)
			ui->Push(new Dialog(text, player, destination));
		else if(isUnique || trigger != "visit")
			ui->Push(new Dialog(text));
	}
	else if(isOffer && ui)
		player.MissionCallback(Conversation::ACCEPT);

	action.Do(player, ui);
}



// Convert this validated template into a populated action.
MissionAction MissionAction::Instantiate(map<string, string> &subs, const System *origin,
	int jumps, int64_t payload) const
{
	MissionAction result;
	result.trigger = trigger;
	result.system = system;
	// Convert any "distance" specifiers into "near <system>" specifiers.
	result.systemFilter = systemFilter.SetOrigin(origin);

	result.requiredOutfits = requiredOutfits;

	string previousPayment = subs["<payment>"];
	string previousFine = subs["<fine>"];
	result.action = action.Instantiate(subs, jumps, payload);

	// Create any associated dialog text from phrases, or use the directly specified text.
	string dialogText = !dialogPhrase->IsEmpty() ? dialogPhrase->Get() : this->dialogText;
	if(!dialogText.empty())
		result.dialogText = Format::Replace(Phrase::ExpandPhrases(dialogText), subs);

	if(!conversation->IsEmpty())
		result.conversation = ExclusiveItem<Conversation>(conversation->Instantiate(subs, jumps, payload));

	// Restore the "<payment>" and "<fine>" values from the "on complete" condition, for
	// use in other parts of this mission.
	if(result.Payment() && (trigger != "complete" || !previousPayment.empty()))
		subs["<payment>"] = previousPayment;
	if(result.action.Fine() && trigger != "complete")
		subs["<fine>"] = previousFine;

	return result;
}



int64_t MissionAction::Payment() const noexcept
{
	return action.Payment();
}
