/* PlayerInfo.cpp
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

#include "PlayerInfo.h"

#include "AI.h"
#include "Audio.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "DistanceMap.h"
#include "Files.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "Messages.h"
#include "Outfit.h"
#include "Person.h"
#include "Planet.h"
#include "Plugins.h"
#include "Politics.h"
#include "Port.h"
#include "Preferences.h"
#include "RaidFleet.h"
#include "Random.h"
#include "SavedGame.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "ShipJumpNavigation.h"
#include "StartConditions.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <functional>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {
	// Move the flagship to the start of your list of ships. It does not make sense
	// that the flagship would change if you are reunited with a different ship that
	// was higher up the list.
	void MoveFlagshipBegin(vector<shared_ptr<Ship>> &ships, const shared_ptr<Ship> &flagship)
	{
		if(!flagship)
			return;

		// Find the current location of the flagship.
		auto it = find(ships.begin(), ships.end(), flagship);
		if(it != ships.begin() && it != ships.end())
		{
			// Move all ships before the flagship one position backwards (which overwrites
			// the flagship at its position).
			move_backward(ships.begin(), it, next(it));
			// Re-add the flagship at the beginning of the list.
			ships[0] = flagship;
		}
	}

	string EntryToString(SystemEntry entryType)
	{
		switch(entryType)
		{
			case SystemEntry::HYPERDRIVE:
				return "hyperdrive";
			case SystemEntry::JUMP:
				return "jump drive";
			case SystemEntry::WORMHOLE:
				return "wormhole";
			default:
			case SystemEntry::TAKE_OFF:
				return "takeoff";
		}
	}

	SystemEntry StringToEntry(const string &entry)
	{
		if(entry == "hyperdrive")
			return SystemEntry::HYPERDRIVE;
		else if(entry == "jump drive")
			return SystemEntry::JUMP;
		else if(entry == "wormhole")
			return SystemEntry::WORMHOLE;
		return SystemEntry::TAKE_OFF;
	}

	bool HasClearance(const PlayerInfo &player, const Planet *planet)
	{
		auto CheckClearance = [&planet](const Mission &mission) -> bool
		{
			return mission.HasClearance(planet);
		};
		return any_of(player.Missions().begin(), player.Missions().end(), CheckClearance);
	}
}



// Completely clear all loaded information, to prepare for loading a file or
// creating a new pilot.
void PlayerInfo::Clear()
{
	*this = PlayerInfo();

	Random::Seed(time(nullptr));
	GameData::Revert();
	Messages::Reset();

	conditions.Clear();

	delete transactionSnapshot;
	transactionSnapshot = nullptr;
}



// Check if a player has been loaded.
bool PlayerInfo::IsLoaded() const
{
	return !firstName.empty();
}



// Make a new player.
void PlayerInfo::New(const StartConditions &start)
{
	// Clear any previously loaded data.
	Clear();

	// Copy the core information from the full starting scenario.
	startData = start;
	// Copy any ships in the start conditions.
	for(const Ship &ship : start.Ships())
	{
		ships.emplace_back(new Ship(ship));
		ships.back()->SetSystem(&start.GetSystem());
		ships.back()->SetPlanet(&start.GetPlanet());
		ships.back()->SetIsSpecial();
		ships.back()->SetIsYours();
		ships.back()->SetGovernment(GameData::PlayerGovernment());
	}
	// Load starting conditions from a "start" item in the data files. If no
	// such item exists, StartConditions defines default values.
	date = start.GetDate();
	GameData::SetDate(date);
	// Make sure the fleet depreciation object knows it is tracking the player's
	// fleet, not the planet's stock.
	depreciation.Init(ships, date.DaysSinceEpoch());

	SetSystem(start.GetSystem());
	SetPlanet(&start.GetPlanet());
	accounts = start.GetAccounts();
	RegisterDerivedConditions();
	start.GetConditions().Apply(conditions);

	// Generate missions that will be available on the first day.
	CreateMissions();

	// Add to the list of events that should happen on certain days.
	for(const auto &it : GameData::Events())
		if(it.second.GetDate())
			AddEvent(it.second, it.second.GetDate());
}



// Load player information from a saved game file.
void PlayerInfo::Load(const string &path)
{
	// Make sure any previously loaded data is cleared.
	Clear();

	// A listing of missions and the ships where their cargo or passengers were when the game was saved.
	// Missions and ships are referred to by string UUIDs.
	// Any mission cargo or passengers that were in the player's system will not be recorded here,
	// as that can be safely redistributed from the player's overall CargoHold to any ships in their system.
	map<string, map<string, int>> missionCargoToDistribute;
	map<string, map<string, int>> missionPassengersToDistribute;

	filePath = path;
	// Strip anything after the "~" from snapshots, so that the file we save
	// will be the auto-save, not the snapshot.
	size_t pos = filePath.find('~');
	size_t namePos = filePath.length() - Files::Name(filePath).length();
	if(pos != string::npos && pos > namePos)
		filePath = filePath.substr(0, pos) + ".txt";

	// The player may have bribed their current planet in the last session. Ensure
	// we provide the same access to services in this session, too.
	bool hasFullClearance = false;

	// Register derived conditions now, so old primary versions can load into them.
	RegisterDerivedConditions();

	DataFile file(path);
	for(const DataNode &child : file)
	{
		// Basic player information and persistent UI settings:
		if(child.Token(0) == "pilot" && child.Size() >= 3)
		{
			firstName = child.Token(1);
			lastName = child.Token(2);
		}
		else if(child.Token(0) == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "system entry method" && child.Size() >= 2)
			entry = StringToEntry(child.Token(1));
		else if(child.Token(0) == "previous system" && child.Size() >= 2)
			previousSystem = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			system = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			planet = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "clearance")
			hasFullClearance = true;
		else if(child.Token(0) == "launching")
			shouldLaunch = true;
		else if(child.Token(0) == "playtime" && child.Size() >= 2)
			playTime = child.Value(1);
		else if(child.Token(0) == "travel" && child.Size() >= 2)
			travelPlan.push_back(GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "travel destination" && child.Size() >= 2)
			travelDestination = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "map coloring" && child.Size() >= 2)
			mapColoring = child.Value(1);
		else if(child.Token(0) == "map zoom" && child.Size() >= 2)
			mapZoom = child.Value(1);
		else if(child.Token(0) == "collapsed" && child.Size() >= 2)
		{
			for(const DataNode &grand : child)
				collapsed[child.Token(1)].insert(grand.Token(0));
		}
		else if(child.Token(0) == "reputation with")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					reputationChanges.emplace_back(
						GameData::Governments().Get(grand.Token(0)), grand.Value(1));
		}
		else if(child.Token(0) == "tribute received")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					tributeReceived[GameData::Planets().Get(grand.Token(0))] = grand.Value(1);
		}
		// Records of things you own:
		else if(child.Token(0) == "ship")
		{
			// Ships owned by the player have various special characteristics:
			ships.push_back(make_shared<Ship>(child));
			ships.back()->SetIsSpecial();
			ships.back()->SetIsYours();
			// Defer finalizing this ship until we have processed all changes to game state.
		}
		else if(child.Token(0) == "groups" && child.Size() >= 2 && !ships.empty())
			groups[ships.back().get()] = child.Value(1);
		else if(child.Token(0) == "storage")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2 && grand.Token(0) == "planet")
					for(const DataNode &grandGrand : grand)
						if(grandGrand.Token(0) == "cargo")
						{
							CargoHold &storage = planetaryStorage[GameData::Planets().Get(grand.Token(1))];
							storage.Load(grandGrand);
						}
		}
		else if(child.Token(0) == "licenses")
		{
			for(const DataNode &grand : child)
				AddLicense(grand.Token(0));
		}
		else if(child.Token(0) == "account")
			accounts.Load(child, true);
		else if(child.Token(0) == "cargo")
			cargo.Load(child);
		else if(child.Token(0) == "basis")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					costBasis[grand.Token(0)] += grand.Value(1);
		}
		else if(child.Token(0) == "stock")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					stock[GameData::Outfits().Get(grand.Token(0))] += grand.Value(1);
		}
		else if(child.Token(0) == "fleet depreciation")
			depreciation.Load(child);
		else if(child.Token(0) == "stock depreciation")
			stockDepreciation.Load(child);

		// Records of things you have done or are doing, or have happened to you:
		else if(child.Token(0) == "mission")
		{
			missions.emplace_back(child);
			cargo.AddMissionCargo(&missions.back());
		}
		else if((child.Token(0) == "mission cargo" || child.Token(0) == "mission passengers") && child.HasChildren())
		{
			map<string, map<string, int>> &toDistribute = (child.Token(0) == "mission cargo")
					? missionCargoToDistribute : missionPassengersToDistribute;
			for(const DataNode &grand : child)
				if(grand.Token(0) == "player ships" && grand.HasChildren())
					for(const DataNode &great : grand)
					{
						if(great.Size() != 3)
							continue;
						toDistribute[great.Token(0)][great.Token(1)] = great.Value(2);
					}
		}
		else if(child.Token(0) == "available job")
			availableJobs.emplace_back(child);
		else if(child.Token(0) == "sort type")
			availableSortType = static_cast<SortType>(child.Value(1));
		else if(child.Token(0) == "sort descending")
			availableSortAsc = false;
		else if(child.Token(0) == "separate deadline")
			sortSeparateDeadline = true;
		else if(child.Token(0) == "separate possible")
			sortSeparatePossible = true;
		else if(child.Token(0) == "available mission")
			availableMissions.emplace_back(child);
		else if(child.Token(0) == "conditions")
			conditions.Load(child);
		else if(child.Token(0) == "gifted ships" && child.HasChildren())
		{
			for(const DataNode &grand : child)
				giftedShips[grand.Token(0)] = EsUuid::FromString(grand.Token(1));
		}
		else if(child.Token(0) == "event")
			gameEvents.emplace_back(child);
		else if(child.Token(0) == "changes")
		{
			for(const DataNode &grand : child)
				dataChanges.push_back(grand);
		}
		else if(child.Token(0) == "economy")
			economy = child;
		else if(child.Token(0) == "destroyed" && child.Size() >= 2)
			destroyedPersons.push_back(child.Token(1));

		// Records of things you have discovered:
		else if(child.Token(0) == "visited" && child.Size() >= 2)
			Visit(*GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "visited planet" && child.Size() >= 2)
			Visit(*GameData::Planets().Get(child.Token(1)));
		else if(child.Token(0) == "harvested")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					harvested.emplace(
						GameData::Systems().Get(grand.Token(0)),
						GameData::Outfits().Get(grand.Token(1)));
		}
		else if(child.Token(0) == "logbook")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Size() >= 3)
				{
					Date date(grand.Value(0), grand.Value(1), grand.Value(2));
					string text;
					for(const DataNode &great : grand)
					{
						if(!text.empty())
							text += "\n\t";
						text += great.Token(0);
					}
					logbook.emplace(date, text);
				}
				else if(grand.Size() >= 2)
				{
					string &text = specialLogs[grand.Token(0)][grand.Token(1)];
					for(const DataNode &great : grand)
					{
						if(!text.empty())
							text += "\n\t";
						text += great.Token(0);
					}
				}
			}
		}
		else if(child.Token(0) == "start")
			startData.Load(child);
	}
	// Modify the game data with any changes that were loaded from this file.
	ApplyChanges();
	// Ensure the player is in a valid state after loading & applying changes.
	ValidateLoad();

	// Restore access to services, if it was granted previously.
	if(planet && hasFullClearance)
		planet->Bribe();

	// Based on the ships that were loaded, calculate the player's capacity for
	// cargo and passengers.
	UpdateCargoCapacities();

	auto DistributeMissionCargo = [](map<string, map<string, int>> &toDistribute, const list<Mission> &missions,
			vector<shared_ptr<Ship>> &ships, CargoHold &cargo, bool passengers) -> void
	{
		for(const auto &it : toDistribute)
		{
			const auto missionIt = find_if(missions.begin(), missions.end(),
					[&it](const Mission &mission) { return mission.UUID().ToString() == it.first; });
			if(missionIt != missions.end())
			{
				const Mission *cargoOf = &*missionIt;
				for(const auto &shipCargo : it.second)
				{
					auto shipIt = find_if(ships.begin(), ships.end(),
							[&shipCargo](const shared_ptr<Ship> &ship) { return ship->UUID().ToString() == shipCargo.first; });
					if(shipIt != ships.end())
					{
						Ship *destination = shipIt->get();
						if(passengers)
							cargo.TransferPassengers(cargoOf, shipCargo.second, destination->Cargo());
						else
							cargo.Transfer(cargoOf, shipCargo.second, destination->Cargo());
					}
				}
			}
		}
	};

	DistributeMissionCargo(missionCargoToDistribute, missions, ships, cargo, false);
	DistributeMissionCargo(missionPassengersToDistribute, missions, ships, cargo, true);

	// If no depreciation record was loaded, every item in the player's fleet
	// will count as non-depreciated.
	if(!depreciation.IsLoaded())
		depreciation.Init(ships, date.DaysSinceEpoch());
}



// Load the most recently saved player (if any). Returns false when no save was loaded.
bool PlayerInfo::LoadRecent()
{
	string recentPath = Files::Read(Files::Config() + "recent.txt");
	// Trim trailing whitespace (including newlines) from the path.
	while(!recentPath.empty() && recentPath.back() <= ' ')
		recentPath.pop_back();

	if(recentPath.empty() || !Files::Exists(recentPath))
	{
		Clear();
		return false;
	}

	Load(recentPath);
	return true;
}



// Save this player. The file name is based on the player's name.
void PlayerInfo::Save() const
{
	// Don't save dead players or players that are not fully created.
	if(!CanBeSaved())
		return;

	// Remember that this was the most recently saved player.
	Files::Write(Files::Config() + "recent.txt", filePath + '\n');

	if(filePath.rfind(".txt") == filePath.length() - 4)
	{
		// Only update the backups if this save will have a newer date.
		SavedGame saved(filePath);
		if(saved.GetDate() != date.ToString())
		{
			string root = filePath.substr(0, filePath.length() - 4);
			const int previousCount = Preferences::GetPreviousSaveCount();
			const string rootPrevious = root + "~~previous-";
			for(int i = previousCount - 1; i > 0; --i)
			{
				const string toMove = rootPrevious + to_string(i) + ".txt";
				if(Files::Exists(toMove))
					Files::Move(toMove, rootPrevious + to_string(i + 1) + ".txt");
			}
			if(Files::Exists(filePath))
				Files::Move(filePath, rootPrevious + "1.txt");
			if(planet->HasServices())
				Save(rootPrevious + "spaceport.txt");
		}
	}

	Save(filePath);

	// Save global conditions:
	DataWriter globalConditions(Files::Config() + "global conditions.txt");
	GameData::GlobalConditions().Save(globalConditions);
}



// Get the base file name for the player, without the ".txt" extension. This
// will usually be "<first> <last>", but may be different if multiple players
// exist with the same name, in which case a number is appended.
string PlayerInfo::Identifier() const
{
	string name = Files::Name(filePath);
	return (name.length() < 4) ? "" : name.substr(0, name.length() - 4);
}



void PlayerInfo::StartTransaction()
{
	assert(!transactionSnapshot && "Starting PlayerInfo transaction while one is already active");

	// Create in-memory DataWriter and save to it.
	transactionSnapshot = new DataWriter();
	Save(*transactionSnapshot);
}



void PlayerInfo::FinishTransaction()
{
	assert(transactionSnapshot && "Finishing PlayerInfo while one hasn't been started");
	delete transactionSnapshot;
	transactionSnapshot = nullptr;
}



// Apply the given set of changes to the game data.
void PlayerInfo::AddChanges(list<DataNode> &changes)
{
	bool changedSystems = false;
	for(const DataNode &change : changes)
	{
		changedSystems |= (change.Token(0) == "system");
		changedSystems |= (change.Token(0) == "link");
		changedSystems |= (change.Token(0) == "unlink");
		GameData::Change(change);
	}
	if(changedSystems)
	{
		// Recalculate what systems have been seen.
		GameData::UpdateSystems();
		seen.clear();
		for(const System *system : visitedSystems)
		{
			seen.insert(system);
			for(const System *neighbor : system->VisibleNeighbors())
				if(!neighbor->Hidden() || system->Links().count(neighbor))
					seen.insert(neighbor);
		}
	}

	// Only move the changes into my list if they are not already there.
	if(&changes != &dataChanges)
		dataChanges.splice(dataChanges.end(), changes);
}



// Add an event that will happen at the given date.
void PlayerInfo::AddEvent(const GameEvent &event, const Date &date)
{
	// Check if the event should be applied directly.
	if(date <= this->date)
	{
		GameEvent eventCopy = event;
		list<DataNode> eventChanges = {eventCopy.Apply(*this)};
		if(!eventChanges.empty())
			AddChanges(eventChanges);
	}
	else
	{
		gameEvents.push_back(event);
		gameEvents.back().SetDate(date);
	}
}



// Mark this player as dead, and handle the changes to the player's fleet.
void PlayerInfo::Die(int response, const shared_ptr<Ship> &capturer)
{
	isDead = true;
	// The player loses access to all their ships if they die on a planet.
	if(GetPlanet() || !flagship)
		ships.clear();
	// If the flagship should explode due to choices made in a mission's
	// conversation, it should still appear in the player's ship list (but
	// will be red, because it is dead). The player's escorts will scatter
	// automatically, as they have a now-dead parent.
	else if(response == Conversation::EXPLODE)
		flagship->Destroy();
	// If it died in open combat, it is already marked destroyed.
	else if(!flagship->IsDestroyed())
	{
		// The player died due to the failed capture of an NPC or a
		// "mutiny". The flagship is either captured or changes government.
		if(!flagship->IsYours())
		{
			// The flagship was already captured, via BoardingPanel,
			// and its parent-escort relationships were updated in
			// Ship::WasCaptured().
		}
		// The referenced ship may not be boarded by the player, so before
		// letting it capture the flagship it must be near the flagship.
		else if(capturer && capturer->Position().Distance(flagship->Position()) <= 1.)
			flagship->WasCaptured(capturer);
		else
		{
			// A "mutiny" occurred.
			flagship->SetIsYours(false);
			// TODO: perhaps allow missions to set the new government.
			flagship->SetGovernment(GameData::Governments().Get("Independent"));
			// Your escorts do not follow it, nor does it wait for them.
			for(const shared_ptr<Ship> &ship : ships)
				ship->SetParent(nullptr);
		}
		// Remove the flagship from the player's ship list.
		auto it = find(ships.begin(), ships.end(), flagship);
		if(it != ships.end())
			ships.erase(it);
	}
}



// Query whether this player is dead.
bool PlayerInfo::IsDead() const
{
	return isDead;
}



// Get the player's first name.
const string &PlayerInfo::FirstName() const
{
	return firstName;
}



// Get the player's last name.
const string &PlayerInfo::LastName() const
{
	return lastName;
}



// Set the player's name. This will also set the saved game file name.
void PlayerInfo::SetName(const string &first, const string &last)
{
	firstName = first;
	lastName = last;

	string fileName = first + " " + last;

	// If there are multiple pilots with the same name, append a number to the
	// pilot name to generate a unique file name.
	filePath = Files::Saves() + fileName;
	int index = 0;
	while(true)
	{
		string path = filePath;
		if(index++)
			path += " " + to_string(index);
		path += ".txt";

		if(!Files::Exists(path))
		{
			filePath.swap(path);
			break;
		}
	}
}



// Get the current date (game world, not real world).
const Date &PlayerInfo::GetDate() const
{
	return date;
}



// Set the date to the next day, and perform all daily actions.
void PlayerInfo::IncrementDate()
{
	++date;

	// Check if any special events should happen today.
	auto it = gameEvents.begin();
	list<DataNode> eventChanges;
	while(it != gameEvents.end())
	{
		if(date < it->GetDate())
			++it;
		else
		{
			eventChanges.splice(eventChanges.end(), it->Apply(*this));
			it = gameEvents.erase(it);
		}
	}
	if(!eventChanges.empty())
		AddChanges(eventChanges);

	// Check if any missions have failed because of deadlines and
	// do any daily mission actions for those that have not failed.
	for(Mission &mission : missions)
	{
		if(mission.CheckDeadline(date) && mission.IsVisible())
			Messages::Add("You failed to meet the deadline for the mission \"" + mission.Name() + "\".",
				Messages::Importance::Highest);
		if(!mission.IsFailed(*this))
			mission.Do(Mission::DAILY, *this);
	}

	// Check what salaries and tribute the player receives.
	int64_t salariesIncome = accounts.SalariesIncomeTotal();
	int64_t tributeIncome = GetTributeTotal();
	FleetBalance b = MaintenanceAndReturns();
	if(salariesIncome || tributeIncome || b.assetsReturns)
	{
		string message = "You receive ";
		if(salariesIncome)
			message += Format::CreditString(salariesIncome) + " salary";
		if(salariesIncome && tributeIncome)
		{
			if(b.assetsReturns)
				message += ", ";
			else
				message += " and ";
		}
		if(tributeIncome)
			message += Format::CreditString(tributeIncome) + " in tribute";
		if(salariesIncome && tributeIncome && b.assetsReturns)
			message += ",";
		if((salariesIncome || tributeIncome) && b.assetsReturns)
			message += " and ";
		if(b.assetsReturns)
			message += Format::CreditString(b.assetsReturns) + " based on outfits and ships";
		message += ".";
		Messages::Add(message, Messages::Importance::High);
		accounts.AddCredits(salariesIncome + tributeIncome + b.assetsReturns);
	}

	// For accounting, keep track of the player's net worth. This is for
	// calculation of yearly income to determine maximum mortgage amounts.
	int64_t assets = depreciation.Value(ships, date.DaysSinceEpoch());
	for(const shared_ptr<Ship> &ship : ships)
		assets += ship->Cargo().Value(system);

	// Have the player pay salaries, mortgages, etc. and print a message that
	// summarizes the payments that were made.
	string message = accounts.Step(assets, Salaries(), b.maintenanceCosts);
	if(!message.empty())
		Messages::Add(message, Messages::Importance::High);

	// Reset the reload counters for all your ships.
	for(const shared_ptr<Ship> &ship : ships)
		ship->GetArmament().ReloadAll();
}



const CoreStartData &PlayerInfo::StartData() const noexcept
{
	return startData;
}



void PlayerInfo::SetSystemEntry(SystemEntry entryType)
{
	entry = entryType;
}



SystemEntry PlayerInfo::GetSystemEntry() const
{
	return entry;
}



// Set the player's current start system, and mark that system as visited.
void PlayerInfo::SetSystem(const System &system)
{
	this->previousSystem = this->system;
	this->system = &system;
	Visit(system);
}



// Get the player's current star system.
const System *PlayerInfo::GetSystem() const
{
	return system;
}



const System *PlayerInfo::GetPreviousSystem() const
{
	return previousSystem;
}



// Set the planet the player is landed on.
void PlayerInfo::SetPlanet(const Planet *planet)
{
	this->planet = planet;
}



// Get the planet the player is landed on.
const Planet *PlayerInfo::GetPlanet() const
{
	return planet;
}



// If the player is landed, return the stellar object they are on. Some planets
// (e.g. ringworlds) may include multiple stellar objects in the same system.
const StellarObject *PlayerInfo::GetStellarObject() const
{
	if(!system || !planet)
		return nullptr;

	double closestDistance = numeric_limits<double>::infinity();
	const StellarObject *closestObject = nullptr;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet() == planet)
		{
			if(!Flagship())
				return &object;

			double distance = Flagship()->Position().Distance(object.Position());
			if(distance < closestDistance)
			{
				closestDistance = distance;
				closestObject = &object;
			}
		}
	return closestObject;
}



// Check if the player must take off immediately.
bool PlayerInfo::ShouldLaunch() const
{
	return shouldLaunch;
}



// Access the player's account information.
const Account &PlayerInfo::Accounts() const
{
	return accounts;
}



// Access the player's account information (and allow modifying it).
Account &PlayerInfo::Accounts()
{
	return accounts;
}



// Calculate how much the player pays in daily salaries.
int64_t PlayerInfo::Salaries() const
{
	// Don't count extra crew on anything but the flagship.
	int64_t crew = 0;
	const Ship *flagship = Flagship();
	if(flagship)
		crew = flagship->Crew() - flagship->RequiredCrew();

	// A ship that is "parked" remains on a planet and requires no salaries.
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && !ship->IsDestroyed())
			crew += ship->RequiredCrew();
	if(!crew)
		return 0;

	// Every crew member except the player receives 100 credits per day.
	return 100 * (crew - 1);
}



// Calculate the daily maintenance cost and generated income for all ships and in cargo outfits.
PlayerInfo::FleetBalance PlayerInfo::MaintenanceAndReturns() const
{
	FleetBalance b;

	// If the player is landed, then cargo will be in the player's
	// pooled cargo. Check there so that the bank panel can display the
	// correct total maintenance costs. When launched all cargo will be
	// in the player's ships instead of in the pooled cargo, so no outfit
	// will be counted twice.
	for(const auto &outfit : Cargo().Outfits())
	{
		b.maintenanceCosts += max<int64_t>(0, outfit.first->Get("maintenance costs")) * outfit.second;
		b.assetsReturns += max<int64_t>(0, outfit.first->Get("income")) * outfit.second;
	}
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsDestroyed())
		{
			b.maintenanceCosts += max<int64_t>(0, ship->Attributes().Get("maintenance costs"));
			b.assetsReturns += max<int64_t>(0, ship->Attributes().Get("income"));
			for(const auto &outfit : ship->Cargo().Outfits())
			{
				b.maintenanceCosts += max<int64_t>(0, outfit.first->Get("maintenance costs")) * outfit.second;
				b.assetsReturns += max<int64_t>(0, outfit.first->Get("income")) * outfit.second;
			}
			if(!ship->IsParked())
			{
				b.maintenanceCosts += max<int64_t>(0, ship->Attributes().Get("operating costs"));
				b.assetsReturns += max<int64_t>(0, ship->Attributes().Get("operating income"));
			}
		}
	return b;
}



void PlayerInfo::AddLicense(const string &name)
{
	licenses.insert(name);
}



void PlayerInfo::RemoveLicense(const string &name)
{
	licenses.erase(name);
}



bool PlayerInfo::HasLicense(const string &name) const
{
	// TODO: This should be changed to use std::set<>::contains when we move to C++20.
	return licenses.count(name);
}



const set<string> &PlayerInfo::Licenses() const
{
	return licenses;
}



// Get a pointer to the ship that the player controls. This is usually the first
// ship in the list.
const Ship *PlayerInfo::Flagship() const
{
	return const_cast<PlayerInfo *>(this)->FlagshipPtr().get();
}



// Get a pointer to the ship that the player controls. This is usually the first
// ship in the list.
Ship *PlayerInfo::Flagship()
{
	return FlagshipPtr().get();
}



// Determine which ship is the flagship and return the shared pointer to it.
const shared_ptr<Ship> &PlayerInfo::FlagshipPtr()
{
	if(!flagship)
	{
		bool clearance = false;
		if(planet)
			clearance = planet->CanLand() || HasClearance(*this, planet);
		for(const shared_ptr<Ship> &it : ships)
		{
			if(it->IsParked())
				continue;
			if(it->GetSystem() != system)
				continue;
			if(!it->CanBeFlagship())
				continue;
			const bool sameLocation = !planet || it->GetPlanet() == planet;
			if(sameLocation || (clearance && !it->GetPlanet() && planet->IsAccessible(it.get())))
			{
				flagship = it;
				break;
			}
		}
	}

	static const shared_ptr<Ship> empty;
	return (flagship && flagship->IsYours()) ? flagship : empty;
}



// Set the flagship (on departure or during flight).
void PlayerInfo::SetFlagship(Ship &other)
{
	// Remove active data in the old flagship.
	if(flagship && flagship.get() != &other)
		flagship->ClearTargetsAndOrders();

	// Set the new flagship pointer.
	flagship = other.shared_from_this();

	// Make sure your ships all know who the flagship is.
	for(const shared_ptr<Ship> &ship : ships)
	{
		bool shouldFollowFlagship = (ship != flagship && !ship->IsParked());
		ship->SetParent(shouldFollowFlagship ? flagship : shared_ptr<Ship>());
	}

	// Move the flagship to the beginning to the list of ships.
	MoveFlagshipBegin(ships, flagship);

	// Make sure your flagship is not included in the escort selection.
	for(auto it = selectedShips.begin(); it != selectedShips.end(); )
	{
		shared_ptr<Ship> ship = it->lock();
		if(!ship || ship == flagship)
			it = selectedShips.erase(it);
		else
			++it;
	}
}



// Access the full list of ships that the player owns.
const vector<shared_ptr<Ship>> &PlayerInfo::Ships() const
{
	return ships;
}



// Inspect the flightworthiness of the player's active fleet, individually and
// as a whole, to determine which ships cannot travel with the group.
// Returns a mapping of ships to the reason their flight check failed.
map<const shared_ptr<Ship>, vector<string>> PlayerInfo::FlightCheck() const
{
	// Count of all bay types in the active fleet.
	auto bayCount = map<string, size_t>{};
	// Classification of the present ships by category. Parked ships are ignored.
	auto categoryCount = map<string, vector<shared_ptr<Ship>>>{};

	auto flightChecks = map<const shared_ptr<Ship>, vector<string>>{};
	for(const auto &ship : ships)
		if(ship->GetSystem() && !ship->IsDisabled() && !ship->IsParked())
		{
			auto checks = ship->FlightCheck();
			if(!checks.empty())
				flightChecks.emplace(ship, checks);

			// Only check bays for in-system ships.
			if(ship->GetSystem() != system)
				continue;

			categoryCount[ship->Attributes().Category()].emplace_back(ship);
			// Ensure bayCount has an entry for this category for the special case
			// where we have no bays at all available for this type of ship.
			if(ship->CanBeCarried())
				bayCount.emplace(ship->Attributes().Category(), 0);

			if(ship->CanBeCarried() || !ship->HasBays())
				continue;

			for(auto &bay : ship->Bays())
			{
				++bayCount[bay.category];
				// The bays should always be empty. But if not, count that ship too.
				if(bay.ship)
				{
					Logger::LogError("Expected bay to be empty for " + ship->TrueModelName() + ": " + ship->Name());
					categoryCount[bay.ship->Attributes().Category()].emplace_back(bay.ship);
				}
			}
		}

	// Identify transportable ships that cannot jump and have no bay to be carried in.
	for(auto &bayType : bayCount)
	{
		const auto &shipsOfType = categoryCount[bayType.first];
		if(shipsOfType.empty())
			continue;
		for(const auto &carriable : shipsOfType)
		{
			if(carriable->JumpsRemaining() != 0)
			{
				// This ship can travel between systems and does not require a bay.
			}
			// This ship requires a bay to travel between systems.
			else if(bayType.second > 0)
				--bayType.second;
			else
			{
				// Include the lack of bay availability amongst any other
				// warnings for this carriable ship.
				auto it = flightChecks.find(carriable);
				string warning = "no bays?";
				if(it != flightChecks.end())
					it->second.emplace_back(warning);
				else
					flightChecks.emplace(carriable, vector<string>{warning});
			}
		}
	}
	return flightChecks;
}



// Add a captured ship to your fleet.
void PlayerInfo::AddShip(const shared_ptr<Ship> &ship)
{
	ships.push_back(ship);
	ship->SetIsSpecial();
	ship->SetIsYours();
	if(ship->HasBays())
		displayCarrierHelp = true;
}



// Adds a ship of the given model with the given name to the player's fleet.
void PlayerInfo::BuyShip(const Ship *model, const string &name)
{
	if(!model)
		return;

	int day = date.DaysSinceEpoch();
	int64_t cost = stockDepreciation.Value(*model, day);
	if(accounts.Credits() >= cost)
	{
		AddStockShip(model, name);

		accounts.AddCredits(-cost);
		flagship.reset();

		depreciation.Buy(*model, day, &stockDepreciation);
		for(const auto &it : model->Outfits())
			stock[it.first] -= it.second;

		if(ships.back()->HasBays())
			displayCarrierHelp = true;
	}
}



// Because this ship is being gifted, it costs nothing and starts fully depreciated.
const Ship *PlayerInfo::GiftShip(const Ship *model, const string &name, const string &id)
{
	if(!model)
		return nullptr;

	AddStockShip(model, name);

	flagship.reset();

	// If an id was given, associate and store it with the UUID of the gifted ship.
	if(!id.empty())
		giftedShips[id].clone(ships.back()->UUID());

	return ships.back().get();
}



// Sell the given ship (if it belongs to the player).
void PlayerInfo::SellShip(const Ship *selected, bool storeOutfits)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			int day = date.DaysSinceEpoch();
			int64_t cost;

			// Passing a pointer to Value gets only the hull cost. Passing a reference
			// gets hull and outfit costs.
			if(storeOutfits)
				cost = depreciation.Value(selected, day);
			else
				cost = depreciation.Value(*selected, day);

			// Record the transfer of this ship in the depreciation and stock info.
			stockDepreciation.Buy(*selected, day, &depreciation, storeOutfits);
			if(storeOutfits)
			{
				CargoHold &storage = Storage();
				for(const auto &it : selected->Outfits())
					storage.Add(it.first, it.second);
			}
			else
			{
				for(const auto &it : selected->Outfits())
					stock[it.first] += it.second;
			}

			accounts.AddCredits(cost);
			ForgetGiftedShip(*it->get());
			ships.erase(it);
			flagship.reset();
			break;
		}
}



// Take the ship from the player, if a model is specified this will permanently remove outfits in said model,
// instead of allowing the player to buy them back by putting them in the stock.
void PlayerInfo::TakeShip(const Ship *shipToTake, const Ship *model, bool takeOutfits)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == shipToTake)
		{
			// Record the transfer of this ship in the depreciation and stock info.
			stockDepreciation.Buy(*shipToTake, date.DaysSinceEpoch(), &depreciation);
			if(takeOutfits)
				for(const auto &it : shipToTake->Outfits())
				{
					// We only take all of the outfits specified in the model without putting them in the stock.
					// The extra outfits of this ship are transfered into the stock.
					int amountToTake = 0;
					if(model)
					{
						auto outfit = model->Outfits().find(it.first);
						if(outfit != model->Outfits().end())
							amountToTake = max(it.second, outfit->second);
					}
					stock[it.first] += it.second - amountToTake;
				}
			ForgetGiftedShip(*it->get(), false);
			ships.erase(it);
			flagship.reset();
			break;
		}
}



vector<shared_ptr<Ship>>::iterator PlayerInfo::DisownShip(const Ship *selected)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			flagship.reset();
			ForgetGiftedShip(*it->get());
			it = ships.erase(it);
			return (it == ships.begin()) ? it : --it;
		}
	return ships.begin();
}



// Park or unpark the given ship. A parked ship remains on a planet instead of
// flying with the player, and requires no daily crew payments.
void PlayerInfo::ParkShip(const Ship *selected, bool isParked)
{
	for(auto &ship : ships)
		if(ship.get() == selected)
		{
			isParked &= !ship->IsDisabled();
			ship->SetIsParked(isParked);
			UpdateCargoCapacities();
			flagship.reset();
			return;
		}
}



// Rename the given ship.
void PlayerInfo::RenameShip(const Ship *selected, const string &name)
{
	for(auto &ship : ships)
		if(ship.get() == selected)
		{
			ship->SetName(name);
			return;
		}
}



// Change the order of the given ship in the list.
void PlayerInfo::ReorderShip(int fromIndex, int toIndex)
{
	// Make sure the indices are valid.
	if(fromIndex == toIndex)
		return;
	if(static_cast<unsigned>(fromIndex) >= ships.size())
		return;
	if(static_cast<unsigned>(toIndex) >= ships.size())
		return;

	// Reorder the list.
	shared_ptr<Ship> ship = ships[fromIndex];
	ships.erase(ships.begin() + fromIndex);
	ships.insert(ships.begin() + toIndex, ship);
	flagship.reset();
}



void PlayerInfo::SetShipOrder(const vector<shared_ptr<Ship>> &newOrder)
{
	// Check if the incoming vector contains the same elements
	if(std::is_permutation(ships.begin(), ships.end(), newOrder.begin()))
	{
		ships = newOrder;
		flagship.reset();
	}
	else
		throw runtime_error("Cannot reorder ships because the new order does not contain the same ships");
}



// Find out how attractive the player's fleet is to pirates. Aside from a
// heavy freighter, no single ship should attract extra pirate attention.
pair<double, double> PlayerInfo::RaidFleetFactors() const
{
	double attraction = 0.;
	double deterrence = 0.;
	for(const shared_ptr<Ship> &ship : Ships())
	{
		if(ship->IsParked() || ship->IsDestroyed())
			continue;

		attraction += ship->Attraction();
		deterrence += ship->Deterrence();
	}

	return make_pair(attraction, deterrence);
}



double PlayerInfo::RaidFleetAttraction(const RaidFleet &raid, const System *system) const
{
	double attraction = 0.;
	const Fleet *raidFleet = raid.GetFleet();
	const Government *raidGov = raidFleet ? raidFleet->GetGovernment() : nullptr;
	if(raidGov && raidGov->IsEnemy())
	{
		// The player's base attraction to a fleet is determined by their fleet attraction minus
		// their fleet deterrence, minus whatever the minimum attraction of this raid fleet is.
		pair<double, double> factors = RaidFleetFactors();
		// If there is a maximum attraction for this fleet, and we are above it, it will not spawn.
		if(raid.MaxAttraction() > 0 && factors.first > raid.MaxAttraction())
			return 0;

		attraction = .005 * (factors.first - factors.second - raid.MinAttraction());
		// Then we consider the strength of other fleets in the system.
		int64_t raidStrength = raidFleet->Strength();
		if(system && raidStrength)
			for(const auto &fleet : system->Fleets())
			{
				const Government *gov = fleet.Get()->GetGovernment();
				if(gov)
				{
					// If this fleet is neutral or hostile to both the player and raid fleet, it has
					// no impact on the attraction. If the fleet is hostile to only the player, the
					// raid attraction will increase. If the fleet is hostile to only the raid fleet,
					// the raid attraction will decrease. The amount of increase or decrease is determined
					// by the strength of the fleet relative to the raid fleet. System fleets which are
					// stronger have a larger impact.
					double strength = fleet.Get()->Strength() / fleet.Period();
					attraction -= (gov->IsEnemy(raidGov) - gov->IsEnemy()) * (strength / raidStrength);
				}
			}
	}
	return max(0., min(1., attraction));
}



// Get cargo information.
CargoHold &PlayerInfo::Cargo()
{
	return cargo;
}



// Get cargo information.
const CargoHold &PlayerInfo::Cargo() const
{
	return cargo;
}



// Get items stored on the player's current planet.
CargoHold &PlayerInfo::Storage()
{
	assert(planet && "can't get planetary storage in-flight");
	return planetaryStorage[planet];
}



// Get planetary storage information for all planets (for map and overviews).
const std::map<const Planet *, CargoHold> &PlayerInfo::PlanetaryStorage() const
{
	return planetaryStorage;
}



// Adjust the cost basis for the given commodity.
void PlayerInfo::AdjustBasis(const string &commodity, int64_t adjustment)
{
	costBasis[commodity] += adjustment;
}



// Get the cost basis for some number of tons of the given commodity. Each ton
// of the commodity that you own is assumed to have the same basis.
int64_t PlayerInfo::GetBasis(const string &commodity, int tons) const
{
	// Handle cost basis even when not landed on a planet.
	int total = cargo.Get(commodity);
	for(const auto &ship : ships)
		total += ship->Cargo().Get(commodity);
	if(!total)
		return 0;

	auto it = costBasis.find(commodity);
	int64_t basis = (it == costBasis.end()) ? 0 : it->second;
	return (basis * tons) / total;
}



// Switch cargo from being stored in ships to being stored here. Also recharge
// ships, check for mission completion, and apply fines for contraband.
void PlayerInfo::Land(UI *ui)
{
	// This can only be done while landed.
	if(!system || !planet)
		return;

	if(!freshlyLoaded)
	{
		Audio::Play(Audio::Get("landing"));
		Audio::PlayMusic(planet->MusicName());
	}

	// Mark this planet as visited.
	Visit(*planet);
	if(planet == travelDestination)
		travelDestination = nullptr;

	// Remove any ships that have been destroyed or captured.
	map<string, int> lostCargo;
	vector<shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if((*it)->IsDestroyed() || !(*it)->IsYours())
		{
			// If any of your ships are destroyed, your cargo "cost basis" should
			// be adjusted based on what you lost.
			for(const auto &cargo : (*it)->Cargo().Commodities())
				if(cargo.second)
					lostCargo[cargo.first] += cargo.second;
			// Also, the ship and everything in it should be removed from your
			// depreciation records. Transfer it to a throw-away record:
			Depreciation().Buy(**it, date.DaysSinceEpoch(), &depreciation);

			ForgetGiftedShip(*it->get());
			it = ships.erase(it);
		}
		else
			++it;
	}

	// "Unload" all fighters, so they will get recharged, etc.
	for(const shared_ptr<Ship> &ship : ships)
		ship->UnloadBays();

	// Ships that are landed with you on the planet should fully recharge.
	// Those in remote systems restore what they can without landing.
	bool clearance = HasClearance(*this, planet);
	const bool canUseServices = planet->CanUseServices();
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && !ship->IsDisabled())
		{
			if(ship->GetSystem() == system)
			{
				const bool alreadyLanded = ship->GetPlanet() == planet;
				if(alreadyLanded || planet->CanLand(*ship) || (clearance && planet->IsAccessible(ship.get())))
				{
					ship->Recharge(canUseServices ? planet->GetPort().GetRecharges() : Port::RechargeType::None,
						planet->GetPort().HasService(Port::ServicesType::HireCrew));
					if(!ship->GetPlanet())
						ship->SetPlanet(planet);
				}
				// Ships that cannot land with the flagship choose the most suitable planet
				// in the system.
				else
				{
					const StellarObject *landingObject = AI::FindLandingLocation(*ship);
					const bool foundSpaceport = landingObject;
					if(!landingObject)
						landingObject = AI::FindLandingLocation(*ship, false);
					if(landingObject)
						ship->SetPlanet(landingObject->GetPlanet());
					ship->Recharge(foundSpaceport);
				}
			}
			else
				ship->Recharge(Port::RechargeType::None, false);
		}

	// Cargo management needs to be done after updating ship locations (above).
	UpdateCargoCapacities();
	// If the player is actually landing (rather than simply loading the game),
	// new fines may be levied.
	if(!freshlyLoaded)
		Fine(ui);
	// Ships that are landed with you on the planet should pool all their cargo together.
	PoolCargo();
	// Adjust cargo cost basis for any cargo lost due to a ship being destroyed.
	for(const auto &it : lostCargo)
		AdjustBasis(it.first, -(costBasis[it.first] * it.second) / (cargo.Get(it.first) + it.second));

	// Evaluate changes to NPC spawning criteria.
	if(!freshlyLoaded)
		UpdateMissionNPCs();

	// Update missions that are completed, or should be failed.
	StepMissions(ui);
	UpdateCargoCapacities();

	// Create whatever missions this planet has to offer.
	if(!freshlyLoaded)
		CreateMissions();
	// Upon loading the game, prompt the player about any paused missions, but if there are many
	// do not name them all (since this would overflow the screen).
	else if(ui && !inactiveMissions.empty())
	{
		string message = "These active missions or jobs were deactivated due to a missing definition"
			" - perhaps you recently removed a plugin?\n";
		auto mit = inactiveMissions.rbegin();
		int named = 0;
		while(mit != inactiveMissions.rend() && (++named < 10))
		{
			message += "\t\"" + mit->Name() + "\"\n";
			++mit;
		}
		if(mit != inactiveMissions.rend())
			message += " and " + to_string(distance(mit, inactiveMissions.rend())) + " more.\n";
		message += "They will be reactivated when the necessary plugin is reinstalled.";
		ui->Push(new Dialog(message));
	}

	// Hire extra crew back if any were lost in-flight (i.e. boarding) or
	// some bunks were freed up upon landing (i.e. completed missions).
	if(Preferences::Has("Rehire extra crew when lost")
			&& (planet->GetPort().HasService(Port::ServicesType::HireCrew) && canUseServices) && flagship)
	{
		int added = desiredCrew - flagship->Crew();
		if(added > 0)
		{
			flagship->AddCrew(added);
			Messages::Add("You hire " + to_string(added) + (added == 1
					? " extra crew member to fill your now-empty bunk."
					: " extra crew members to fill your now-empty bunks."), Messages::Importance::High);
		}
	}

	freshlyLoaded = false;
	flagship.reset();
}



// Load the cargo back into your ships. This may require selling excess, in
// which case a message will be returned.
bool PlayerInfo::TakeOff(UI *ui, const bool distributeCargo)
{
	// This can only be done while landed.
	if(!system || !planet)
		return false;

	flagship = FlagshipPtr();
	if(!flagship)
		return false;

	shouldLaunch = false;
	Audio::Play(Audio::Get("takeoff"));

	// Jobs are only available when you are landed.
	availableJobs.clear();
	availableMissions.clear();
	doneMissions.clear();
	stock.clear();

	// Special persons who appeared last time you left the planet, can appear again.
	GameData::ResetPersons();

	// Store the total cargo counts in case we need to adjust cost bases below.
	map<string, int> originalTotals = cargo.Commodities();

	// Move the flagship to the start of the list of ships and ensure that all
	// escorts know which ship is acting as flagship.
	SetFlagship(*flagship);

	// Recharge any ships that can be recharged, and load available cargo.
	const bool canUseServices = planet->CanUseServices();
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && !ship->IsDisabled())
		{
			// Recalculate the weapon cache in case a mass-less change had an effect.
			ship->GetAICache().Calibrate(*ship.get());
			if(ship->GetSystem() != system)
			{
				ship->Recharge(Port::RechargeType::None, false);
				continue;
			}
			else
				ship->Recharge(canUseServices ? planet->GetPort().GetRecharges() : Port::RechargeType::None,
					planet->GetPort().HasService(Port::ServicesType::HireCrew));
		}

	if(distributeCargo)
		DistributeCargo();

	if(cargo.Passengers())
	{
		int extra = min(cargo.Passengers(), flagship->Crew() - flagship->RequiredCrew());
		if(extra)
		{
			flagship->AddCrew(-extra);
			if(extra == 1)
				Messages::Add("You fired a crew member to free up a bunk for a passenger.", Messages::Importance::High);
			else
				Messages::Add("You fired " + to_string(extra) + " crew members to free up bunks for passengers.",
						Messages::Importance::High);
			flagship->Cargo().SetBunks(flagship->Attributes().Get("bunks") - flagship->Crew());
			cargo.TransferAll(flagship->Cargo());
		}
	}

	int extra = flagship->Crew() + flagship->Cargo().Passengers() - flagship->Attributes().Get("bunks");
	if(extra > 0)
	{
		flagship->AddCrew(-extra);
		if(extra == 1)
			Messages::Add("You fired a crew member because you have no bunk for them.", Messages::Importance::High);
		else
			Messages::Add("You fired " + to_string(extra) + " crew members because you have no bunks for them.",
					Messages::Importance::High);
		flagship->Cargo().SetBunks(flagship->Attributes().Get("bunks") - flagship->Crew());
	}

	// For each active, carriable ship you own, try to find an active ship that has a bay for it.
	auto carriers = vector<Ship *>{};
	auto toLoad = vector<shared_ptr<Ship>>{};
	for(auto &ship : ships)
		if(!ship->IsParked() && !ship->IsDisabled())
		{
			if(ship->CanBeCarried() && ship != flagship)
				toLoad.emplace_back(ship);
			else if(ship->HasBays())
				carriers.emplace_back(ship.get());
		}
	if(!toLoad.empty())
	{
		size_t uncarried = toLoad.size();
		if(!carriers.empty())
		{
			// Order carried ships such that those requiring bays are loaded first. For
			// jump-capable carried ships, prefer loading those with a shorter range.
			stable_sort(toLoad.begin(), toLoad.end(),
				[](const shared_ptr<Ship> &a, const shared_ptr<Ship> &b)
				{
					return a->JumpsRemaining() < b->JumpsRemaining();
				});
			// We are guaranteed that each carried `ship` is not parked and not disabled, and that
			// all possible parents are also not parked, not disabled, and not `ship`.
			for(auto &ship : toLoad)
				for(auto &parent : carriers)
					if(parent->GetSystem() == ship->GetSystem() && parent->Carry(ship))
					{
						--uncarried;
						break;
					}
		}

		if(uncarried)
		{
			// The remaining uncarried ships are launched alongside the player.
			string message = (uncarried > 1) ? "Some escorts were" : "One escort was";
			Messages::Add(message + " unable to dock with a carrier.", Messages::Importance::High);
		}
	}

	// By now, all cargo should have been divvied up among your ships. So, any
	// mission cargo or passengers left behind cannot be carried, and those
	// missions have aborted.
	vector<const Mission *> missionsToRemove;
	for(const auto &it : cargo.MissionCargo())
		if(it.second)
		{
			if(it.first->IsVisible())
				Messages::Add("Mission \"" + it.first->Name()
					+ "\" aborted because you do not have space for the cargo."
						, Messages::Importance::Highest);
			missionsToRemove.push_back(it.first);
		}
	for(const auto &it : cargo.PassengerList())
		if(it.second)
		{
			if(it.first->IsVisible())
				Messages::Add("Mission \"" + it.first->Name()
					+ "\" aborted because you do not have enough passenger bunks free."
						, Messages::Importance::Highest);
			missionsToRemove.push_back(it.first);

		}
	for(const Mission *mission : missionsToRemove)
		RemoveMission(Mission::ABORT, *mission, ui);

	// Any ordinary cargo left behind can be sold.
	int64_t income = 0;
	int day = date.DaysSinceEpoch();
	int64_t sold = cargo.Used();
	int64_t totalBasis = 0;
	if(sold)
	{
		for(const auto &commodity : cargo.Commodities())
		{
			if(!commodity.second)
				continue;

			// Figure out how much income you get for selling this cargo.
			int64_t value = commodity.second * static_cast<int64_t>(system->Trade(commodity.first));
			income += value;

			int original = originalTotals[commodity.first];
			auto it = costBasis.find(commodity.first);
			if(!original || it == costBasis.end() || !it->second)
				continue;

			// Now, figure out how much of that income is profit by calculating
			// the cost basis for this cargo (which is just the total cost basis
			// multiplied by the percent of the cargo you are selling).
			int64_t basis = it->second * commodity.second / original;
			it->second -= basis;
			totalBasis += basis;
		}
		if(!planet->HasOutfitter())
			for(const auto &outfit : cargo.Outfits())
			{
				// Compute the total value for each type of excess outfit.
				if(!outfit.second)
					continue;
				int64_t cost = depreciation.Value(outfit.first, day, outfit.second);
				for(int i = 0; i < outfit.second; ++i)
					stockDepreciation.Buy(outfit.first, day, &depreciation);
				income += cost;
			}
		else
			for(const auto &outfit : cargo.Outfits())
			{
				// Transfer the outfits from cargo to the storage on this planet.
				if(!outfit.second)
					continue;
				cargo.Transfer(outfit.first, outfit.second, Storage());
			}
	}
	accounts.AddCredits(income);
	cargo.Clear();
	stockDepreciation = Depreciation();
	if(sold)
	{
		// Report how much excess cargo was sold, and what profit you earned.
		ostringstream out;
		out << "You sold " << Format::CargoString(sold, "excess cargo") << " for " << Format::CreditString(income);
		if(totalBasis && totalBasis != income)
			out << " (for a profit of " << Format::CreditString(income - totalBasis) << ").";
		else
			out << ".";
		Messages::Add(out.str(), Messages::Importance::High);
	}

	return true;
}



void PlayerInfo::PoolCargo()
{
	// This can only be done while landed.
	if(!planet)
		return;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetPlanet() == planet && !ship->IsParked())
			ship->Cargo().TransferAll(cargo);
}



const CargoHold &PlayerInfo::DistributeCargo()
{
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && !ship->IsDisabled() && ship->GetPlanet() == planet)
		{
			if(ship != flagship)
			{
				ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - ship->RequiredCrew());
				cargo.TransferAll(ship->Cargo());
			}
			else
			{
				// Your flagship takes first priority for passengers but last for cargo.
				desiredCrew = ship->Crew();
				ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - desiredCrew);
				for(const auto &it : cargo.PassengerList())
					cargo.TransferPassengers(it.first, it.second, ship->Cargo());
			}
		}
	// Load up your flagship last, so that it will have space free for any
	// plunder that you happen to acquire.
	cargo.TransferAll(flagship->Cargo());

	return cargo;
}



void PlayerInfo::AddPlayTime(chrono::nanoseconds timeVal)
{
	playTime += timeVal.count() * .000000001;
}



double PlayerInfo::GetPlayTime() const noexcept
{
	return playTime;
}



// Get the player's logbook.
const multimap<Date, string> &PlayerInfo::Logbook() const
{
	return logbook;
}



void PlayerInfo::AddLogEntry(const string &text)
{
	logbook.emplace(date, text);
}



const map<string, map<string, string>> &PlayerInfo::SpecialLogs() const
{
	return specialLogs;
}



void PlayerInfo::AddSpecialLog(const string &type, const string &name, const string &text)
{
	string &entry = specialLogs[type][name];
	if(!entry.empty())
		entry += "\n\t";
	entry += text;
}



bool PlayerInfo::HasLogs() const
{
	return !logbook.empty() || !specialLogs.empty();
}



// Call this after missions update, or if leaving the outfitter, shipyard, or
// hiring panel. Updates the information on how much space is available.
void PlayerInfo::UpdateCargoCapacities()
{
	int size = 0;
	int bunks = 0;
	flagship = FlagshipPtr();
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetPlanet() == planet && !ship->IsParked())
		{
			size += ship->Attributes().Get("cargo space");
			int crew = (ship == flagship ? ship->Crew() : ship->RequiredCrew());
			bunks += ship->Attributes().Get("bunks") - crew;
		}
	cargo.SetSize(size);
	cargo.SetBunks(bunks);
}



// Get the list of active missions.
const list<Mission> &PlayerInfo::Missions() const
{
	return missions;
}



// Get the list of ordinary jobs that are available on the job board.
const list<Mission> &PlayerInfo::AvailableJobs() const
{
	return availableJobs;
}



const PlayerInfo::SortType PlayerInfo::GetAvailableSortType() const
{
	return availableSortType;
}



void PlayerInfo::NextAvailableSortType()
{
	availableSortType = static_cast<SortType>((availableSortType + 1) % (CONVENIENT + 1));
	SortAvailable();
}



const bool PlayerInfo::ShouldSortAscending() const
{
	return availableSortAsc;
}



void PlayerInfo::ToggleSortAscending()
{
	availableSortAsc = !availableSortAsc;
	SortAvailable();
}



const bool PlayerInfo::ShouldSortSeparateDeadline() const
{
	return sortSeparateDeadline;
}



void PlayerInfo::ToggleSortSeparateDeadline()
{
	sortSeparateDeadline = !sortSeparateDeadline;
	SortAvailable();
}



const bool PlayerInfo::ShouldSortSeparatePossible() const
{
	return sortSeparatePossible;
}



void PlayerInfo::ToggleSortSeparatePossible()
{
	sortSeparatePossible = !sortSeparatePossible;
	SortAvailable();
}



// Return a pointer to the mission that was most recently accepted while in-flight.
const Mission *PlayerInfo::ActiveBoardingMission() const
{
	return activeBoardingMission;
}



// Update mission NPCs with the player's current conditions.
void PlayerInfo::UpdateMissionNPCs()
{
	for(Mission &mission : missions)
		mission.UpdateNPCs(*this);
}



// Accept the given job.
void PlayerInfo::AcceptJob(const Mission &mission, UI *ui)
{
	for(auto it = availableJobs.begin(); it != availableJobs.end(); ++it)
		if(&*it == &mission)
		{
			cargo.AddMissionCargo(&mission);
			it->Do(Mission::OFFER, *this);
			it->Do(Mission::ACCEPT, *this, ui);
			auto spliceIt = it->IsUnique() ? missions.begin() : missions.end();
			missions.splice(spliceIt, availableJobs, it);
			SortAvailable(); // Might not have cargo anymore, so some jobs can be sorted to end
			break;
		}
}



// Look at the list of available missions and see if any of them can be offered
// right now, in the given location (landing or spaceport). If there are no
// missions that can be accepted, return a null pointer.
Mission *PlayerInfo::MissionToOffer(Mission::Location location)
{
	if(ships.empty())
		return nullptr;

	// If a mission can be offered right now, move it to the start of the list
	// so we know what mission the callback is referring to, and return it.
	for(auto it = availableMissions.begin(); it != availableMissions.end(); ++it)
		if(it->IsAtLocation(location) && it->CanOffer(*this) && it->CanAccept(*this))
		{
			availableMissions.splice(availableMissions.begin(), availableMissions, it);
			return &availableMissions.front();
		}
	return nullptr;
}



// Check if any of the game's missions can be offered from this ship, given its
// relationship with the player. If none offer, return nullptr.
Mission *PlayerInfo::BoardingMission(const shared_ptr<Ship> &ship)
{
	// Do not create missions from existing mission NPC's, or the player's ships.
	if(ship->IsSpecial())
		return nullptr;
	// Ensure that boarding this NPC again does not create a mission.
	ship->SetIsSpecial();

	// "boardingMissions" is emptied by MissionCallback, but to be sure:
	boardingMissions.clear();

	Mission::Location location = (ship->GetGovernment()->IsEnemy()
			? Mission::BOARDING : Mission::ASSISTING);

	// Check for available boarding or assisting missions.
	for(const auto &it : GameData::Missions())
		if(it.second.IsAtLocation(location) && it.second.CanOffer(*this, ship))
		{
			boardingMissions.push_back(it.second.Instantiate(*this, ship));
			if(boardingMissions.back().IsFailed(*this))
				boardingMissions.pop_back();
			else
				return &boardingMissions.back();
		}

	return nullptr;
}



bool PlayerInfo::CaptureOverriden(const shared_ptr<Ship> &ship) const
{
	if(ship->IsCapturable())
		return false;
	// Check if there's a boarding mission being offered which allows this ship to be captured. If the boarding
	// mission was declined, then this results in one-time capture access to the ship. If it was accepted, then
	// the next boarding attempt will have the boarding mission in the player's active missions list, checked below.
	const Mission *mission = boardingMissions.empty() ? nullptr : &boardingMissions.back();
	// Otherwise, check if there's an already active mission which grants access. This allows trying to board the
	// ship again after accepting the mission.
	if(!mission)
		for(const Mission &mission : Missions())
			if(mission.OverridesCapture() && !mission.IsFailed(*this) && mission.SourceShip() == ship.get())
				return true;
	return mission && mission->OverridesCapture() && !mission->IsFailed(*this) && mission->SourceShip() == ship.get();
}



// Engine calls this after placing the boarding mission's NPCs.
void PlayerInfo::ClearActiveBoardingMission()
{
	activeBoardingMission = nullptr;
}



// If one of your missions cannot be offered because you do not have enough
// space for it, and it specifies a message to be shown in that situation,
// show that message.
void PlayerInfo::HandleBlockedMissions(Mission::Location location, UI *ui)
{
	list<Mission> &missionList = availableMissions.empty() ? boardingMissions : availableMissions;
	if(ships.empty() || missionList.empty())
		return;

	for(auto &it : missionList)
		if(it.IsAtLocation(location) && it.CanOffer(*this) && !it.CanAccept(*this))
		{
			string message = it.BlockedMessage(*this);
			if(!message.empty())
			{
				ui->Push(new Dialog(message));
				return;
			}
		}
}



// Callback for accepting or declining whatever mission has been offered.
// Responses which would kill the player are handled before the on offer
// conversation ended.
void PlayerInfo::MissionCallback(int response)
{
	list<Mission> &missionList = availableMissions.empty() ? boardingMissions : availableMissions;
	if(missionList.empty())
		return;

	Mission &mission = missionList.front();

	// If landed, this conversation may require the player to immediately depart.
	shouldLaunch |= (GetPlanet() && Conversation::RequiresLaunch(response));
	if(response == Conversation::ACCEPT || response == Conversation::LAUNCH)
	{
		bool shouldAutosave = mission.RecommendsAutosave();
		if(planet)
		{
			cargo.AddMissionCargo(&mission);
			UpdateCargoCapacities();
		}
		else if(Flagship())
			flagship->Cargo().AddMissionCargo(&mission);
		else
			return;

		// Move this mission from the offering list into the "accepted"
		// list, viewable on the MissionPanel. Unique missions are moved
		// to the front, so they appear at the top of the list if viewed.
		auto spliceIt = mission.IsUnique() ? missions.begin() : missions.end();
		missions.splice(spliceIt, missionList, missionList.begin());
		mission.Do(Mission::ACCEPT, *this);
		if(shouldAutosave)
			Autosave();
		// If this is a mission offered in-flight, expose a pointer to it
		// so Engine::SpawnFleets can add its ships without requiring the
		// player to land.
		if(mission.IsAtLocation(Mission::BOARDING) || mission.IsAtLocation(Mission::ASSISTING))
			activeBoardingMission = &*--spliceIt;
	}
	else if(response == Conversation::DECLINE || response == Conversation::FLEE)
	{
		mission.Do(Mission::DECLINE, *this);
		missionList.pop_front();
	}
	else if(response == Conversation::DEFER || response == Conversation::DEPART)
	{
		mission.Do(Mission::DEFER, *this);
		missionList.pop_front();
	}
}



// Basic callback, allowing conversations to force the player to depart from a
// planet without requiring a mission to offer.
void PlayerInfo::BasicCallback(int response)
{
	// If landed, this conversation may require the player to immediately depart.
	shouldLaunch |= (GetPlanet() && Conversation::RequiresLaunch(response));
}



// Mark a mission for removal, either because it was completed, or it failed,
// or because the player aborted it.
void PlayerInfo::RemoveMission(Mission::Trigger trigger, const Mission &mission, UI *ui)
{
	for(auto it = missions.begin(); it != missions.end(); ++it)
		if(&*it == &mission)
		{
			// Don't delete the mission yet, because the conversation or dialog
			// panel may still be showing. Instead, just mark it as done. Doing
			// this first avoids the possibility of an infinite loop, e.g. if a
			// mission's "on fail" fails the mission itself.
			doneMissions.splice(doneMissions.end(), missions, it);

			it->Do(trigger, *this, ui);
			cargo.RemoveMissionCargo(&mission);
			for(shared_ptr<Ship> &ship : ships)
				ship->Cargo().RemoveMissionCargo(&mission);
			return;
		}
}



// Mark a mission as failed, but do not remove it from the mission list yet.
void PlayerInfo::FailMission(const Mission &mission)
{
	for(auto &it : missions)
		if(&it == &mission)
		{
			it.Fail();
			return;
		}
}



// Update mission status based on an event.
void PlayerInfo::HandleEvent(const ShipEvent &event, UI *ui)
{
	// Combat rating increases when you disable an enemy ship.
	if(event.ActorGovernment() && event.ActorGovernment()->IsPlayer())
		if((event.Type() & ShipEvent::DISABLE) && event.Target() && !event.Target()->IsYours())
		{
			auto &rating = conditions["combat rating"];
			static const int64_t maxRating = 2000000000;
			rating = min(maxRating, rating + (event.Target()->Cost() + 250000) / 500000);
		}

	for(Mission &mission : missions)
		mission.Do(event, *this, ui);

	// If the player's flagship was destroyed, the player is dead.
	if((event.Type() & ShipEvent::DESTROY) && !ships.empty() && event.Target().get() == Flagship())
		Die();
}



// Get mutable access to the player's list of conditions.
ConditionsStore &PlayerInfo::Conditions()
{
	return conditions;
}



// Access the player's list of conditions.
const ConditionsStore &PlayerInfo::Conditions() const
{
	return conditions;
}



// Uuid for the gifted ships, with the ship class follow by the names they had when they were gifted to the player.
const map<string, EsUuid> &PlayerInfo::GiftedShips() const
{
	return giftedShips;
}



map<string, string> PlayerInfo::GetSubstitutions() const
{
	map<string, string> subs;
	GameData::GetTextReplacements().Substitutions(subs, Conditions());

	subs["<first>"] = FirstName();
	subs["<last>"] = LastName();
	if(Flagship())
		subs["<ship>"] = Flagship()->Name();

	subs["<system>"] = GetSystem()->Name();
	subs["<date>"] = GetDate().ToString();
	subs["<day>"] = GetDate().LongString();
	return subs;
}



bool PlayerInfo::SetTribute(const Planet *planet, int64_t payment)
{
	if(payment > 0)
	{
		tributeReceived[planet] = payment;
		// Properly connect this function to the dominated property of planets.
		GameData::GetPolitics().DominatePlanet(planet);
	}
	else
	{
		tributeReceived.erase(planet);
		// Properly connect this function to the (no longer) dominated property of planets.
		GameData::GetPolitics().DominatePlanet(planet, false);
	}

	return true;
}



bool PlayerInfo::SetTribute(const string &planetTrueName, int64_t payment)
{
	const Planet *planet = GameData::Planets().Find(planetTrueName);
	if(!planet)
		return false;

	return SetTribute(planet, payment);
}



// Get a list of all tribute that the player receives.
const map<const Planet *, int64_t> &PlayerInfo::GetTribute() const
{
	return tributeReceived;
}



// Get the total sum of the tribute the player receives.
int64_t PlayerInfo::GetTributeTotal() const
{
	return accumulate(
		tributeReceived.begin(),
		tributeReceived.end(),
		0,
		[](int64_t value, const std::map<const Planet *, int64_t>::value_type &tribute)
		{
			return value + tribute.second;
		}
	);
}



// Check if the player knows the location of the given system (whether or not
// they have actually visited it).
bool PlayerInfo::HasSeen(const System &system) const
{
	if(&system == this->system)
		return true;

	// Shrouded systems have special considerations as to whether they're currently seen or not.
	bool shrouded = system.Shrouded();
	if(!shrouded && seen.count(&system))
		return true;

	auto usesSystem = [&system](const Mission &m) noexcept -> bool
	{
		if(!m.IsVisible())
			return false;
		if(m.Waypoints().count(&system))
			return true;
		if(m.MarkedSystems().count(&system))
			return true;
		for(auto &&p : m.Stopovers())
			if(p->IsInSystem(&system))
				return true;
		return m.Destination()->IsInSystem(&system);
	};
	if(any_of(availableJobs.begin(), availableJobs.end(), usesSystem))
		return true;
	if(any_of(missions.begin(), missions.end(), usesSystem))
		return true;

	if(shrouded)
	{
		// All systems linked to a system the player can view are visible.
		if(any_of(system.Links().begin(), system.Links().end(),
				[&](const System *s) noexcept -> bool { return CanView(*s); }))
			return true;
		// A shrouded system not linked to a viewable system must be visible from the current system.
		if(!system.VisibleNeighbors().count(this->system))
			return false;
		// If a shrouded system is in visible range, then it can be seen if it is not also hidden.
		return !system.Hidden();
	}

	return KnowsName(system);
}



// Check if the player can view the contents of the given system.
bool PlayerInfo::CanView(const System &system) const
{
	// A player can always view the contents of the system they are in. Otherwise,
	// the system must have been visited before and not be shrouded.
	return (HasVisited(system) && !system.Shrouded()) || &system == this->system;
}



// Check if the player has visited the given system.
bool PlayerInfo::HasVisited(const System &system) const
{
	return visitedSystems.count(&system);
}



// Check if the player has visited the given planet.
bool PlayerInfo::HasVisited(const Planet &planet) const
{
	return visitedPlanets.count(&planet);
}



// Check if the player knows the name of a system, either from visiting there or
// because a job or active mission includes the name of that system.
bool PlayerInfo::KnowsName(const System &system) const
{
	if(CanView(system))
		return true;

	for(const Mission &mission : availableJobs)
		if(mission.Destination()->IsInSystem(&system))
			return true;

	for(const Mission &mission : missions)
		if(mission.IsVisible() && mission.Destination()->IsInSystem(&system))
			return true;

	return false;
}


// Mark the given system as visited, and mark all its neighbors as seen.
void PlayerInfo::Visit(const System &system)
{
	visitedSystems.insert(&system);
	seen.insert(&system);
	for(const System *neighbor : system.VisibleNeighbors())
		if(!neighbor->Hidden() || system.Links().count(neighbor))
			seen.insert(neighbor);
}



// Mark the given planet as visited.
void PlayerInfo::Visit(const Planet &planet)
{
	visitedPlanets.insert(&planet);
}



// Mark a system as unvisited, even if visited previously.
void PlayerInfo::Unvisit(const System &system)
{
	visitedSystems.erase(&system);
	for(const StellarObject &object : system.Objects())
		if(object.GetPlanet())
			Unvisit(*object.GetPlanet());
}



void PlayerInfo::Unvisit(const Planet &planet)
{
	visitedPlanets.erase(&planet);
}



bool PlayerInfo::HasMapped(int mapSize) const
{
	DistanceMap distance(GetSystem(), mapSize);
	for(const System *system : distance.Systems())
		if(!HasVisited(*system))
			return false;

	return true;
}



void PlayerInfo::Map(int mapSize)
{
	DistanceMap distance(GetSystem(), mapSize);
	for(const System *system : distance.Systems())
		if(!HasVisited(*system))
			Visit(*system);
}



// Check if the player has a hyperspace route set.
bool PlayerInfo::HasTravelPlan() const
{
	return !travelPlan.empty();
}



// Access the player's travel plan.
const vector<const System *> &PlayerInfo::TravelPlan() const
{
	return travelPlan;
}



vector<const System *> &PlayerInfo::TravelPlan()
{
	return travelPlan;
}



// This is called when the player enters the system that is their current
// hyperspace target.
void PlayerInfo::PopTravel()
{
	if(!travelPlan.empty())
	{
		Visit(*travelPlan.back());
		travelPlan.pop_back();
	}
}



// Get the planet to land on at the end of the travel path.
const Planet *PlayerInfo::TravelDestination() const
{
	return travelDestination;
}



// Set the planet to land on at the end of the travel path.
void PlayerInfo::SetTravelDestination(const Planet *planet)
{
	travelDestination = planet;
	if(planet && planet->IsInSystem(system) && Flagship())
		Flagship()->SetTargetStellar(system->FindStellar(planet));
}



// Check which secondary weapons the player has selected.
const set<const Outfit *> &PlayerInfo::SelectedSecondaryWeapons() const
{
	return selectedWeapons;
}



// Cycle through all available secondary weapons.
void PlayerInfo::SelectNextSecondary()
{
	if(!flagship || flagship->Outfits().empty())
		return;

	// If multiple weapons were selected, then switch to selecting none.
	if(selectedWeapons.size() > 1)
	{
		selectedWeapons.clear();
		return;
	}

	// If no weapon was selected, then we scan from the beginning.
	auto it = flagship->Outfits().begin();
	bool hadSingleWeaponSelected = (selectedWeapons.size() == 1);

	// If a single weapon was selected, then move the iterator to the
	// outfit directly after it.
	if(hadSingleWeaponSelected)
	{
		auto selectedOutfit = *(selectedWeapons.begin());
		it = flagship->Outfits().find(selectedOutfit);
		if(it != flagship->Outfits().end())
			++it;
	}

	// Find the next secondary weapon.
	for( ; it != flagship->Outfits().end(); ++it)
		if(it->first->Icon())
		{
			selectedWeapons.clear();
			selectedWeapons.insert(it->first);
			return;
		}

	// If no weapon was selected and we didn't find any weapons at this point,
	// then the player just doesn't have any secondary weapons.
	if(!hadSingleWeaponSelected)
		return;

	// Reached the end of the list. Select all possible secondary weapons here.
	it = flagship->Outfits().begin();
	for( ; it != flagship->Outfits().end(); ++it)
		if(it->first->Icon())
			selectedWeapons.insert(it->first);

	// If we have only one weapon selected at this point, then the player
	// only has a single secondary weapon. Clear the list, since the weapon
	// was selected when we entered this function.
	if(selectedWeapons.size() == 1)
		selectedWeapons.clear();
}



void PlayerInfo::DeselectAllSecondaries()
{
	selectedWeapons.clear();
}



void PlayerInfo::ToggleAnySecondary(const Outfit *outfit)
{
	if(!flagship)
		return;

	const auto it = selectedWeapons.insert(outfit);
	if(!it.second)
		selectedWeapons.erase(it.first);
}



// Escorts currently selected for giving orders.
const vector<weak_ptr<Ship>> &PlayerInfo::SelectedShips() const
{
	return selectedShips;
}



// Select any player ships in the given box or list. Return true if any were
// selected, so we know not to search further for a match.
bool PlayerInfo::SelectShips(const Rectangle &box, bool hasShift)
{
	// If shift is not held down, replace the current selection.
	if(!hasShift)
		selectedShips.clear();
	// If shift is not held, the first ship in the box will also become the
	// player's flagship's target.
	bool first = !hasShift;

	bool matched = false;
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsDestroyed() && !ship->IsParked() && ship->GetSystem() == system && ship.get() != Flagship()
				&& box.Contains(ship->Position()))
		{
			matched = true;
			SelectShip(ship, &first);
		}
	return matched;
}



bool PlayerInfo::SelectShips(const vector<const Ship *> &stack, bool hasShift)
{
	// If shift is not held down, replace the current selection.
	if(!hasShift)
		selectedShips.clear();
	// If shift is not held, the first ship in the stack will also become the
	// player's flagship's target.
	bool first = !hasShift;

	// Loop through all the player's ships and check which of them are in the
	// given stack.
	bool matched = false;
	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = find(stack.begin(), stack.end(), ship.get());
		if(it != stack.end())
		{
			matched = true;
			SelectShip(ship, &first);
		}
	}
	return matched;
}



void PlayerInfo::SelectShip(const Ship *ship, bool hasShift)
{
	// If shift is not held down, replace the current selection.
	if(!hasShift)
		selectedShips.clear();

	bool first = !hasShift;
	for(const shared_ptr<Ship> &it : ships)
		if(it.get() == ship)
			SelectShip(it, &first);
}



void PlayerInfo::SelectGroup(int group, bool hasShift)
{
	int bit = (1 << group);
	// If the shift key is held down and all the ships in the given group are
	// already selected, deselect them all. Otherwise, select them all. The easy
	// way to do this is first to remove all the ships that match in one pass,
	// then add them in a subsequent pass if any were not selected.
	const Ship *oldTarget = nullptr;
	if(Flagship() && Flagship()->GetTargetShip())
	{
		oldTarget = Flagship()->GetTargetShip().get();
		Flagship()->SetTargetShip(shared_ptr<Ship>());
	}
	if(hasShift)
	{
		bool allWereSelected = true;
		for(const shared_ptr<Ship> &ship : ships)
			if(groups[ship.get()] & bit)
			{
				auto it = selectedShips.begin();
				for( ; it != selectedShips.end(); ++it)
					if(it->lock() == ship)
						break;
				if(it != selectedShips.end())
					selectedShips.erase(it);
				else
					allWereSelected = false;
			}
		if(allWereSelected)
			return;
	}
	else
		selectedShips.clear();

	// Now, go through and add any ships in the group to the selection. Even if
	// shift is held they won't be added twice, because we removed them above.
	for(const shared_ptr<Ship> &ship : ships)
		if(groups[ship.get()] & bit)
		{
			selectedShips.push_back(ship);
			if(ship.get() == oldTarget)
				Flagship()->SetTargetShip(ship);
		}
}



void PlayerInfo::SetGroup(int group, const set<Ship *> *newShips)
{
	int bit = (1 << group);
	int mask = ~bit;
	// First, remove any of your ships that are in the group.
	for(const shared_ptr<Ship> &ship : ships)
		groups[ship.get()] &= mask;
	// Then, add all the currently selected ships to the group.
	if(newShips)
	{
		for(const Ship *ship : *newShips)
			groups[ship] |= bit;
	}
	else
	{
		for(const weak_ptr<Ship> &ptr : selectedShips)
		{
			shared_ptr<Ship> ship = ptr.lock();
			if(ship)
				groups[ship.get()] |= bit;
		}
	}
}



set<Ship *> PlayerInfo::GetGroup(int group)
{
	int bit = (1 << group);
	set<Ship *> result;

	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = groups.find(ship.get());
		if(it != groups.end() && (it->second & bit))
			result.insert(ship.get());
	}
	return result;
}



// Keep track of any outfits that you have sold since landing. These will be
// available to buy back until you take off.
const map<const Outfit *, int> &PlayerInfo::GetStock() const
{
	return stock;
}



int PlayerInfo::Stock(const Outfit *outfit) const
{
	auto it = stock.find(outfit);
	return (it == stock.end() ? 0 : it->second);
}



// Transfer outfits from the player to the planet or vice versa.
void PlayerInfo::AddStock(const Outfit *outfit, int count)
{
	// If you sell an individual outfit that is not sold here and that you
	// acquired by buying a ship here, have it appear as "in stock" in case you
	// change your mind about selling it. (On the other hand, if you sell an
	// entire ship right after buying it, its outfits will not be "in stock.")
	if(count > 0 && stock[outfit] < 0)
		stock[outfit] = 0;
	stock[outfit] += count;

	int day = date.DaysSinceEpoch();
	if(count > 0)
	{
		// Remember how depreciated these items are.
		for(int i = 0; i < count; ++i)
			stockDepreciation.Buy(outfit, day, &depreciation);
	}
	else
	{
		// If the count is negative, outfits are being transferred from stock
		// into the player's possession.
		for(int i = 0; i < -count; ++i)
			depreciation.Buy(outfit, day, &stockDepreciation);
	}
}



// Get depreciation information.
const Depreciation &PlayerInfo::FleetDepreciation() const
{
	return depreciation;
}



const Depreciation &PlayerInfo::StockDepreciation() const
{
	return stockDepreciation;
}



void PlayerInfo::Harvest(const Outfit *type)
{
	if(type && system)
		harvested.insert(make_pair(system, type));
}



const set<pair<const System *, const Outfit *>> &PlayerInfo::Harvested() const
{
	return harvested;
}



const pair<const System *, Point> &PlayerInfo::GetEscortDestination() const
{
	return interstellarEscortDestination;
}



// Determine if a system and nonzero position were specified.
bool PlayerInfo::HasEscortDestination() const
{
	return interstellarEscortDestination.first && interstellarEscortDestination.second;
}



// Set (or clear) the stored escort travel destination.
void PlayerInfo::SetEscortDestination(const System *system, Point pos)
{
	interstellarEscortDestination.first = system;
	interstellarEscortDestination.second = pos;
}



// Get what coloring is currently selected in the map.
int PlayerInfo::MapColoring() const
{
	return mapColoring;
}



// Set what the map is being colored by.
void PlayerInfo::SetMapColoring(int index)
{
	mapColoring = index;
}



// Get the map zoom level.
int PlayerInfo::MapZoom() const
{
	return mapZoom;
}



// Set the map zoom level.
void PlayerInfo::SetMapZoom(int level)
{
	mapZoom = level;
}



// Get the set of collapsed categories for the named panel.
set<string> &PlayerInfo::Collapsed(const string &name)
{
	return collapsed[name];
}



// Apply any "changes" saved in this player info to the global game state.
void PlayerInfo::ApplyChanges()
{
	for(const auto &it : reputationChanges)
		it.first->SetReputation(it.second);
	reputationChanges.clear();
	AddChanges(dataChanges);
	GameData::ReadEconomy(economy);
	economy = DataNode();

	// Make sure all stellar objects are correctly positioned. This is needed
	// because EnterSystem() is not called the first time through.
	GameData::SetDate(GetDate());
	// SetDate() clears any bribes from yesterday, so restore any auto-clearance.
	for(const Mission &mission : Missions())
		if(mission.ClearanceMessage() == "auto")
		{
			mission.Destination()->Bribe(mission.HasFullClearance());
			for(const Planet *planet : mission.Stopovers())
				planet->Bribe(mission.HasFullClearance());
		}

	// Check if any special persons have been destroyed.
	GameData::DestroyPersons(destroyedPersons);
	destroyedPersons.clear();

	// Check which planets you have dominated.
	for(auto &it : tributeReceived)
		GameData::GetPolitics().DominatePlanet(it.first);

	// Issue warnings for any data which has been mentioned but not actually defined, and
	// ensure that all "undefined" data is appropriately named.
	GameData::CheckReferences();

	// Now that all outfits have names, we can finish loading the player's ships.
	for(auto &&ship : ships)
	{
		// Government changes may have changed the player's ship swizzles.
		ship->SetGovernment(GameData::PlayerGovernment());
		ship->FinishLoading(false);
	}

	// Recalculate jumps that the available jobs will need
	for(Mission &mission : availableJobs)
		mission.CalculateJumps(system);
}



// Make change's to the player's planet, system, & ship locations as needed, to ensure the player and
// their ships are in valid locations, even if the player did something drastic, such as remove a mod.
void PlayerInfo::ValidateLoad()
{
	// If a system was not specified in the player data, use the flagship's system.
	if(!planet && !ships.empty())
	{
		string warning = "Warning: no planet specified for player";
		auto it = find_if(ships.begin(), ships.end(), [](const shared_ptr<Ship> &ship) noexcept -> bool
			{ return ship->GetPlanet() && ship->GetPlanet()->IsValid() && !ship->IsParked() && ship->CanBeFlagship(); });
		if(it != ships.end())
		{
			planet = (*it)->GetPlanet();
			system = (*it)->GetSystem();
			warning += ". Defaulting to location of flagship \"" + (*it)->Name() + "\", " + planet->TrueName() + ".";
		}
		else
			warning += " (no ships could supply a valid player location).";

		Logger::LogError(warning);
	}

	// As a result of external game data changes (e.g. unloading a mod) it's possible the player ended up
	// with an undefined system or planet. In that case, move them to the starting system to avoid crashing.
	if(planet && !system)
	{
		system = planet->GetSystem();
		Logger::LogError("Warning: player system was not specified. Defaulting to the specified planet's system.");
	}
	if(!planet || !planet->IsValid() || !system || !system->IsValid())
	{
		system = &startData.GetSystem();
		planet = &startData.GetPlanet();
		Logger::LogError("Warning: player system and/or planet was not valid. Defaulting to the starting location.");
	}

	// Every ship ought to have specified a valid location, but if not,
	// move it to the player's location to avoid invalid states.
	for(auto &&ship : ships)
	{
		if(!ship->GetSystem() || !ship->GetSystem()->IsValid())
		{
			ship->SetSystem(system);
			Logger::LogError("Warning: player ship \"" + ship->Name()
				+ "\" did not specify a valid system. Defaulting to the player's system.");
		}
		// In-system ships that aren't on a valid planet should get moved to the player's planet
		// (but e.g. disabled ships or those that didn't have a planet should remain in space).
		if(ship->GetSystem() == system && ship->GetPlanet() && !ship->GetPlanet()->IsValid())
		{
			ship->SetPlanet(planet);
			Logger::LogError("Warning: in-system player ship \"" + ship->Name()
				+ "\" specified an invalid planet. Defaulting to the player's planet.");
		}
		// Owned ships that are not in the player's system always start in flight.
	}

	// Validate the travel plan.
	if(travelDestination && !travelDestination->IsValid())
	{
		Logger::LogError("Warning: removed invalid travel plan destination \"" + travelDestination->TrueName() + ".\"");
		travelDestination = nullptr;
	}
	if(!travelPlan.empty() && any_of(travelPlan.begin(), travelPlan.end(),
			[](const System *waypoint) noexcept -> bool { return !waypoint->IsValid(); }))
	{
		travelPlan.clear();
		travelDestination = nullptr;
		Logger::LogError("Warning: reset the travel plan due to use of invalid system(s).");
	}

	// For old saves, default to the first start condition (the default "Endless Sky" start).
	if(startData.Identifier().empty())
	{
		// It is possible that there are no start conditions defined (e.g. a bad installation or
		// incomplete total conversion plugin). In that case, it is not possible to continue.
		const auto startCount = GameData::StartOptions().size();
		if(startCount >= 1)
		{
			startData = GameData::StartOptions().front();
			// When necessary, record in the pilot file that the starting data is just an assumption.
			if(startCount >= 2)
				conditions["unverified start scenario"] = true;
		}
		else
			throw runtime_error("Unable to set a starting scenario for an existing pilot. (No valid \"start\" "
				"nodes were found in data files or loaded plugins--make sure you've installed the game properly.)");
	}

	// Validate the missions that were loaded. Active-but-invalid missions are removed from
	// the standard mission list, effectively pausing them until necessary data is restored.
	auto mit = stable_partition(missions.begin(), missions.end(), mem_fn(&Mission::IsValid));
	if(mit != missions.end())
		inactiveMissions.splice(inactiveMissions.end(), missions, mit, missions.end());

	// Invalid available jobs or missions are erased (since there is no guarantee
	// the player will be on the correct planet when a plugin is re-added).
	auto isInvalidMission = [](const Mission &m) noexcept -> bool { return !m.IsValid(); };
	availableJobs.remove_if(isInvalidMission);
	availableMissions.remove_if(isInvalidMission);
}



// Helper to register derived conditions.
void PlayerInfo::RegisterDerivedConditions()
{
	// Read-only date functions.
	auto &&dayProvider = conditions.GetProviderNamed("day");
	dayProvider.SetGetFunction([this](const string &name) { return date.Day(); });

	auto &&monthProvider = conditions.GetProviderNamed("month");
	monthProvider.SetGetFunction([this](const string &name) { return date.Month(); });

	auto &&yearProvider = conditions.GetProviderNamed("year");
	yearProvider.SetGetFunction([this](const string &name) { return date.Year(); });

	auto &&daysSinceYearStartProvider = conditions.GetProviderNamed("days since year start");
	daysSinceYearStartProvider.SetGetFunction([this](const string &name) { return date.DaysSinceYearStart(); });

	auto &&daysUntilYearEndProvider = conditions.GetProviderNamed("days until year end");
	daysUntilYearEndProvider.SetGetFunction([this](const string &name) { return date.DaysUntilYearEnd(); });

	auto &&daysSinceEpochProvider = conditions.GetProviderNamed("days since epoch");
	daysSinceEpochProvider.SetGetFunction([this](const string &name) { return date.DaysSinceEpoch(); });

	auto &&daysSinceStartProvider = conditions.GetProviderNamed("days since start");
	daysSinceStartProvider.SetGetFunction([this](const string &name)
	{
		return date.DaysSinceEpoch() - StartData().GetDate().DaysSinceEpoch();
	});

	// Read-only account conditions.
	// Bound financial conditions to +/- 4.6 x 10^18 credits, within the range of a 64-bit int.
	static constexpr int64_t limit = static_cast<int64_t>(1) << 62;

	auto &&netWorthProvider = conditions.GetProviderNamed("net worth");
	netWorthProvider.SetGetFunction([this](const string &name)
		{ return min(limit, max(-limit, accounts.NetWorth())); });

	auto &&creditsProvider = conditions.GetProviderNamed("credits");
	creditsProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.Credits()); });

	auto &&unpaidMortgagesProvider = conditions.GetProviderNamed("unpaid mortgages");
	unpaidMortgagesProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.TotalDebt("Mortgage")); });

	auto &&unpaidFinesProvider = conditions.GetProviderNamed("unpaid fines");
	unpaidFinesProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.TotalDebt("Fine")); });

	auto &&unpaidDebtsProvider = conditions.GetProviderNamed("unpaid debts");
	unpaidDebtsProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.TotalDebt("Debt")); });

	auto &&unpaidSalariesProvider = conditions.GetProviderNamed("unpaid salaries");
	unpaidSalariesProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.CrewSalariesOwed()); });

	auto &&unpaidMaintenanceProvider = conditions.GetProviderNamed("unpaid maintenance");
	unpaidMaintenanceProvider.SetGetFunction([this](const string &name) {
		return min(limit, accounts.MaintenanceDue()); });

	auto &&creditScoreProvider = conditions.GetProviderNamed("credit score");
	creditScoreProvider.SetGetFunction([this](const string &name) {
		return accounts.CreditScore(); });

	// Read/write assets and debts.
	auto &&salaryIncomeProvider = conditions.GetProviderPrefixed("salary: ");
	auto salaryIncomeHasGetFun = [this](const string &name) -> int64_t
	{
		const map<string, int64_t> &si = accounts.SalariesIncome();
		auto it = si.find(name.substr(strlen("salary: ")));
		if(it == si.end())
			return 0;
		return it->second;
	};
	salaryIncomeProvider.SetHasFunction(salaryIncomeHasGetFun);
	salaryIncomeProvider.SetGetFunction(salaryIncomeHasGetFun);
	salaryIncomeProvider.SetSetFunction([this](const string &name, int64_t value) -> bool
	{
		accounts.SetSalaryIncome(name.substr(strlen("salary: ")), value);
		return true;
	});
	salaryIncomeProvider.SetEraseFunction([this](const string &name) -> bool
	{
		accounts.SetSalaryIncome(name.substr(strlen("salary: ")), 0);
		return true;
	});

	auto &&tributeProvider = conditions.GetProviderPrefixed("tribute: ");
	auto tributeHasGetFun = [this](const string &name) -> int64_t
	{
		const Planet *planet = GameData::Planets().Find(name.substr(strlen("tribute: ")));
		if(!planet)
			return 0;

		auto it = tributeReceived.find(planet);
		if(it == tributeReceived.end())
			return 0;

		return it->second;
	};
	tributeProvider.SetHasFunction(tributeHasGetFun);
	tributeProvider.SetGetFunction(tributeHasGetFun);
	tributeProvider.SetSetFunction([this](const string &name, int64_t value) -> bool {
		return SetTribute(name.substr(strlen("tribute: ")), value);
	});
	tributeProvider.SetEraseFunction([this](const string &name) -> bool {
		return SetTribute(name.substr(strlen("tribute: ")), 0);
	});

	auto &&licenseProvider = conditions.GetProviderPrefixed("license: ");
	licenseProvider.SetHasFunction([this](const string &name) -> bool {
		return HasLicense(name.substr(strlen("license: ")));
	});
	licenseProvider.SetGetFunction([this](const string &name) -> int64_t {
		return HasLicense(name.substr(strlen("license: ")));
	});

	licenseProvider.SetSetFunction([this](const string &name, int64_t value) -> bool
	{
		if(!value)
			RemoveLicense(name.substr(strlen("license: ")));
		else
			AddLicense(name.substr(strlen("license: ")));
		return true;
	});

	licenseProvider.SetEraseFunction([this](const string &name) -> bool
	{
		RemoveLicense(name.substr(strlen("license: ")));
		return true;
	});

	// Read-only flagship conditions.
	auto &&flagshipCrewProvider = conditions.GetProviderNamed("flagship crew");
	flagshipCrewProvider.SetGetFunction([this](const string &name) -> int64_t {
		return flagship ? flagship->Crew() : 0; });

	auto &&flagshipRequiredCrewProvider = conditions.GetProviderNamed("flagship required crew");
	flagshipRequiredCrewProvider.SetGetFunction([this](const string &name) -> int64_t {
		return flagship ? flagship->RequiredCrew() : 0; });

	auto &&flagshipBunksProvider = conditions.GetProviderNamed("flagship bunks");
	flagshipBunksProvider.SetGetFunction([this](const string &name) -> int64_t {
		return flagship ? flagship->Attributes().Get("bunks") : 0; });

	auto &&flagshipModelProvider = conditions.GetProviderPrefixed("flagship model: ");
	auto flagshipModelFun = [this](const string &name) -> bool
	{
		if(!flagship)
			return false;
		return name == "flagship model: " + flagship->TrueModelName();
	};
	flagshipModelProvider.SetHasFunction(flagshipModelFun);
	flagshipModelProvider.SetGetFunction(flagshipModelFun);

	auto &&flagshipDisabledProvider = conditions.GetProviderNamed("flagship disabled");
	auto flagshipDisabledFun = [this](const string &name) -> bool
	{
		return flagship && flagship->IsDisabled();
	};
	flagshipDisabledProvider.SetHasFunction(flagshipDisabledFun);
	flagshipDisabledProvider.SetGetFunction(flagshipDisabledFun);

	auto flagshipAttributeHelper = [](const Ship *flagship, const string &attribute, bool base) -> int64_t
	{
		if(!flagship)
			return 0;

		const Outfit &attributes = base ? flagship->BaseAttributes() : flagship->Attributes();
		if(attribute == "cost")
			return attributes.Cost();
		if(attribute == "mass")
			return round(attributes.Mass() * 1000.);
		return round(attributes.Get(attribute) * 1000.);
	};

	auto &&flagshipBaseAttributeProvider = conditions.GetProviderPrefixed("flagship base attribute: ");
	auto flagshipBaseAttributeFun = [this, flagshipAttributeHelper](const string &name) -> int64_t
	{
		return flagshipAttributeHelper(this->Flagship(), name.substr(strlen("flagship base attribute: ")), true);
	};
	flagshipBaseAttributeProvider.SetGetFunction(flagshipBaseAttributeFun);
	flagshipBaseAttributeProvider.SetHasFunction(flagshipBaseAttributeFun);

	auto &&flagshipAttributeProvider = conditions.GetProviderPrefixed("flagship attribute: ");
	auto flagshipAttributeFun = [this, flagshipAttributeHelper](const string &name) -> int64_t
	{
		return flagshipAttributeHelper(this->Flagship(), name.substr(strlen("flagship attribute: ")), false);
	};
	flagshipAttributeProvider.SetGetFunction(flagshipAttributeFun);
	flagshipAttributeProvider.SetHasFunction(flagshipAttributeFun);

	auto &&flagshipBaysProvider = conditions.GetProviderPrefixed("flagship bays: ");
	auto flagshipBaysFun = [this](const string &name) -> int64_t
	{
		if(!flagship)
			return 0;

		return flagship->BaysTotal(name.substr(strlen("flagship bays: ")));
	};
	flagshipBaysProvider.SetGetFunction(flagshipBaysFun);
	flagshipBaysProvider.SetHasFunction(flagshipBaysFun);

	auto &&playerNameProvider = conditions.GetProviderPrefixed("name: ");
	auto playerNameFun = [this](const string &name) -> bool
	{
		return name == "name: " + firstName + " " + lastName;
	};
	playerNameProvider.SetHasFunction(playerNameFun);
	playerNameProvider.SetGetFunction(playerNameFun);

	auto &&playerNameFirstProvider = conditions.GetProviderPrefixed("first name: ");
	auto playerNameFirstFun = [this](const string &name) -> bool
	{
		return name == "first name: " + firstName;
	};
	playerNameFirstProvider.SetHasFunction(playerNameFirstFun);
	playerNameFirstProvider.SetGetFunction(playerNameFirstFun);

	auto &&playerNameLastProvider = conditions.GetProviderPrefixed("last name: ");
	auto playerNameLastFun = [this](const string &name) -> bool
	{
		return name == "last name: " + lastName;
	};
	playerNameLastProvider.SetHasFunction(playerNameLastFun);
	playerNameLastProvider.SetGetFunction(playerNameLastFun);


	// Conditions for your fleet's attractiveness to pirates.
	auto &&cargoAttractivenessProvider = conditions.GetProviderNamed("cargo attractiveness");
	cargoAttractivenessProvider.SetGetFunction([this](const string &name) -> int64_t {
		return RaidFleetFactors().first; });

	auto &&armamentDeterrence = conditions.GetProviderNamed("armament deterrence");
	armamentDeterrence.SetGetFunction([this](const string &name) -> int64_t {
		return RaidFleetFactors().second; });

	auto &&pirateAttractionProvider = conditions.GetProviderNamed("pirate attraction");
	pirateAttractionProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		auto rff = RaidFleetFactors();
		return rff.first - rff.second;
	});

	auto &&systemAttractionProvider = conditions.GetProviderPrefixed("raid chance in system: ");
	auto systemAttractionFun = [this](const string &name) -> double
	{
		const System *system = GameData::Systems().Find(name.substr(strlen("raid chance in system: ")));
		if(!system)
			return 0.;

		// This variable represents the probability of no raid fleets spawning.
		double safeChance = 1.;
		for(const auto &raidFleet : system->RaidFleets())
		{
			// The attraction is the % chance for a single instance of this fleet to appear.
			double attraction = RaidFleetAttraction(raidFleet, system);
			// Calculate the % chance for no instances to appear from 10 rolls.
			double noFleetProb = pow(1. - attraction, 10.);
			// The chance of neither of two fleets appearing is the chance of the first not appearing
			// times the chance of the second not appearing.
			safeChance *= noFleetProb;
		}
		// The probability of any single fleet appearing is 1 - chance.
		return round((1. - safeChance) * 1000.);
	};
	systemAttractionProvider.SetGetFunction(systemAttractionFun);
	systemAttractionProvider.SetHasFunction(systemAttractionFun);

	// Special conditions for cargo and passenger space.
	// If boarding a ship, missions should not consider the space available
	// in the player's entire fleet. The only fleet parameter offered to a
	// boarding mission is the fleet composition (e.g. 4 Heavy Warships).
	auto &&cargoSpaceProvider = conditions.GetProviderNamed("cargo space");
	cargoSpaceProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		if(flagship && !boardingMissions.empty())
			return flagship->Cargo().Free();
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsParked() && !ship->IsDisabled() && ship->GetActualSystem() == system)
				retVal += ship->Attributes().Get("cargo space");
		return retVal;
	});

	auto &&passengerSpaceProvider = conditions.GetProviderNamed("passenger space");
	passengerSpaceProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		if(flagship && !boardingMissions.empty())
			return flagship->Cargo().BunksFree();
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsParked() && !ship->IsDisabled() && ship->GetActualSystem() == system)
				retVal += ship->Attributes().Get("bunks") - ship->RequiredCrew();
		return retVal;
	});

	// The number of active, present ships the player has of the given category
	// (e.g. Heavy Warships).
	auto &&shipTypesProvider = conditions.GetProviderPrefixed("ships: ");
	shipTypesProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsParked() && !ship->IsDisabled() && ship->GetActualSystem() == system
					&& name == "ships: " + ship->Attributes().Category())
				++retVal;
		return retVal;
	});

	// The number of ships the player has of the given category anywhere in their fleet.
	auto &&shipTypesAllProvider = conditions.GetProviderPrefixed("ships (all): ");
	shipTypesAllProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsDestroyed() && name == "ships (all): " + ship->Attributes().Category())
				++retVal;
		return retVal;
	});

	// The number of ships the player has of the given model active and present.
	auto &&shipModelProvider = conditions.GetProviderPrefixed("ship model: ");
	shipModelProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsParked() && !ship->IsDisabled() && ship->GetActualSystem() == system
					&& name == "ship model: " + ship->TrueModelName())
				++retVal;
		return retVal;
	});

	// The number of ships that the player has of the given model anywhere in their fleet.
	auto &&shipModelAllProvider = conditions.GetProviderPrefixed("ship model (all): ");
	shipModelAllProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsDestroyed() && name == "ship model (all): " + ship->TrueModelName())
				++retVal;
		return retVal;
	});

	// The total number of ships the player has active and present.
	auto &&totalPresentShipsProvider = conditions.GetProviderNamed("total ships");
	totalPresentShipsProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsParked() && !ship->IsDisabled() && ship->GetActualSystem() == system)
				++retVal;
		return retVal;
	});

	// The total number of ships the player has anywhere.
	auto &&totalAnywhereShipsProvider = conditions.GetProviderNamed("total ships (all)");
	totalAnywhereShipsProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsDestroyed())
				++retVal;
		return retVal;
	});

	// The following condition checks all sources of outfits which are present with the player.
	// If in orbit, this means checking all ships in-system for installed and in cargo outfits.
	// If landed, this means checking all landed ships for installed outfits, the pooled cargo
	// hold, and the planetary storage of the planet. Excludes parked ships.
	auto &&presentOutfitProvider = conditions.GetProviderPrefixed("outfit: ");
	presentOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit: ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		if(planet)
		{
			retVal += Cargo().Get(outfit);
			auto it = planetaryStorage.find(planet);
			if(it != planetaryStorage.end())
				retVal += it->second.Get(outfit);
		}
		for(const shared_ptr<Ship> &ship : ships)
		{
			// Destroyed and parked ships aren't checked.
			// If not on a planet, the ship's system must match.
			// If on a planet, the ship's planet must match.
			if(ship->IsDestroyed() || ship->IsParked()
					|| (planet && ship->GetPlanet() != planet)
					|| (!planet && ship->GetActualSystem() != system))
				continue;
			retVal += ship->OutfitCount(outfit);
			retVal += ship->Cargo().Get(outfit);
		}
		return retVal;
	});

	// Conditions to determine what outfits the player owns, with various possible locations to check.
	// The following condition checks all possible locations for outfits in the player's possession.
	auto &&allOutfitProvider = conditions.GetProviderPrefixed("outfit (all): ");
	allOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (all): ")));
		if(!outfit)
			return 0;
		int64_t retVal = Cargo().Get(outfit);
		for(const shared_ptr<Ship> &ship : ships)
		{
			if(ship->IsDestroyed())
				continue;
			retVal += ship->OutfitCount(outfit);
			retVal += ship->Cargo().Get(outfit);
		}
		for(const auto &storage : planetaryStorage)
			retVal += storage.second.Get(outfit);
		return retVal;
	});

	// The following condition checks the player's fleet for installed outfits on active
	// escorts local to the player.
	auto &presentInstalledOutfitProvider = conditions.GetProviderPrefixed("outfit (installed): ");
	presentInstalledOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (installed): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
		{
			// Destroyed and parked ships aren't checked.
			// If not on a planet, the ship's system must match.
			// If on a planet, the ship's planet must match.
			if(ship->IsDestroyed() || ship->IsParked()
					|| (planet && ship->GetPlanet() != planet)
					|| (!planet && ship->GetActualSystem() != system))
				continue;
			retVal += ship->OutfitCount(outfit);
		}
		return retVal;
	});

	// The following condition checks the player's fleet for installed outfits on parked escorts
	// which are local to the player.
	auto &parkedInstalledOutfitProvider = conditions.GetProviderPrefixed("outfit (parked): ");
	parkedInstalledOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		// If the player isn't landed then there can be no parked ships local to them.
		if(!planet)
			return 0;
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (parked): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
		{
			if(!ship->IsParked() || ship->GetPlanet() != planet)
				continue;
			retVal += ship->OutfitCount(outfit);
		}
		return retVal;
	});

	// The following condition checks the player's entire fleet for installed outfits.
	auto &&allInstalledOutfitProvider = conditions.GetProviderPrefixed("outfit (all installed): ");
	allInstalledOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (all installed): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsDestroyed())
				retVal += ship->OutfitCount(outfit);
		return retVal;
	});

	// The following condition checks the flagship's installed outfits.
	auto &&flagshipInstalledOutfitProvider = conditions.GetProviderPrefixed("outfit (flagship installed): ");
	flagshipInstalledOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		if(!flagship)
			return 0;
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (flagship installed): ")));
		if(!outfit)
			return 0;
		return flagship->OutfitCount(outfit);
	});

	// The following condition checks the player's fleet for outfits in the cargo of escorts
	// local to the player.
	auto &&presentCargoOutfitProvider = conditions.GetProviderPrefixed("outfit (cargo): ");
	presentCargoOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (cargo): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		if(planet)
			retVal += Cargo().Get(outfit);
		for(const shared_ptr<Ship> &ship : ships)
		{
			// If not on a planet, parked ships in system don't count.
			// If on a planet, the ship's planet must match.
			if(ship->IsDestroyed() || (planet && ship->GetPlanet() != planet)
					|| (!planet && (ship->GetActualSystem() != system || ship->IsParked())))
				continue;
			retVal += ship->Cargo().Get(outfit);
		}
		return retVal;
	});

	// The following condition checks all cargo locations in the player's fleet.
	auto &&allCargoOutfitProvider = conditions.GetProviderPrefixed("outfit (all cargo): ");
	allCargoOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (all cargo): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		if(planet)
			retVal += Cargo().Get(outfit);
		for(const shared_ptr<Ship> &ship : ships)
			if(!ship->IsDestroyed())
				retVal += ship->Cargo().Get(outfit);
		return retVal;
	});

	// The following condition checks the flagship's cargo or the pooled cargo if landed.
	auto &&flagshipCargoOutfitProvider = conditions.GetProviderPrefixed("outfit (flagship cargo): ");
	flagshipCargoOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (flagship cargo): ")));
		if(!outfit)
			return 0;
		return (flagship ? flagship->Cargo().Get(outfit) : 0) + (planet ? Cargo().Get(outfit) : 0);
	});

	// The following condition checks planetary storage on the current planet, or on
	// planets in the current system if in orbit.
	auto &&presentStorageOutfitProvider = conditions.GetProviderPrefixed("outfit (storage): ");
	presentStorageOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (storage): ")));
		if(!outfit)
			return 0;
		if(planet)
		{
			auto it = planetaryStorage.find(planet);
			return it != planetaryStorage.end() ? it->second.Get(outfit) : 0;
		}
		else
		{
			int64_t retVal = 0;
			for(const StellarObject &object : system->Objects())
			{
				auto it = planetaryStorage.find(object.GetPlanet());
				if(object.HasValidPlanet() && it != planetaryStorage.end())
					retVal += it->second.Get(outfit);
			}
			return retVal;
		}
	});

	// The following condition checks all planetary storage.
	auto &&allStorageOutfitProvider = conditions.GetProviderPrefixed("outfit (all storage): ");
	allStorageOutfitProvider.SetGetFunction([this](const string &name) -> int64_t
	{
		const Outfit *outfit = GameData::Outfits().Find(name.substr(strlen("outfit (all storage): ")));
		if(!outfit)
			return 0;
		int64_t retVal = 0;
		for(const auto &storage : planetaryStorage)
			retVal += storage.second.Get(outfit);
		return retVal;
	});

	// This condition corresponds to the method by which the flagship entered the current system.
	auto &&systemEntryProvider = conditions.GetProviderPrefixed("entered system by: ");
	auto systemEntryFun = [this](const string &name) -> bool
	{
		return name == "entered system by: " + EntryToString(entry);
	};
	systemEntryProvider.SetHasFunction(systemEntryFun);
	systemEntryProvider.SetGetFunction(systemEntryFun);

	// This condition corresponds to the last system the flagship was in.
	auto &&previousSystemProvider = conditions.GetProviderPrefixed("previous system: ");
	auto previousSystemFun = [this](const string &name) -> bool
	{
		if(!previousSystem)
			return false;
		return name == "previous system: " + previousSystem->Name();
	};
	previousSystemProvider.SetHasFunction(previousSystemFun);
	previousSystemProvider.SetGetFunction(previousSystemFun);

	// Conditions to determine if flagship is in a system and on a planet.
	auto &&flagshipSystemProvider = conditions.GetProviderPrefixed("flagship system: ");
	auto flagshipSystemFun = [this](const string &name) -> bool
	{
		if(!flagship || !flagship->GetSystem())
			return false;
		return name == "flagship system: " + flagship->GetSystem()->Name();
	};
	flagshipSystemProvider.SetHasFunction(flagshipSystemFun);
	flagshipSystemProvider.SetGetFunction(flagshipSystemFun);

	auto &&flagshipLandedProvider = conditions.GetProviderNamed("flagship landed");
	auto flagshipLandedFun = [this](const string &name) -> bool
	{
		return (flagship && flagship->GetPlanet());
	};
	flagshipLandedProvider.SetHasFunction(flagshipLandedFun);
	flagshipLandedProvider.SetGetFunction(flagshipLandedFun);

	auto &&flagshipPlanetProvider = conditions.GetProviderPrefixed("flagship planet: ");
	auto flagshipPlanetFun = [this](const string &name) -> bool
	{
		if(!flagship || !flagship->GetPlanet())
			return false;
		return name == "flagship planet: " + flagship->GetPlanet()->TrueName();
	};
	flagshipPlanetProvider.SetHasFunction(flagshipPlanetFun);
	flagshipPlanetProvider.SetGetFunction(flagshipPlanetFun);

	auto &&flagshipPlanetAttributesProvider = conditions.GetProviderPrefixed("flagship planet attribute: ");
	auto flagshipPlanetAttributesFun = [this](const string &name) -> bool
	{
		if(!flagship || !flagship->GetPlanet())
			return false;
		string attribute = name.substr(strlen("flagship planet attribute: "));
		return flagship->GetPlanet()->Attributes().count(attribute);
	};
	flagshipPlanetAttributesProvider.SetGetFunction(flagshipPlanetAttributesFun);
	flagshipPlanetAttributesProvider.SetHasFunction(flagshipPlanetAttributesFun);

	// Read only exploration conditions.
	auto &&visitedPlanetProvider = conditions.GetProviderPrefixed("visited planet: ");
	auto visitedPlanetFun = [this](const string &name) -> bool
	{
		const Planet *planet = GameData::Planets().Find(name.substr(strlen("visited planet: ")));
		return planet ? HasVisited(*planet) : false;
	};
	visitedPlanetProvider.SetGetFunction(visitedPlanetFun);
	visitedPlanetProvider.SetHasFunction(visitedPlanetFun);

	auto &&visitedSystemProvider = conditions.GetProviderPrefixed("visited system: ");
	auto visitedSystemFun = [this](const string &name) -> bool
	{
		const System *system = GameData::Systems().Find(name.substr(strlen("visited system: ")));
		return system ? HasVisited(*system) : false;
	};
	visitedSystemProvider.SetGetFunction(visitedSystemFun);
	visitedSystemProvider.SetHasFunction(visitedSystemFun);

	auto &&landingAccessProvider = conditions.GetProviderPrefixed("landing access: ");
	auto landingAccessFun = [this](const string &name) -> bool
	{
		const Planet *planet = GameData::Planets().Find(name.substr(strlen("landing access: ")));
		return (planet && flagship) ? planet->CanLand(*flagship) : false;
	};
	landingAccessProvider.SetGetFunction(landingAccessFun);
	landingAccessProvider.SetHasFunction(landingAccessFun);

	auto &&pluginProvider = conditions.GetProviderPrefixed("installed plugin: ");
	auto pluginFun = [](const string &name) -> bool
	{
		const Plugin *plugin = Plugins::Get().Find(name.substr(strlen("installed plugin: ")));
		return plugin ? plugin->IsValid() && plugin->enabled : false;
	};
	pluginProvider.SetHasFunction(pluginFun);
	pluginProvider.SetGetFunction(pluginFun);

	auto &&destroyedPersonProvider = conditions.GetProviderPrefixed("person destroyed: ");
	auto destroyedPersonFun = [](const string &name) -> bool
	{
		const Person *person = GameData::Persons().Find(name.substr(strlen("person destroyed: ")));
		return person ? person->IsDestroyed() : false;
	};
	destroyedPersonProvider.SetGetFunction(destroyedPersonFun);
	destroyedPersonProvider.SetHasFunction(destroyedPersonFun);

	// Read-only navigation conditions.
	auto HyperspaceTravelDays = [](const System *origin, const System *destination) -> int
	{
		if(!origin)
			return -1;

		auto distanceMap = DistanceMap(origin);
		if(!distanceMap.HasRoute(destination))
			return -1;
		return distanceMap.Days(destination);
	};

	auto &&hyperjumpsToSystemProvider = conditions.GetProviderPrefixed("hyperjumps to system: ");
	auto hyperjumpsToSystemFun = [this, HyperspaceTravelDays](const string &name) -> int
	{
		const System *system = GameData::Systems().Find(name.substr(strlen("hyperjumps to system: ")));
		if(!system)
		{
			Logger::LogError("Warning: System \"" + name.substr(strlen("hyperjumps to system: "))
					+ "\" referred to in condition is not valid.");
			return -1;
		}
		return HyperspaceTravelDays(this->GetSystem(), system);
	};
	hyperjumpsToSystemProvider.SetGetFunction(hyperjumpsToSystemFun);
	hyperjumpsToSystemProvider.SetHasFunction(hyperjumpsToSystemFun);

	auto &&hyperjumpsToPlanetProvider = conditions.GetProviderPrefixed("hyperjumps to planet: ");
	auto hyperjumpsToPlanetFun = [this, HyperspaceTravelDays](const string &name) -> int
	{
		const Planet *planet = GameData::Planets().Find(name.substr(strlen("hyperjumps to planet: ")));
		if(!planet)
		{
			Logger::LogError("Warning: Planet \"" + name.substr(strlen("hyperjumps to planet: "))
					+ "\" referred to in condition is not valid.");
			return -1;
		}
		const System *system = planet->GetSystem();
		if(!system)
		{
			Logger::LogError("Warning: Planet \"" + name.substr(strlen("hyperjumps to planet: "))
					+ "\" referred to in condition is not in any system.");
			return -1;
		}
		return HyperspaceTravelDays(this->GetSystem(), system);
	};
	hyperjumpsToPlanetProvider.SetGetFunction(hyperjumpsToPlanetFun);
	hyperjumpsToPlanetProvider.SetHasFunction(hyperjumpsToPlanetFun);

	// Read/write government reputation conditions.
	// The erase function is still default (since we cannot erase government conditions).
	auto &&reputationProvider = conditions.GetProviderPrefixed("reputation: ");
	reputationProvider.SetHasFunction([](const string &name) -> bool
	{
		string govName = name.substr(strlen("reputation: "));
		return GameData::Governments().Has(govName);
	});
	reputationProvider.SetGetFunction([](const string &name) -> int64_t
	{
		string govName = name.substr(strlen("reputation: "));
		auto gov = GameData::Governments().Get(govName);
		if(!gov)
			return 0;
		return gov->Reputation();
	});
	reputationProvider.SetSetFunction([](const string &name, int64_t value) -> bool
	{
		string govName = name.substr(strlen("reputation: "));
		auto gov = GameData::Governments().Get(govName);
		if(!gov)
			return false;
		gov->SetReputation(value);
		return true;
	});

	// A condition for returning a random integer in the range [0, input). Input may be a number,
	// or it may be the name of a condition. For example, "roll: 100" would roll a random
	// integer in the range [0, 100), but if you had a condition "max roll" with a value of 100,
	// calling "roll: max roll" would provide a value from the same range.
	// Returns 0 if the input condition's value is <= 1.
	auto &&randomRollProvider = conditions.GetProviderPrefixed("roll: ");
	auto randomRollFun = [this](const string &name) -> int64_t
	{
		string input = name.substr(strlen("roll: "));
		int64_t value = 0;
		if(DataNode::IsNumber(input))
			value = static_cast<int64_t>(DataNode::Value(input));
		else
			value = conditions.Get(input);
		if(value <= 1)
			return 0;
		return Random::Int(value);
	};
	randomRollProvider.SetHasFunction(randomRollFun);
	randomRollProvider.SetGetFunction(randomRollFun);

	// Global conditions setters and getters:
	auto &&globalProvider = conditions.GetProviderPrefixed("global: ");
	globalProvider.SetHasFunction([](const string &name) -> bool
	{
		string condition = name.substr(strlen("global: "));
		return GameData::GlobalConditions().Has(condition);
	});
	globalProvider.SetGetFunction([](const string &name) -> int64_t
	{
		string condition = name.substr(strlen("global: "));
		return GameData::GlobalConditions().Get(condition);
	});
	globalProvider.SetSetFunction([](const string &name, int64_t value) -> bool
	{
		string condition = name.substr(strlen("global: "));
		return GameData::GlobalConditions().Set(condition, value);
	});
	globalProvider.SetEraseFunction([](const string &name) -> bool
	{
		string condition = name.substr(strlen("global: "));
		return GameData::GlobalConditions().Erase(condition);
	});
}



// New missions are generated each time you land on a planet.
void PlayerInfo::CreateMissions()
{
	boardingMissions.clear();

	// Check for available missions.
	bool skipJobs = planet && !planet->GetPort().HasService(Port::ServicesType::JobBoard);
	bool hasPriorityMissions = false;
	for(const auto &it : GameData::Missions())
	{
		if(it.second.IsAtLocation(Mission::BOARDING) || it.second.IsAtLocation(Mission::ASSISTING))
			continue;
		if(skipJobs && it.second.IsAtLocation(Mission::JOB))
			continue;

		if(it.second.CanOffer(*this))
		{
			list<Mission> &missions =
				it.second.IsAtLocation(Mission::JOB) ? availableJobs : availableMissions;

			missions.push_back(it.second.Instantiate(*this));
			if(missions.back().IsFailed(*this))
				missions.pop_back();
			else if(!it.second.IsAtLocation(Mission::JOB))
				hasPriorityMissions |= missions.back().HasPriority();
		}
	}

	// If any of the available missions are "priority" missions, no other
	// special missions will be offered in the spaceport.
	if(hasPriorityMissions)
	{
		auto it = availableMissions.begin();
		while(it != availableMissions.end())
		{
			bool hasLowerPriorityLocation = it->IsAtLocation(Mission::SPACEPORT)
				|| it->IsAtLocation(Mission::SHIPYARD)
				|| it->IsAtLocation(Mission::OUTFITTER);
			if(hasLowerPriorityLocation && !it->HasPriority())
				it = availableMissions.erase(it);
			else
				++it;
		}
	}
	else if(availableMissions.size() > 1)
	{
		// Minor missions only get offered if no other missions (including other
		// minor missions) are competing with them. This is to avoid having two
		// or three missions pop up as soon as you enter the spaceport.
		auto it = availableMissions.begin();
		while(it != availableMissions.end())
		{
			if(it->IsMinor())
			{
				it = availableMissions.erase(it);
				if(availableMissions.size() <= 1)
					break;
			}
			else
				++it;
		}
	}
}



void PlayerInfo::SortAvailable()
{
	// Destinations: planets OR system. Only counting them, so the type doesn't matter.
	set<const void *> destinations;
	if(availableSortType == CONVENIENT)
	{
		for(const Mission &mission : Missions())
		{
			if(mission.IsVisible())
			{
				destinations.insert(mission.Destination());
				destinations.insert(mission.Destination()->GetSystem());

				for(const Planet *stopover : mission.Stopovers())
				{
					destinations.insert(stopover);
					destinations.insert(stopover->GetSystem());
				}

				for(const System *waypoint : mission.Waypoints())
					destinations.insert(waypoint);
			}
		}
	}
	availableJobs.sort([&](const Mission &lhs, const Mission &rhs) {
		// First, separate rush orders with deadlines, if wanted
		if(sortSeparateDeadline)
		{
			// availableSortAsc instead of true, to counter the reverse below
			if(!lhs.Deadline() && rhs.Deadline())
				return availableSortAsc;
			if(lhs.Deadline() && !rhs.Deadline())
				return !availableSortAsc;
		}
		// Then, separate greyed-out jobs you can't accept
		if(sortSeparatePossible)
		{
			if(lhs.CanAccept(*this) && !rhs.CanAccept(*this))
				return availableSortAsc;
			if(!lhs.CanAccept(*this) && rhs.CanAccept(*this))
				return !availableSortAsc;
		}
		// Sort by desired type:
		switch(availableSortType)
		{
			case CONVENIENT:
			{
				// Sorting by "convenience" means you already have a mission to a
				// planet. Missions at the same planet are sorted higher.
				// 0 : No convenient mission; 1: same system; 2: same planet (because both system+planet means 1+1 = 2)
				const int lConvenient = destinations.count(lhs.Destination()) + destinations.count(lhs.Destination()->GetSystem());
				const int rConvenient = destinations.count(rhs.Destination()) + destinations.count(rhs.Destination()->GetSystem());
				if(lConvenient < rConvenient)
					return true;
				if(lConvenient > rConvenient)
					return false;
			}
			// Tiebreaker for equal CONVENIENT is SPEED.
			case SPEED:
			{
				// A higher "Speed" means the mission takes less time, i.e. fewer
				// jumps.
				const int lJumps = lhs.ExpectedJumps();
				const int rJumps = rhs.ExpectedJumps();

				if(lJumps == rJumps)
				{
					// SPEED compares equal - follow through to tiebreaker 'case PAY' below
				}
				else if(lJumps > 0 && rJumps > 0)
				{
					// Lower values are better, so this '>' is not '<' as expected
					return lJumps > rJumps;
				}
				else
				{
					// Negative values indicate indeterminable mission paths.
					// e.g. through a wormhole, meaning lower values are worse.

					// A value of 0 indicates the mission destination is the
					// source, implying the actual path is complicated; consider
					// that slow, but not as bad as an indeterminable path.

					// Positive values are 'greater' because at least the number
					// of jumps is known. (Comparing two positive values is already
					// handled above, so the actual positive value doesn't matter.)

					// Compare the value when at least one value is not positive.
					return lJumps < rJumps;
				}
			}
			// Tiebreaker for equal SPEED is PAY.
			case PAY:
			{
				const int64_t lPay = lhs.DisplayedPayment();
				const int64_t rPay = rhs.DisplayedPayment();
				if(lPay < rPay)
					return true;
				else if(lPay > rPay)
					return false;
			}
			// Tiebreaker for equal PAY is ABC.
			case ABC:
			{
				if(lhs.Name() < rhs.Name())
					return true;
				else if(lhs.Name() > rhs.Name())
					return false;
			}
			// Tiebreaker fallback to keep sorting consistent is unique UUID:
			default:
				return lhs.UUID() < rhs.UUID();
		}
	});

	if(!availableSortAsc)
		availableJobs.reverse();
}



// Updates each mission upon landing, to perform landing actions (Stopover,
// Visit, Complete, Fail), and remove now-complete or now-failed missions.
void PlayerInfo::StepMissions(UI *ui)
{
	// Check for NPCs that have been destroyed without their destruction
	// being registered, e.g. by self-destruct:
	for(Mission &mission : missions)
		for(const NPC &npc : mission.NPCs())
			for(const shared_ptr<Ship> &ship : npc.Ships())
				if(ship->IsDestroyed())
					mission.Do(ShipEvent(nullptr, ship, ShipEvent::DESTROY), *this, ui);

	// Check missions for status changes from landing.
	string visitText;
	int missionVisits = 0;
	auto substitutions = map<string, string>{
		{"<first>", firstName},
		{"<last>", lastName}
	};
	if(Flagship())
		substitutions["<ship>"] = Flagship()->Name();

	auto mit = missions.begin();
	while(mit != missions.end())
	{
		Mission &mission = *mit;
		++mit;

		// If this is a stopover for the mission, perform the stopover action.
		mission.Do(Mission::STOPOVER, *this, ui);

		if(mission.IsFailed(*this))
			RemoveMission(Mission::FAIL, mission, ui);
		else if(mission.CanComplete(*this))
			RemoveMission(Mission::COMPLETE, mission, ui);
		else if(mission.Destination() == GetPlanet() && !freshlyLoaded)
		{
			mission.Do(Mission::VISIT, *this, ui);
			if(mission.IsUnique() || !mission.IsVisible())
				continue;

			// On visit dialogs are handled separately as to avoid a player
			// getting spammed by on visit dialogs if they are stacking jobs
			// from the same destination.
			if(visitText.empty())
			{
				const auto &text = mission.GetAction(Mission::VISIT).DialogText();
				if(!text.empty())
					visitText = Format::Replace(text, substitutions);
			}
			++missionVisits;
		}
	}
	if(!visitText.empty())
	{
		if(missionVisits > 1)
			visitText += "\n\t(You have " + Format::Number(missionVisits - 1) + " other unfinished "
				+ ((missionVisits > 2) ? "missions" : "mission") + " at this location.)";
		ui->Push(new Dialog(visitText));
	}
	// One mission's actions may influence another mission, so loop through one
	// more time to see if any mission is now completed or failed due to a change
	// that happened in another mission the first time through.
	mit = missions.begin();
	while(mit != missions.end())
	{
		Mission &mission = *mit;
		++mit;

		if(mission.IsFailed(*this))
			RemoveMission(Mission::FAIL, mission, ui);
		else if(mission.CanComplete(*this))
			RemoveMission(Mission::COMPLETE, mission, ui);
	}

	// Search for any missions that have failed but for which we are still
	// holding on to some cargo.
	set<const Mission *> active;
	for(const Mission &it : missions)
		active.insert(&it);

	vector<const Mission *> missionsToRemove;
	for(const auto &it : cargo.MissionCargo())
		if(!active.count(it.first))
			missionsToRemove.push_back(it.first);
	for(const auto &it : cargo.PassengerList())
		if(!active.count(it.first))
			missionsToRemove.push_back(it.first);
	for(const Mission *mission : missionsToRemove)
		cargo.RemoveMissionCargo(mission);
}



void PlayerInfo::Autosave() const
{
	if(!CanBeSaved() || filePath.length() < 4)
		return;

	string path = filePath.substr(0, filePath.length() - 4) + "~autosave.txt";
	Save(path);
}



void PlayerInfo::Save(const string &filePath) const
{
	if(transactionSnapshot)
		transactionSnapshot->SaveToPath(filePath);
	else
	{
		DataWriter out(filePath);
		Save(out);
	}
}



void PlayerInfo::Save(DataWriter &out) const
{
	// Basic player information and persistent UI settings:

	// Pilot information:
	out.Write("pilot", firstName, lastName);
	out.Write("date", date.Day(), date.Month(), date.Year());
	out.Write("system entry method", EntryToString(entry));
	if(previousSystem)
		out.Write("previous system", previousSystem->Name());
	if(system)
		out.Write("system", system->Name());
	if(planet)
		out.Write("planet", planet->TrueName());
	if(planet && planet->CanUseServices())
		out.Write("clearance");
	out.Write("playtime", playTime);
	// This flag is set if the player must leave the planet immediately upon
	// entering their ship (i.e. because a mission forced them to take off).
	if(shouldLaunch)
		out.Write("launching");
	for(const System *system : travelPlan)
		out.Write("travel", system->Name());
	if(travelDestination)
		out.Write("travel destination", travelDestination->TrueName());
	// Detect which ship number is the current flagship, for showing on LoadPanel.
	if(flagship)
	{
		for(auto it = ships.begin(); it != ships.end(); ++it)
			if(*it == flagship)
			{
				out.Write("flagship index", distance(ships.begin(), it));
				break;
			}
	}
	else
		out.Write("flagship index", -1);

	// Save the current setting for the map coloring;
	out.Write("map coloring", mapColoring);
	out.Write("map zoom", mapZoom);
	// Remember what categories are collapsed.
	for(const auto &it : collapsed)
	{
		// Skip panels where nothing was collapsed.
		if(it.second.empty())
			continue;

		out.Write("collapsed", it.first);
		out.BeginChild();
		{
			for(const auto &cit : it.second)
				out.Write(cit);
		}
		out.EndChild();
	}

	out.Write("reputation with");
	out.BeginChild();
	{
		for(const auto &it : GameData::Governments())
			if(!it.second.IsPlayer())
				out.Write(it.first, it.second.Reputation());
	}
	out.EndChild();

	out.Write("tribute received");
	out.BeginChild();
	{
		for(const auto &it : tributeReceived)
			if(it.second > 0)
				out.Write((it.first)->TrueName(), it.second);
	}
	out.EndChild();

	// Records of things you own:
	out.Write();
	out.WriteComment("What you own:");

	// Save all the data for all the player's ships.
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->Save(out);
		auto it = groups.find(ship.get());
		if(it != groups.end() && it->second)
			out.Write("groups", it->second);
	}
	if(!planetaryStorage.empty())
	{
		out.Write("storage");
		out.BeginChild();
		{
			for(const auto &it : planetaryStorage)
				if(!it.second.IsEmpty())
				{
					out.Write("planet", it.first->TrueName());
					out.BeginChild();
					{
						it.second.Save(out);
					}
					out.EndChild();
				}
		}
		out.EndChild();
	}
	if(!licenses.empty())
	{
		out.Write("licenses");
		out.BeginChild();
		{
			for(const string &license : licenses)
				out.Write(license);
		}
		out.EndChild();
	}

	// Save accounting information, cargo, and cargo cost bases.
	accounts.Save(out);
	cargo.Save(out);
	if(!costBasis.empty())
	{
		out.Write("basis");
		out.BeginChild();
		{
			for(const auto &it : costBasis)
				if(it.second)
					out.Write(it.first, it.second);
		}
		out.EndChild();
	}

	if(!stock.empty())
	{
		out.Write("stock");
		out.BeginChild();
		{
			using StockElement = pair<const Outfit *const, int>;
			WriteSorted(stock,
				[](const StockElement *lhs, const StockElement *rhs)
					{ return lhs->first->TrueName() < rhs->first->TrueName(); },
				[&out](const StockElement &it)
				{
					if(it.second)
						out.Write(it.first->TrueName(), it.second);
				});
		}
		out.EndChild();
	}
	depreciation.Save(out, date.DaysSinceEpoch());
	stockDepreciation.Save(out, date.DaysSinceEpoch());


	// Records of things you have done or are doing, or have happened to you:
	out.Write();
	out.WriteComment("What you've done:");

	// Save all missions (accepted, accepted-but-invalid, and available).
	for(const Mission &mission : missions)
		mission.Save(out);
	for(const Mission &mission : inactiveMissions)
		mission.Save(out);
	map<string, map<string, int>> offWorldMissionCargo;
	map<string, map<string, int>> offWorldMissionPassengers;
	for(const auto &it : ships)
	{
		const Ship &ship = *it;
		// If the ship is at the player's planet, its mission cargo allocation does not need to be saved.
		if(ship.GetPlanet() == planet)
			continue;
		for(const auto &cargo : ship.Cargo().MissionCargo())
			offWorldMissionCargo[cargo.first->UUID().ToString()][ship.UUID().ToString()] = cargo.second;
		for(const auto &passengers : ship.Cargo().PassengerList())
			offWorldMissionPassengers[passengers.first->UUID().ToString()][ship.UUID().ToString()] = passengers.second;
	}
	auto SaveMissionCargoDistribution = [&out](const map<string, map<string, int>> &toSave, bool passengers) -> void
	{
		if(passengers)
			out.Write("mission passengers");
		else
			out.Write("mission cargo");
		out.BeginChild();
		{
			out.Write("player ships");
			out.BeginChild();
			{
				for(const auto &it : toSave)
					for(const auto &sit : it.second)
						out.Write(it.first, sit.first, sit.second);
			}
			out.EndChild();
		}
		out.EndChild();
	};
	if(!offWorldMissionCargo.empty())
		SaveMissionCargoDistribution(offWorldMissionCargo, false);
	if(!offWorldMissionPassengers.empty())
		SaveMissionCargoDistribution(offWorldMissionPassengers, true);

	for(const Mission &mission : availableJobs)
		mission.Save(out, "available job");
	for(const Mission &mission : availableMissions)
		mission.Save(out, "available mission");
	out.Write("sort type", static_cast<int>(availableSortType));
	if(!availableSortAsc)
		out.Write("sort descending");
	if(sortSeparateDeadline)
		out.Write("separate deadline");
	if(sortSeparatePossible)
		out.Write("separate possible");

	// Save any "primary condition" flags that are set.
	conditions.Save(out);

	// Save the UUID of any ships given to the player with a specified name, and ship class.
	if(!giftedShips.empty())
	{
		out.Write("gifted ships");
		out.BeginChild();
		{
			for(const auto &it : giftedShips)
				out.Write(it.first, it.second.ToString());
		}
		out.EndChild();
	}

	// Save pending events, and changes that have happened due to past events.
	for(const GameEvent &event : gameEvents)
		event.Save(out);
	if(!dataChanges.empty())
	{
		out.Write("changes");
		out.BeginChild();
		{
			for(const DataNode &node : dataChanges)
				out.Write(node);
		}
		out.EndChild();
	}
	GameData::WriteEconomy(out);

	// Check which persons have been captured or destroyed.
	for(const auto &it : GameData::Persons())
		if(it.second.IsDestroyed())
			out.Write("destroyed", it.first);


	// Records of things you have discovered:
	out.Write();
	out.WriteComment("What you know:");

	// Save a list of systems the player has visited.
	WriteSorted(visitedSystems,
		[](const System *const *lhs, const System *const *rhs)
			{ return (*lhs)->Name() < (*rhs)->Name(); },
		[&out](const System *system)
		{
			out.Write("visited", system->Name());
		});

	// Save a list of planets the player has visited.
	WriteSorted(visitedPlanets,
		[](const Planet *const *lhs, const Planet *const *rhs)
			{ return (*lhs)->TrueName() < (*rhs)->TrueName(); },
		[&out](const Planet *planet)
		{
			out.Write("visited planet", planet->TrueName());
		});

	if(!harvested.empty())
	{
		out.Write("harvested");
		out.BeginChild();
		{
			using HarvestLog = pair<const System *, const Outfit *>;
			WriteSorted(harvested,
				[](const HarvestLog *lhs, const HarvestLog *rhs) -> bool
				{
					// Sort by system name and then by outfit name.
					if(lhs->first != rhs->first)
						return lhs->first->Name() < rhs->first->Name();
					else
						return lhs->second->TrueName() < rhs->second->TrueName();
				},
				[&out](const HarvestLog &it)
				{
					out.Write(it.first->Name(), it.second->TrueName());
				});
		}
		out.EndChild();
	}

	out.Write("logbook");
	out.BeginChild();
	{
		for(auto &&it : logbook)
		{
			out.Write(it.first.Day(), it.first.Month(), it.first.Year());
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(it.second, "\n\t"))
					out.Write(line);
			}
			out.EndChild();
		}
		for(auto &&it : specialLogs)
			for(auto &&eit : it.second)
			{
				out.Write(it.first, eit.first);
				out.BeginChild();
				{
					// Break the text up into paragraphs.
					for(const string &line : Format::Split(eit.second, "\n\t"))
						out.Write(line);
				}
				out.EndChild();
			}
	}
	out.EndChild();

	out.Write();
	out.WriteComment("How you began:");
	startData.Save(out);

	// Write plugins to player's save file for debugging.
	out.Write();
	out.WriteComment("Installed plugins:");
	out.Write("plugins");
	out.BeginChild();
	for(const auto &it : Plugins::Get())
	{
		const auto &plugin = it.second;
		if(plugin.IsValid() && plugin.enabled)
			out.Write(plugin.name);
	}
	out.EndChild();
}



// Check (and perform) any fines incurred by planetary security. If the player
// has dominated the planet, or was given clearance to this planet by a mission,
// planetary security is avoided. Infiltrating implies evasion of security.
void PlayerInfo::Fine(UI *ui)
{
	const Planet *planet = GetPlanet();
	// Dominated planets should never fine you.
	// By default, uninhabited planets should not fine the player.
	if(GameData::GetPolitics().HasDominated(planet)
		|| !(planet->IsInhabited() || planet->HasCustomSecurity()))
		return;

	// Planets should not fine you if you have mission clearance or are infiltrating.
	for(const Mission &mission : missions)
		if(mission.HasClearance(planet) || (!mission.HasFullClearance() &&
					(mission.Destination() == planet || mission.Stopovers().count(planet))))
			return;

	// The planet's government must have the authority to enforce laws.
	const Government *gov = planet->GetGovernment();
	if(!gov->CanEnforce(planet))
		return;

	string message = gov->Fine(*this, 0, nullptr, planet->Security());
	if(!message.empty())
	{
		if(message == "atrocity")
		{
			const Conversation *conversation = gov->DeathSentence();
			if(conversation)
				ui->Push(new ConversationPanel(*this, *conversation));
			else
			{
				message = "Before you can leave your ship, the " + gov->GetName()
					+ " authorities show up and begin scanning it. They say, \"Captain "
					+ LastName()
					+ ", we detect highly illegal material on your ship.\""
					"\n\tYou are sentenced to lifetime imprisonment on a penal colony."
					" Your days of traveling the stars have come to an end.";
				ui->Push(new Dialog(message));
			}
			// All ships belonging to the player should be removed.
			Die();
		}
		else
			ui->Push(new Dialog(message));
	}
}



// Helper function to update the ship selection.
void PlayerInfo::SelectShip(const shared_ptr<Ship> &ship, bool *first)
{
	// Make sure this ship is not already selected.
	auto it = selectedShips.begin();
	for( ; it != selectedShips.end(); ++it)
		if(it->lock() == ship)
			break;
	if(it == selectedShips.end())
	{
		// This ship is not yet selected.
		selectedShips.push_back(ship);
		Ship *flagship = Flagship();
		if(*first && flagship && ship.get() != flagship)
		{
			flagship->SetTargetShip(ship);
			*first = false;
		}
	}
}

bool PlayerInfo::DisplayCarrierHelp() const
{
	return displayCarrierHelp;
}



// Instantiate the given model and add it to the player's fleet.
void PlayerInfo::AddStockShip(const Ship *model, const string &name)
{
	ships.push_back(make_shared<Ship>(*model));
	ships.back()->SetName(!name.empty() ? name : GameData::Phrases().Get("civilian")->Get());
	ships.back()->SetSystem(system);
	ships.back()->SetPlanet(planet);
	ships.back()->SetIsSpecial();
	ships.back()->SetIsYours();
	ships.back()->SetGovernment(GameData::PlayerGovernment());
}



// When we remove a ship, forget its stored ID.
void PlayerInfo::ForgetGiftedShip(const Ship &oldShip, bool failsMissions)
{
	const EsUuid &id = oldShip.UUID();
	auto shipToForget = find_if(giftedShips.begin(), giftedShips.end(),
		[&id](const pair<const string, EsUuid> &shipId) { return shipId.second == id; });
	if(shipToForget != giftedShips.end())
	{
		if(failsMissions)
			for(auto &mission : missions)
				if(mission.RequiresGiftedShip(shipToForget->first))
					mission.Fail();
		giftedShips.erase(shipToForget);
	}
}



// Check that this player's current state can be saved.
bool PlayerInfo::CanBeSaved() const
{
	return (!isDead && planet && system && !firstName.empty() && !lastName.empty());
}
