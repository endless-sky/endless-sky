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
#include "DistanceMap.h"
#include "Files.h"
#include "GameData.h"
#include "Messages.h"
#include "Mission.h"
#include "Outfit.h"
#include "Random.h"
#include "Ship.h"
#include "System.h"

#include <fstream>
#include <random>
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
	accounts = Account();
	
	ships.clear();
	cargo.Clear();
	missions.clear();
	jobs.clear();
	
	seen.clear();
	visited.clear();
	travelPlan.clear();
	
	selectedWeapon = nullptr;
	
	random_device rd;
	Random::Seed(rd());
}



bool PlayerInfo::IsLoaded() const
{
	return !firstName.empty();
}



void PlayerInfo::Load(const string &path, const GameData &data)
{
	Clear();
	gameData = &data;
	
	filePath = path;
	DataFile file(path);
	
	playerGovernment = data.Governments().Get("Escort");
	
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
			system = data.Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
			planet = data.Planets().Get(child.Token(1));
		else if(child.Token(0) == "account")
			accounts.Load(child);
		else if(child.Token(0) == "visited" && child.Size() >= 2)
			Visit(data.Systems().Get(child.Token(1)));
		else if(child.Token(0) == "cargo")
			cargo.Load(child, data);
		else if(child.Token(0) == "mission")
		{
			missions.push_back(Mission());
			missions.back().Load(child, data);
			cargo.AddMissionCargo(&missions.back());
		}
		else if(child.Token(0) == "job")
		{
			jobs.push_back(Mission());
			jobs.back().Load(child, data);
		}
		else if(child.Token(0) == "ship")
		{
			ships.push_back(shared_ptr<Ship>(new Ship()));
			ships.back()->Load(child, data);
			ships.back()->FinishLoading();
			ships.back()->SetIsSpecial();
			ships.back()->SetGovernment(playerGovernment);
			if(ships.size() > 1)
			{
				ships.back()->SetParent(ships.front());
				ships.front()->AddEscort(ships.back());
			}
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
	{
		string recentPath = Files::Config() + "recent.txt";
		ofstream recent(recentPath);
		recent << filePath << "\n";
	}
	
	ofstream out(filePath);
	
	out << "pilot \"" << firstName << "\" \"" << lastName << "\"\n";
	out << "date " << date.Day() << " " << date.Month() <<  " " << date.Year() << "\n";
	if(system)
		out << "system \"" << system->Name() << "\"\n";
	if(planet)
		out << "planet \"" << planet->Name() << "\"\n";
	
	accounts.Save(out);
	cargo.Save(out);
	
	for(const std::shared_ptr<Ship> &ship : ships)
		ship->Save(out);
	
	for(const Mission &mission : missions)
		mission.Save(out);
	for(const Mission &mission : jobs)
		mission.Save(out, "job");
	
	for(const System *system : visited)
		out << "visited \"" << system->Name() << "\"\n";
}



string PlayerInfo::Identifier() const
{
	size_t pos = Files::Saves().size();
	size_t length = filePath.length() - 4 - pos;
	return filePath.substr(pos, length);
}



// Load the most recently saved player.
void PlayerInfo::LoadRecent(const GameData &data)
{
	string recentPath = Files::Config() + "recent.txt";
	ifstream recent(recentPath);
	getline(recent, recentPath);
	
	if(recentPath.empty())
		Clear();
	else
		Load(recentPath, data);
}



// Make a new player.
void PlayerInfo::New(const GameData &data)
{
	Clear();
	gameData = &data;
	
	SetSystem(data.Systems().Get("Rutilicus"));
	SetPlanet(data.Planets().Get("New Boston"));
	playerGovernment = data.Governments().Get("Escort");
	
	accounts.AddMortgage(295000);
	
	CreateMissions();
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
	
	// For accounting, keep track of the player's net worth. This is for
	// calculation of yearly income to determine maximum mortgage amounts.
	int assets = 0;
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



const Account &PlayerInfo::Accounts() const
{
	return accounts;
}



Account &PlayerInfo::Accounts()
{
	return accounts;
}



int PlayerInfo::Salaries() const
{
	int crew = 0;
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
		ships.back()->SetGovernment(playerGovernment);
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
				|| (*it)->GetGovernment() != gameData->Governments().Get("Escort"))
			it = ships.erase(it);
		else
			++it; 
	}
	
	// "Unload" all fighters, so they will get recharged, etc.
	vector<shared_ptr<Ship>> fighters;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
			ship->UnloadFighters(fighters);
	ships.insert(ships.end(), fighters.begin(), fighters.end());
	fighters.clear();
	
	UpdateCargoCapacities();
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			if(planet->HasSpaceport())
				ship->Recharge();
			
			ship->Cargo().TransferAll(&cargo);
		}
	
	// TODO: This is not quite right, because in theory a planet may have no
	// jobs generated, in which case it should still be empty when re-loading
	// the saved game.
	if(jobs.empty())
		CreateMissions();
	
	auto mit = missions.begin();
	while(mit != missions.end())
	{
		if(mit->Failed())
		{
			cargo.RemoveMissionCargo(&*mit);
			mit = missions.erase(mit);
		}
		else
			++mit;
	}
}



