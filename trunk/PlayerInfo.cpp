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
#include "GameData.h"
#include "Outfit.h"
#include "Ship.h"
#include "System.h"

#include <boost/filesystem.hpp>

#include <fstream>

namespace fs = boost::filesystem;

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
	seen.clear();
	visited.clear();
	travelPlan.clear();
	
	selectedWeapon = nullptr;
}



bool PlayerInfo::IsLoaded() const
{
	return !firstName.empty();
}



void PlayerInfo::Load(const string &path, const GameData &data)
{
	Clear();
	
	filePath = path;
	DataFile file(path);
	
	playerGovernment = data.Governments().Get("Escort");
	
	for(const DataFile::Node &child : file)
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
	
	if(!system && !ships.empty())
		system = ships.front()->GetSystem();
}



void PlayerInfo::Save() const
{
	{
		string recentPath = getenv("HOME") + string("/.config/endless-sky/recent.txt");
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
	
	for(const std::shared_ptr<Ship> &ship : ships)
		ship->Save(out);
	
	for(const System *system : visited)
		out << "visited \"" << system->Name() << "\"\n";
}



// Load the most recently saved player.
void PlayerInfo::LoadRecent(const GameData &data)
{
	string recentPath = getenv("HOME") + string("/.config/endless-sky/recent.txt");
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
	
	SetSystem(data.Systems().Get("Sabik"));
	SetPlanet(data.Planets().Get("Longjump"));
	playerGovernment = data.Governments().Get("Escort");
	
	accounts.AddMortgage(250000);
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
	filePath = getenv("HOME") + ("/.config/endless-sky/saves/" + fileName);
	int index = 0;
	while(true)
	{
		string path = filePath;
		if(index++)
			path += " " + to_string(index);
		path += ".txt";
		
		if(!fs::exists(path))
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
	return accounts.Step();
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



// Set the player ship.
void PlayerInfo::AddShip(shared_ptr<Ship> &ship)
{
	ships.push_back(ship);
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



void PlayerInfo::BuyShip(const Ship *model)
{
	if(model)
	{
		ships.push_back(shared_ptr<Ship>(new Ship(*model)));
		ships.back()->SetName("Starship One");
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
			ships.erase(it);
			accounts.AddCredits(selected->Cost());
			return;
		}
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
		if(it->first->Ammo())
		{
			selectedWeapon = it->first;
			return;
		}
	selectedWeapon = nullptr;
}
