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

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "DistanceMap.h"
#include "Files.h"
#include "GameData.h"
#include "Messages.h"
#include "Mission.h"
#include "Outfit.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"

#include <sstream>

using namespace std;



PlayerInfo::PlayerInfo()
	: date(16, 11, 3013), system(nullptr), planet(nullptr), selectedWeapon(nullptr)
{
}



void PlayerInfo::Clear()
{
	filePath.clear();
	firstName.clear();
	lastName.clear();
	
	date = Date(16, 11, 3013);
	system = nullptr;
	planet = nullptr;
	shouldLaunch = false;
	isDead = false;
	accounts = Account();
	
	ships.clear();
	cargo.Clear();
	
	missions.clear();
	availableJobs.clear();
	availableMissions.clear();
	
	conditions.clear();
	
	seen.clear();
	visited.clear();
	travelPlan.clear();
	
	selectedWeapon = nullptr;
	freshlyLoaded = true;
	
#ifdef __APPLE__
	Random::Seed(time(NULL));
#else
	random_device rd;
	Random::Seed(rd());
#endif
}



// Don't allow copying, because the mission pointers won't get transferred properly.
void PlayerInfo::Steal(PlayerInfo &other)
{
	firstName.swap(other.firstName);
	lastName.swap(other.lastName);
	filePath.swap(other.filePath);
	
	Date date = other.date;
	system = other.system;
	planet = other.planet;
	shouldLaunch = false;
	isDead = other.isDead;
	accounts = other.accounts;
	
	ships = other.ships;
	cargo = other.cargo;
	
	missions.swap(other.missions);
	availableJobs.swap(other.availableJobs);
	availableMissions.swap(other.availableMissions);
	
	conditions.swap(other.conditions);
	
	seen.swap(other.seen);
	visited.swap(other.visited);
	travelPlan.swap(other.travelPlan);
	
	selectedWeapon = other.selectedWeapon;
	
	reputationChanges.swap(other.reputationChanges);
	
	freshlyLoaded = other.freshlyLoaded;
	
	other.Clear();
}



bool PlayerInfo::IsLoaded() const
{
	return !firstName.empty();
}



void PlayerInfo::Load(const string &path)
{
	Clear();
	
	filePath = path;
	DataFile file(path);
	
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
		else if(child.Token(0) == "cargo")
			cargo.Load(child);
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
				if(grand.Size() >= 2)
					conditions[grand.Token(0)] = grand.Value(1);
		}
		else if(child.Token(0) == "ship")
		{
			ships.push_back(shared_ptr<Ship>(new Ship()));
			ships.back()->Load(child);
			ships.back()->SetIsSpecial();
			ships.back()->SetGovernment(GameData::PlayerGovernment());
			if(ships.size() > 1)
			{
				ships.back()->SetParent(ships.front());
				ships.front()->AddEscort(ships.back());
			}
			ships.back()->FinishLoading();
		}
	}
	UpdateCargoCapacities();
	
	// Strip anything after the "~" from snapshots, so that the file we save
	// will be the auto-save, not the snapshot.
	size_t pos = filePath.rfind('~');
	if(pos != string::npos && pos > Files::Saves().size())
		filePath = filePath.substr(0, pos) + ".txt";
	
	if(!system && !ships.empty())
		system = ships.front()->GetSystem();
}



void PlayerInfo::Save() const
{
	if(isDead)
		return;
	
	{
		string recentPath = Files::Config() + "recent.txt";
		ofstream recent(recentPath);
		recent << filePath << "\n";
	}
	
	DataWriter out(filePath);
	
	out.Write("pilot", firstName, lastName);
	out.Write("date", date.Day(), date.Month(), date.Year());
	if(system)
		out.Write("system", system->Name());
	if(planet)
		out.Write("planet", planet->Name());
	for(const System *system : travelPlan)
		out.Write("travel", system->Name());
	out.Write("reputation with");
	out.BeginChild();
		for(const auto &it : GameData::Governments())
			if(&it.second != GameData::PlayerGovernment())
				out.Write(it.first, GameData::GetPolitics().Reputation(&it.second));
	out.EndChild();
		
	
	for(const std::shared_ptr<Ship> &ship : ships)
		ship->Save(out);
	
	cargo.Save(out);
	accounts.Save(out);
	
	for(const Mission &mission : missions)
		mission.Save(out);
	for(const Mission &mission : availableJobs)
		mission.Save(out, "available job");
	for(const Mission &mission : availableMissions)
		mission.Save(out, "available mission");
	
	if(!conditions.empty())
	{
		out.Write("conditions");
		out.BeginChild();
			for(const auto &it : conditions)
				if(it.second)
					out.Write(it.first, it.second);
		out.EndChild();
	}
	
	for(const System *system : visited)
		out.Write("visited", system->Name());
}



