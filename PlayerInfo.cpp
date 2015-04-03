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
#include "UI.h"

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
	
	reputationChanges.clear();
	gameEvents.clear();
	dataChanges.clear();
	
#ifdef __APPLE__
	Random::Seed(time(NULL));
#else
	random_device rd;
	Random::Seed(rd());
#endif
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
			ships.back()->SetIsYours();
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
	
	for(shared_ptr<Ship> &ship : ships)
	{
		if(!ship->GetSystem())
			ship->SetSystem(system);
		if(ship->GetSystem() == system)
			ship->SetPlanet(planet);
	}
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
	if(planet && planet->CanUseServices())
		out.Write("clearance");
	for(const System *system : travelPlan)
		out.Write("travel", system->Name());
	out.Write("reputation with");
	out.BeginChild();
		for(const auto &it : GameData::Governments())
			if(!it.second.IsPlayer())
				out.Write(it.first, it.second.Reputation());
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
	if(shouldLaunch)
		out.Write("launching");
	
	for(const GameEvent &event : gameEvents)
		event.Save(out);
	if(!dataChanges.empty())
	{
		out.Write("changes");
		out.BeginChild();
		
		for(const DataNode &node : dataChanges)
			out.Write(node);
		
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
	
	date = GameData::Start().GetDate();
	GameData::SetDate(date);
	
	SetSystem(GameData::Start().GetSystem());
	SetPlanet(GameData::Start().GetPlanet());
	accounts = GameData::Start().GetAccounts();
	GameData::Start().GetConditions().Apply(conditions);
	
	CreateMissions();
	
	for(const auto &it : GameData::Events())
		if(it.second.GetDate())
			AddEvent(it.second, it.second.GetDate());
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
	
	if(planet && hasFullClearance)
		planet->Bribe();
	hasFullClearance = false;
}



void PlayerInfo::AddChanges(std::list<DataNode> &changes)
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
	
	string message = accounts.Step(assets, Salaries());
	if(!message.empty())
		Messages::Add(message);
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
		if(!ship->IsParked())
			crew += ship->Crew();
	if(!crew)
		return 0;
	
	return 100 * (crew - 1);
}



// Add a captured ship to your fleet.
void PlayerInfo::AddShip(shared_ptr<Ship> &ship)
{
	ships.push_back(ship);
	ship->SetIsSpecial();
	ship->SetIsYours();
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
		ships.back()->SetIsYours();
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
			for(const auto &it : selected->Outfits())
				soldOutfits[it.first] += it.second;
			
			accounts.AddCredits(selected->Cost());
			ships.erase(it);
			return;
		}
}



