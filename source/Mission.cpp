/* Mission.cpp
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

#include "Mission.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "DistanceMap.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "Messages.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "UI.h"

#include <cmath>
#include <limits>
#include <sstream>

using namespace std;

namespace {
	// Pick a random commodity that would make sense to be exported from the
	// first system to the second.
	const Trade::Commodity *PickCommodity(const System &from, const System &to)
	{
		vector<int> weight;
		int total = 0;
		for(const Trade::Commodity &commodity : GameData::Commodities())
		{
			// For every 100 credits in profit you can make, double the chance
			// of this commodity being chosen.
			double profit = to.Trade(commodity.name) - from.Trade(commodity.name);
			int w = max<int>(1, 100. * pow(2., profit * .01));
			weight.push_back(w);
			total += w;
		}
		total += !total;
		// Pick a random commodity based on those weights.
		int r = Random::Int(total);
		for(unsigned i = 0; i < weight.size(); ++i)
		{
			r -= weight[i];
			if(r < 0)
				return &GameData::Commodities()[i];
		}
		// Control will never reach here, but to satisfy the compiler:
		return nullptr;
	}

	// If a source, destination, waypoint, or stopover supplies more than one explicit choice
	// or a mixture of explicit choice and location filter, print a warning.
	void ParseMixedSpecificity(const DataNode &node, string &&kind, int expected)
	{
		if(node.Size() >= expected + 1)
			node.PrintTrace("Use a location filter to choose from multiple " + kind + "s:");
		if(node.HasChildren())
			node.PrintTrace("Location filter ignored due to use of explicit " + kind + ":");
	}

	string TriggerToText(Mission::Trigger trigger)
	{
		switch(trigger)
		{
			case Mission::Trigger::ABORT:
				return "on abort";
			case Mission::Trigger::ACCEPT:
				return "on accept";
			case Mission::Trigger::COMPLETE:
				return "on complete";
			case Mission::Trigger::DECLINE:
				return "on decline";
			case Mission::Trigger::DEFER:
				return "on defer";
			case Mission::Trigger::FAIL:
				return "on fail";
			case Mission::Trigger::OFFER:
				return "on offer";
			case Mission::Trigger::STOPOVER:
				return "on stopover";
			case Mission::Trigger::VISIT:
				return "on visit";
			case Mission::Trigger::WAYPOINT:
				return "on waypoint";
			case Mission::Trigger::DAILY:
				return "on daily";
			case Mission::Trigger::DISABLED:
				return "on disabled";
			default:
				return "unknown trigger";
		}
	}
}



// Construct and Load() at the same time.
Mission::Mission(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	Load(node, playerConditions, visitedSystems, visitedPlanets);
}



// Load a mission, either from the game data or from a saved game.
void Mission::Load(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	// All missions need a name.
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for mission:");
		return;
	}
	// If a mission object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a mission with the same name as the base game
	// or another plugin). This class is not designed to allow merging or
	// overriding of mission data from two different definitions.
	if(!trueName.empty())
	{
		node.PrintTrace("Duplicate definition of mission:");
		return;
	}
	trueName = node.Token(1);
	if(!DataNode::IsConditionName(trueName))
		node.PrintTrace("Invalid mission name:");

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "name" && hasValue)
			displayName = child.Token(1);
		else if(key == "uuid" && hasValue)
			uuid = EsUuid::FromString(child.Token(1));
		else if(key == "description" && hasValue)
			description = child.Token(1);
		else if(key == "blocked" && hasValue)
			blocked = child.Token(1);
		else if(key == "deadline" && child.Size() >= 4)
			deadline = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "deadline")
		{
			if(child.Size() == 1)
				deadlineMultiplier += 2;
			if(child.Size() >= 2)
				deadlineBase += child.Value(1);
			if(child.Size() >= 3)
				deadlineMultiplier += child.Value(2);
		}
		else if(key == "distance calculation settings" && child.HasChildren())
		{
			distanceCalcSettings.Load(child);
		}
		else if(key == "cargo" && child.Size() >= 3)
		{
			cargo = child.Token(1);
			cargoSize = child.Value(2);
			if(child.Size() >= 4)
				cargoLimit = child.Value(3);
			if(child.Size() >= 5)
				cargoProb = child.Value(4);

			for(const DataNode &grand : child)
			{
				if(!ParseContraband(grand))
					grand.PrintTrace("Skipping unrecognized attribute:");
				else
					grand.PrintTrace("Deprecated use of \"stealth\" and \"illegal\" as a child of \"cargo\"."
						" They are now mission-level properties:");
			}
		}
		else if(key == "passengers" && hasValue)
		{
			passengers = child.Value(1);
			if(child.Size() >= 3)
				passengerLimit = child.Value(2);
			if(child.Size() >= 4)
				passengerProb = child.Value(3);
		}
		else if(key == "apparent payment" && hasValue)
			paymentApparent = child.Value(1);
		else if(ParseContraband(child))
		{
			// This was an "illegal" or "stealth" entry. It has already been
			// parsed, so nothing more needs to be done here.
		}
		else if(key == "invisible")
			isVisible = false;
		else if(key == "priority")
			hasPriority = true;
		else if(key == "non-blocking")
			isNonBlocking = true;
		else if(key == "minor")
			isMinor = true;
		else if(key == "offer precedence" && hasValue)
			offerPrecedence = child.Value(1);
		else if(key == "autosave")
			autosave = true;
		else if(key == "job")
			location = JOB;
		else if(key == "landing")
			location = LANDING;
		else if(key == "assisting")
			location = ASSISTING;
		else if(key == "boarding")
		{
			location = BOARDING;
			for(const DataNode &grand : child)
				if(grand.Token(0) == "override capture")
					overridesCapture = true;
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "shipyard")
			location = SHIPYARD;
		else if(key == "outfitter")
			location = OUTFITTER;
		else if(key == "job board")
			location = JOB_BOARD;
		else if(key == "entering")
			location = ENTERING;
		else if(key == "repeat")
			repeat = (child.Size() == 1 ? 0 : static_cast<int>(child.Value(1)));
		else if(key == "clearance")
		{
			clearance = (child.Size() == 1 ? "auto" : child.Token(1));
			clearanceFilter.Load(child, visitedSystems, visitedPlanets);
		}
		else if(child.Size() == 2 && key == "ignore" && child.Token(1) == "clearance")
			ignoreClearance = true;
		else if(key == "infiltrating")
			hasFullClearance = false;
		else if(key == "failed")
			hasFailed = true;
		else if(key == "to" && hasValue)
		{
			if(child.Token(1) == "offer")
				toOffer.Load(child, playerConditions);
			else if(child.Token(1) == "complete")
				toComplete.Load(child, playerConditions);
			else if(child.Token(1) == "fail")
				toFail.Load(child, playerConditions);
			else if(child.Token(1) == "accept")
				toAccept.Load(child, playerConditions);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "source" && hasValue)
		{
			source = GameData::Planets().Get(child.Token(1));
			ParseMixedSpecificity(child, "planet", 2);
		}
		else if(key == "source")
			sourceFilter.Load(child, visitedSystems, visitedPlanets);
		else if(key == "destination" && child.Size() == 2)
		{
			destination = GameData::Planets().Get(child.Token(1));
			ParseMixedSpecificity(child, "planet", 2);
		}
		else if(key == "destination")
			destinationFilter.Load(child, visitedSystems, visitedPlanets);
		else if(key == "complete")
		{
			if(!hasValue)
				child.PrintTrace("Incomplete \"complete\" option. Follow \"complete\" with \"at\".");
			else if(child.Token(1) == "at")
			{
				LocationFilter loaded(child, visitedSystems, visitedPlanets);
				if(loaded.IsEmpty())
					child.PrintTrace("The \"complete at\" filter must not be empty. Ignoring this filter.");
				else
					completionFilter = loaded;
			}
			else
				child.PrintTrace("Unknown \"complete\" option \"" + child.Token(1) + '"');
		}
		else if(key == "waypoint" && hasValue)
		{
			bool visited = child.Size() >= 3 && child.Token(2) == "visited";
			set<const System *> &set = visited ? visitedWaypoints : waypoints;
			set.insert(GameData::Systems().Get(child.Token(1)));
			ParseMixedSpecificity(child, "system", 2 + visited);
		}
		else if(key == "waypoint" && child.HasChildren())
			waypointFilters.emplace_back(child, visitedSystems, visitedPlanets);
		else if(key == "stopover" && hasValue)
		{
			bool visited = child.Size() >= 3 && child.Token(2) == "visited";
			set<const Planet *> &set = visited ? visitedStopovers : stopovers;
			set.insert(GameData::Planets().Get(child.Token(1)));
			ParseMixedSpecificity(child, "planet", 2 + visited);
		}
		else if(key == "stopover" && child.HasChildren())
			stopoverFilters.emplace_back(child, visitedSystems, visitedPlanets);
		else if(key == "mark" && hasValue)
		{
			bool unmarked = child.Size() >= 3 && child.Token(2) == "unmarked";
			set<const System *> &set = unmarked ? unmarkedSystems : markedSystems;
			set.insert(GameData::Systems().Get(child.Token(1)));
		}
		else if(key == "substitutions" && child.HasChildren())
			substitutions.Load(child, playerConditions);
		else if(key == "npc")
			npcs.emplace_back(child, playerConditions, visitedSystems, visitedPlanets);
		else if(key == "timer" && hasValue)
			timers.emplace_back(child, playerConditions, visitedSystems, visitedPlanets);
		else if(key == "on" && hasValue && child.Token(1) == "enter")
		{
			// "on enter" nodes may either name a specific system or use a LocationFilter
			// to control the triggering system.
			if(child.Size() >= 3)
			{
				MissionAction &action = onEnter[GameData::Systems().Get(child.Token(2))];
				action.Load(child, playerConditions, visitedSystems, visitedPlanets);
			}
			else
				genericOnEnter.emplace_back(child, playerConditions, visitedSystems, visitedPlanets);
		}
		else if(key == "on" && hasValue)
		{
			static const map<string, Trigger> trigger = {
				{"complete", COMPLETE},
				{"offer", OFFER},
				{"accept", ACCEPT},
				{"decline", DECLINE},
				{"fail", FAIL},
				{"abort", ABORT},
				{"defer", DEFER},
				{"visit", VISIT},
				{"stopover", STOPOVER},
				{"waypoint", WAYPOINT},
				{"daily", DAILY},
				{"disabled", DISABLED},
			};
			auto it = trigger.find(child.Token(1));
			if(it != trigger.end())
				actions[it->second].Load(child, playerConditions, visitedSystems, visitedPlanets);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "color" && child.Size() >= 3)
		{
			auto setColor = [&child](ExclusiveItem<Color> &color) noexcept -> void {
				if(child.Size() >= 5)
					color = ExclusiveItem<Color>(Color(child.Value(2), child.Value(3), child.Value(4)));
				else
					color = ExclusiveItem<Color>(GameData::Colors().Get(child.Token(2)));
			};
			const string &value = child.Token(1);
			if(value == "unavailable")
				setColor(unavailable);
			else if(value == "unselected")
				setColor(unselected);
			else if(value == "selected")
				setColor(selected);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(displayName.empty())
		displayName = trueName;
	hasTrackedNpcs = ranges::any_of(npcs, [](const NPC &npc) { return npc.GetPersonality().IsTracked(); });
}



// Save a mission. It is safe to assume that any mission that is being saved
// is already "instantiated," so only a subset of the data must be saved.
void Mission::Save(DataWriter &out, const string &tag) const
{
	out.Write(tag, trueName);
	out.BeginChild();
	{
		out.Write("name", displayName);
		out.Write("uuid", uuid.ToString());
		if(!description.empty())
			out.Write("description", description);
		if(!blocked.empty())
			out.Write("blocked", blocked);
		if(deadline)
			out.Write("deadline", deadline.Day(), deadline.Month(), deadline.Year());
		if(cargoSize)
			out.Write("cargo", cargo, cargoSize);
		if(passengers)
			out.Write("passengers", passengers);
		if(paymentApparent)
			out.Write("apparent payment", paymentApparent);
		if(fine)
			out.Write("illegal", fine, fineMessage);
		if(failIfDiscovered)
			out.Write("stealth");
		if(!isVisible)
			out.Write("invisible");
		auto saveColor = [&out](const ExclusiveItem<Color> &color, string tokenName) noexcept -> void {
			if(!color->IsLoaded())
				return;
			if(!color->TrueName().empty())
				out.Write("color", tokenName, color->TrueName());
			else
			{
				const float *rgba = color->Get();
				out.Write("color", tokenName, rgba[0], rgba[1], rgba[2]);
			}
		};
		saveColor(unavailable, "unavailable");
		saveColor(unselected, "unselected");
		saveColor(selected, "selected");
		if(hasPriority)
			out.Write("priority");
		if(isNonBlocking)
			out.Write("non-blocking");
		if(isMinor)
			out.Write("minor");
		if(offerPrecedence)
			out.Write("offer precedence", offerPrecedence);
		if(autosave)
			out.Write("autosave");
		if(location == LANDING)
			out.Write("landing");
		else if(location == SHIPYARD)
			out.Write("shipyard");
		else if(location == OUTFITTER)
			out.Write("outfitter");
		else if(location == JOB_BOARD)
			out.Write("job board");
		else if(location == ASSISTING)
			out.Write("assisting");
		else if(location == BOARDING)
		{
			out.Write("boarding");
			if(overridesCapture)
			{
				out.BeginChild();
				{
					out.Write("override capture");
				}
				out.EndChild();
			}
		}
		else if(location == JOB)
			out.Write("job");
		else if(location == ENTERING)
			out.Write("entering");
		if(!clearance.empty())
		{
			out.Write("clearance", clearance);
			clearanceFilter.Save(out);
		}
		if(ignoreClearance)
			out.Write("ignore", "clearance");
		if(!hasFullClearance)
			out.Write("infiltrating");
		if(hasFailed)
			out.Write("failed");
		if(repeat != 1)
			out.Write("repeat", repeat);

		if(!toOffer.IsEmpty())
		{
			out.Write("to", "offer");
			out.BeginChild();
			{
				toOffer.Save(out);
			}
			out.EndChild();
		}
		if(!toAccept.IsEmpty())
		{
			out.Write("to", "accept");
			out.BeginChild();
			{
				toAccept.Save(out);
			}
			out.EndChild();
		}
		if(!toComplete.IsEmpty())
		{
			out.Write("to", "complete");
			out.BeginChild();
			{
				toComplete.Save(out);
			}
			out.EndChild();
		}
		if(!toFail.IsEmpty())
		{
			out.Write("to", "fail");
			out.BeginChild();
			{
				toFail.Save(out);
			}
			out.EndChild();
		}
		if(destination)
			out.Write("destination", destination->TrueName());
		if(!completionFilter.IsEmpty() && completionFilter.IsValid())
		{
			out.Write("complete", "at");
			completionFilter.Save(out);
		}
		for(const System *system : waypoints)
			out.Write("waypoint", system->TrueName());
		for(const System *system : visitedWaypoints)
			out.Write("waypoint", system->TrueName(), "visited");

		for(const Planet *planet : stopovers)
			out.Write("stopover", planet->TrueName());
		for(const Planet *planet : visitedStopovers)
			out.Write("stopover", planet->TrueName(), "visited");

		for(const System *system : markedSystems)
			out.Write("mark", system->TrueName());
		for(const System *system : unmarkedSystems)
			out.Write("mark", system->TrueName(), "unmarked");

		for(const NPC &npc : npcs)
			npc.Save(out);
		for(const MissionTimer &timer : timers)
			timer.Save(out);

		// Save all the actions, because this might be an "available mission" that
		// has not been received yet but must still be included in the saved game.
		for(const auto &it : actions)
			it.second.Save(out);
		// Save any "on enter" actions that have not been performed.
		for(const auto &it : onEnter)
			if(!didEnter.contains(&it.second))
				it.second.Save(out);
		for(const MissionAction &action : genericOnEnter)
			if(!didEnter.contains(&action))
				action.Save(out);
	}
	out.EndChild();
}



void Mission::NeverOffer()
{
	toOffer.MakeNever();
}



// Basic mission information.
const EsUuid &Mission::UUID() const noexcept
{
	return uuid;
}



// Get the internal name used for this mission. This name is unique and is
// never modified by string substitution, so it can be used in condition
// variables, etc.
const string &Mission::TrueName() const
{
	return trueName;
}



const string &Mission::DisplayName() const
{
	return displayName;
}



const string &Mission::Description() const
{
	return description;
}



// Check if this mission should be shown in your mission list. If not, the
// player will not know this mission exists (which is sometimes useful).
bool Mission::IsVisible() const
{
	return isVisible;
}



// The colors that should be used to display the mission name if it is shown
// in your mission list.
const Color &Mission::Unavailable() const
{
	return *unavailable;
}



const Color &Mission::Unselected() const
{
	return *unselected;
}



const Color &Mission::Selected() const
{
	return *selected;
}



// Check if this instantiated mission uses any systems, planets, or ships that are
// not fully defined. If everything is fully defined, this is a valid mission.
bool Mission::IsValid() const
{
	// Planets must be defined and in a system. However, a source system does not necessarily exist.
	if(source && !source->IsValid())
		return false;
	// Every mission is required to have a destination.
	if(!destination || !destination->IsValid())
		return false;

	// Missions with completion filters must have valid filters.
	if(!completionFilter.IsEmpty() && !completionFilter.IsValid())
		return false;

	// All stopovers must be valid.
	for(auto &&planet : Stopovers())
		if(!planet->IsValid())
			return false;
	for(auto &&planet : VisitedStopovers())
		if(!planet->IsValid())
			return false;

	// Systems must have a defined position.
	for(auto &&system : Waypoints())
		if(!system->IsValid())
			return false;
	for(auto &&system : VisitedWaypoints())
		if(!system->IsValid())
			return false;
	for(auto &&system : MarkedSystems())
		if(!system->IsValid())
			return false;
	for(auto &&system : UnmarkedSystems())
		if(!system->IsValid())
			return false;

	// Actions triggered when entering a system should reference valid systems.
	for(auto &&it : onEnter)
		if(!it.first->IsValid() || !it.second.Validate().empty())
			return false;
	for(auto &&it : actions)
		if(!it.second.Validate().empty())
			return false;
	// Generic "on enter" may use a LocationFilter that exclusively references invalid content.
	for(auto &&action : genericOnEnter)
		if(!action.Validate().empty())
			return false;
	if(!clearanceFilter.IsValid())
		return false;

	// The instantiated NPCs should also be valid.
	for(auto &&npc : NPCs())
		if(!npc.Validate().empty())
			return false;

	return true;
}



// Check if this mission has high priority. If any priority missions
// are available, only other priority missions and non-blocking ones can offer alongside it.
bool Mission::HasPriority() const
{
	return hasPriority;
}



// Check if this mission is a "non-blocking" mission.
// Such missions will not prevent minor missions from being offered alongside them,
// and will not be prevented from offering by priority missions.
bool Mission::IsNonBlocking() const
{
	return isNonBlocking;
}



// Check if this mission is a "minor" mission. Minor missions will only be
// offered if no other missions (minor or otherwise) are being offered.
bool Mission::IsMinor() const
{
	return isMinor;
}



int Mission::OfferPrecedence() const
{
	return offerPrecedence;
}



bool Mission::IsAtLocation(Location location) const
{
	return (this->location == location);
}



// Information about what you are doing.
const Ship *Mission::SourceShip() const
{
	return sourceShip;
}



const Planet *Mission::Destination() const
{
	return destination;
}



const set<const System *> &Mission::Waypoints() const
{
	return waypoints;
}



const set<const System *> &Mission::VisitedWaypoints() const
{
	return visitedWaypoints;
}



const set<const Planet *> &Mission::Stopovers() const
{
	return stopovers;
}



const set<const Planet *> &Mission::VisitedStopovers() const
{
	return visitedStopovers;
}



const set<const System *> &Mission::MarkedSystems() const
{
	return markedSystems;
}



const set<const System *> &Mission::UnmarkedSystems() const
{
	return unmarkedSystems;
}



const set<const System *> &Mission::TrackedSystems() const
{
	return trackedSystems;
}



void Mission::RecalculateTrackedSystems()
{
	if(!hasTrackedNpcs)
		return;

	trackedSystems.clear();
	for(const NPC &npc : npcs)
	{
		if(!npc.GetPersonality().IsTracked())
			continue;
		for(const auto &ship : npc.Ships())
			if(!ship->IsDestroyed() && ship->GetSystem())
				trackedSystems.insert(ship->GetSystem());
	}
	// If all tracked NPCs are destroyed, then we can ignore
	// future calls to recalculate the tracked systems.
	if(trackedSystems.empty())
		hasTrackedNpcs = false;
}



void Mission::Mark(const set<const System *> &systems) const
{
	for(const System *system : systems)
	{
		markedSystems.insert(system);
		unmarkedSystems.erase(system);
	}
}



void Mission::Unmark(const set<const System *> &systems) const
{
	for(const System *system : systems)
		if(markedSystems.erase(system))
			unmarkedSystems.insert(system);
}



const string &Mission::Cargo() const
{
	return cargo;
}



int Mission::CargoSize() const
{
	return cargoSize;
}



int Mission::Fine() const
{
	return fine;
}



string Mission::FineMessage() const
{
	return fineMessage;
}



bool Mission::FailIfDiscovered() const
{
	return failIfDiscovered;
}



int Mission::Passengers() const
{
	return passengers;
}



int64_t Mission::DisplayedPayment() const
{
	return paymentApparent ? paymentApparent : GetAction(Mission::Trigger::COMPLETE).Payment();
}



const int Mission::ExpectedJumps() const
{
	return expectedJumps;
}



// The mission must be completed by this deadline (if there is a deadline).
const Date &Mission::Deadline() const
{
	return deadline;
}



// If this mission's deadline was before the given date and it has not been
// marked as failing already, mark it and return true.
bool Mission::CheckDeadline(const Date &today)
{
	if(!hasFailed && deadline && deadline < today)
	{
		hasFailed = true;
		return true;
	}
	return false;
}



// Check if you have special clearance to land on your destination.
bool Mission::HasClearance(const Planet *planet) const
{
	if(clearance.empty())
		return false;
	if(planet == destination || stopovers.contains(planet) || visitedStopovers.contains(planet))
		return true;
	return (!clearanceFilter.IsEmpty() && clearanceFilter.Matches(planet));
}



// Get the string to be shown in the destination planet's hailing dialog. If
// this is "auto", you don't have to hail them to get landing permission.
const string &Mission::ClearanceMessage() const
{
	return clearance;
}



// Check whether we have full clearance to land and use the planet's
// services, or whether we are landing in secret ("infiltrating").
bool Mission::HasFullClearance() const
{
	return hasFullClearance;
}



// Check if it's possible to offer or complete this mission right now.
bool Mission::CanOffer(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	if(location == BOARDING || location == ASSISTING)
	{
		if(!boardingShip)
			return false;

		if(!sourceFilter.Matches(*boardingShip))
			return false;
	}
	else if(location == ENTERING)
	{
		if(!sourceFilter.Matches(player.GetSystem()))
			return false;
	}
	else
	{
		if(source && source != player.GetPlanet())
			return false;

		if(!sourceFilter.Matches(player.GetPlanet()))
			return false;
	}

	if(!toOffer.Test())
		return false;

	if(!toFail.IsEmpty() && toFail.Test())
		return false;

	if(repeat && player.Conditions().Get(trueName + ": offered") >= repeat)
		return false;

	bool isFailed = IsFailed();
	auto it = actions.find(OFFER);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed, boardingShip))
		return false;

	it = actions.find(ACCEPT);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed, boardingShip))
		return false;

	it = actions.find(DECLINE);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed, boardingShip))
		return false;

	it = actions.find(DEFER);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed, boardingShip))
		return false;

	return true;
}



bool Mission::CanAccept(const PlayerInfo &player) const
{
	if(!toAccept.Test())
		return false;

	bool isFailed = IsFailed();
	auto it = actions.find(OFFER);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed))
		return false;

	it = actions.find(ACCEPT);
	if(it != actions.end() && !it->second.CanBeDone(player, isFailed))
		return false;
	return HasSpace(player);
}



bool Mission::HasSpace(const PlayerInfo &player) const
{
	int extraCrew = 0;
	if(player.Flagship())
		extraCrew = player.Flagship()->Crew() - player.Flagship()->RequiredCrew();
	return (cargoSize <= player.Cargo().Free() + player.Cargo().CommoditiesSize()
		&& passengers <= player.Cargo().BunksFree() + extraCrew);
}



// Check if this mission's cargo can fit entirely on the referenced ship.
bool Mission::HasSpace(const Ship &ship) const
{
	return (cargoSize <= ship.Cargo().Free() && passengers <= ship.Cargo().BunksFree());
}



bool Mission::CanComplete(const PlayerInfo &player) const
{
	if(completionFilter.IsEmpty() && player.GetPlanet() != destination)
		return false;
	else if(!completionFilter.IsEmpty() && !completionFilter.Matches(player.GetPlanet(), player.GetSystem()))
		return false;

	return IsSatisfied(player);
}



// This function dictates whether missions on the player's map are shown in
// bright or dim text colors, and may be called while in-flight or landed.
bool Mission::IsSatisfied(const PlayerInfo &player) const
{
	if(!waypoints.empty() || !stopovers.empty())
		return false;

	// Test the completion conditions for this mission.
	if(!toComplete.Test())
		return false;

	// Determine if any fines or outfits that must be transferred, can.
	auto it = actions.find(COMPLETE);
	if(it != actions.end() && !it->second.CanBeDone(player, IsFailed()))
		return false;

	// NPCs which must be accompanied or evaded must be present (or not),
	// and any needed scans, boarding, or assisting must also be completed.
	for(const NPC &npc : npcs)
		if(!npc.HasSucceeded(player.GetSystem()))
			return false;
	// All non-optional timers must be complete.
	for(const MissionTimer &timer : timers)
		if(!timer.IsOptional() && !timer.IsComplete())
			return false;

	// If any of the cargo for this mission is being carried by a ship that is
	// not in this system, the mission cannot be completed right now.
	for(const auto &ship : player.Ships())
	{
		// Skip in-system ships, and carried ships whose parent is in-system.
		if(ship->GetSystem() == player.GetSystem() || (!ship->GetSystem() && ship->CanBeCarried()
				&& ship->GetParent() && ship->GetParent()->GetSystem() == player.GetSystem()))
			continue;

		if(ship->Cargo().GetPassengers(this))
			return false;
		// Check for all mission cargo, including that which has 0 mass.
		auto &cargo = ship->Cargo().MissionCargo();
		if(cargo.find(this) != cargo.end())
			return false;
	}

	return true;
}



bool Mission::IsFailed() const
{
	if(!toFail.IsEmpty() && toFail.Test())
		return true;

	for(const NPC &npc : npcs)
		if(npc.HasFailed())
			return true;

	return hasFailed;
}



bool Mission::OverridesCapture() const
{
	return overridesCapture;
}



// Mark a mission failed (e.g. due to a "fail" action in another mission).
void Mission::Fail()
{
	hasFailed = true;
}



// Get a string to show if this mission is "blocked" from being offered
// because it requires you to have more passenger or cargo space free. After
// calling this function, any future calls to it will return an empty string
// so that you do not display the same message multiple times.
string Mission::BlockedMessage(const PlayerInfo &player)
{
	if(blocked.empty())
		return "";

	int extraCrew = 0;
	const Ship *flagship = player.Flagship();
	// You cannot fire crew in space.
	if(flagship && player.GetPlanet())
		extraCrew = flagship->Crew() - flagship->RequiredCrew();

	int cargoNeeded = cargoSize;
	int bunksNeeded = passengers;
	if(player.GetPlanet())
	{
		cargoNeeded -= (player.Cargo().Free() + player.Cargo().CommoditiesSize());
		bunksNeeded -= (player.Cargo().BunksFree() + extraCrew);
	}
	else
	{
		// Boarding a ship, so only use the flagship's space.
		cargoNeeded -= flagship->Cargo().Free();
		bunksNeeded -= flagship->Cargo().BunksFree();
	}

	map<string, string> subs;
	GameData::GetTextReplacements().Substitutions(subs);
	substitutions.Substitutions(subs);
	player.AddPlayerSubstitutions(subs);

	subs["<conditions>"] = toAccept.Test() ? "meet" : "do not meet";

	ostringstream out;
	if(bunksNeeded > 0)
		out << (bunksNeeded == 1 ? "another bunk" : to_string(bunksNeeded) + " more bunks");
	if(bunksNeeded > 0 && cargoNeeded > 0)
		out << " and ";
	if(cargoNeeded > 0)
		out << (cargoNeeded == 1 ? "another ton" : to_string(cargoNeeded) + " more tons") << " of cargo space";
	if(bunksNeeded <= 0 && cargoNeeded <= 0)
		out << "no additional space";
	subs["<capacity>"] = out.str();

	for(const auto &keyValue : subs)
		subs[keyValue.first] = Phrase::ExpandPhrases(keyValue.second);
	Format::Expand(subs);

	string message = Format::Replace(blocked, subs);
	blocked.clear();
	return message;
}



// Check if this mission recommends that the game be autosaved when it is
// accepted. This should be set for main story line missions that have a
// high chance of failing, such as escort missions.
bool Mission::RecommendsAutosave() const
{
	return autosave;
}



// Check if this mission is unique, i.e. not something that will be offered
// over and over again in different variants.
bool Mission::IsUnique() const
{
	return (repeat == 1);
}



// When the state of this mission changes, it may make changes to the player
// information or show new UI panels. PlayerInfo::MissionCallback() will be
// used as the callback for any UI panel that returns a value.
bool Mission::Do(Trigger trigger, PlayerInfo &player, UI *ui, const shared_ptr<Ship> &boardingShip)
{
	if(trigger == STOPOVER)
	{
		// If this is not one of this mission's stopover planets, or if it is
		// not the very last one that must be visited, do nothing.
		auto it = stopovers.find(player.GetPlanet());
		if(it == stopovers.end())
			return false;

		for(const NPC &npc : npcs)
			if(npc.IsLeftBehind(player.GetSystem()))
			{
				ui->Push(new Dialog("This is a stop for one of your missions, but you have left a ship behind."));
				return false;
			}

		visitedStopovers.insert(*it);
		stopovers.erase(it);
		if(!stopovers.empty())
			return false;
	}
	if(trigger == ABORT && IsFailed())
		return false;
	if(trigger == WAYPOINT && !waypoints.empty())
		return false;

	auto it = actions.find(trigger);
	// If this mission was aborted but no ABORT action exists, look for a FAIL
	// action instead. This is done for backwards compatibility purposes from
	// when aborting a mission activated the FAIL trigger.
	if(trigger == ABORT && it == actions.end())
		it = actions.find(FAIL);

	// Fail and abort conditions get updated regardless of whether the action
	// can be done, as a fail or abort action not being able to be done does
	// not prevent a mission from being failed or aborted.
	if(trigger == FAIL)
	{
		--player.Conditions()[trueName + ": active"];
		++player.Conditions()[trueName + ": failed"];
	}
	else if(trigger == ABORT)
	{
		--player.Conditions()[trueName + ": active"];
		++player.Conditions()[trueName + ": aborted"];
		// Set the failed mission condition here as well for
		// backwards compatibility.
		++player.Conditions()[trueName + ": failed"];
	}

	// Don't update any further conditions if this action exists and can't be completed.
	if(it != actions.end() && !it->second.CanBeDone(player, IsFailed(), boardingShip))
		return false;

	if(trigger == ACCEPT)
	{
		++player.Conditions()[trueName + ": offered"];
		++player.Conditions()[trueName + ": active"];
		// Any potential on offer conversation has been finished, so update
		// the active NPCs for the first time and cache any necessary information.
		UpdateNPCs(player);
		DistanceMap here(player);
		player.CacheMissionInformation(*this, here);
	}
	else if(trigger == DECLINE)
	{
		++player.Conditions()[trueName + ": offered"];
		++player.Conditions()[trueName + ": declined"];
	}
	else if(trigger == COMPLETE)
	{
		--player.Conditions()[trueName + ": active"];
		++player.Conditions()[trueName + ": done"];
	}

	// "Jobs" should never show dialogs when offered, nor should they call the
	// player's mission callback.
	if(trigger == OFFER && location == JOB)
		ui = nullptr;

	// If this trigger has actions tied to it, perform them. Otherwise, check
	// if this is a non-job mission that just got offered and if so,
	// automatically accept it.
	// Actions that are performed only receive the mission destination
	// system if the mission is visible. This is because the purpose of
	// a MissionAction being given the destination system is for drawing
	// a special marker at the destination if the map is opened during any
	// mission dialog or conversation. Invisible missions don't show this
	// marker.
	if(it != actions.end())
		it->second.Do(player, ui, this, (destination && isVisible) ? destination->GetSystem() : nullptr,
			boardingShip, IsUnique());
	else if(trigger == OFFER && location != JOB)
		player.MissionCallback(Conversation::ACCEPT);

	return true;
}



bool Mission::RequiresGiftedShip(const string &shipId) const
{
	// Check if any uncompleted actions required for the mission needs this ship.
	set<Trigger> requiredActions;
	{
		requiredActions.insert(Trigger::COMPLETE);
		if(!stopovers.empty())
			requiredActions.insert(Trigger::STOPOVER);
		if(!waypoints.empty())
			requiredActions.insert(Trigger::WAYPOINT);
	}
	for(const auto &it : actions)
		if(requiredActions.contains(it.first) && it.second.RequiresGiftedShip(shipId))
			return true;

	return false;
}



// Get a list of NPCs associated with this mission. Every time the player
// takes off from a planet, they should be added to the active ships.
const list<NPC> &Mission::NPCs() const
{
	return npcs;
}



// Update which NPCs are active based on their spawn and despawn conditions.
void Mission::UpdateNPCs(const PlayerInfo &player)
{
	for(auto &npc : npcs)
		npc.UpdateSpawning(player);
}



// Iterate through the timers and progress them if applicable.
void Mission::StepTimers(PlayerInfo &player, UI *ui)
{
	for(MissionTimer &timer : timers)
		timer.Step(player, ui, *this);
}



// Checks if the given ship belongs to one of the mission's NPCs.
bool Mission::HasShip(const shared_ptr<Ship> &ship) const
{
	for(const auto &npc : npcs)
		for(const auto &npcShip : npc.Ships())
			if(npcShip == ship)
				return true;
	return false;
}



// If any event occurs between two ships, check to see if this mission cares
// about it. This may affect the mission status or display a message.
void Mission::Do(const ShipEvent &event, PlayerInfo &player, UI *ui)
{
	if(event.TargetGovernment()->IsPlayer() && !IsFailed())
	{
		bool failed = false;
		string message;
		if(event.Type() & ShipEvent::DESTROY)
		{
			// Destroyed ships carrying mission cargo result in failed missions.
			// Mission cargo may have a quantity of zero (i.e. 0 mass).
			for(const auto &it : event.Target()->Cargo().MissionCargo())
				failed |= (it.first == this);
			// If any mission passengers were present, this mission is failed.
			for(const auto &it : event.Target()->Cargo().PassengerList())
				failed |= (it.first == this && it.second);
		}
		else if(event.Type() & ShipEvent::BOARD)
		{
			// Fail missions whose cargo is stolen by a boarding vessel.
			for(const auto &it : event.Actor()->Cargo().MissionCargo())
				failed |= (it.first == this);
			if(failed)
				message = "Your " + event.Target()->DisplayModelName() +
					" \"" + event.Target()->GivenName() + "\" has been plundered. ";
		}

		if(failed)
		{
			hasFailed = true;
			if(isVisible)
				Messages::Add({message + "Mission failed: \"" + displayName + "\".",
					GameData::MessageCategories().Get("high")});
		}
	}

	if((event.Type() & ShipEvent::DISABLE) && event.Target() == player.FlagshipPtr())
		Do(DISABLED, player, ui);

	const Ship *flagship = player.Flagship();
	if((event.Type() & ShipEvent::JUMP) && flagship && event.Actor().get() == flagship)
	{
		const System *system = event.Actor()->GetSystem();
		// If this was a waypoint, clear it.
		if(waypoints.erase(system))
		{
			visitedWaypoints.insert(system);
			Do(WAYPOINT, player, ui);
		}

		// Perform an "on enter" action for this system, if possible, and if
		// any was performed, update this mission's NPC spawn states.
		if(Enter(system, player, ui))
			UpdateNPCs(player);
	}

	for(NPC &npc : npcs)
	{
		bool isTarget = npc.Do(event, player, ui, this, isVisible);
		// If this event is a DESTROY or JUMP event for one of the NPCs in this mission, then
		// recalculate all tracked NPC locations.
		if((event.Type() & (ShipEvent::JUMP | ShipEvent::DESTROY)) && isTarget)
			RecalculateTrackedSystems();
	}
}



// Get a specific mission action from this mission.
// If a mission action is not found for the given trigger, returns an empty
// mission action.
const MissionAction &Mission::GetAction(Trigger trigger) const
{
	auto ait = actions.find(trigger);
	static const MissionAction EMPTY{};
	return ait != actions.end() ? ait->second : EMPTY;
}



// "Instantiate" a mission by replacing randomly selected values and places
// with a single choice, and then replacing any wildcard text as well.
Mission Mission::Instantiate(const PlayerInfo &player, const shared_ptr<Ship> &boardingShip) const
{
	Mission result;
	// If anything goes wrong below, this mission should not be offered.
	result.hasFailed = true;
	result.isVisible = isVisible;
	result.unavailable = unavailable;
	result.unselected = unselected;
	result.selected = selected;
	result.hasPriority = hasPriority;
	result.isNonBlocking = isNonBlocking;
	result.isMinor = isMinor;
	result.offerPrecedence = offerPrecedence;
	result.autosave = autosave;
	result.location = location;
	result.overridesCapture = overridesCapture;
	result.sourceShip = boardingShip.get();
	result.repeat = repeat;
	result.trueName = trueName;
	result.waypoints = waypoints;
	result.completionFilter = completionFilter;
	result.markedSystems = markedSystems;
	// Handle waypoint systems that are chosen randomly.
	const System *const sourceSystem = player.GetSystem();
	for(const LocationFilter &filter : waypointFilters)
	{
		const System *system = filter.PickSystem(sourceSystem);
		if(!system)
			return result;
		result.waypoints.insert(system);
	}
	// If one of the waypoints is the current system, it is already visited.
	if(result.waypoints.erase(sourceSystem))
		result.visitedWaypoints.insert(sourceSystem);

	// Copy the template's stopovers, and add planets that match the template's filters.
	result.stopovers = stopovers;
	// Make sure they all exist in a valid system.
	for(auto it = result.stopovers.begin(); it != result.stopovers.end(); )
	{
		if((*it)->IsValid())
			++it;
		else
			it = result.stopovers.erase(it);
	}
	for(const LocationFilter &filter : stopoverFilters)
	{
		// Unlike destinations, we can allow stopovers on planets that don't have a spaceport.
		const Planet *planet = filter.PickPlanet(sourceSystem, ignoreClearance || !clearance.empty(), false);
		if(!planet)
			return result;
		result.stopovers.insert(planet);
	}

	// First, pick values for all the variables.

	// If a specific destination is not specified in the mission, pick a random
	// one out of all the destinations that satisfy the mission requirements.
	result.destination = destination;
	if(!result.destination && !destinationFilter.IsEmpty())
	{
		result.destination = destinationFilter.PickPlanet(sourceSystem, ignoreClearance || !clearance.empty());
		if(!result.destination)
			return result;
	}
	// If no destination is specified, it is the same as the source planet. Also
	// use the source planet if the given destination is not a valid planet.
	if(!result.destination || !result.destination->IsValid())
	{
		if(player.GetPlanet())
			result.destination = player.GetPlanet();
		else
			return result;
	}

	// If cargo is being carried, see if we are supposed to replace a generic
	// cargo name with something more specific.
	if(!cargo.empty())
	{
		const string expandedCargo = Phrase::ExpandPhrases(cargo);
		const Trade::Commodity *commodity = nullptr;
		if(expandedCargo == "random")
			commodity = PickCommodity(*sourceSystem, *result.destination->GetSystem());
		else
		{
			for(const Trade::Commodity &option : GameData::Commodities())
				if(option.name == expandedCargo)
				{
					commodity = &option;
					break;
				}
			for(const Trade::Commodity &option : GameData::SpecialCommodities())
				if(option.name == expandedCargo)
				{
					commodity = &option;
					break;
				}
		}
		if(commodity)
			result.cargo = commodity->items[Random::Int(commodity->items.size())];
		else
			result.cargo = expandedCargo;
	}
	// Pick a random cargo amount, if requested.
	if(cargoSize || cargoLimit)
	{
		if(cargoProb)
			result.cargoSize = Random::Polya(cargoLimit, cargoProb) + cargoSize;
		else if(cargoLimit > cargoSize)
			result.cargoSize = cargoSize + Random::Int(cargoLimit - cargoSize + 1);
		else
			result.cargoSize = cargoSize;
	}
	// Pick a random passenger count, if requested.
	if(passengers || passengerLimit)
	{
		if(passengerProb)
			result.passengers = Random::Polya(passengerLimit, passengerProb) + passengers;
		else if(passengerLimit > passengers)
			result.passengers = passengers + Random::Int(passengerLimit - passengers + 1);
		else
			result.passengers = passengers;
	}
	result.paymentApparent = paymentApparent;
	result.fine = fine;
	result.fineMessage = Phrase::ExpandPhrases(fineMessage);
	result.failIfDiscovered = failIfDiscovered;

	result.distanceCalcSettings = distanceCalcSettings;
	int jumps = result.CalculateJumps(sourceSystem);

	int64_t payload = static_cast<int64_t>(result.cargoSize) + 10 * static_cast<int64_t>(result.passengers);

	// Set the deadline, if requested.
	if(deadlineBase || deadlineMultiplier)
		result.deadline = player.GetDate() + deadlineBase + deadlineMultiplier * jumps;

	// Copy the conditions. The offer conditions must be copied too, because they
	// may depend on a condition that other mission offers might change.
	result.toOffer = toOffer;
	result.toAccept = toAccept;
	result.toComplete = toComplete;
	result.toFail = toFail;

	// Generate the substitutions map.
	map<string, string> subs;
	GameData::GetTextReplacements().Substitutions(subs);
	substitutions.Substitutions(subs);
	subs["<commodity>"] = result.cargo;
	subs["<tons>"] = Format::MassString(result.cargoSize);
	subs["<cargo>"] = Format::CargoString(result.cargoSize, subs["<commodity>"]);
	subs["<bunks>"] = to_string(result.passengers);
	subs["<passengers>"] = (result.passengers == 1) ? "passenger" : "passengers";
	subs["<fare>"] = (result.passengers == 1) ? "a passenger" : (subs["<bunks>"] + " passengers");
	if(player.GetPlanet())
		subs["<origin>"] = player.GetPlanet()->DisplayName();
	else if(boardingShip)
		subs["<origin>"] = boardingShip->GivenName();
	subs["<planet>"] = result.destination ? result.destination->DisplayName() : "";
	subs["<system>"] = result.destination ? result.destination->GetSystem()->DisplayName() : "";
	subs["<destination>"] = subs["<planet>"] + " in the " + subs["<system>"] + " system";
	subs["<date>"] = result.deadline.ToString();
	subs["<day>"] = result.deadline.LongString();
	if(result.paymentApparent)
		subs["<payment>"] = Format::CreditString(abs(result.paymentApparent));
	// Stopovers: "<name> in the <system name> system" with "," and "and".
	auto getDisplayName = [](const auto *const &item)
	{
		return item->DisplayName();
	};
	if(!result.stopovers.empty())
	{
		subs["<stopovers>"] = Format::List<set, const Planet *>(result.stopovers,
			[](const Planet *const &planet)
			{
				return planet->DisplayName() + " in the " + planet->GetSystem()->DisplayName() + " system";
			});
		subs["<planet stopovers>"] = Format::List<set, const Planet *>(result.stopovers, getDisplayName);
	}
	// Waypoints and marks: "<system name>" with "," and "and".
	if(!result.waypoints.empty())
		subs["<waypoints>"] = Format::List<set, const System *>(result.waypoints, getDisplayName);
	if(!result.markedSystems.empty())
		subs["<marks>"] = Format::List<set, const System *>(result.markedSystems, getDisplayName);

	// Done making subs, so expand the phrases and recursively substitute.
	for(const auto &keyValue : subs)
		subs[keyValue.first] = Phrase::ExpandPhrases(keyValue.second);
	Format::Expand(subs);

	// Instantiate the NPCs. This also fills in the "<npc>" substitution.
	string reason;
	for(auto &&n : npcs)
		reason = n.Validate(true);
	if(!reason.empty())
	{
		Logger::Log("Instantiation Error: NPC template in mission \""
			+ TrueName() + "\" uses invalid " + std::move(reason), Logger::Level::WARNING);
		return result;
	}
	for(const NPC &npc : npcs)
		result.npcs.push_back(npc.Instantiate(player, subs, sourceSystem, result.destination->GetSystem(), jumps, payload));
	result.hasTrackedNpcs = hasTrackedNpcs;

	// Instantiate the timers.
	for(const MissionTimer &timer : timers)
		result.timers.push_back(timer.Instantiate(subs, sourceSystem, jumps, payload));

	// Instantiate the actions. The "complete" action is always first so that
	// the "<payment>" substitution can be filled in.
	auto ait = actions.begin();
	for( ; ait != actions.end(); ++ait)
	{
		reason = ait->second.Validate();
		if(!reason.empty())
			break;
	}
	if(ait != actions.end())
	{
		Logger::Log("Instantiation Error: Action \"" + TriggerToText(ait->first) + "\" in mission \""
			+ TrueName() + "\" uses invalid " + std::move(reason), Logger::Level::WARNING);
		return result;
	}
	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(subs, sourceSystem, jumps, payload);

	auto oit = onEnter.begin();
	for( ; oit != onEnter.end(); ++oit)
	{
		reason = oit->first->IsValid() ? oit->second.Validate() : "trigger system";
		if(!reason.empty())
			break;
	}
	if(oit != onEnter.end())
	{
		Logger::Log("Instantiation Error: Action \"on enter '" + oit->first->TrueName() + "'\" in mission \""
			+ TrueName() + "\" uses invalid " + std::move(reason), Logger::Level::WARNING);
		return result;
	}
	for(const auto &it : onEnter)
		result.onEnter[it.first] = it.second.Instantiate(subs, sourceSystem, jumps, payload);

	auto eit = genericOnEnter.begin();
	for( ; eit != genericOnEnter.end(); ++eit)
	{
		reason = eit->Validate();
		if(!reason.empty())
			break;
	}
	if(eit != genericOnEnter.end())
	{
		Logger::Log("Instantiation Error: Generic \"on enter\" action in mission \""
			+ TrueName() + "\" uses invalid " + std::move(reason), Logger::Level::WARNING);
		return result;
	}
	for(const MissionAction &action : genericOnEnter)
		result.genericOnEnter.emplace_back(action.Instantiate(subs, sourceSystem, jumps, payload));

	// Perform substitution in the name and description.
	result.displayName = Format::Replace(Phrase::ExpandPhrases(displayName), subs);
	result.description = Format::Replace(Phrase::ExpandPhrases(description), subs);
	result.clearance = Format::Replace(Phrase::ExpandPhrases(clearance), subs);
	result.blocked = Format::Replace(Phrase::ExpandPhrases(blocked), subs);
	result.clearanceFilter = clearanceFilter;
	result.hasFullClearance = hasFullClearance;

	result.hasFailed = false;
	return result;
}



int Mission::CalculateJumps(const System *sourceSystem)
{
	expectedJumps = 0;

	// Estimate how far the player will have to travel to visit all the waypoints
	// and stopovers and then to land on the destination planet. Rather than a
	// full traveling salesman path, just calculate a greedy approximation.
	list<const System *> destinations;
	for(const System *system : waypoints)
		destinations.push_back(system);
	for(const Planet *planet : stopovers)
		destinations.push_back(planet->GetSystem());

	while(!destinations.empty())
	{
		// Find the closest destination to this location.
		DistanceMap distance(sourceSystem,
				distanceCalcSettings.WormholeStrat(),
				distanceCalcSettings.AssumesJumpDrive());
		auto it = destinations.begin();
		auto bestIt = it;
		int bestDays = distance.Days(**bestIt);
		if(bestDays < 0)
			bestDays = numeric_limits<int>::max();
		for(++it; it != destinations.end(); ++it)
		{
			int days = distance.Days(**it);
			if(days >= 0 && days < bestDays)
			{
				bestIt = it;
				bestDays = days;
			}
		}

		sourceSystem = *bestIt;
		// If currently unreachable, this system adds -1 to the deadline, to match previous behavior.
		expectedJumps += bestDays == numeric_limits<int>::max() ? -1 : bestDays;
		destinations.erase(bestIt);
	}
	DistanceMap distance(sourceSystem,
			distanceCalcSettings.WormholeStrat(),
			distanceCalcSettings.AssumesJumpDrive());
	// If currently unreachable, this system adds -1 to the deadline, to match previous behavior.
	expectedJumps += distance.Days(*destination->GetSystem());

	return expectedJumps;
}



// Perform an "on enter" MissionAction associated with the current system.
// Returns true if an action was performed.
bool Mission::Enter(const System *system, PlayerInfo &player, UI *ui)
{
	const auto eit = onEnter.find(system);
	const auto originalSize = didEnter.size();
	if(eit != onEnter.end() && !didEnter.contains(&eit->second) && eit->second.CanBeDone(player, IsFailed()))
	{
		eit->second.Do(player, ui, this);
		didEnter.insert(&eit->second);
	}
	// If no specific `on enter` was performed, try matching to a generic "on enter,"
	// which may use a LocationFilter to govern which systems it can be performed in.
	else
		for(MissionAction &action : genericOnEnter)
			if(!didEnter.contains(&action) && action.CanBeDone(player, IsFailed()))
			{
				action.Do(player, ui, this);
				didEnter.insert(&action);
				break;
			}

	return didEnter.size() > originalSize;
}



// For legacy code, contraband definitions can be placed in two different
// locations, so move that parsing out to a helper function.
bool Mission::ParseContraband(const DataNode &node)
{
	const string &key = node.Token(0);
	if(key == "illegal" && node.Size() == 2)
		fine = node.Value(1);
	else if(key == "illegal" && node.Size() == 3)
	{
		fine = node.Value(1);
		fineMessage = node.Token(2);
	}
	else if(key == "stealth")
		failIfDiscovered = true;
	else
		return false;

	return true;
}
