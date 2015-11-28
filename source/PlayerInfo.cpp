/* PlayerInfo.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PlayerInfo.h"

#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Files.h"
#include "GameData.h"
#include "Government.h"
#include "Messages.h"
#include "Mission.h"
#include "Outfit.h"
#include "Person.h"
#include "Planet.h"
#include "Politics.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "StartConditions.h"
#include "System.h"
#include "UI.h"

#include <ctime>
#include <sstream>

using namespace std;



// Completely clear all loaded information, to prepare for loading a file or
// creating a new pilot.
void PlayerInfo::Clear()
{
	*this = PlayerInfo();
	
	Random::Seed(time(NULL));
}



// Check if a player has been loaded.
bool PlayerInfo::IsLoaded() const
{
	return !firstName.empty();
}



// Make a new player.
void PlayerInfo::New()
{
	// Clear any previously loaded data.
	Clear();
	
	// Load starting conditions from a "start" item in the data files. If no
	// such item exists, StartConditions defines default values.
	date = GameData::Start().GetDate();
	GameData::SetDate(date);
	
	SetSystem(GameData::Start().GetSystem());
	SetPlanet(GameData::Start().GetPlanet());
	accounts = GameData::Start().GetAccounts();
	GameData::Start().GetConditions().Apply(conditions);
	
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
	
	filePath = path;
	DataFile file(path);
	
	hasFullClearance = false;
	for(const DataNode &child : file)
	{
		if(child.Token(0) == "pilot" && child.Size() >= 3)
		{
			firstName = child.Token(1);
			lastName = child.Token(2);
		}
		else if(child.Token(0) == "date" && child.Size() >= 4)
			date = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			system = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			planet = GameData::Planets().Get(child.Token(1));
		else if(child.Token(0) == "travel" && child.Size() >= 2)
			travelPlan.push_back(GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "reputation with")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					reputationChanges.emplace_back(
						GameData::Governments().Get(grand.Token(0)), grand.Value(1));
		}
		else if(child.Token(0) == "account")
			accounts.Load(child);
		else if(child.Token(0) == "visited" && child.Size() >= 2)
			Visit(GameData::Systems().Get(child.Token(1)));
		else if(child.Token(0) == "destroyed" && child.Size() >= 2)
			destroyedPersons.push_back(GameData::Persons().Get(child.Token(1)));
		else if(child.Token(0) == "cargo")
			cargo.Load(child);
		else if(child.Token(0) == "basis")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					costBasis[grand.Token(0)] += grand.Value(1);
		}
		else if(child.Token(0) == "mission")
		{
			missions.push_back(Mission());
			missions.back().Load(child);
			cargo.AddMissionCargo(&missions.back());
		}
		else if(child.Token(0) == "available job")
		{
			availableJobs.push_back(Mission());
			availableJobs.back().Load(child);
		}
		else if(child.Token(0) == "available mission")
		{
			availableMissions.push_back(Mission());
			availableMissions.back().Load(child);
		}
		else if(child.Token(0) == "conditions")
		{
			for(const DataNode &grand : child)
				conditions[grand.Token(0)] = (grand.Size() >= 2) ? grand.Value(1) : 1;
		}
		else if(child.Token(0) == "launching")
			shouldLaunch = true;
		else if(child.Token(0) == "changes")
		{
			for(const DataNode &grand : child)
				dataChanges.push_back(grand);
		}
		else if(child.Token(0) == "event")
		{
			gameEvents.push_back(GameEvent());
			gameEvents.back().Load(child);
		}
		else if(child.Token(0) == "clearance")
			hasFullClearance = true;
		else if(child.Token(0) == "ship")
		{
			// Ships owned by the player have various special characteristics:
			ships.push_back(shared_ptr<Ship>(new Ship()));
			ships.back()->Load(child);
			ships.back()->SetIsSpecial();
			ships.back()->SetGovernment(GameData::PlayerGovernment());
			ships.back()->FinishLoading();
			ships.back()->SetIsYours();
		}
	}
	// Based on the ships that were loaded, calculate the player's capacity for
	// cargo and passengers.
	UpdateCargoCapacities();
	
	// Strip anything after the "~" from snapshots, so that the file we save
	// will be the auto-save, not the snapshot.
	size_t pos = filePath.rfind('~');
	size_t namePos = filePath.length() - Files::Name(filePath).length();
	if(pos != string::npos && pos > namePos)
		filePath = filePath.substr(0, pos) + ".txt";
	
	// If a system was not specified in the player data, but the flagship is in
	// a particular system, set the system to that.
	if(!system && !ships.empty())
		system = ships.front()->GetSystem();
	
	// For any ship that did not store what system it is in or what planet it is
	// on, place it with the player. (In practice, every ship ought to have
	// specified its location, but this is to avoid null locations.)
	for(shared_ptr<Ship> &ship : ships)
	{
		if(!ship->GetSystem())
			ship->SetSystem(system);
		if(ship->GetSystem() == system)
			ship->SetPlanet(planet);
	}
}



// Load the most recently saved player (if any).
void PlayerInfo::LoadRecent()
{
	string recentPath = Files::Read(Files::Config() + "recent.txt");
	size_t pos = recentPath.find('\n');
	if(pos != string::npos)
		recentPath.erase(pos);
	
	if(recentPath.empty())
		Clear();
	else
		Load(recentPath);
}



// Save this player. The file name is based on the player's name.
void PlayerInfo::Save() const
{
	// Don't save dead players.
	if(isDead)
		return;
	
	// Remember that this was the most recently saved player.
	Files::Write(Files::Config() + "recent.txt", filePath + '\n');
	
	Save(filePath);
}



// Get the base file name for the player, without the ".txt" extension. This
// will usually be "<first> <last>", but may be different if multiple players
// exist with the same name, in which case a number is appended.
string PlayerInfo::Identifier() const
{
	string name = Files::Name(filePath);
	return (name.length() < 4) ? "" : name.substr(0, name.length() - 4);
}



// Apply any "changes" saved in this player info to the global game state.
void PlayerInfo::ApplyChanges()
{
	for(const auto &it : reputationChanges)
		it.first->SetReputation(it.second);
	reputationChanges.clear();
	AddChanges(dataChanges);
	
	// Make sure all stellar objects are correctly positioned. This is needed
	// because EnterSystem() is not called the first time through.
	GameData::SetDate(GetDate());
	// SetDate() clears any bribes from yesterday, so restore any auto-clearance.
	for(const Mission &mission : Missions())
		if(mission.ClearanceMessage() == "auto")
			mission.Destination()->Bribe(mission.HasFullClearance());
	if(system)
		GameData::GetPolitics().Bribe(system->GetGovernment());
	
	// It is sometimes possible for the player to be landed on a planet where
	// they do not have access to any services. So, this flag is used to specify
	// that in this case, the player has access to the planet's services.
	if(planet && hasFullClearance)
		planet->Bribe();
	hasFullClearance = false;
	
	// Check if any special persons have been destroyed.
	while(!destroyedPersons.empty())
	{
		destroyedPersons.back()->GetShip()->Destroy();
		destroyedPersons.pop_back();
	}
	
	// Check which planets you have dominated.
	static const string prefix = "tribute: ";
	for(auto it = conditions.lower_bound(prefix); it != conditions.end(); ++it)
	{
		if(it->first.compare(0, prefix.length(), prefix))
			break;
		
		const Planet *planet = GameData::Planets().Get(it->first.substr(prefix.length()));
		GameData::GetPolitics().DominatePlanet(planet);
	}
}



// Apply the given set of changes to the game data.
void PlayerInfo::AddChanges(list<DataNode> &changes)
{
	for(const DataNode &change : changes)
		GameData::Change(change);
	
	// Only move the changes into my list if they are not already there.
	if(&changes != &dataChanges)
		dataChanges.splice(dataChanges.end(), changes);
}



// Add an event that will happen at the given date.
void PlayerInfo::AddEvent(const GameEvent &event, const Date &date)
{
	gameEvents.push_back(event);
	gameEvents.back().SetDate(date);
}



// Mark this player as dead.
void PlayerInfo::Die(bool allShipsDie)
{
	isDead = true;
	if(allShipsDie)
		ships.clear();
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
	while(it != gameEvents.end())
	{
		if(date < it->GetDate())
			++it;
		else
		{
			it->Apply(*this);
			it = gameEvents.erase(it);
		}
	}
	
	// Check if any missions have failed because of deadlines.
	for(Mission &mission : missions)
		if(mission.CheckDeadline(date) && mission.IsVisible())
			Messages::Add("You failed to meet the deadline for the mission \"" + mission.Name() + "\".");
	
	// Check what salaries and tribute the player receives.
	int total[2] = {0, 0};
	static const string prefix[2] = {"salary: ", "tribute: "};
	for(int i = 0; i < 2; ++i)
	{
		auto it = conditions.lower_bound(prefix[i]);
		for( ; it != conditions.end() && !it->first.compare(0, prefix[i].length(), prefix[i]); ++it)
			total[i] += it->second;
	}
	if(total[0] || total[1])
	{
		string message = "You receive ";
		if(total[0])
			message += to_string(total[0]) + " credits salary";
		if(total[0] && total[1])
			message += " and ";
		if(total[1])
			message += to_string(total[1]) + " credits in tribute";
		message += ".";
		Messages::Add(message);
		accounts.AddCredits(total[0] + total[1]);
	}
	
	// For accounting, keep track of the player's net worth. This is for
	// calculation of yearly income to determine maximum mortgage amounts.
	int64_t assets = 0;
	for(const shared_ptr<Ship> &ship : ships)
		assets += ship->Cost() + ship->Cargo().Value(system);
	
	// Have the player pay salaries, mortgages, etc. and print a message that
	// summarizes the payments that were made.
	string message = accounts.Step(assets, Salaries());
	if(!message.empty())
		Messages::Add(message);
}



// Set the player's current start system, and mark that system as visited.
void PlayerInfo::SetSystem(const System *system)
{
	this->system = system;
	Visit(system);
}



// Get the player's current star system.
const System *PlayerInfo::GetSystem() const
{
	return system;
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



// Get a pointer to the ship that the player controls. This is always the first
// ship in the list.
const Ship *PlayerInfo::Flagship() const
{
	return const_cast<PlayerInfo *>(this)->FlagshipPtr().get();
}



// Get a pointer to the ship that the player controls. This is always the first
// ship in the list.
Ship *PlayerInfo::Flagship()
{
	return FlagshipPtr().get();
}



const shared_ptr<Ship> &PlayerInfo::FlagshipPtr()
{
	if(!flagship)
	{
		for(const shared_ptr<Ship> &it : ships)
			if(!it->IsParked() && it->GetSystem() == system && !it->CanBeCarried())
				return it;
	}
	
	return flagship;
}



// Access the full list of ships that the player owns.
const vector<shared_ptr<Ship>> &PlayerInfo::Ships() const
{
	return ships;
}



// Add a captured ship to your fleet.
void PlayerInfo::AddShip(shared_ptr<Ship> &ship)
{
	ships.push_back(ship);
	ship->SetIsSpecial();
	ship->SetIsYours();
}



// Buy a ship of the given model, and give it the given name.
void PlayerInfo::BuyShip(const Ship *model, const string &name)
{
	if(model && accounts.Credits() >= model->Cost())
	{
		ships.push_back(shared_ptr<Ship>(new Ship(*model)));
		ships.back()->SetName(name);
		ships.back()->SetSystem(system);
		ships.back()->SetPlanet(planet);
		ships.back()->SetIsSpecial();
		ships.back()->SetIsYours();
		ships.back()->SetGovernment(GameData::PlayerGovernment());
		
		accounts.AddCredits(-model->Cost());
	}
}



// Sell the given ship (if it belongs to the player).
void PlayerInfo::SellShip(const Ship *selected)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			for(const auto &it : selected->Outfits())
				soldOutfits[it.first] += it.second;
			
			accounts.AddCredits(selected->Cost());
			ships.erase(it);
			return;
		}
}



// Park or unpark the given ship. A parked ship remains on a planet instead of
// flying with the player, and requires no daily upkeep.
void PlayerInfo::ParkShip(const Ship *selected, bool isParked)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			(*it)->SetIsParked(isParked);
			UpdateCargoCapacities();
			return;
		}
}



// Rename the given ship.
void PlayerInfo::RenameShip(const Ship *selected, const string &name)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			(*it)->SetName(name);
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



// Adjust the cost basis for the given commodity.
void PlayerInfo::AdjustBasis(const string &commodity, int64_t adjustment)
{
	costBasis[commodity] += adjustment;
}



// Get the cost basis for some number of tons of the given commodity. Each ton
// of the commodity that you own is assumed to have the same basis.
int64_t PlayerInfo::GetBasis(const string &commodity, int tons) const
{
	int total = cargo.Get(commodity);
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
	
	// Remove any ships that have been destroyed or captured.
	map<string, int> lostCargo;
	vector<shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if(!*it || (*it)->IsDestroyed() || !(*it)->GetGovernment()->IsPlayer())
		{
			// If any of your ships are destroyed, your cargo "cost basis" should
			// be adjusted based on what you lost.
			for(const auto &cargo : (*it)->Cargo().Commodities())
				if(cargo.second)
					lostCargo[cargo.first] += cargo.second;
			
			it = ships.erase(it);
		}
		else
			++it; 
	}
	
	// "Unload" all fighters, so they will get recharged, etc.
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsDisabled())
			ship->UnloadFighters();
	
	// Recharge any ships that are landed with you on the planet.
	bool canRecharge = planet->HasSpaceport() && planet->CanUseServices();
	UpdateCargoCapacities();
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system && !ship->IsDisabled())
		{
			if(canRecharge)
				ship->Recharge();
			
			ship->Cargo().TransferAll(&cargo);
		}
	// Adjust cargo cost basis for any cargo lost due to a ship being destroyed.
	for(const auto &it : lostCargo)
		AdjustBasis(it.first, -(costBasis[it.first] * it.second) / (cargo.Get(it.first) + it.second));
	
	// Check for missions that are completed.
	auto mit = missions.begin();
	while(mit != missions.end())
	{
		const Mission &mission = *mit;
		++mit;
		
		if(mission.HasFailed(*this))
			RemoveMission(Mission::FAIL, mission, ui);
		else if(mission.CanComplete(*this))
			RemoveMission(Mission::COMPLETE, mission, ui);
		else if(mission.Destination() == GetPlanet())
			mission.Do(Mission::VISIT, *this, ui);
	}
	UpdateCargoCapacities();
	
	// Create whatever missions this planet has to offer.
	if(!freshlyLoaded)
		CreateMissions();
	freshlyLoaded = false;
	
	// Search for any missions that have failed but for which we are still
	// holding on to some cargo.
	set<const Mission *> active;
	for(const Mission &it : missions)
		active.insert(&it);
	
	vector<const Mission *> missionsToRemove;
	for(const auto &it : cargo.MissionCargo())
		if(active.find(it.first) == active.end())
			missionsToRemove.push_back(it.first);
	for(const auto &it : cargo.PassengerList())
		if(active.find(it.first) == active.end())
			missionsToRemove.push_back(it.first);
	for(const Mission *mission : missionsToRemove)
		cargo.RemoveMissionCargo(mission);
	
	// Check if the player is doing anything illegal.
	const Government *gov = GetSystem()->GetGovernment();
	string message = gov->Fine(*this, 0, nullptr, GetPlanet()->Security());
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
			Die();
		}
		else
			ui->Push(new Dialog(message));
	}
	
	flagship.reset();
}



// Load the cargo back into your ships. This may require selling excess, in
// which case a message will be returned.
void PlayerInfo::TakeOff(UI *ui)
{
	shouldLaunch = false;
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	// Jobs are only available when you are landed.
	availableJobs.clear();
	availableMissions.clear();
	doneMissions.clear();
	soldOutfits.clear();
	
	// Special persons who appeared last time you left the planet, can appear
	// again.
	for(const auto &it : GameData::Persons())
		it.second.GetShip()->SetSystem(nullptr);
	
	// Store the total cargo counts in case we need to adjust cost bases below.
	map<string, int> originalTotals = cargo.Commodities();
	
	flagship = FlagshipPtr();
	if(!flagship)
		return;
	// Move the flagship to the start of your list of ships. It does not make
	// sense that the flagship would change if you are reunited with a different
	// ship that was higher up the list.
	auto it = find(ships.begin(), ships.end(), flagship);
	if(it != ships.begin() && it != ships.end())
	{
		ships.erase(it);
		ships.insert(ships.begin(), flagship);
	}
	// Make sure your ships all know who the flagship is.
	flagship->SetParent(shared_ptr<Ship>());
	for(const shared_ptr<Ship> &ship : ships)
		if(ship != flagship)
			ship->SetParent(flagship);
	
	// Recharge any ships that can be recharged.
	bool canRecharge = planet->HasSpaceport() && planet->CanUseServices();
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && ship->GetSystem() == system && !ship->IsDisabled())
		{
			if(canRecharge)
				ship->Recharge();
			if(ship != flagship)
			{
				ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - ship->RequiredCrew());
				cargo.TransferAll(&ship->Cargo());
			}
		}
	// Load up your flagship last, so that it will have space free for any
	// plunder that you happen to acquire.
	flagship->Cargo().SetBunks(flagship->Attributes().Get("bunks") - flagship->RequiredCrew());
	cargo.TransferAll(&flagship->Cargo());

	if(cargo.Passengers())
	{
		int extra = min(cargo.Passengers(), flagship->Crew() - flagship->RequiredCrew());
		if(extra)
		{
			flagship->AddCrew(-extra);
			Messages::Add("You fired " + to_string(extra) + " crew members to free up bunks for passengers.");
			flagship->Cargo().SetBunks(flagship->Attributes().Get("bunks") - flagship->Crew());
			cargo.TransferAll(&flagship->Cargo());
		}
	}

	int extra = flagship->Crew() + flagship->Cargo().Passengers() - flagship->Attributes().Get("bunks");
	if(extra > 0)
	{
		flagship->AddCrew(-extra);
		Messages::Add("You fired " + to_string(extra) + " crew members because you have no bunks for them.");
		flagship->Cargo().SetBunks(flagship->Attributes().Get("bunks") - flagship->Crew());
	}
	
	// For each fighter and drone you own, try to find a ship that has a bay to
	// carry it in. Any excess ships will need to be sold.
	vector<shared_ptr<Ship>> fighters;
	vector<shared_ptr<Ship>> drones;
	for(shared_ptr<Ship> &ship : ships)
	{
		if(ship->IsParked() || ship->IsDisabled())
			continue;
		
		bool fit = false;
		const string &category = ship->Attributes().Category();
		if(category == "Fighter")
		{
			for(shared_ptr<Ship> &parent : ships)
				if(parent->GetSystem() == ship->GetSystem() && !parent->IsParked()
						&& !parent->IsDisabled() && parent->FighterBaysFree())
				{
					parent->AddFighter(ship);
					fit = true;
					break;
				}
			if(!fit)
				fighters.push_back(ship);
		}
		else if(category == "Drone")
		{
			for(shared_ptr<Ship> &parent : ships)
				if(parent->GetSystem() == ship->GetSystem() && !parent->IsParked()
						&& !parent->IsDisabled() && parent->DroneBaysFree())
				{
					parent->AddFighter(ship);
					fit = true;
					break;
				}
			if(!fit)
				drones.push_back(ship);
		}
	}
	if(!drones.empty() || !fighters.empty())
	{
		// If your fleet contains more fighters or drones than you can carry,
		// some of them must be sold.
		ostringstream out;
		out << "Because none of your ships can carry them, you sold ";
		if(!fighters.empty() && !drones.empty())
			out << fighters.size()
				<< (fighters.size() == 1 ? " fighter and " : " fighters and ")
				<< drones.size()
				<< (drones.size() == 1 ? " drone" : " drones");
		else if(fighters.size())
			out << fighters.size()
				<< (fighters.size() == 1 ? " fighter" : " fighters");
		else
			out << drones.size()
				<< (drones.size() == 1 ? " drone" : " drones");
		
		int64_t income = 0;
		for(const shared_ptr<Ship> &ship : fighters)
		{
			auto it = find(ships.begin(), ships.end(), ship);
			if(it != ships.end())
			{
				income += ship->Cost();
				ships.erase(it);
			}
		}
		for(const shared_ptr<Ship> &ship : drones)
		{
			auto it = find(ships.begin(), ships.end(), ship);
			if(it != ships.end())
			{
				income += ship->Cost();
				ships.erase(it);
			}
		}
		
		out << ", earning " << income << " credits.";
		accounts.AddCredits(income);
		Messages::Add(out.str());
	}
	
	// By now, all cargo should have been divvied up among your ships. So, any
	// mission cargo or passengers left behind cannot be carried, and those
	// missions have failed.
	vector<const Mission *> missionsToRemove;
	for(const auto &it : cargo.MissionCargo())
		if(it.second)
		{
			Messages::Add("Mission \"" + it.first->Name()
				+ "\" failed because you do not have space for the cargo.");
			missionsToRemove.push_back(it.first);
		}
	for(const auto &it : cargo.PassengerList())
		if(it.second)
		{
			Messages::Add("Mission \"" + it.first->Name()
				+ "\" failed because you do not have enough passenger bunks free.");
			missionsToRemove.push_back(it.first);
			
		}
	for(const Mission *mission : missionsToRemove)
		RemoveMission(Mission::FAIL, *mission, ui);
	
	// Any ordinary cargo left behind can be sold.
	int64_t sold = cargo.Used();
	int64_t income = 0;
	int64_t totalBasis = 0;
	if(sold)
		for(const auto &commodity : cargo.Commodities())
		{
			if(!commodity.second)
				continue;
			
			// Figure out how much income you get for selling this cargo.
			int64_t value = commodity.second * system->Trade(commodity.first);
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
	accounts.AddCredits(income);
	cargo.Clear();
	if(sold)
	{
		// Report how much excess cargo was sold, and what profit you earned.
		ostringstream out;
		out << "You sold " << sold << " tons of excess cargo for " << income << " credits";
		if(totalBasis && totalBasis != income)
			out << " (for a profit of " << (income - totalBasis) << " credits).";
		else
			out << ".";
		Messages::Add(out.str());
	}
}



// Call this when leaving the outfitter, shipyard, or hiring panel, to update
// the information on how much space is available.
void PlayerInfo::UpdateCargoCapacities()
{
	int size = 0;
	int bunks = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system && !ship->IsParked() && !ship->IsDisabled())
		{
			size += ship->Attributes().Get("cargo space");
			bunks += ship->Attributes().Get("bunks") - ship->Crew();
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



// Accept the given job.
void PlayerInfo::AcceptJob(const Mission &mission)
{
	for(auto it = availableJobs.begin(); it != availableJobs.end(); ++it)
		if(&*it == &mission)
		{
			cargo.AddMissionCargo(&mission);
			it->Do(Mission::OFFER, *this);
			it->Do(Mission::ACCEPT, *this);
			auto spliceIt = it->IsUnique() ? missions.begin() : missions.end();
			missions.splice(spliceIt, availableJobs, it);
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
		if(it->IsAtLocation(location) && it->CanOffer(*this) && it->HasSpace(*this))
		{
			availableMissions.splice(availableMissions.begin(), availableMissions, it);
			return &availableMissions.front();
		}
	return nullptr;
}



Mission *PlayerInfo::BoardingMission(const shared_ptr<Ship> &ship)
{
	// Do not create missions from NPC's or the player's ships.
	if(ship->IsSpecial())
		return nullptr;
	ship->SetIsSpecial();
	
	UpdateAutoConditions();
	boardingMissions.clear();
	boardingShip = ship;
	
	bool isEnemy = ship->GetGovernment()->IsEnemy();
	Mission::Location location = (isEnemy ? Mission::BOARDING : Mission::ASSISTING);
	
	// Check for available missions.
	for(const auto &it : GameData::Missions())
	{
		if(!it.second.IsAtLocation(location))
			continue;
		
		conditions["random"] = Random::Int(100);
		if(it.second.CanOffer(*this))
		{
			boardingMissions.push_back(it.second.Instantiate(*this));
			if(boardingMissions.back().HasFailed(*this))
				boardingMissions.pop_back();
			else
				return &boardingMissions.back();
		}
	}
	boardingShip.reset();
	return nullptr;
}



const shared_ptr<Ship> &PlayerInfo::BoardingShip() const
{
	return boardingShip;
}



// If one of your missions cannot be offered because you do not have enough
// space for it, and it specifies a message to be shown in that situation,
// show that message.
void PlayerInfo::HandleBlockedMissions(Mission::Location location, UI *ui)
{
	if(ships.empty())
		return;
	
	// If a mission can be offered right now, move it to the start of the list
	// so we know what mission the callback is referring to, and return it.
	for(auto it = availableMissions.begin(); it != availableMissions.end(); ++it)
		if(it->IsAtLocation(location) && it->CanOffer(*this) && !it->HasSpace(*this))
		{
			string message = it->BlockedMessage(*this);
			if(!message.empty())
			{
				ui->Push(new Dialog(message));
				return;
			}
		}
}



// Callback for accepting or declining whatever mission has been offered.
void PlayerInfo::MissionCallback(int response)
{
	boardingShip.reset();
	list<Mission> &missionList = availableMissions.empty() ? boardingMissions : availableMissions;
	if(missionList.empty())
		return;
	
	Mission &mission = missionList.front();
	
	shouldLaunch = (response == Conversation::LAUNCH || response == Conversation::FLEE);
	if(response == Conversation::ACCEPT || response == Conversation::LAUNCH)
	{
		bool shouldAutosave = mission.RecommendsAutosave();
		cargo.AddMissionCargo(&mission);
		auto spliceIt = mission.IsUnique() ? missions.begin() : missions.end();
		missions.splice(spliceIt, missionList, missionList.begin());
		UpdateCargoCapacities();
		mission.Do(Mission::ACCEPT, *this);
		if(shouldAutosave)
			Autosave();
	}
	else if(response == Conversation::DECLINE)
	{
		mission.Do(Mission::DECLINE, *this);
		missionList.pop_front();
	}
	else if(response == Conversation::DEFER)
	{
		mission.Do(Mission::DEFER, *this);
		missionList.pop_front();
	}
	else if(response == Conversation::DIE)
		Die(true);
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
			
			mission.Do(trigger, *this, ui);
			cargo.RemoveMissionCargo(&mission);
			for(shared_ptr<Ship> &ship : ships)
				ship->Cargo().RemoveMissionCargo(&mission);
			return;
		}
}



// Update mission status based on an event.
void PlayerInfo::HandleEvent(const ShipEvent &event, UI *ui)
{
	// Combat rating increases when you disable an enemy ship.
	if(event.ActorGovernment()->IsPlayer())
		if((event.Type() & ShipEvent::DISABLE) && event.Target())
			conditions["combat rating"] += event.Target()->RequiredCrew();
	
	for(Mission &mission : missions)
		mission.Do(event, *this, ui);
	
	// If the player's flagship was destroyed, the player is dead.
	if((event.Type() & ShipEvent::DESTROY) && !ships.empty() && event.Target().get() == Flagship())
		Die();
}



// Get the value of the given condition (default 0).
int PlayerInfo::GetCondition(const string &name) const
{
	auto it = conditions.find(name);
	return (it == conditions.end()) ? 0 : it->second;
}



// Get mutable access to the player's list of conditions.
map<string, int> &PlayerInfo::Conditions()
{
	return conditions;
}



// Access the player's list of conditions.
const map<string, int> &PlayerInfo::Conditions() const
{
	return conditions;
}



// Check if the player knows the location of the given system (whether or not
// they have actually visited it).
bool PlayerInfo::HasSeen(const System *system) const
{
	return (seen.find(system) != seen.end() || KnowsName(system));
}



// Check if the player has visited the given system.
bool PlayerInfo::HasVisited(const System *system) const
{
	return (visited.find(system) != visited.end());
}



// Check if the player knows the name of a system, either from visiting there or
// because a job or active mission includes the name of that system.
bool PlayerInfo::KnowsName(const System *system) const
{
	if(HasVisited(system))
		return true;
	
	for(const Mission &mission : availableJobs)
		if(mission.Destination()->GetSystem() == system)
			return true;
	
	for(const Mission &mission : missions)
		if(mission.Destination()->GetSystem() == system)
			return true;
	
	return false;
}



// Mark the given system as visited, and mark all its neighbors as seen.
void PlayerInfo::Visit(const System *system)
{
	visited.insert(system);
	seen.insert(system);
	for(const System *neighbor : system->Neighbors())
		seen.insert(neighbor);
}



// Mark a system as unvisited, even if visited previously.
void PlayerInfo::Unvisit(const System *system)
{
	auto it = visited.find(system);
	if(it != visited.end())
		visited.erase(it);
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



// Clear the player's travel plan.
void PlayerInfo::ClearTravel()
{
	travelPlan.clear();
}



// Add to the travel plan, starting with the last system in the journey.
void PlayerInfo::AddTravel(const System *system)
{
	travelPlan.push_back(system);
}



// This is called when the player enters the system that is their current
// hyperspace target.
void PlayerInfo::PopTravel()
{
	if(!travelPlan.empty())
	{
		Visit(travelPlan.back());
		travelPlan.pop_back();
	}
}



// Check which secondary weapon the player has selected.
const Outfit *PlayerInfo::SelectedWeapon() const
{
	return selectedWeapon;
}



// Cycle through all available secondary weapons.
void PlayerInfo::SelectNext()
{
	if(!flagship || flagship->Outfits().empty())
		return;
	
	// Start with the currently selected weapon, if any.
	auto it = flagship->Outfits().find(selectedWeapon);
	if(it == flagship->Outfits().end())
		it = flagship->Outfits().begin();
	else
		++it;
	
	// Find the next secondary weapon.
	for( ; it != flagship->Outfits().end(); ++it)
		if(it->first->Icon())
		{
			selectedWeapon = it->first;
			return;
		}
	selectedWeapon = nullptr;
}



// Keep track of any outfits that you have sold since landing. These will be
// available to buy back until you take off.
map<const Outfit *, int> &PlayerInfo::SoldOutfits()
{
	return soldOutfits;
}



// Update the conditions that reflect the current status of the player.
void PlayerInfo::UpdateAutoConditions()
{
	// Set up the "conditions" for the current status of the player.
	for(const auto &it : GameData::Governments())
	{
		int rep = it.second.Reputation();
		conditions["reputation: " + it.first] = rep;
		if(&it.second == system->GetGovernment())
			conditions["reputation"] = rep;
	}
	// Store special conditions for cargo and passenger space.
	conditions["cargo space"] = 0;
	conditions["passenger space"] = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && !ship->IsDisabled() && ship->GetSystem() == system)
		{
			conditions["cargo space"] += ship->Attributes().Get("cargo space");
			conditions["passenger space"] += ship->Attributes().Get("bunks") - ship->RequiredCrew();
		}
}



// New missions are generated each time you land on a planet.
void PlayerInfo::CreateMissions()
{
	UpdateAutoConditions();
	boardingMissions.clear();
	boardingShip.reset();
	
	// Check for available missions.
	bool skipJobs = planet && !planet->HasSpaceport();
	bool hasPriorityMissions = false;
	for(const auto &it : GameData::Missions())
	{
		if(it.second.IsAtLocation(Mission::BOARDING))
			continue;
		if(skipJobs && it.second.IsAtLocation(Mission::JOB))
			continue;
		
		conditions["random"] = Random::Int(100);
		if(it.second.CanOffer(*this))
		{
			list<Mission> &missions =
				it.second.IsAtLocation(Mission::JOB) ? availableJobs : availableMissions;
			
			missions.push_back(it.second.Instantiate(*this));
			if(missions.back().HasFailed(*this))
				missions.pop_back();
			else
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
			if(it->IsAtLocation(Mission::SPACEPORT) && !it->HasPriority())
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



void PlayerInfo::Autosave() const
{
	if(filePath.length() < 4)
		return;
	
	string path = filePath.substr(0, filePath.length() - 4) + "~autosave.txt";
	Save(path);
}



void PlayerInfo::Save(const string &path) const
{
	DataWriter out(path);
	
	out.Write("pilot", firstName, lastName);
	out.Write("date", date.Day(), date.Month(), date.Year());
	if(system)
		out.Write("system", system->Name());
	if(planet)
		out.Write("planet", planet->Name());
	if(planet && planet->CanUseServices())
		out.Write("clearance");
	for(const System *system : travelPlan)
		out.Write("travel", system->Name());
	out.Write("reputation with");
	out.BeginChild();
	{
		for(const auto &it : GameData::Governments())
			if(!it.second.IsPlayer())
				out.Write(it.first, it.second.Reputation());
	}
	out.EndChild();
		
	// Save all the data for all the player's ships.
	for(const shared_ptr<Ship> &ship : ships)
		ship->Save(out);
	
	// Save accounting information, cargo, and cargo cost bases.
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
	accounts.Save(out);
	
	// Save all missions (accepted or available).
	for(const Mission &mission : missions)
		mission.Save(out);
	for(const Mission &mission : availableJobs)
		mission.Save(out, "available job");
	for(const Mission &mission : availableMissions)
		mission.Save(out, "available mission");
	
	// Save any "condition" flags that are set.
	if(!conditions.empty())
	{
		out.Write("conditions");
		out.BeginChild();
		{
			for(const auto &it : conditions)
			{
				// If the condition's value is 1, don't bother writing the 1.
				if(it.second == 1)
					out.Write(it.first);
				else if(it.second)
					out.Write(it.first, it.second);
			}
		}
		out.EndChild();
	}
	// This flag is set if the player must leave the planet immediately upon
	// loading the game (i.e. because a mission forced them to take off).
	if(shouldLaunch)
		out.Write("launching");
	
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
	
	for(const auto &it : GameData::Persons())
		if(it.second.GetShip()->IsDestroyed())
			out.Write("destroyed", it.first);
	
	// Save a list of systems the player has visited.
	for(const System *system : visited)
		out.Write("visited", system->Name());
}