void PlayerInfo::ParkShip(const Ship *selected, bool isParked)
{
	for(auto it = ships.begin(); it != ships.end(); ++it)
		if(it->get() == selected)
		{
			(*it)->SetIsParked(isParked);
			return;
		}
}



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
		if(ships[fromIndex]->IsDisabled() || ships[fromIndex]->IsDestroyed())
			++toIndex;
		if(ships[fromIndex]->GetSystem() != system)
			++toIndex;
	}
	
	shared_ptr<Ship> ship = ships[fromIndex];
	ships.erase(ships.begin() + fromIndex);
	ships.insert(ships.begin() + toIndex, ship);
	
	// Make sure all the ships know who the flagship is.
	for(const shared_ptr<Ship> &it : ships)
	{
		it->ClearEscorts();
		if(it != ships.front())
		{
			it->SetParent(ships.front());
			ships.front()->AddEscort(it);
		}
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
void PlayerInfo::Land(UI *ui)
{
	// This can only be done while landed.
	if(!system || !planet)
		return;
	
	// Remove any ships that have been destroyed. Recharge the others if this is
	// a planet with a spaceport.
	vector<std::shared_ptr<Ship>>::iterator it = ships.begin();
	while(it != ships.end())
	{
		if(!*it || (*it)->IsDestroyed() || (*it)->IsDisabled()
				|| !(*it)->GetGovernment()->IsPlayer())
			it = ships.erase(it);
		else
			++it; 
	}
	
	// "Unload" all fighters, so they will get recharged, etc.
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
			ship->UnloadFighters();
	
	bool canRecharge = planet->HasSpaceport() && planet->CanUseServices();
	UpdateCargoCapacities();
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == system)
		{
			if(canRecharge)
				ship->Recharge();
			
			ship->Cargo().TransferAll(&cargo);
		}
	
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
	
	for(const auto &it : cargo.MissionCargo())
		if(active.find(it.first) == active.end())
			cargo.RemoveMissionCargo(it.first);
	for(const auto &it : cargo.PassengerList())
		if(active.find(it.first) == active.end())
			cargo.RemoveMissionCargo(it.first);
	
	// Check if the player is doing anything illegal.
	const Government *gov = GetSystem()->GetGovernment();
	string message = gov->Fine(*this, 0, GetPlanet()->Security());
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
	soldOutfits.clear();
	
	bool canRecharge = planet->HasSpaceport() && planet->CanUseServices();
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsParked() && ship->GetSystem() == system)
		{
			if(canRecharge)
				ship->Recharge();
			ship->Cargo().SetBunks(ship->Attributes().Get("bunks") - ship->Crew());
			cargo.TransferAll(&ship->Cargo());
		}
	if(cargo.Passengers() && ships.size())
	{
		Ship &flagship = *ships.front();
		int extra = min(cargo.Passengers(), flagship.Crew() - flagship.RequiredCrew());
		if(extra)
		{
			flagship.AddCrew(-extra);
			Messages::Add("You fired " + to_string(extra) + " crew members to free up bunks for passengers.");
			flagship.Cargo().SetBunks(flagship.Attributes().Get("bunks") - flagship.Crew());
			cargo.TransferAll(&flagship.Cargo());
		}
	}
	if(ships.size())
	{
		Ship &flagship = *ships.front();
		int extra = flagship.Crew() + flagship.Cargo().Passengers() - flagship.Attributes().Get("bunks");
		if(extra > 0)
		{
			flagship.AddCrew(-extra);
			Messages::Add("You fired " + to_string(extra) + " crew members because you have no bunks for them.");
			flagship.Cargo().SetBunks(flagship.Attributes().Get("bunks") - flagship.Crew());
		}
	}
	
	// Extract the fighters from the list.
	vector<shared_ptr<Ship>> fighters;
	vector<shared_ptr<Ship>> drones;
	for(shared_ptr<Ship> &ship : ships)
	{
		if(ship->IsParked() || ship->GetSystem() != system)
			continue;
		
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



// Call this when leaving the outfitter, shipyard, or hiring panel.
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
	shouldLaunch = (response == Conversation::LAUNCH || response == Conversation::FLEE);
	if(response == Conversation::ACCEPT || response == Conversation::LAUNCH)
	{
		availableMissions.front().Do(Mission::ACCEPT, *this);
		cargo.AddMissionCargo(&*availableMissions.begin());
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
	if(event.ActorGovernment()->IsPlayer())
		if((event.Type() & ShipEvent::DISABLE) && event.Target())
			conditions["combat rating"] += event.Target()->RequiredCrew();
	
	for(Mission &mission : missions)
		mission.Do(event, *this, ui);
	
	if((event.Type() & ShipEvent::DESTROY) && !ships.empty() && event.Target() == ships.front())
		Die();
}



int PlayerInfo::GetCondition(const string &name) const
{
	auto it = conditions.find(name);
	return (it == conditions.end()) ? 0 : it->second;
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
		if(it->first->Ammo() || it->first->FiringFuel())
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



// New missions are generated each time you land on a planet. This also means
// that random missions that didn't show up might show up if you reload the
// game, but that's a minor detail and I can fix it later.
void PlayerInfo::CreateMissions()
{
	// Set up the "conditions" for the current status of the player.
	for(const auto &it : GameData::Governments())
	{
		int rep = it.second.Reputation();
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
	bool skipJobs = planet && !planet->HasSpaceport();
	bool hasPriorityMissions = false;
	for(const auto &it : GameData::Missions())
	{
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
}
