/* GameAction.cpp
Copyright (c) 2020 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GameAction.h"

#include "audio/Audio.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "text/Format.h"
#include "GameData.h"
#include "GameEvent.h"
#include "Messages.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "System.h"
#include "UI.h"

#include <cstdlib>

using namespace std;

namespace {
	void DoGift(PlayerInfo &player, const Outfit *outfit, int count, UI *ui)
	{
		// Maps are not transferable; they represent the player's spatial awareness.
		int mapSize = outfit->Get("map");
		if(mapSize > 0)
		{
			bool mapMinables = outfit->Get("map minables");
			if(!player.HasMapped(mapSize, mapMinables))
				player.Map(mapSize, mapMinables);
			Messages::Add(*GameData::Messages().Get("map received"));
			return;
		}

		Ship *flagship = player.Flagship();
		bool isSingle = (abs(count) == 1);
		string nameWas = (isSingle ? outfit->DisplayName() : outfit->PluralName());
		if(!flagship || !count || nameWas.empty())
			return;

		nameWas += (isSingle ? " was" : " were");
		string message;
		if(isSingle)
		{
			char c = tolower(static_cast<unsigned char>(nameWas.front()));
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
		Messages::Add({message, GameData::MessageCategories().Get("normal")});
	}
}



// Construct and Load() at the same time.
GameAction::GameAction(const DataNode &node, const ConditionsStore *playerConditions)
{
	Load(node, playerConditions);
}



void GameAction::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	for(const DataNode &child : node)
		LoadSingle(child, playerConditions);
}



// Load a single child at a time, used for streamlining MissionAction::Load.
void GameAction::LoadSingle(const DataNode &child, const ConditionsStore *playerConditions)
{
	isEmpty = false;

	const string &key = child.Token(0);
	bool hasValue = child.Size() >= 2;

	if(key == "remove" && child.Size() >= 3 && child.Token(1) == "log")
	{
		auto &type = specialLogClear[child.Token(2)];
		if(child.Size() > 3)
			type.push_back(child.Token(3));
	}
	else if(key == "log")
	{
		// Special log format: log <category> <heading> [<log message>|scene <sprite>]
		// Normal log format: log [<log message>|scene <sprite>]
		// Note: the key of `log` or `log <category> <heading>` may be on a line unto itself, with the child nodes
		// distributed beneath it. But this must be distinguished from `log scene <image_name>`.
		// This means that there can never be a special category named 'scene' or there will be problems with the
		// player logbook format.
		if(child.Size() < 3 || (child.Size() == 3 && child.Token(1) == "scene"))
			logEntries.Load(child, 1);
		else if(child.Size() >= 3)
			specialLogEntries[child.Token(1)][child.Token(2)].Load(child, 3);
	}
	else if((key == "give" || key == "take") && child.Size() >= 3 && child.Token(1) == "ship")
	{
		giftShips.emplace_back();
		giftShips.back().Load(child);
	}
	else if(key == "outfit" && hasValue)
	{
		int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
		if(count)
			giftOutfits[GameData::Outfits().Get(child.Token(1))] = count;
		else
			child.PrintTrace("Skipping invalid outfit quantity:");
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
	else if(key == "fine" && hasValue)
	{
		int64_t value = child.Value(1);
		if(value > 0)
			fine += value;
		else
			child.PrintTrace("Skipping invalid \"fine\" with non-positive value:");
	}
	else if(key == "debt" && hasValue)
	{
		GameAction::Debt &debtEntry = debt.emplace_back(max<int64_t>(0, child.Value(1)));
		for(const DataNode &grand : child)
		{
			const string &grandKey = grand.Token(0);
			bool grandHasValue = grand.Size() >= 2;
			if(grandKey == "term" && grandHasValue)
				debtEntry.term = max<int>(1, grand.Value(1));
			else if(grandKey == "interest" && grandHasValue)
				debtEntry.interest = clamp(grand.Value(1), 0., 0.999);
			else
				grand.PrintTrace("Skipping unrecognized \"debt\" attribute:");
		}
	}
	else if(key == "event" && hasValue)
	{
		int minDays = (child.Size() >= 3 ? child.Value(2) : 1);
		int maxDays = (child.Size() >= 4 ? child.Value(3) : minDays);
		if(maxDays < minDays)
			swap(minDays, maxDays);
		events[GameData::Events().Get(child.Token(1))] = make_pair(minDays, maxDays);
	}
	else if(key == "music" && hasValue)
		music = child.Token(1);
	else if(key == "mute")
		music = "";
	else if(key == "mark" && hasValue)
	{
		set<const System *> &toMark = child.Size() == 2 ? mark : markOther[child.Token(2)];
		toMark.insert(GameData::Systems().Get(child.Token(1)));
	}
	else if(key == "unmark" && hasValue)
	{
		set<const System *> &toUnmark = (child.Size() == 2) ? unmark : unmarkOther[child.Token(2)];
		toUnmark.insert(GameData::Systems().Get(child.Token(1)));
	}
	else if(key == "fail" && hasValue)
		fail.insert(child.Token(1));
	else if(key == "fail")
		failCaller = true;
	else if(key == "message")
	{
		if(hasValue)
			messages.push_back(ExclusiveItem<Message>{GameData::Messages().Get(child.Token(1))});
		else
			messages.push_back(ExclusiveItem<Message>{Message{child}});
	}
	else
		conditions.Add(child, playerConditions);
}



void GameAction::Save(DataWriter &out) const
{
	if(!logEntries.IsEmpty())
	{
		out.Write("log");
		logEntries.Save(out);
	}
	for(const auto &[category, headings] : specialLogEntries)
		for(const auto &[heading, specialLogEntry] : headings)
		{
			if(!specialLogEntry.IsEmpty())
			{
				out.Write("log", category, heading);
				specialLogEntry.Save(out);
			}
		}
	for(auto &&it : specialLogClear)
	{
		if(it.second.empty())
			out.Write("remove", "log", it.first);
		else
			for(auto &&jt : it.second)
				out.Write("remove", "log", it.first, jt);
	}
	for(auto &&it : giftShips)
		it.Save(out);
	for(auto &&it : giftOutfits)
		out.Write("outfit", it.first->TrueName(), it.second);
	if(payment)
		out.Write("payment", payment);
	if(fine)
		out.Write("fine", fine);
	for(auto &&debtEntry : debt)
	{
		out.Write("debt", debtEntry.amount);
		out.BeginChild();
		{
			if(debtEntry.interest)
				out.Write("interest", *debtEntry.interest);
			out.Write("term", debtEntry.term);
		}
		out.EndChild();
	}
	for(auto &&it : events)
		out.Write("event", it.first->TrueName(), it.second.first, it.second.second);
	for(const System *system : mark)
		out.Write("mark", system->TrueName());
	for(const auto &[mission, marks] : markOther)
		for(const System *system : marks)
			out.Write("mark", system->TrueName(), mission);
	for(const System *system : unmark)
		out.Write("unmark", system->TrueName());
	for(const auto &[mission, unmarks] : unmarkOther)
		for(const System *system : unmarks)
			out.Write("unmark", system->TrueName(), mission);
	for(const string &name : fail)
		out.Write("fail", name);
	if(failCaller)
		out.Write("fail");
	if(music.has_value())
	{
		if(music->empty())
			out.Write("mute");
		else
			out.Write("music", music.value());
	}
	for(const auto &msg : messages)
		msg->Save(out);

	conditions.Save(out);
}



// Check this template or instantiated GameAction to see if any used content
// is not fully defined (e.g. plugin removal, typos in names, etc.).
string GameAction::Validate() const
{
	// Events which get activated by this action must be valid.
	for(auto &&event : events)
	{
		string reason = event.first->IsValid();
		if(!reason.empty())
			return "event \"" + event.first->TrueName() + "\" - Reason: " + reason;
	}

	// Transferred content must be defined & valid.
	for(auto &&it : giftShips)
		if(!it.ShipModel()->IsValid())
			return "gift ship model \"" + it.ShipModel()->VariantName() + "\"";
	for(auto &&outfit : giftOutfits)
		if(!outfit.first->IsDefined())
			return "gift outfit \"" + outfit.first->TrueName() + "\"";

	// Marked and unmarked system must be valid.
	for(auto &&system : mark)
		if(!system->IsValid())
			return "system \"" + system->TrueName() + "\"";
	for(const auto &[mission, marks] : markOther)
		for(const System *system : marks)
			if(!system->IsValid())
				return "system \"" + system->TrueName() + "\"";
	for(auto &&system : unmark)
		if(!system->IsValid())
			return "system \"" + system->TrueName() + "\"";
	for(const auto &[mission, unmarks] : unmarkOther)
		for(const System *system : unmarks)
			if(!system->IsValid())
				return "system \"" + system->TrueName() + "\"";

	// It is OK for this action to try to fail a mission that does not exist.
	// (E.g. a plugin may be designed for interoperability with other plugins.)

	return "";
}



bool GameAction::IsEmpty() const noexcept
{
	return isEmpty;
}



int64_t GameAction::Payment() const noexcept
{
	return payment;
}



int64_t GameAction::Fine() const noexcept
{
	return fine;
}



const map<const Outfit *, int> &GameAction::Outfits() const noexcept
{
	return giftOutfits;
}



const vector<ShipManager> &GameAction::Ships() const noexcept
{
	return giftShips;
}



// Perform the specified tasks.
void GameAction::Do(PlayerInfo &player, UI *ui, const Mission *caller) const
{
	if(!logEntries.IsEmpty())
		player.AddLogEntry(logEntries);
	for(const auto &[category, nextMap] : specialLogEntries)
		for(const auto &[heading, specialLogEntry] : nextMap)
			player.AddSpecialLog(category, heading, specialLogEntry);
	for(auto &&it : specialLogClear)
	{
		if(it.second.empty())
			player.RemoveSpecialLog(it.first);
		else
			for(auto &&jt : it.second)
				player.RemoveSpecialLog(it.first, jt);
	}

	// If multiple outfits, ships are being transferred, first remove the ships,
	// then the outfits, before adding any new ones.
	for(auto &&it : giftShips)
		if(!it.Giving())
			it.Do(player);
	for(auto &&it : giftOutfits)
		if(it.second < 0)
			DoGift(player, it.first, it.second, ui);
	for(auto &&it : giftOutfits)
		if(it.second > 0)
			DoGift(player, it.first, it.second, ui);
	for(auto &&it : giftShips)
		if(it.Giving())
			it.Do(player);

	if(payment)
	{
		// Conversation actions don't block a mission from offering if a
		// negative payment would drop the player's account balance below
		// zero, so negative payments need to be handled.
		int64_t account = player.Accounts().Credits();
		// If the payment is negative and the player doesn't have enough
		// in their account, then the player's credits are reduced to 0.
		if(account + payment >= 0)
			player.Accounts().AddCredits(payment);
		else if(account > 0)
			player.Accounts().AddCredits(-account);
		// If a MissionAction has a negative payment that can't be met
		// then this action won't offer, so MissionAction payment behavior
		// is unchanged.
	}
	if(fine)
		player.Accounts().AddFine(fine);
	for(const auto &debtEntry : debt)
		player.Accounts().AddDebt(debtEntry.amount, debtEntry.interest, debtEntry.term);

	for(const auto &it : events)
		player.AddEvent(*it.first, player.GetDate() + it.second.first);

	if(caller)
	{
		caller->Mark(mark);
		caller->Unmark(unmark);
	}

	if(!fail.empty() || !markOther.empty() || !unmarkOther.empty())
	{
		for(const Mission &mission : player.Missions())
		{
			// If this action causes another mission to fail, mark that
			// mission as failed. It will not be removed from the player's mission
			// list until it is safe to do so.
			if(fail.contains(mission.TrueName()))
				player.FailMission(mission);

			auto mit = markOther.find(mission.TrueName());
			if(mit != markOther.end())
				mission.Mark(mit->second);

			auto uit = unmarkOther.find(mission.TrueName());
			if(uit != unmarkOther.end())
				mission.Unmark(uit->second);
		}
	}

	// If this action causes this mission to fail, mark it as failed.
	// It will not be removed from the player's mission list until it is safe to do so.
	if(failCaller && caller)
		player.FailMission(*caller);
	if(music.has_value())
	{
		if(*music == "<ambient>")
		{
			if(player.GetPlanet())
				Audio::PlayMusic(player.GetPlanet()->MusicName());
			else
				Audio::PlayMusic(player.GetSystem()->MusicName());
		}
		else
			Audio::PlayMusic(music.value());
	}

	for(const auto &msg : messages)
		Messages::Add(*msg);

	// Check if applying the conditions changes the player's reputations.
	conditions.Apply();
}



GameAction GameAction::Instantiate(map<string, string> &subs, int jumps, int payload) const
{
	GameAction result;
	result.isEmpty = isEmpty;

	for(auto &&it : events)
	{
		// Allow randomization of event times. The second value in the pair is
		// always greater than or equal to the first, so Random::Int() will
		// never be called with a value less than 1.
		int day = it.second.first + Random::Int(it.second.second - it.second.first + 1);
		result.events[it.first] = make_pair(day, day);
	}

	for(auto &&it : giftShips)
		result.giftShips.push_back(it.Instantiate(subs));
	result.giftOutfits = giftOutfits;

	result.music = music;

	result.payment = payment + (jumps + 1) * payload * paymentMultiplier;
	if(result.payment)
		subs["<payment>"] = Format::CreditString(abs(result.payment));

	result.fine = fine;
	if(result.fine)
		subs["<fine>"] = Format::CreditString(result.fine);

	result.debt = debt;

	result.logEntries = logEntries.Instantiate(subs);
	for(const auto &[category, headings] : specialLogEntries)
		for(const auto &[heading, specialLogEntry] : headings)
			result.specialLogEntries[category][heading] = specialLogEntry.Instantiate(subs);
	result.specialLogClear = specialLogClear;

	result.fail = fail;
	result.failCaller = failCaller;

	for(const auto &it : messages)
	{
		if(it->IsPhrase())
			result.messages.push_back(it);
		else
			result.messages.push_back(ExclusiveItem<Message>{{it->Text(subs), it->GetCategory()}});
	}

	result.conditions = conditions;

	result.mark = mark;
	result.markOther = markOther;
	result.unmark = unmark;
	result.unmarkOther = unmarkOther;

	return result;
}