string PlayerInfo::Identifier() const
{
	size_t pos = Files::Saves().size();
	if(filePath.length() < pos + 4)
		return "";
	size_t length = filePath.length() - 4 - pos;
	return filePath.substr(pos, length);
}



// Load the most recently saved player.
void PlayerInfo::LoadRecent()
{
	string recentPath = Files::Config() + "recent.txt";
	ifstream recent(recentPath);
	getline(recent, recentPath);
	
	if(recentPath.empty())
		Clear();
	else
		Load(recentPath);
}



// Make a new player.
void PlayerInfo::New()
{
	Clear();
	
	SetSystem(GameData::Systems().Get("Rutilicus"));
	SetPlanet(GameData::Planets().Get("New Boston"));
	
	accounts.AddMortgage(295000);
	
	CreateMissions();
}



// Apply any "changes" saved in this player info to the global game state.
void PlayerInfo::ApplyChanges()
{
	for(const auto &it : reputationChanges)
		GameData::GetPolitics().SetReputation(it.first, it.second);
	reputationChanges.clear();
	
	// TODO: Allow changes to the galaxy.
}



void PlayerInfo::Die()
{
	isDead = true;
}



bool PlayerInfo::IsDead() const
{
	return isDead;
}



const string &PlayerInfo::FirstName() const
{
	return firstName;
}



const string &PlayerInfo::LastName() const
{
	return lastName;
}



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



const Date &PlayerInfo::GetDate() const
{
	return date;
}



string PlayerInfo::IncrementDate()
{
	++date;
	
	// Check if any missions have failed because of deadlines.
	for(Mission &mission : missions)
		if(mission.CheckDeadline(date))
			Messages::Add("You failed to meet the deadline for the mission \"" + mission.Name() + "\".");
	
	// For accounting, keep track of the player's net worth. This is for
	// calculation of yearly income to determine maximum mortgage amounts.
	int64_t assets = 0;
	for(const shared_ptr<Ship> &ship : ships)
		assets += ship->Cost() + ship->Cargo().Value(system);
	
	return accounts.Step(assets, Salaries());
}



void PlayerInfo::SetSystem(const System *system)
{
	this->system = system;
	Visit(system);
}



const System *PlayerInfo::GetSystem() const
{
	return system;
}



void PlayerInfo::SetPlanet(const Planet *planet)
{
	this->planet = planet;
}



const Planet *PlayerInfo::GetPlanet() const
{
	return planet;
}



bool PlayerInfo::ShouldLaunch() const
{
	return shouldLaunch;
}



const Account &PlayerInfo::Accounts() const
{
	return accounts;
}



Account &PlayerInfo::Accounts()
{
	return accounts;
}



int64_t PlayerInfo::Salaries() const
{
	int64_t crew = 0;
	for(const shared_ptr<Ship> &ship : ships)
		crew += ship->Crew();
	if(!crew)
		return 0;
	
	return 100 * (crew - 1);
}



// Set the player ship.
void PlayerInfo::AddShip(shared_ptr<Ship> &ship)
{
	ships.push_back(ship);
}



void PlayerInfo::RemoveShip(const shared_ptr<Ship> &ship)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(*it == ship)
		{
			ships.erase(it);
			break;
		}
}



const Ship *PlayerInfo::GetShip() const
{
	return ships.empty() ? nullptr : ships.front().get();
}



Ship *PlayerInfo::GetShip()
{
	return ships.empty() ? nullptr : ships.front().get();
}



