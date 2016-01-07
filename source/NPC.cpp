/* NPC.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "NPC.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "UI.h"

#include <vector>

using namespace std;



void NPC::Load(const DataNode &node)
{
	// Any tokens after the "npc" tag list the things that must happen for this
	// mission to succeed.
	for(int i = 1; i < node.Size(); ++i)
	{
		if(node.Token(i) == "save")
			failIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "kill")
			succeedIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "board")
			succeedIf |= ShipEvent::BOARD;
		else if(node.Token(i) == "assist")
			succeedIf |= ShipEvent::ASSIST;
		else if(node.Token(i) == "disable")
			succeedIf |= ShipEvent::DISABLE;
		else if(node.Token(i) == "scan cargo")
			succeedIf |= ShipEvent::SCAN_CARGO;
		else if(node.Token(i) == "scan outfits")
			succeedIf |= ShipEvent::SCAN_OUTFITS;
		else if(node.Token(i) == "evade")
			mustEvade = true;
		else if(node.Token(i) == "accompany")
			mustAccompany = true;
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
		{
			if(child.Size() >= 2)
				system = GameData::Systems().Get(child.Token(1));
			else
				location.Load(child);
		}
		else if(child.Token(0) == "succeed" && child.Size() >= 2)
			succeedIf = child.Value(1);
		else if(child.Token(0) == "fail" && child.Size() >= 2)
			failIf = child.Value(1);
		else if(child.Token(0) == "evade")
			mustEvade = true;
		else if(child.Token(0) == "accompany")
			mustAccompany = true;
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "dialog")
		{
			for(int i = 1; i < child.Size(); ++i)
			{
				if(!dialogText.empty())
					dialogText += "\n\t";
				dialogText += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!dialogText.empty())
						dialogText += "\n\t";
					dialogText += grand.Token(i);
				}
		}
		else if(child.Token(0) == "conversation" && child.HasChildren())
			conversation.Load(child);
		else if(child.Token(0) == "conversation" && child.Size() > 1)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else if(child.Token(0) == "ship")
		{
			if(child.HasChildren())
			{
				ships.push_back(make_shared<Ship>());
				ships.back()->Load(child);
				for(const DataNode &grand : child)
					if(grand.Token(0) == "actions" && grand.Size() >= 2)
						actions[ships.back().get()] = grand.Value(1);
			}
			else if(child.Size() >= 2)
			{
				stockShips.push_back(GameData::Ships().Get(child.Token(1)));
				shipNames.push_back(child.Token(1 + (child.Size() > 2)));
			}
		}
		else if(child.Token(0) == "fleet")
		{
			if(child.HasChildren())
			{
				fleets.push_back(Fleet());
				fleets.back().Load(child);
			}
			else if(child.Size() >= 2)
				stockFleets.push_back(GameData::Fleets().Get(child.Token(1)));
		}
	}
	
	// Since a ship's government is not serialized, set it now.
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->FinishLoading();
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		ship->SetIsSpecial();
	}
}



// Note: the Save() function can assume this is an instantiated mission, not
// a template, so fleets will be replaced by individual ships already.
void NPC::Save(DataWriter &out) const
{
	out.Write("npc");
	out.BeginChild();
	{
		if(succeedIf)
			out.Write("succeed", succeedIf);
		if(failIf)
			out.Write("fail", failIf);
		if(mustEvade)
			out.Write("evade");
		if(mustAccompany)
			out.Write("accompany");
		
		if(government)
			out.Write("government", government->GetName());
		personality.Save(out);
		
		if(!dialogText.empty())
		{
			out.Write("dialog");
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				size_t begin = 0;
				while(true)
				{
					size_t pos = dialogText.find("\n\t", begin);
					if(pos == string::npos)
						pos = dialogText.length();
					out.Write(dialogText.substr(begin, pos - begin));
					if(pos == dialogText.length())
						break;
					begin = pos + 2;
				}
			}
			out.EndChild();
		}
		if(!conversation.IsEmpty())
			conversation.Save(out);
		
		for(const shared_ptr<Ship> &ship : ships)
		{
			ship->Save(out);
			auto it = actions.find(ship.get());
			if(it != actions.end() && it->second)
			{
				// Append an "actions" tag to the end of the ship data.
				out.BeginChild();
				{
					out.Write("actions", it->second);
				}
				out.EndChild();
			}
		}
	}
	out.EndChild();
}



// Get the ships associated with this set of NPCs.
const list<shared_ptr<Ship>> NPC::Ships() const
{
	return ships;
}



// Handle the given ShipEvent.
void NPC::Do(const ShipEvent &event, PlayerInfo &player, UI *ui)
{
	bool hasSucceeded = HasSucceeded(player.GetSystem());
	bool hasFailed = HasFailed();
	for(shared_ptr<Ship> &ship : ships)
		if(ship == event.Target())
		{
			actions[ship.get()] |= event.Type();
			vector<shared_ptr<Ship>> carried = ship->CarriedShips();
			for(const shared_ptr<Ship> &fighter : carried)
				actions[fighter.get()] |= event.Type();
			
			// If a mission ship is captured, let it live on under its new
			// ownership but mark our copy of it as destroyed.
			if(event.Type() & ShipEvent::CAPTURE)
			{
				Ship *copy = new Ship(*ship);
				copy->Destroy();
				actions[copy] = actions[ship.get()] | ShipEvent::DESTROY;
				ship.reset(copy);
			}
			break;
		}
	
	if(HasFailed() && !hasFailed)
		Messages::Add("Mission failed.");
	else if(ui && HasSucceeded(player.GetSystem()) && !hasSucceeded)
	{
		if(!conversation.IsEmpty())
			ui->Push(new ConversationPanel(player, conversation));
		else if(!dialogText.empty())
			ui->Push(new Dialog(dialogText));
	}
}



bool NPC::HasSucceeded(const System *playerSystem) const
{
	if(HasFailed())
		return false;
	
	// Check what system each ship is in, if there is a requirement that we
	// either evade them, or accompany them. If you are accompanying a ship, it
	// must not be disabled (so that it can land with you). If trying to evade
	// it, disabling it is sufficient (you do not have to kill it).
	if(mustEvade || mustAccompany)
		for(const shared_ptr<Ship> &ship : ships)
			if((ship->GetSystem() == playerSystem && !ship->IsDisabled()) ^ mustAccompany)
				return false;
	
	if(!succeedIf)
		return true;
	
	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = actions.find(ship.get());
		if(it == actions.end() || (it->second & succeedIf) != succeedIf)
			return false;
	}
	
	return true;
}



// Check if the NPC is supposed to be accompanied and is not.
bool NPC::IsLeftBehind(const System *playerSystem) const
{
	if(HasFailed())
		return true;
	if(!mustAccompany)
		return false;
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->IsDisabled() || ship->GetSystem() != playerSystem)
			return true;
	
	return false;
}



bool NPC::HasFailed() const
{
	for(const auto &it : actions)
		if(it.second & failIf)
			return true;
	
	return false;
}



// Create a copy of this NPC but with the fleets replaced by the actual
// ships they represent, wildcards in the conversation text replaced, etc.
NPC NPC::Instantiate(map<string, string> &subs, const System *origin) const
{
	NPC result;
	result.government = government;
	if(!result.government)
		result.government = GameData::PlayerGovernment();
	result.personality = personality;
	result.succeedIf = succeedIf;
	result.failIf = failIf;
	result.mustEvade = mustEvade;
	result.mustAccompany = mustAccompany;
	
	// Pick the system for this NPC to start out in.
	result.system = system;
	if(!result.system && !location.IsEmpty())
	{
		// Find a destination that satisfies the filter.
		vector<const System *> options;
		for(const auto &it : GameData::Systems())
		{
			// Skip entries with incomplete data.
			if(it.second.Name().empty())
				continue;
			if(location.Matches(&it.second, origin))
				options.push_back(&it.second);
		}
		if(!options.empty())
			result.system = options[Random::Int(options.size())];
	}
	if(!result.system)
		result.system = origin;
	
	// Convert fleets into instances of ships.
	for(const shared_ptr<Ship> &ship : ships)
	{
		result.ships.push_back(make_shared<Ship>(*ship));
		result.ships.back()->FinishLoading();
	}
	auto shipIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for( ; shipIt != stockShips.end() && nameIt != shipNames.end(); ++shipIt, ++nameIt)
	{
		result.ships.push_back(make_shared<Ship>(**shipIt));
		result.ships.back()->SetName(*nameIt);
	}
	for(const Fleet &fleet : fleets)
		fleet.Place(*result.system, result.ships, false);
	for(const Fleet *fleet : stockFleets)
		fleet->Place(*result.system, result.ships, false);
	// Ships should either "enter" the system or start out there.
	for(const shared_ptr<Ship> &ship : result.ships)
	{
		ship->SetGovernment(result.government);
		ship->SetIsSpecial();
		ship->SetPersonality(result.personality);
		
		if(personality.IsEntering())
			Fleet::Enter(*result.system, *ship);
		else
			Fleet::Place(*result.system, *ship);
	}
	
	// String replacement:
	if(!result.ships.empty())
		subs["<npc>"] = result.ships.front()->Name();
	
	// Do string replacement on any dialog or conversation.
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Substitute(subs);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Substitute(subs);
	
	return result;
}
