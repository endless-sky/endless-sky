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
#include "text/Format.h"
#include "GameData.h"
#include "GameEvent.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "UI.h"

#include <cstdlib>

using namespace std;

namespace {
	void DoGift(PlayerInfo &player, const Ship *model, const string &name)
	{
		if(model->ModelName().empty())
			return;
		
		player.BuyShip(model, name, true);
		Messages::Add("The " + model->ModelName() + " \"" + name + "\" was added to your fleet.");
	}
	
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
		
		if(key == "log")
		{
			bool isSpecial = (child.Size() >= 3);
			string &text = (isSpecial ?
				specialLogText[child.Token(1)][child.Token(2)] : logText);
			Dialog::ParseTextNode(child, isSpecial ? 3 : 1, text);
		}
		else if(key == "dialog")
		{
			// Dialog text may be supplied from a stock named phrase, a
			// private unnamed phrase, or directly specified.
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
			conversation.Load(child);
		else if(key == "conversation" && hasValue)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else if(key == "give" && hasValue)
		{
			if(child.Token(1) == "ship" && child.Size() >= 3)
				giftShips.emplace_back(GameData::Ships().Get(child.Token(2)), child.Size() >= 4 ? child.Token(3) : "");
			else
				child.PrintTrace("Skipping unsupported \"give\" syntax:");
		}
		else if(key == "outfit" && hasValue)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			if(count)
				giftOutfits[GameData::Outfits().Get(child.Token(1))] = count;
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
		else if(key == "payment")
		{
			if(child.Size() == 1)
				paymentMultiplier += 150;
			if(child.Size() >= 2)
				payment += child.Value(1);
			if(child.Size() >= 3)
				paymentMultiplier += child.Value(2);
		}
		else if(key == "event" && hasValue)
		{
			int minDays = (child.Size() >= 3 ? child.Value(2) : 0);
			int maxDays = (child.Size() >= 4 ? child.Value(3) : minDays);
			if(maxDays < minDays)
				swap(minDays, maxDays);
			events[GameData::Events().Get(child.Token(1))] = make_pair(minDays, maxDays);
		}
		else if(key == "fail")
		{
			string toFail = child.Size() >= 2 ? child.Token(1) : missionName;
			fail.insert(toFail);
			// Create a GameData reference to this mission name.
			GameData::Missions().Get(toFail);
		}
		else if(key == "system")
		{
			if(system.empty() && child.HasChildren())
				systemFilter.Load(child);
			else
				child.PrintTrace("Unsupported use of \"system\" LocationFilter:");
		}
		else
			conditions.Add(child);
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
		if(!logText.empty())
		{
			out.Write("log");
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(logText, "\n\t"))
					out.Write(line);
			}
			out.EndChild();
		}
		for(const auto &it : specialLogText)
			for(const auto &eit : it.second)
			{
				out.Write("log", it.first, eit.first);
				out.BeginChild();
				{
					// Break the text up into paragraphs.
					for(const string &line : Format::Split(eit.second, "\n\t"))
						out.Write(line);
				}
				out.EndChild();
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
		
		for(const auto &it : giftShips)
			out.Write("give", "ship", it.first->VariantName(), it.second);
		for(const auto &it : giftOutfits)
			out.Write("outfit", it.first->Name(), it.second);
		for(const auto &it : requiredOutfits)
			out.Write("require", it.first->Name(), it.second);
		if(payment)
			out.Write("payment", payment);
		for(const auto &it : events)
		{
			if(it.second.first == it.second.second)
				out.Write("event", it.first->Name(), it.second.first);
			else
				out.Write("event", it.first->Name(), it.second.first, it.second.second);
		}
		for(auto &&missionName : fail)
			out.Write("fail", missionName);
		
		conditions.Save(out);
	}
	out.EndChild();
}



