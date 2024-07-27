/* NPC.cpp
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

#include "NPC.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <vector>

using namespace std;

namespace {
	string TriggerToText(NPC::Trigger trigger)
	{
		switch(trigger)
		{
			case NPC::Trigger::ASSIST:
				return "on assist";
			case NPC::Trigger::SCAN_CARGO:
				return "on 'scan cargo'";
			case NPC::Trigger::SCAN_OUTFITS:
				return "on 'scan outfits'";
			case NPC::Trigger::PROVOKE:
				return "on provoke";
			case NPC::Trigger::DISABLE:
				return "on disable";
			case NPC::Trigger::BOARD:
				return "on board";
			case NPC::Trigger::CAPTURE:
				return "on capture";
			case NPC::Trigger::DESTROY:
				return "on destroy";
			case NPC::Trigger::KILL:
				return "on kill";
			case NPC::Trigger::ENCOUNTER:
				return "on encounter";
			default:
				return "unknown trigger";
		}
	}
}



// Construct and Load() at the same time.
NPC::NPC(const DataNode &node)
{
	Load(node);
}



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
		else if(node.Token(i) == "capture")
			succeedIf |= ShipEvent::CAPTURE;
		else if(node.Token(i) == "provoke")
			succeedIf |= ShipEvent::PROVOKE;
		else if(node.Token(i) == "evade")
			mustEvade = true;
		else if(node.Token(i) == "accompany")
			mustAccompany = true;
		else
			node.PrintTrace("Warning: Skipping unrecognized NPC completion condition \"" + node.Token(i) + "\":");
	}

	// Check for incorrect objective combinations.
	if(failIf & ShipEvent::DESTROY && (succeedIf & ShipEvent::DESTROY || succeedIf & ShipEvent::CAPTURE))
		node.PrintTrace("Error: conflicting NPC mission objective to save and destroy or capture.");
	if(mustEvade && mustAccompany)
		node.PrintTrace("Warning: NPC mission objective to accompany and evade is synonymous with kill.");
	if(mustEvade && (succeedIf & ShipEvent::DESTROY || succeedIf & ShipEvent::CAPTURE))
		node.PrintTrace("Warning: redundant NPC mission objective to evade and destroy or capture.");

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
		{
			if(child.Size() >= 2)
			{
				if(child.Token(1) == "destination")
					isAtDestination = true;
				else
					system = GameData::Systems().Get(child.Token(1));
			}
			else
				location.Load(child);
		}
		else if(child.Token(0) == "uuid" && child.Size() >= 2)
			uuid = EsUuid::FromString(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			planet = GameData::Planets().Get(child.Token(1));
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
		else if(child.Token(0) == "cargo settings" && child.HasChildren())
		{
			cargo.Load(child);
			overrideFleetCargo = true;
		}
		else if(child.Token(0) == "dialog")
		{
			bool hasValue = (child.Size() > 1);
			// Dialog text may be supplied from a stock named phrase, a
			// private unnamed phrase, or directly specified.
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
		else if(child.Token(0) == "conversation" && child.HasChildren())
			conversation = ExclusiveItem<Conversation>(Conversation(child));
		else if(child.Token(0) == "conversation" && child.Size() > 1)
			conversation = ExclusiveItem<Conversation>(GameData::Conversations().Get(child.Token(1)));
		else if(child.Token(0) == "to" && child.Size() >= 2)
		{
			if(child.Token(1) == "spawn")
				toSpawn.Load(child);
			else if(child.Token(1) == "despawn")
				toDespawn.Load(child);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(child.Token(0) == "on" && child.Size() >= 2)
		{
			static const map<string, Trigger> trigger = {
				{"assist", Trigger::ASSIST},
				{"scan cargo", Trigger::SCAN_CARGO},
				{"scan outfits", Trigger::SCAN_OUTFITS},
				{"provoke", Trigger::PROVOKE},
				{"disable", Trigger::DISABLE},
				{"board", Trigger::BOARD},
				{"capture", Trigger::CAPTURE},
				{"destroy", Trigger::DESTROY},
				{"kill", Trigger::KILL},
				{"encounter", Trigger::ENCOUNTER},
			};
			auto it = trigger.find(child.Token(1));
			if(it != trigger.end())
				npcActions[it->second].Load(child);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(child.Token(0) == "ship")
		{
			if(child.HasChildren() && child.Size() == 2)
			{
				// Loading an NPC from a save file, or an entire ship specification.
				// The latter may result in references to non-instantiated outfits.
				ships.emplace_back(make_shared<Ship>(child));
				for(const DataNode &grand : child)
					if(grand.Token(0) == "actions" && grand.Size() >= 2)
						shipEvents[ships.back().get()] = grand.Value(1);
			}
			else if(child.Size() >= 2)
			{
				// Loading a ship managed by GameData, i.e. "base models" and variants.
				stockShips.push_back(GameData::Ships().Get(child.Token(1)));
				shipNames.push_back(child.Token(1 + (child.Size() > 2)));
			}
			else
			{
				string message = "Error: Skipping unsupported use of a ship token and child nodes: ";
				if(child.Size() >= 3)
					message += "to both name and customize a ship, create a variant and then reference it here.";
				else
					message += "the \'ship\' token must be followed by the name of a ship, e.g. ship \"Bulk Freighter\"";
				child.PrintTrace(message);
			}
		}
		else if(child.Token(0) == "fleet")
		{
			if(child.HasChildren())
			{
				fleets.emplace_back(ExclusiveItem<Fleet>(Fleet(child)));
				if(child.Size() >= 2)
				{
					// Copy the custom fleet in lieu of reparsing the same DataNode.
					size_t numAdded = child.Value(1);
					for(size_t i = 1; i < numAdded; ++i)
						fleets.push_back(fleets.back());
				}
			}
			else if(child.Size() >= 2)
			{
				auto fleet = ExclusiveItem<Fleet>(GameData::Fleets().Get(child.Token(1)));
				if(child.Size() >= 3 && child.Value(2) > 1.)
					fleets.insert(fleets.end(), child.Value(2), fleet);
				else
					fleets.push_back(fleet);
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// Empty spawning conditions imply that an instantiated NPC has spawned (or
	// if this is an NPC template, that any NPCs created from this will spawn).
	passedSpawnConditions = toSpawn.IsEmpty();
	// (Any non-empty `toDespawn` set is guaranteed to evaluate to false, otherwise the NPC would never
	// have been serialized. Thus, `passedDespawnConditions` is always false if the NPC is being Loaded.)

	// Since a ship's government is not serialized, set it now.
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		ship->SetIsSpecial();
		ship->FinishLoading(false);
	}
}



// Note: the Save() function can assume this is an instantiated NPC, not
// a template, so fleets will be replaced by individual ships already.
void NPC::Save(DataWriter &out) const
{
	// If this NPC should no longer appear in-game, don't serialize it.
	if(passedDespawnConditions)
		return;

	out.Write("npc");
	out.BeginChild();
	{
		out.Write("uuid", uuid.ToString());
		if(succeedIf)
			out.Write("succeed", succeedIf);
		if(failIf)
			out.Write("fail", failIf);
		if(mustEvade)
			out.Write("evade");
		if(mustAccompany)
			out.Write("accompany");

		// Only save out spawn conditions if they have yet to be met.
		// This is so that if a player quits the game and returns, NPCs that
		// were spawned do not then become despawned because they no longer
		// pass the spawn conditions.
		if(!toSpawn.IsEmpty() && !passedSpawnConditions)
		{
			out.Write("to", "spawn");
			out.BeginChild();
			{
				toSpawn.Save(out);
			}
			out.EndChild();
		}
		if(!toDespawn.IsEmpty())
		{
			out.Write("to", "despawn");
			out.BeginChild();
			{
				toDespawn.Save(out);
			}
			out.EndChild();
		}

		for(auto &it : npcActions)
			it.second.Save(out);

		if(government)
			out.Write("government", government->GetTrueName());
		personality.Save(out);

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

		for(const shared_ptr<Ship> &ship : ships)
		{
			ship->Save(out);
			auto it = shipEvents.find(ship.get());
			if(it != shipEvents.end() && it->second)
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



string NPC::Validate(bool asTemplate) const
{
	// An NPC with no government will take the player's government

	// NPC templates have certain fields to validate that instantiated NPCs do not:
	if(asTemplate)
	{
		// A location filter may be used to set the starting system.
		// If given, it must be able to resolve to a valid system.
		if(!location.IsValid())
			return "location filter";

		// A null system reference is allowed, since it will be set during
		// instantiation if not given explicitly.
		if(system && !system->IsValid())
			return "system \"" + system->Name() + "\"";

		// A planet is optional, but if given must be valid.
		if(planet && !planet->IsValid())
			return "planet \"" + planet->TrueName() + "\"";

		// If a stock phrase or conversation is given, it must not be empty.
		if(dialogPhrase.IsStock() && dialogPhrase->IsEmpty())
			return "stock phrase";
		if(conversation.IsStock() && conversation->IsEmpty())
			return "stock conversation";

		// NPC fleets, unlike stock fleets, do not need a valid government
		// since they will unconditionally inherit this NPC's government.
		for(auto &&fleet : fleets)
			if(!fleet->IsValid(false))
				return fleet.IsStock() ? "stock fleet" : "custom fleet";
	}

	// Ships must always be valid.
	for(auto &&ship : ships)
		if(!ship->IsValid())
			return "ship \"" + ship->Name() + "\"";
	for(auto &&ship : stockShips)
		if(!ship->IsValid())
			return "stock model \"" + ship->VariantName() + "\"";

	return "";
}



const EsUuid &NPC::UUID() const noexcept
{
	return uuid;
}



// Update spawning and despawning for this NPC.
void NPC::UpdateSpawning(const PlayerInfo &player)
{
	checkedSpawnConditions = true;
	// The conditions are tested every time this function is called until
	// they pass. This is so that a change in a player's conditions don't
	// cause an NPC to "un-spawn" or "un-despawn." Despawn conditions are
	// only checked after the spawn conditions have passed so that an NPC
	// doesn't "despawn" before spawning in the first place.
	if(!passedSpawnConditions)
		passedSpawnConditions = toSpawn.Test(player.Conditions());

	// It is allowable for an NPC to pass its spawning conditions and then immediately pass its despawning
	// conditions. (Any such NPC will never be spawned in-game.)
	if(passedSpawnConditions && !toDespawn.IsEmpty() && !passedDespawnConditions)
		passedDespawnConditions = toDespawn.Test(player.Conditions());
}



// Determine if this NPC should be placed in-flight.
bool NPC::ShouldSpawn() const
{
	return passedSpawnConditions && !passedDespawnConditions;
}



// Get the ships associated with this set of NPCs.
const list<shared_ptr<Ship>> NPC::Ships() const
{
	return ships;
}



// Handle the given ShipEvent.
void NPC::Do(const ShipEvent &event, PlayerInfo &player, UI *ui, const Mission *caller, bool isVisible)
{
	// First, check if this ship is part of this NPC. If not, do nothing. If it
	// is an NPC and it just got captured, replace it with a destroyed copy of
	// itself so that this class thinks the ship is destroyed.
	shared_ptr<Ship> ship;
	int type = event.Type();
	for(shared_ptr<Ship> &ptr : ships)
		if(ptr == event.Target())
		{
			// If a mission ship is captured, let it live on under its new
			// ownership but mark our copy of it as destroyed. This must be done
			// before we check the mission's success status because otherwise
			// momentarily reactivating a ship you're supposed to evade would
			// clear the success status and cause the success message to be
			// displayed a second time below.
			if(event.Type() & ShipEvent::CAPTURE)
			{
				Ship *copy = new Ship(*ptr);
				copy->SetUUID(ptr->UUID());
				copy->Destroy();
				shipEvents[copy] = shipEvents[ptr.get()];
				// Count this ship as destroyed, as well as captured.
				type |= ShipEvent::DESTROY;
				ptr.reset(copy);
			}
			ship = ptr;
			break;
		}
	if(!ship)
		return;

	// Determine if this NPC is already in the succeeded state,
	// regardless of whether it will despawn on the next landing.
	bool alreadySucceeded = HasSucceeded(player.GetSystem(), false);
	bool alreadyFailed = HasFailed();

	// If this event was "ASSIST", the ship is now known as not disabled.
	if(type == ShipEvent::ASSIST)
		shipEvents[ship.get()] &= ~(ShipEvent::DISABLE);

	// Certain events only count towards the NPC's status if originated by
	// the player: scanning, boarding, assisting, capturing, or provoking.
	if(!event.ActorGovernment() || !event.ActorGovernment()->IsPlayer())
		type &= ~(ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS | ShipEvent::ASSIST
				| ShipEvent::BOARD | ShipEvent::CAPTURE | ShipEvent::PROVOKE);

	// Determine if this event is new for this ship.
	bool newEvent = ~(shipEvents[ship.get()]) & type;
	// Apply this event to the ship and any ships it is carrying.
	shipEvents[ship.get()] |= type;
	for(const Ship::Bay &bay : ship->Bays())
		if(bay.ship)
			shipEvents[bay.ship.get()] |= type;

	// Run any mission actions that trigger on this event.
	DoActions(event, newEvent, player, ui, caller);

	// Check if the success status has changed. If so, display a message.
	if(isVisible && !alreadyFailed && HasFailed())
		Messages::Add("Mission failed" + (caller ? ": \"" + caller->Name() + "\"" : "") + ".",
			Messages::Importance::Highest);
	else if(ui && !alreadySucceeded && HasSucceeded(player.GetSystem(), false))
	{
		// If "completing" this NPC displays a conversation, reference
		// it, to allow the completing event's target to be destroyed.
		if(!conversation->IsEmpty())
			ui->Push(new ConversationPanel(player, *conversation, caller, nullptr, ship));
		if(!dialogText.empty())
			ui->Push(new Dialog(dialogText));
	}
}



bool NPC::HasSucceeded(const System *playerSystem, bool ignoreIfDespawnable) const
{
	// If this NPC has not yet spawned, or has fully despawned, then ignore its
	// objectives. An NPC that will despawn on landing is allowed to still enter
	// a "completed" state and trigger related completion events.
	if(checkedSpawnConditions && (!passedSpawnConditions
			|| (ignoreIfDespawnable && passedDespawnConditions)))
		return true;

	if(HasFailed())
		return false;

	// Evaluate the status of each ship in this NPC block. If it has `accompany`
	// and is alive then it cannot be disabled and must be in the player's system.
	// If the NPC block has `evade`, the ship can be disabled, destroyed, captured,
	// or not present.
	if(mustEvade || mustAccompany)
		for(const shared_ptr<Ship> &ship : ships)
		{
			auto it = shipEvents.find(ship.get());
			// If a derelict ship has not received any ShipEvents, it is immobile.
			bool isImmobile = ship->GetPersonality().IsDerelict();
			// The success status calculation can only be based on recorded
			// events (and the current system).
			if(it != shipEvents.end())
			{
				// Captured or destroyed ships have either succeeded or no longer count.
				if(it->second & (ShipEvent::DESTROY | ShipEvent::CAPTURE))
					continue;
				// A ship that was disabled is considered 'immobile'.
				isImmobile = (it->second & ShipEvent::DISABLE);
				// If this NPC is 'derelict' and has no ASSIST on record, it is immobile.
				isImmobile |= ship->GetPersonality().IsDerelict()
					&& !(it->second & ShipEvent::ASSIST);
			}
			bool isHere;
			// If this ship is being carried, check the parent's system.
			if(!ship->GetSystem() && ship->CanBeCarried() && ship->GetParent())
				isHere = ship->GetParent()->GetSystem() == playerSystem;
			else
				isHere = (!ship->GetSystem() || ship->GetSystem() == playerSystem);
			if((isHere && !isImmobile) ^ mustAccompany)
				return false;
		}

	if(!succeedIf)
		return true;

	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = shipEvents.find(ship.get());
		if(it == shipEvents.end() || (it->second & succeedIf) != succeedIf)
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
	// An unspawned NPC, one which will despawn on landing, or that has
	// already despawned, is not considered "failed."
	if(!passedSpawnConditions || passedDespawnConditions)
		return false;

	for(const auto &it : shipEvents)
	{
		if(it.second & failIf)
			return true;

		// If we still need to perform an action on this NPC, then that ship
		// being destroyed should cause the mission to fail.
		if((~it.second & succeedIf) && (it.second & ShipEvent::DESTROY))
			return true;
	}

	return false;
}



// Create a copy of this NPC but with the fleets replaced by the actual
// ships they represent, wildcards in the conversation text replaced, etc.
NPC NPC::Instantiate(const ConditionsStore &conditions, map<string, string> &subs, const System *origin,
		const System *destination, int jumps, int64_t payload) const
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

	result.passedSpawnConditions = passedSpawnConditions;
	result.toSpawn = toSpawn;
	result.toDespawn = toDespawn;

	// Instantiate the actions.
	string reason;
	auto ait = npcActions.begin();
	for( ; ait != npcActions.end(); ++ait)
	{
		reason = ait->second.Validate();
		if(!reason.empty())
			break;
	}
	if(ait != npcActions.end())
	{
		Logger::LogError("Instantiation Error: Action \"" + TriggerToText(ait->first) +
				"\" in NPC uses invalid " + std::move(reason));
		return result;
	}
	for(const auto &it : npcActions)
		result.npcActions[it.first] = it.second.Instantiate(conditions, subs, origin, jumps, payload);

	// Pick the system for this NPC to start out in.
	result.system = system;
	if(!result.system && !location.IsEmpty())
		result.system = location.PickSystem(origin);
	if(!result.system)
		result.system = (isAtDestination && destination) ? destination : origin;
	// If a planet was specified in the template, it must be in this system.
	if(planet && result.system->FindStellar(planet))
		result.planet = planet;

	// Convert fleets into instances of ships.
	for(const shared_ptr<Ship> &ship : ships)
	{
		// This ship is being defined from scratch.
		result.ships.push_back(make_shared<Ship>(*ship));
		result.ships.back()->FinishLoading(true);
	}
	auto shipIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for( ; shipIt != stockShips.end() && nameIt != shipNames.end(); ++shipIt, ++nameIt)
	{
		result.ships.push_back(make_shared<Ship>(**shipIt));
		result.ships.back()->SetName(*nameIt);
	}
	for(const ExclusiveItem<Fleet> &fleet : fleets)
		fleet->Place(*result.system, result.ships, false, !overrideFleetCargo);
	// Ships should either "enter" the system or start out there.
	for(const shared_ptr<Ship> &ship : result.ships)
	{
		ship->SetGovernment(result.government);
		ship->SetIsSpecial();
		ship->SetPersonality(result.personality);
		if(result.personality.IsDerelict())
			ship->Disable();

		if(personality.IsEntering())
			Fleet::Enter(*result.system, *ship);
		else if(result.planet)
		{
			// A valid planet was specified in the template, so these NPCs start out landed.
			ship->SetSystem(result.system);
			ship->SetPlanet(result.planet);
		}
		else
			Fleet::Place(*result.system, *ship);
	}

	// Set the cargo for each ship in the NPC if the NPC itself has cargo settings.
	if(overrideFleetCargo)
		for(const auto &ship : result.ships)
			cargo.SetCargo(&*ship);

	// String replacement:
	if(!result.ships.empty())
	{
		subs["<npc>"] = result.ships.front()->Name();
		subs["<npc model>"] = result.ships.front()->DisplayModelName();
	}
	// Do string replacement on any dialog or conversation.
	string dialogText = !dialogPhrase->IsEmpty() ? dialogPhrase->Get() : this->dialogText;
	if(!dialogText.empty())
		result.dialogText = Format::Replace(Phrase::ExpandPhrases(dialogText), subs);

	if(!conversation->IsEmpty())
		result.conversation = ExclusiveItem<Conversation>(conversation->Instantiate(subs));

	return result;
}



// Handle any NPC mission actions that may have been triggered by a ShipEvent.
void NPC::DoActions(const ShipEvent &event, bool newEvent, PlayerInfo &player, UI *ui, const Mission *caller)
{
	// Map the ShipEvent that was received to the Triggers it could flip.
	static const map<int, vector<Trigger>> eventTriggers = {
		{ShipEvent::ASSIST, {Trigger::ASSIST}},
		{ShipEvent::SCAN_CARGO, {Trigger::SCAN_CARGO}},
		{ShipEvent::SCAN_OUTFITS, {Trigger::SCAN_OUTFITS}},
		{ShipEvent::PROVOKE, {Trigger::PROVOKE}},
		{ShipEvent::DISABLE, {Trigger::DISABLE}},
		{ShipEvent::BOARD, {Trigger::BOARD}},
		{ShipEvent::CAPTURE, {Trigger::CAPTURE, Trigger::KILL}},
		{ShipEvent::DESTROY, {Trigger::DESTROY, Trigger::KILL}},
		{ShipEvent::ENCOUNTER, {Trigger::ENCOUNTER}},
	};

	int type = event.Type();

	// Ships are capable of receiving multiple DESTROY events. Only
	// handle the first such event, because a ship can't actually be
	// destroyed multiple times.
	if((type & ShipEvent::DESTROY) && !newEvent)
		type &= ~ShipEvent::DESTROY;

	// Get the actions for the Triggers that could potentially run.
	set<Trigger> triggers;
	for(const auto &it : eventTriggers)
		if(type & it.first)
			triggers.insert(it.second.begin(), it.second.end());

	for(Trigger trigger : triggers)
	{
		auto it = npcActions.find(trigger);
		if(it == npcActions.end())
			continue;

		static const map<Trigger, int> triggerRequirements = {
			{Trigger::ASSIST, ShipEvent::ASSIST},
			{Trigger::SCAN_CARGO, ShipEvent::SCAN_CARGO},
			{Trigger::SCAN_OUTFITS, ShipEvent::SCAN_OUTFITS},
			{Trigger::PROVOKE, ShipEvent::PROVOKE},
			{Trigger::DISABLE, ShipEvent::DISABLE},
			{Trigger::BOARD, ShipEvent::BOARD},
			{Trigger::CAPTURE, ShipEvent::CAPTURE},
			{Trigger::DESTROY, ShipEvent::DESTROY},
			{Trigger::KILL, ShipEvent::CAPTURE | ShipEvent::DESTROY},
			{Trigger::ENCOUNTER, ShipEvent::ENCOUNTER},
		};

		// Some Triggers cannot be met if any of the ships in this NPC have certain events.
		// If any of the ships were captured, the DESTROY trigger will not run.
		static const map<Trigger, int> triggerExclusions = {
			{Trigger::DESTROY, ShipEvent::CAPTURE}
		};

		const auto requiredIt = triggerRequirements.find(trigger);
		const int requiredEvents = requiredIt == triggerRequirements.end() ? 0 : requiredIt->second;
		const auto excludedIt = triggerExclusions.find(trigger);
		const int excludedEvents = excludedIt == triggerExclusions.end() ? 0 : excludedIt->second;

		// The PROVOKE and ENCOUNTER Triggers only requires a single ship to receive the
		// event in order to run. All other Triggers require that all ships
		// be affected.
		if(trigger == Trigger::ENCOUNTER || trigger == Trigger::PROVOKE || all_of(ships.begin(), ships.end(),
				[&](const shared_ptr<Ship> &ship) -> bool
				{
					auto it = shipEvents.find(ship.get());
					return it != shipEvents.end() && (it->second & requiredEvents) && !(it->second & excludedEvents);
				}))
		{
			it->second.Do(player, ui, caller);
		}
	}
}
