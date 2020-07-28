/* MissionAction.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MissionAction.h"

#include "CargoHold.h"
#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "GameEvent.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "UI.h"

#include <cstdlib>
#include <vector>

using namespace std;

namespace {
	void DoGift(PlayerInfo &player, const Outfit *outfit, int count, UI *ui)
	{
		Ship *flagship = player.Flagship();
		bool isSingle = (abs(count) == 1);
		string nameWas = (isSingle ? outfit->Name() : outfit->PluralName());
		if(!flagship || !count || nameWas.empty())
			return;
		
		nameWas += (isSingle ? " was" : " were");
		string message;
		if(isSingle)
		{
			char c = tolower(nameWas.front());
			bool isVowel = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
			message = (isVowel ? "An " : "A ");
		}
		else
			message = to_string(abs(count)) + " ";
		
		message += nameWas;
		if(count > 0)
			message += " added to your ";
		else
			message += " removed from your ";
		
		bool didCargo = false;
		bool didShip = false;
		// If not landed, transfers must be done into the flagship's CargoHold.
		CargoHold &cargo = (player.GetPlanet() ? player.Cargo() : flagship->Cargo());
		int cargoCount = cargo.Get(outfit);
		if(count < 0 && cargoCount)
		{
			int moved = min(cargoCount, -count);
			count += moved;
			cargo.Remove(outfit, moved);
			didCargo = true;
		}
		while(count)
		{
			int moved = (count > 0) ? 1 : -1;
			if(flagship->Attributes().CanAdd(*outfit, moved))
			{
				flagship->AddOutfit(outfit, moved);
				didShip = true;
			}
			else
				break;
			count -= moved;
		}
		if(count > 0)
		{
			// Ignore cargo size limits.
			int size = cargo.Size();
			cargo.SetSize(-1);
			cargo.Add(outfit, count);
			cargo.SetSize(size);
			didCargo = true;
			if(ui)
			{
				string special = "The " + nameWas;
				special += " put in your cargo hold because there is not enough space to install ";
				special += (isSingle ? "it" : "them");
				special += " in your ship.";
				ui->Push(new Dialog(special));
			}
		}
		if(didCargo && didShip)
			message += "cargo hold and your flagship.";
		else if(didCargo)
			message += "cargo hold.";
		else
			message += "flagship.";
		Messages::Add(message);
	}
	
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
					stockDialogPhrase = GameData::Phrases().Get(child.Token(2));
				else
					child.PrintTrace("Skipping unsupported dialog phrase syntax:");
			}
			else if(!hasValue && child.HasChildren() && (*child.begin()).Token(0) == "phrase")
			{
				const DataNode &firstGrand = (*child.begin());
				if(firstGrand.Size() == 1 && firstGrand.HasChildren())
					dialogPhrase.Load(firstGrand);
				else
					firstGrand.PrintTrace("Skipping unsupported dialog phrase syntax:");
			}
			else
				Dialog::ParseTextNode(child, 1, dialogText);
		}
		else if(key == "conversation" && child.HasChildren())
		{
			conversation.Load(child, missionName);
		}
		else if(key == "conversation" && hasValue)
		{
			stockConversation = GameData::Conversations().Get(child.Token(1));
		}
		else if(key == "outfit" && hasValue)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			if(count)
				gifts[GameData::Outfits().Get(child.Token(1))] = count;
			else
			{
				// outfit <outfit> 0 means the player must have this outfit.
				child.PrintTrace("Warning: deprecated use of \"outfit\" with count of 0. Use \"require <outfit>\" instead:");
				requiredOutfits[GameData::Outfits().Get(child.Token(1))] = 1;
			}
		}
		else if(key == "require" && hasValue)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			if(count >= 0)
				requiredOutfits[GameData::Outfits().Get(child.Token(1))] = count;
			else
				child.PrintTrace("Skipping invalid \"require\" amount:");
		}
		else if(key == "system")
		{
			if(system.empty() && child.HasChildren())
				systemFilter.Load(child);
			else
				child.PrintTrace("Unsupported use of \"system\" LocationFilter:");
		}
		else
			actions.LoadSingle(child, missionName);
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
		if(!conversation.IsEmpty())
			conversation.Save(out);
		
		for(const auto &it : gifts)
			out.Write("outfit", it.first->Name(), it.second);
		for(const auto &it : requiredOutfits)
			out.Write("require", it.first->Name(), it.second);
		
		actions.Save(out);
	}
	out.EndChild();
}



int MissionAction::Payment() const
{
	return actions.Payment();
}



const string &MissionAction::DialogText() const
{
	return dialogText;
}



// Check if this action can be completed right now. It cannot be completed
// if it takes away money or outfits that the player does not have.
bool MissionAction::CanBeDone(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	if(player.Accounts().Credits() < -actions.Payment())
		return false;
	
	const Ship *flagship = player.Flagship();
	for(const auto &it : gifts)
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
	
	for(const auto &it : requiredOutfits)
	{
		int available = 0;
		// Requiring the player to have 0 of this outfit means all ships and all cargo holds
		// must be checked, even if the ship is disabled, parked, or out-of-system.
		bool checkAll = !it.second;
		if(checkAll)
		{
			for(const auto &ship : player.Ships())
				if(!ship->IsDestroyed())
				{
					available += ship->Cargo().Get(it.first);
					available += ship->OutfitCount(it.first);
				}
		}
		else
		{
			// Required outfits must be present on able ships in the
			// player's location (or the respective cargo hold).
			available += flagship ? flagship->OutfitCount(it.first) : 0;
			available += boardingShip ? flagship->Cargo().Get(it.first)
					: CountInCargo(it.first, player);
		}
		
		if(available < it.second)
			return false;
		
		// If the required count is 0, the player must not have any of the outfit.
		if(checkAll && available)
			return false;
	}
	
	// An `on enter` MissionAction may have defined a LocationFilter that
	// specifies the systems in which it can occur.
	if(!systemFilter.IsEmpty() && !systemFilter.Matches(player.GetSystem()))
		return false;
	return true;
}



void MissionAction::Do(PlayerInfo &player, UI *ui, const System *destination, const shared_ptr<Ship> &ship, const bool isUnique) const
{
	bool isOffer = (trigger == "offer");
	if(!conversation.IsEmpty() && ui)
	{
		// Conversations offered while boarding or assisting reference a ship,
		// which may be destroyed depending on the player's choices.
		ConversationPanel *panel = new ConversationPanel(player, conversation, destination, ship);
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
	
	// If multiple outfits are being transferred, first remove them before
	// adding any new ones.
	for(const auto &it : gifts)
		if(it.second < 0)
			DoGift(player, it.first, it.second, ui);
	for(const auto &it : gifts)
		if(it.second > 0)
			DoGift(player, it.first, it.second, ui);
	
	actions.Do(player);
}



MissionAction MissionAction::Instantiate(map<string, string> &subs, const System *origin, int jumps, int payload) const
{
	MissionAction result;
	result.trigger = trigger;
	result.system = system;
	// Convert any "distance" specifiers into "near <system>" specifiers.
	result.systemFilter = systemFilter.SetOrigin(origin);
	
	result.gifts = gifts;
	result.requiredOutfits = requiredOutfits;
	
	string previousPayment = subs["<payment>"];
	result.actions = actions.Instantiate(subs, jumps, payload);
	
	// Create any associated dialog text from phrases, or use the directly specified text.
	string dialogText = stockDialogPhrase ? stockDialogPhrase->Get()
		: (!dialogPhrase.Name().empty() ? dialogPhrase.Get()
		: this->dialogText);
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Instantiate(subs, jumps, payload);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Instantiate(subs, jumps, payload);
	
	// Restore the "<payment>" value from the "on complete" condition, for use
	// in other parts of this mission.
	if(result.Payment() && trigger != "complete")
		subs["<payment>"] = previousPayment;
	
	return result;
}