// Check this template or instantiated MissionAction to see if any used content
// is not fully defined (e.g. plugin removal, typos in names, etc.).
bool MissionAction::IsValid() const
{
	// Any filter used to control where this action triggers must be valid.
	if(!systemFilter.IsValid())
		return false;
	
	// Stock phrases that generate text must be defined.
	if(stockDialogPhrase && stockDialogPhrase->Name().empty())
		return false;
	
	// Stock conversations must be defined.
	if(stockConversation && stockConversation->IsEmpty())
		return false;
	
	// Events which get activated by this action must be valid.
	for(auto &&event : events)
		if(!event.first->IsValid())
			return false;

	// Gifted or required content must be defined & valid.
	for(auto &&it : giftShips)
		if(!it.first->IsValid())
			return false;
	for(auto &&outfit : giftOutfits)
		if(!outfit.first->IsDefined())
			return false;
	for(auto &&outfit : requiredOutfits)
		if(!outfit.first->IsDefined())
			return false;
	
	// It is OK for this action to try to fail a mission that does not exist.
	// (E.g. a plugin may be designed for interoperability with other plugins.)
	
	return true;
}



int MissionAction::Payment() const
{
	return payment;
}



const string &MissionAction::DialogText() const
{
	return dialogText;
}



// Check if this action can be completed right now. It cannot be completed
// if it takes away money or outfits that the player does not have.
bool MissionAction::CanBeDone(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	if(player.Accounts().Credits() < -payment)
		return false;
	
	const Ship *flagship = player.Flagship();
	for(const auto &it : giftOutfits)
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
	
	if(!logText.empty())
		player.AddLogEntry(logText);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			player.AddSpecialLog(it.first, eit.first, eit.second);
	
	for(const auto &it : giftShips)
		DoGift(player, it.first, it.second);
	// If multiple outfits are being transferred, first remove them before
	// adding any new ones.
	for(const auto &it : giftOutfits)
		if(it.second < 0)
			DoGift(player, it.first, it.second, ui);
	for(const auto &it : giftOutfits)
		if(it.second > 0)
			DoGift(player, it.first, it.second, ui);
	
	if(payment)
		player.Accounts().AddCredits(payment);
	
	for(const auto &it : events)
		player.AddEvent(*it.first, player.GetDate() + it.second.first);
	
	if(!fail.empty())
	{
		// If this action causes this or any other mission to fail, mark that
		// mission as failed. It will not be removed from the player's mission
		// list until it is safe to do so.
		for(const Mission &mission : player.Missions())
			if(fail.count(mission.Identifier()))
				player.FailMission(mission);
	}
	
	// Check if applying the conditions changes the player's reputations.
	player.SetReputationConditions();
	conditions.Apply(player.Conditions());
	player.CheckReputationConditions();
}



// Convert this validated template into a populated action.
MissionAction MissionAction::Instantiate(map<string, string> &subs, const System *origin, int jumps, int64_t payload) const
{
	MissionAction result;
	result.trigger = trigger;
	result.system = system;
	// Convert any "distance" specifiers into "near <system>" specifiers.
	result.systemFilter = systemFilter.SetOrigin(origin);
	
	// All contained events are valid, else we would not be calling Instantiate. For these
	// valid events, pick a date within the specified range on which the event will occur.
	for(const auto &it : events)
	{
		int day = it.second.first + Random::Int(it.second.second - it.second.first + 1);
		result.events[it.first] = make_pair(day, day);
	}
	for(const auto &it : giftShips)
		result.giftShips.emplace_back(it.first, !it.second.empty() ? it.second : GameData::Phrases().Get("civilian")->Get());
	result.giftOutfits = giftOutfits;
	result.requiredOutfits = requiredOutfits;
	result.payment = payment + (jumps + 1) * payload * paymentMultiplier;
	// Fill in the payment amount if this is the "complete" action.
	string previousPayment = subs["<payment>"];
	if(result.payment)
		subs["<payment>"] = Format::Credits(abs(result.payment))
			+ (result.payment == 1 ? " credit" : " credits");
	
	if(!logText.empty())
		result.logText = Format::Replace(logText, subs);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			result.specialLogText[it.first][eit.first] = Format::Replace(eit.second, subs);
	
	// Create any associated dialog text from phrases, or use the directly specified text.
	string dialogText = stockDialogPhrase ? stockDialogPhrase->Get()
		: (!dialogPhrase.Name().empty() ? dialogPhrase.Get()
		: this->dialogText);
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Substitute(subs);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Substitute(subs);
	
	result.fail = fail;
	
	result.conditions = conditions;
	
	// Restore the "<payment>" value from the "on complete" condition, for use
	// in other parts of this mission.
	if(result.payment && trigger != "complete")
		subs["<payment>"] = previousPayment;
	
	return result;
}
