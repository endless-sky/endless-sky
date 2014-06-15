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
#include <sstream>

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
	cargo.clear();
	
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
	gameData = &data;
	
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
		else if(child.Token(0) == "cargo")
		{
			for(const DataFile::Node &grand : child)
				if(grand.Size() >= 2)
					cargo[grand.Token(0)] += static_cast<int>(grand.Value(1));
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
	bool first = true;
	for(const pair<string, int> &it : cargo)
		if(it.second)
		{
			if(first)
				out << "cargo\n";
			first = false;
			out << "\t\"" << it.first << "\" " << it.second << '\n';
		}
	
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
	gameData = &data;
	
	SetSystem(data.Systems().Get("Rutilicus"));
	SetPlanet(data.Planets().Get("New Boston"));
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
	
	// For accounting, keep track of the player's net worth. This is for
	// calculation of yearly income to determine maximum mortgage amounts.
	int assets = 0;
	for(const shared_ptr<Ship> &ship : ships)
	{
		assets += ship->Cost();
		
		if(!gameData)
			continue;
		
		for(const Trade::Commodity &commodity : gameData->Commodities())
			assets += ship->GetSystem()->Trade(commodity.name)
				* ship->Cargo(commodity.name);
	}
	return accounts.Step(assets);
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



// Get the cargo capacity of all in-system ships. This works whether or not
// you have unloaded the cargo.
int PlayerInfo::FreeCargo() const
{
	const Ship *flagship = GetShip();
	if(!flagship)
		return 0;
	
	int free = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == flagship->GetSystem())
			free += ship->FreeCargo();
	
	for(const pair<string, int> &it : cargo)
		free -= it.second;
	for(const pair<const Outfit *, int> &it : outfitCargo)
		free -= it.first->Get("mass") * it.second;
	
	return free;
}



// Switch cargo from being stored in ships to being stored here.
void PlayerInfo::Land()
{
	// Remove any ships that have been destroyed.
	vector<std::shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if(!*it || (*it)->Hull() <= 0. || (*it)->IsDisabled())
			it = ships.erase(it);
		else
			++it; 
	}
	
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	const Ship *flagship = GetShip();
	if(!flagship)
		return;
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == flagship->GetSystem())
		{
			for(const pair<string, int> &it : ship->Cargo())
			{
				cargo[it.first] += it.second;
				ship->AddCargo(-it.second, it.first);
			}
			// TODO: handle outfit cargo too.
		}
}



// Load the cargo back into your ships. This may require selling excess, in
// which case a message will be returned.
std::string PlayerInfo::TakeOff()
{
	// This can only be done while landed.
	if(!system || !planet)
		return "";
	
	const Ship *flagship = GetShip();
	if(!flagship)
		return "";
	
	// TODO: handle outfit cargo.
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == flagship->GetSystem())
			for(pair<const string, int> &it : cargo)
			{
				int transfer = min(it.second, ship->FreeCargo());
				if(transfer)
				{
					ship->AddCargo(transfer, it.first);
					it.second -= transfer;
				}
			}
	
	int sold = 0;
	int income = 0;
	for(pair<const string, int> &it : cargo)
		if(it.second)
		{
			int price = system->Trade(it.first);
			accounts.AddCredits(price * it.second);
			sold += it.second;
			income += price * it.second;
		}
	cargo.clear();
	if(!sold)
		return "";
	
	ostringstream out;
	out << "You sold " << sold << " tons of excess cargo for " << income << " credits.";
	return out.str();
}



// Normal cargo and spare parts:
std::map<std::string, int> PlayerInfo::Cargo() const
{
	return cargo;
}



int PlayerInfo::Cargo(const std::string &type) const
{
	auto it = cargo.find(type);
	if(it == cargo.end())
		return 0;
	
	return it->second;
}



std::map<const Outfit *, int> PlayerInfo::OutfitCargo() const
{
	return outfitCargo;
}



void PlayerInfo::BuyCargo(const std::string &type, int amount)
{
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	int price = system->Trade(type);
	amount = min(amount, accounts.Credits() / price);
	amount = min(amount, FreeCargo());
	amount = max(amount, -cargo[type]);
	
	cargo[type] += amount;
	accounts.AddCredits(price * amount);
}



void PlayerInfo::AddOutfitCargo(const Outfit *outfit, int amount)
{
	outfitCargo[outfit] += amount;
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
