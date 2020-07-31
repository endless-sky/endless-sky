/* GameAction.cpp
Copyright (c) 2020 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameAction.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "GameEvent.h"
#include "PlayerInfo.h"
#include "Random.h"

#include <cstdlib>
#include <vector>

using namespace std;



// Construct and Load() at the same time.
GameAction::GameAction(const DataNode &node, const string &missionName)
{
	Load(node, missionName);
}



void GameAction::Load(const DataNode &node, const string &missionName)
{
	for(const DataNode &child : node)
		LoadSingle(child, missionName);
}



// Load a single child at a time, used for streamlining MissionAction::Load.
void GameAction::LoadSingle(const DataNode &child, const string &missionName)
{
	empty = false;
	
	const string &key = child.Token(0);
	bool hasValue = (child.Size() >= 2);
	
	if(key == "log")
	{
		bool isSpecial = (child.Size() >= 3);
		string &text = (isSpecial ?
			specialLogText[child.Token(1)][child.Token(2)] : logText);
		Dialog::ParseTextNode(child, isSpecial ? 3 : 1, text);
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
	else
		conditions.Add(child);
}



void GameAction::Save(DataWriter &out) const
{
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



bool GameAction::IsEmpty() const
{
	return empty;
}



int GameAction::Payment() const
{
	return payment;
}



// Do the actions of the GameAction.
void GameAction::Do(PlayerInfo &player) const
{
	if(!logText.empty())
		player.AddLogEntry(logText);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			player.AddSpecialLog(it.first, eit.first, eit.second);
	
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
	}
	
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



GameAction GameAction::Instantiate(map<string, string> &subs, int jumps, int payload) const
{
	GameAction result;
	
	result.empty = empty;
	
	for(const auto &it : events)
	{
		// Allow randomization of event times. The second value in the pair is
		// always greater than or equal to the first, so Random::Int() will
		// never be called with a value less than 1.
		int day = it.second.first + Random::Int(it.second.second - it.second.first + 1);
		result.events[it.first] = make_pair(day, day);
	}
	result.payment = payment + (jumps + 1) * payload * paymentMultiplier;
	// Fill in the payment amount if this is the "complete" action.
	if(result.payment)
		subs["<payment>"] = Format::Credits(abs(result.payment))
			+ (result.payment == 1 ? " credit" : " credits");
	
	if(!logText.empty())
		result.logText = Format::Replace(logText, subs);
	for(const auto &it : specialLogText)
		for(const auto &eit : it.second)
			result.specialLogText[it.first][eit.first] = Format::Replace(eit.second, subs);
	
	result.fail = fail;
	
	result.conditions = conditions;
	
	return result;
}