const vector<shared_ptr<Ship>> &PlayerInfo::Ships() const
{
	return ships;
}



void PlayerInfo::BuyShip(const Ship *model, const string &name)
{
	if(model && accounts.Credits() >= model->Cost())
	{
		ships.push_back(shared_ptr<Ship>(new Ship(*model)));
		ships.back()->SetName(name);
		ships.back()->SetSystem(system);
		ships.back()->SetPlanet(planet);
		ships.back()->SetIsSpecial();
		ships.back()->SetGovernment(GameData::PlayerGovernment());
		if(ships.size() > 1)
		{
			ships.back()->SetParent(ships.front());
			ships.front()->AddEscort(ships.back());
		}
		
		accounts.AddCredits(-model->Cost());
	}
}



void PlayerInfo::SellShip(const Ship *selected)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			accounts.AddCredits(selected->Cost());
			ships.erase(it);
			return;
		}
}



// Change the order of the given ship in the list.
void PlayerInfo::ReorderShip(int fromIndex, int toIndex)
{
	// Make sure the indices are valid.
	if(static_cast<unsigned>(fromIndex) >= ships.size())
		return;
	if(static_cast<unsigned>(toIndex) >= ships.size())
		return;
	
	if(!fromIndex)
	{
		if(ships.size() < 2)
			return;
		if(ships[1]->IsFighter())
			return;
	}
	
	if(!toIndex)
	{
		// Check that this ship is eligible to be a flagship.
		if(ships[fromIndex]->IsFighter())
			++toIndex;
		if(ships[fromIndex]->IsDisabled() || ships[fromIndex]->Hull() <= 0.)
			++toIndex;
		if(ships[fromIndex]->GetSystem() != system)
			++toIndex;
	}
	
	shared_ptr<Ship> ship = ships[fromIndex];
	ships.erase(ships.begin() + fromIndex);
	ships.insert(ships.begin() + toIndex, ship);
}


	
// Get cargo information.
CargoHold &PlayerInfo::Cargo()
{
	return cargo;
}



const CargoHold &PlayerInfo::Cargo() const
{
	return cargo;
}



// Switch cargo from being stored in ships to being stored here.
void PlayerInfo::Land()
{
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	// Remove any ships that have been destroyed. Recharge the others if this is
	// a planet with a spaceport.
	vector<std::shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if(!*it || (*it)->Hull() <= 0. || (*it)->IsDisabled()
				|| (*it)->GetGovernment() != GameData::PlayerGovernment())
			it = ships.erase(it);
		else
			++it; 
	}
	
	// "Unload" all fighters, so they will get recharged, etc.
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
			ship->UnloadFighters();
	
	UpdateCargoCapacities();
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			if(planet->HasSpaceport())
				ship->Recharge();
			
			ship->Cargo().TransferAll(&cargo);
		}
	
	// Create whatever missions this planet has to offer.
	if(!freshlyLoaded)
		CreateMissions();
	freshlyLoaded = false;
	
	// Search for any missions that have failed but for which we are still
	// holding on to some cargo.
	set<const Mission *> active;
	for(const Mission &it : missions)
		active.insert(&it);
	
	for(const auto &it : cargo.MissionCargo())
		if(active.find(it.first) == active.end())
			cargo.RemoveMissionCargo(it.first);
	for(const auto &it : cargo.PassengerList())
		if(active.find(it.first) == active.end())
			cargo.RemoveMissionCargo(it.first);
}



