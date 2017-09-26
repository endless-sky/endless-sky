/* MissionAction.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MissionAction.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Messages.h"
#include "Minable.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Sale.h"
#include "Ship.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <cmath>
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
		int cargoCount = player.Cargo().Get(outfit);
		if(count < 0 && cargoCount)
		{
			int moved = min(cargoCount, -count);
			count += moved;
			player.Cargo().Remove(outfit, moved);
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
			int size = player.Cargo().Size();
			player.Cargo().SetSize(-1);
			player.Cargo().Add(outfit, count);
			player.Cargo().SetSize(size);
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
	
	// Get every outfit that can be looted from minables.
	const set<const Outfit *> MinableOutfits()
	{
		set<const Outfit *> minedOutfits;
		for(const auto &minable : GameData::Minables())
			for(const auto &outfit : minable.second.Payload())
				minedOutfits.emplace(outfit.first);
		
		return minedOutfits;
	}
	
	const Outfit *BiasedRandomChoice(const set<const Outfit *> &choices, const vector<int> &weights, int total)
	{
		if(total > 0 && !choices.empty())
		{
			// Pick a random outfit based on the supplied weights.
			int r = Random::Int(total);
			auto choice = choices.cbegin();
			for(size_t i = 0; i < weights.size(); ++i)
			{
				r -= weights[i];
				if(r < 0)
					return *choice;
				++choice;
			}
		}
		return nullptr;
	}
	
	// Pick a random minable outfit that would make sense to be exported from the first system
	// to the second. This function only returns nullptr if there are no matching minable outfits.
	const Outfit *PickMinableOutfit(const System *from, const System *to)
	{
		const set<const Outfit *> minableOutfits = MinableOutfits();
		bool useWeights = (from != to);
		vector<int> weight;
		int total = 0;
		for(const Outfit *outfit : minableOutfits)
		{
			// Strongly prefer minables in "from" or its neighbors, to reduce the
			// chance of picking outfits that are not regionally available.
			int fromCount = 3 * from->MinableCount(outfit) + 1;
			int toCount = to->MinableCount(outfit);
			for(const System *linked : from->Links())
				fromCount *= 1 + linked->MinableCount(outfit);
			if(useWeights)
				for(const System *linked : to->Links())
					toCount += linked->MinableCount(outfit);
			
			weight.emplace_back(max(1, useWeights ? fromCount * 3 - toCount : fromCount));
			total += weight.back();
		}
		return BiasedRandomChoice(minableOutfits, weight, total);
	}
	
	// Pick a random outfit that's sold near the destination planet, or the
	// "from" system. Returns nullptr if "from", the destination planet, and
	// any of their linked systems have no outfitters.
	const Outfit *PickOutfit(const System *from, const Planet *destination)
	{
		const Sale<Outfit> &remote = destination->Outfitter();
		Sale<Outfit> local = remote;
		for(const StellarObject &object : from->Objects())
			if(object.GetPlanet() && object.GetPlanet() != destination)
				local.Add(object.GetPlanet()->Outfitter());
		
		// If neither the origin or destination has outfits for sale,
		// consider the outfits sold in each's linked systems.
		if(local.empty())
			for(const System *system : {from, destination->GetSystem()})
				for(const System *linked : system->Links())
					for(const StellarObject &object : linked->Objects())
						if(object.GetPlanet() && !object.GetPlanet()->Outfitter().empty())
							local.Add(object.GetPlanet()->Outfitter());
		
		if(!local.empty())
		{
			vector<int> weight;
			int total = 0;
			// Prefer outfits not available at the destination. High-price
			// outfits are more strongly considered.
			for(const Outfit *outfit : local)
			{
				weight.emplace_back(pow(2., 2 * (1 - remote.Has(outfit))) * outfit->Cost() * .01 + 1);
				total += weight.back();
			}
			return BiasedRandomChoice(local, weight, total);
		}
		return nullptr;
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
		
		if(key == "log" || key == "dialog")
		{
			bool isSpecial = (key == "log" && child.Size() >= 3);
			string &text = (key == "dialog" ? dialogText :
				isSpecial ? specialLogText[child.Token(1)][child.Token(2)] : logText);
			for(int i = isSpecial ? 3 : 1; i < child.Size(); ++i)
			{
				if(!text.empty())
					text += "\n\t";
				text += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!text.empty())
						text += "\n\t";
					text += grand.Token(i);
				}
		}
		else if(key == "conversation" && child.HasChildren())
			conversation.Load(child);
		else if(key == "conversation" && hasValue)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else if(key == "outfit" && hasValue)
		{
			const string &value = child.Token(1);
			int count = (child.Size() < 3) ? 1 : static_cast<int>(child.Value(2));
			// Is this a known outfit, or one to be picked?
			if(value == "random outfit" || value == "random minable")
			{
				// Each type of random request can only be used once in a given action.
				randomGifts[value] = count;
				if(child.Size() >= 4)
					randomGiftsLimit[value] = child.Value(3);
				if(child.Size() >= 5)
					randomGiftsProb[value] = child.Value(4);
			}
			else
			{
				// This is a specific outfit, possibly needing a random quantity.
				const Outfit *gift = GameData::Outfits().Get(value);
				gifts[gift] = count;
				if(child.Size() >= 4)
					giftLimit[gift] = child.Value(3);
				if(child.Size() >= 5)
					giftProb[gift] = child.Value(4);
			}
		}
		else if(key == "require" && hasValue)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			requiredOutfits[GameData::Outfits().Get(child.Token(1))] = count;
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
		
		for(const auto &it : gifts)
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
		for(const auto &name : fail)
			out.Write("fail", name);
		
		conditions.Save(out);
	}
	out.EndChild();
}



int MissionAction::Payment() const
{
	return payment;
}



// Check if this action can be completed right now. It cannot be completed
// if it takes away money or outfits that the player does not have.
bool MissionAction::CanBeDone(const PlayerInfo &player) const
{
	if(player.Accounts().Credits() < -payment)
		return false;
	
	const Ship *flagship = player.Flagship();
	for(const auto &it : gifts)
	{
		if(it.second > 0)
			continue;
		
		// The outfit can be taken from the player's cargo or from the flagship.
		// If the player is landed, all available cargo is transferred from the
		// player's ships to the player. If checking mission completion status
		// in-flight, cargo is present in the player's ships.
		int available = player.Cargo().Get(it.first);
		if(!player.GetPlanet())
			for(const auto &ship : player.Ships())
				if(ship->GetSystem() == player.GetSystem() && !ship->IsDisabled())
					available += ship->Cargo().Get(it.first);
		if(flagship)
			available += flagship->OutfitCount(it.first);
		
		// If the gift "count" is 0, that means to check that the player has at
		// least one of these items. This is for backward compatibility before
		// requiredOutfits was introduced.
		if(available < -it.second + !it.second)
			return false;
	}
	
	for(const auto &it : requiredOutfits)
	{
		// The required outfit can be in the player's cargo or from the flagship.
		int available = player.Cargo().Get(it.first);
		for(const auto &ship : player.Ships())
			available += ship->Cargo().Get(it.first);
		if(flagship)
			available += flagship->OutfitCount(it.first);
		
		if(available < it.second)
			return false;
		
		// If the required count is 0, the player must not have any of the outfit.
		if(it.second == 0 && available > 0)
			return false;
	}
	
	// An `on enter` MissionAction may have defined a LocationFilter that
	// specifies the systems in which it can occur.
	if(!systemFilter.IsEmpty() && !systemFilter.Matches(player.GetSystem()))
		return false;
	return true;
}



void MissionAction::Do(PlayerInfo &player, UI *ui, const System *destination) const
{
	bool isOffer = (trigger == "offer");
	if(!conversation.IsEmpty() && ui)
	{
		ConversationPanel *panel = new ConversationPanel(player, conversation, destination);
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
		
		if(isOffer)
			ui->Push(new Dialog(text, player, destination));
		else
			ui->Push(new Dialog(text));
	}
	else if(isOffer && ui)
		player.MissionCallback(Conversation::ACCEPT);
	
	if(!logText.empty())
		player.AddLogEntry(logText);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			player.AddSpecialLog(it.first, eit.first, eit.second);
	
	// If multiple outfits are being transferred, first remove them before
	// adding any new ones.
	for(const auto &it : gifts)
		if(it.second < 0)
			DoGift(player, it.first, it.second, ui);
	for(const auto &it : gifts)
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



MissionAction MissionAction::Instantiate(map<string, string> &subs, const System *origin, int jumps, int payload, const Planet *destination) const
{
	MissionAction result;
	result.trigger = trigger;
	result.system = system;
	// Convert any "distance" specifiers into "near <system>" specifiers.
	result.systemFilter = systemFilter.SetOrigin(origin);
	
	for(const auto &it : events)
	{
		// Allow randomization of event times. The second value in the pair is
		// always greater than or equal to the first, so Random::Int() will
		// never be called with a value less than 1.
		int day = it.second.first + Random::Int(it.second.second - it.second.first + 1);
		result.events[it.first] = make_pair(day, day);
	}
	
	result.gifts = gifts;
	result.giftProb = giftProb;
	result.giftLimit = giftLimit;
	// Fulfill any outfit requests (e.g. pick the "random minable").
	for(const pair<string, int> &request : randomGifts)
	{
		const Outfit *gift = nullptr;
		if(request.first == "random minable")
			gift = PickMinableOutfit(origin, destination->GetSystem());
		else if(request.first == "random outfit")
			gift = PickOutfit(origin, destination);
		
		if(gift && !gift->Name().empty())
		{
			// The transfer quantity may also be randomly picked.
			const auto &probIt = randomGiftsProb.find(request.first);
			if(probIt != randomGiftsProb.end())
				result.giftProb.emplace(gift, probIt->second);
			
			const auto &limitIt = randomGiftsLimit.find(request.first);
			if(limitIt != randomGiftsLimit.end())
				result.giftLimit.emplace(gift, limitIt->second);
			
			result.gifts[gift] += request.second;
		}
	}
	// Select a random number of each gift, if requested.
	if(!result.giftProb.empty() || !result.giftLimit.empty())
		for(const pair<const Outfit *, int> &gift : result.gifts)
		{
			int &amount = result.gifts[gift.first];
			const auto &probIt = result.giftProb.find(gift.first);
			const auto &limitIt = result.giftLimit.find(gift.first);
			// Use positive values during quantity generation.
			int multiplier = gift.second < 0 ? -1 : 1;
			if(probIt != result.giftProb.end())
				amount = Random::Polya(abs(limitIt->second), fabs(probIt->second)) + abs(gift.second);
			else if(limitIt != result.giftLimit.end())
				amount = abs(gift.second) + Random::Int(abs(limitIt->second) - abs(gift.second) + 1);
			else
			{
				// This gift's transfer amount is unchanged.
				continue;
			}
			// Preserve the original transfer direction.
			amount *= multiplier;
		}
	
	result.payment = payment + (jumps + 1) * payload * paymentMultiplier;
	// Fill in the payment amount if this is the "complete" action.
	string previousPayment = subs["<payment>"];
	if(result.payment)
		subs["<payment>"] = Format::Number(abs(result.payment))
			+ (result.payment == 1 ? " credit" : " credits");
	
	if(!logText.empty())
		result.logText = Format::Replace(logText, subs);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			result.specialLogText[it.first][eit.first] = Format::Replace(eit.second, subs);
	
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