// Load the cargo back into your ships. This may require selling excess, in
// which case a message will be returned.
void PlayerInfo::TakeOff()
{
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	// Jobs are only available when you are landed.
	jobs.clear();
	
	// Reset any governments you provoked yesterday.
	for(const auto &it : gameData->Governments())
		it.second.ResetProvocation();
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - ship->Crew());
			cargo.TransferAll(&ship->Cargo());
		}
	
	// Extract the fighters from the list.
	vector<shared_ptr<Ship>> fighters;
	vector<shared_ptr<Ship>> drones;
	vector<std::shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if((*it)->GetSystem() == system)
		{
			const string &category = (*it)->Attributes().Category();
			bool isFighter = (category == "Fighter");
			bool isDrone = (category == "Drone");
			if(isFighter)
				fighters.push_back(*it);
			else if(isDrone)
				drones.push_back(*it);
			
			if(isFighter || isDrone)
			{
				it = ships.erase(it);
				// Do not increment the iterator.
				continue;
			}
		}
		++it;
	}
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			while(!fighters.empty() && ship->FighterBaysFree())
			{
				ship->AddFighter(fighters.back());
				fighters.pop_back();
			}
			while(!drones.empty() && ship->DroneBaysFree())
			{
				ship->AddFighter(drones.back());
				drones.pop_back();
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
		
		int income = 0;
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
			for(Mission &mission : missions)
				if(&mission == it.first)
				{
					mission.SetFailed();
					Messages::Add("Mission \"" + mission.Name()
						+ "\" failed because you do not have space for the cargo.");
				}
		}
	for(const auto &it : cargo.PassengerList())
		if(it.second)
		{
			for(Mission &mission : missions)
				if(&mission == it.first)
				{
					mission.SetFailed();
					Messages::Add("Mission \"" + mission.Name()
						+ "\" failed because you do not have enough passenger bunks free.");
				}
		}
	
	int sold = cargo.Used();
	int income = cargo.Value(system);
	accounts.AddCredits(income);
	cargo.Clear();
	if(sold)
	{
		ostringstream out;
		out << "You sold " << sold << " tons of excess cargo for " << income << " credits.";
		Messages::Add(out.str());
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
	return jobs;
}



bool PlayerInfo::CanAccept(const Mission &mission) const
{
	return (mission.CargoSize() <= cargo.Free()
		&& mission.Passengers() <= cargo.Bunks());
}



void PlayerInfo::AcceptJob(const Mission &mission)
{
	for(auto it = jobs.begin(); it != jobs.end(); ++it)
		if(&*it == &mission)
		{
			cargo.AddMissionCargo(&mission);
			missions.splice(missions.end(), jobs, it);
			break;
		}
}



void PlayerInfo::AddMission(const Mission &mission)
{
	missions.push_back(mission);
	// It's important to use a pointer to the Mission we just created here in
	// PlayerInfo (which will persist) rather than to the Mission that was
	// passed as an argument to this function (which may be temporary).
	cargo.AddMissionCargo(&missions.back());
}



void PlayerInfo::AbortMission(const Mission &mission)
{
	for(auto it = missions.begin(); it != missions.end(); ++it)
		if(&*it == &mission)
		{
			cargo.RemoveMissionCargo(&mission);
			missions.erase(it);
			break;
		}
}



void PlayerInfo::CompleteMission(const Mission &mission)
{
	accounts.AddCredits(mission.Payment());
	AbortMission(mission);
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
	DistanceMap distance(system);
	
	int cargoCount = Random::Binomial(10);
	for(int i = 0; i < cargoCount; ++i)
		jobs.push_back(Mission::Cargo(*gameData, planet, distance));
	
	int passengerCount = Random::Binomial(10);
	for(int i = 0; i < passengerCount; ++i)
		jobs.push_back(Mission::Passenger(*gameData, planet, distance));
	
	// TODO: Each planet specifies the maximum number of each mission type.
	
	// TODO: eventually, search for missions in GameData that have all their
	// prerequisites met, and add them to the missions list.
}