// Load the cargo back into your ships. This may require selling excess, in
// which case a message will be returned.
void PlayerInfo::TakeOff()
{
	shouldLaunch = false;
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	// Jobs are only available when you are landed.
	availableJobs.clear();
	availableMissions.clear();
	doneMissions.clear();
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - ship->Crew());
			cargo.TransferAll(&ship->Cargo());
		}
	
	// Extract the fighters from the list.
	vector<shared_ptr<Ship>> fighters;
	vector<shared_ptr<Ship>> drones;
	for(shared_ptr<Ship> &ship : ships)
	{
		bool fit = false;
		const string &category = ship->Attributes().Category();
		if(category == "Fighter")
		{
			for(shared_ptr<Ship> &parent : ships)
				if(parent->FighterBaysFree())
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
				if(parent->DroneBaysFree())
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
			income += ship->Cost();
		for(const shared_ptr<Ship> &ship : drones)
			income += ship->Cost();
		
		out << ", earning " << income << " credits.";
		accounts.AddCredits(income);
		Messages::Add(out.str());
	}
	
	for(const auto &it : cargo.MissionCargo())
		if(it.second)
		{
			Messages::Add("Mission \"" + it.first->Name()
				+ "\" failed because you do not have space for the cargo.");
			RemoveMission(Mission::FAIL, *it.first, nullptr);
		}
	for(const auto &it : cargo.PassengerList())
		if(it.second)
		{
			Messages::Add("Mission \"" + it.first->Name()
				+ "\" failed because you do not have enough passenger bunks free.");
			RemoveMission(Mission::FAIL, *it.first, nullptr);
		}
	
	int64_t sold = cargo.Used();
	int64_t income = cargo.Value(system);
	accounts.AddCredits(income);
	cargo.Clear();
	if(sold)
	{
		ostringstream out;
		out << "You sold " << sold << " tons of excess cargo for " << income << " credits.";
		Messages::Add(out.str());
	}
	
	// Transfer all hand to hand weapons to the flagship.
	if(ships.empty())
		return;
	shared_ptr<Ship> flagship = ships.front();
	for(shared_ptr<Ship> &ship : ships)
	{
		if(ship == flagship)
			continue;
		
		auto it = ship->Outfits().begin();
		while(it != ship->Outfits().end())
		{
			const Outfit *outfit = it->first;
			int count = it->second;
			++it;
			
			if(outfit->Category() == "Hand to Hand")
			{
				ship->AddOutfit(outfit, -count);
				flagship->AddOutfit(outfit, count);
			}
		}
	}
}



// Call this when leaving the oufitter, shipyard, or hiring panel.
void PlayerInfo::UpdateCargoCapacities()
{
	int size = 0;
	int bunks = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			size += ship->Attributes().Get("cargo space");
			bunks += ship->Attributes().Get("bunks") - ship->Crew();
		}
	cargo.SetSize(size);
	cargo.SetBunks(bunks);
}



// Get mission information.
const list<Mission> &PlayerInfo::Missions() const
{
	return missions;
}



// Get mission information.
const list<Mission> &PlayerInfo::AvailableJobs() const
{
	return availableJobs;
}



void PlayerInfo::AcceptJob(const Mission &mission)
{
	for(auto it = availableJobs.begin(); it != availableJobs.end(); ++it)
		if(&*it == &mission)
		{
			cargo.AddMissionCargo(&mission);
			it->Do(Mission::OFFER, *this);
			it->Do(Mission::ACCEPT, *this);
			missions.splice(missions.end(), availableJobs, it);
			break;
		}
}



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



// Callback for accepting or declining whatever mission has been offered.
void PlayerInfo::MissionCallback(int response)
{
	shouldLaunch = (response == Conversation::LAUNCH);
	if(response == Conversation::ACCEPT || shouldLaunch)
	{
		availableMissions.front().Do(Mission::ACCEPT, *this);
		missions.splice(missions.end(), availableMissions, availableMissions.begin());
		UpdateCargoCapacities();
	}
	else if(response == Conversation::DECLINE)
	{
		availableMissions.front().Do(Mission::DECLINE, *this);
		availableMissions.pop_front();
	}
	else if(response == Conversation::DEFER)
	{
		availableMissions.front().Do(Mission::DEFER, *this);
		availableMissions.pop_front();
	}
	else if(response == Conversation::DIE)
	{
		Die();
		ships.clear();
	}
}



void PlayerInfo::RemoveMission(Mission::Trigger trigger, const Mission &mission, UI *ui)
{
	for(auto it = missions.begin(); it != missions.end(); ++it)
		if(&*it == &mission)
		{
			it->Do(trigger, *this, ui);
			cargo.RemoveMissionCargo(&mission);
			for(shared_ptr<Ship> &ship : ships)
				ship->Cargo().RemoveMissionCargo(&mission);
			// Don't get rid of the mission yet, because the conversation or
			// dialog panel may still be showing.
			doneMissions.splice(doneMissions.end(), missions, it);
			return;
		}
}



// Update mission status based on an event.
void PlayerInfo::HandleEvent(const ShipEvent &event, UI *ui)
{
	// Combat rating increases when you disable an enemy ship.
	if(event.ActorGovernment() == GameData::PlayerGovernment())
		if((event.Type() & ShipEvent::DISABLE) && event.Target())
			conditions["combat rating"] += event.Target()->RequiredCrew();
	
	for(Mission &mission : missions)
		mission.Do(event, *this, ui);
	
	if((event.Type() & ShipEvent::DESTROY) && !ships.empty() && event.Target() == ships.front())
		Die();
}



map<string, int> &PlayerInfo::Conditions()
{
	return conditions;
}



const map<string, int> &PlayerInfo::Conditions() const
{
	return conditions;
}



bool PlayerInfo::HasSeen(const System *system) const
{
	return (seen.find(system) != seen.end());
}



bool PlayerInfo::HasVisited(const System *system) const
{
	return (visited.find(system) != visited.end());
}



void PlayerInfo::Visit(const System *system)
{
	visited.insert(system);
	seen.insert(system);
	for(const System *neighbor : system->Neighbors())
		seen.insert(neighbor);
}



bool PlayerInfo::HasTravelPlan() const
{
	return !travelPlan.empty();
}



const vector<const System *> &PlayerInfo::TravelPlan() const
{
	return travelPlan;
}



void PlayerInfo::ClearTravel()
{
	travelPlan.clear();
}



// Add to the travel plan, starting with the last system in the journey.
void PlayerInfo::AddTravel(const System *system)
{
	travelPlan.push_back(system);
}



void PlayerInfo::PopTravel()
{
	if(!travelPlan.empty())
	{
		Visit(travelPlan.back());
		travelPlan.pop_back();
	}
}



// Toggle which secondary weapon the player has selected.
const Outfit *PlayerInfo::SelectedWeapon() const
{
	return selectedWeapon;
}



void PlayerInfo::SelectNext()
{
	if(ships.empty())
		return;
	
	shared_ptr<Ship> &ship = ships.front();
	if(ship->Outfits().empty())
		return;
	
	auto it = ship->Outfits().find(selectedWeapon);
	if(it == ship->Outfits().end())
		it = ship->Outfits().begin();
	
	while(++it != ship->Outfits().end())
		if(it->first->Ammo() || it->first->WeaponGet("firing fuel"))
		{
			selectedWeapon = it->first;
			return;
		}
	selectedWeapon = nullptr;
}



// New missions are generated each time you land on a planet. This also means
// that random missions that didn't show up might show up if you reload the
// game, but that's a minor detail and I can fix it later.
void PlayerInfo::CreateMissions()
{
	// Set up the "conditions" for the current status of the player.
	const Politics &politics = GameData::GetPolitics();
	for(const auto &it : GameData::Governments())
	{
		int rep = politics.Reputation(&it.second);
		conditions["reputation: " + it.first] = rep;
		if(&it.second == system->GetGovernment())
			conditions["reputation"] = rep;
	}
	static const vector<string> CATEGORIES = {
		"Transport",
		"Light Freighter",
		"Heavy Freighter",
		"Interceptor",
		"Light Warship",
		"Heavy Warship",
		"Fighter",
		"Drone"
	};
	for(const string &category : CATEGORIES)
		conditions["ships: " + category] = 0;
	for(const shared_ptr<Ship> &ship : ships)
		++conditions["ships: " + ship->Attributes().Category()];
	
	// Check for available missions.
	for(const auto &it : GameData::Missions())
	{
		conditions["random"] = Random::Int(100);
		if(it.second.CanOffer(*this))
		{
			list<Mission> &missions =
				it.second.IsAtLocation(Mission::JOB) ? availableJobs : availableMissions;
			
			missions.push_back(it.second.Instantiate(*this));
			if(missions.back().HasFailed())
				missions.pop_back();
		}
	}
}
