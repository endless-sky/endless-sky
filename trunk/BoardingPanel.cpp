/* BoardingPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "BoardingPanel.h"

#include "CargoHold.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Screen.h"
#include "Ship.h"
#include "System.h"
#include "UI.h"

#include <algorithm>

using namespace std;

namespace {
	// Format the given double with one decimal place.
	string Format(double value)
	{
		int integer = round(value * 10.);
		string result = to_string(integer / 10);
		result += ".0";
		result.back() += integer % 10;
		
		return result;
	}
}



BoardingPanel::BoardingPanel(PlayerInfo &player, const shared_ptr<Ship> &victim)
	: player(player), you(player.Ships().front()), victim(victim),
	selected(0), scroll(0), isCapturing(false),
	attackOdds(player.GetShip(), &*victim), defenseOdds(&*victim, player.GetShip())
{
	TrapAllEvents();
	
	const System &system = *player.GetSystem();
	for(const auto &it : victim->Cargo().Commodities())
		plunder.emplace_back(it.first, it.second, system.Trade(it.first));
	
	// You cannot plunder hand to hand weapons, because they are kept in the
	// crew's quarters, not mounted on the exterior of the ship.
	for(const auto &it : victim->Outfits())
		if(it.first->Category() != "Hand to Hand")
			plunder.emplace_back(it.first, it.second);
	
	sort(plunder.begin(), plunder.end());
}


	
void BoardingPanel::Draw() const
{
	DrawBackdrop();
	
	// Draw the list of plunder.
	Color opaque(.1, 1.);
	Color back = *GameData::Colors().Get("back");
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	FillShader::Fill(Point(-155., -60.), Point(360., 250.), opaque);
	
	int index = (scroll - 10) / 20;
	int y = -170 - scroll + 20 * index;
	int endY = 60;
	
	const Font &font = FontSet::Get(14);
	double fontOff = .5 * (20 - font.Height());
	int freeSpace = you->Cargo().Free();
	for( ; y < endY && static_cast<unsigned>(index) < plunder.size(); y += 20, ++index)
	{
		const Plunder &item = plunder[index];
		
		bool isSelected = (index == selected);
		if(isSelected)
			FillShader::Fill(Point(-155., y + 10.), Point(360., 20.), back);
		
		const Color &color = item.CanTake(freeSpace) ? isSelected ? bright : medium : dim;
		Point pos(-320., y + fontOff);
		font.Draw(item.Name(), pos, color);
		
		Point valuePos(pos.X() + 260. - font.Width(item.Value()), pos.Y());
		font.Draw(item.Value(), valuePos, color);
		
		Point sizePos(pos.X() + 330. - font.Width(item.Size()), pos.Y());
		font.Draw(item.Size(), sizePos, color);
	}
	
	Information info;
	if(CanExit())
		info.SetCondition("can exit");
	if(CanTake())
		info.SetCondition("can take");
	if(CanCapture())
		info.SetCondition("can capture");
	if(CanAttack())
		info.SetCondition("can attack");
	
	// This should always be true, but double check.
	int crew = 0;
	if(you)
	{
		// TODO: Tabulate attack and odds for each number of crew.
		const Ship &ship = *you;
		crew = ship.Crew();
		info.SetString("cargo space", to_string(freeSpace));
		info.SetString("your crew", to_string(crew));
		info.SetString("your attack",
			Format(attackOdds.AttackerPower(crew)));
		info.SetString("your defense",
			Format(defenseOdds.DefenderPower(crew)));
	}
	int vCrew = victim->Crew();
	info.SetString("enemy crew", to_string(vCrew));
	info.SetString("enemy attack",
		Format(defenseOdds.AttackerPower(vCrew)));
	info.SetString("enemy defense",
		Format(attackOdds.DefenderPower(vCrew)));
	
	info.SetString("attack odds",
		Format(100. * attackOdds.Odds(crew, vCrew)) + "%");
	info.SetString("attack casualties",
		Format(attackOdds.AttackerCasualties(crew, vCrew)));
	info.SetString("defense odds",
		Format(100. * (1. - defenseOdds.Odds(vCrew, crew))) + "%");
	info.SetString("defense casualties",
		Format(defenseOdds.DefenderCasualties(vCrew, crew)));
	
	const Interface *interface = GameData::Interfaces().Get("boarding");
	interface->Draw(info);
	
	Point messagePos(50., 55.);
	for(const string &message : messages)
	{
		font.Draw(message, messagePos, bright);
		messagePos.Y() += 20.;
	}
}



bool BoardingPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if((key == 'd' || key == 'x') && CanExit())
		GetUI()->Pop(this);
	else if(key == 't' && CanTake())
	{
		CargoHold &cargo = you->Cargo();
		int count = plunder[selected].CanTake(cargo.Free());
		
		const Outfit *outfit = plunder[selected].GetOutfit();
		if(outfit)
		{
			cargo.Transfer(outfit, -count);
			victim->AddOutfit(outfit, -count);
		}
		else
			victim->Cargo().Transfer(plunder[selected].Name(), count, &cargo);
		
		if(count == plunder[selected].Count())
		{
			plunder.erase(plunder.begin() + selected);
			selected = min(selected, static_cast<int>(plunder.size()));
		}
		else
			plunder[selected].Take(count);
	}
	else if(key == 'c' && CanCapture())
	{
		isCapturing = true;
		messages.push_back("The airlock blasts open. Combat has begun!");
		messages.push_back("(It will end if you both choose to \"defend.\")");
	}
	else if((key == 'a' || key == 'd') && CanAttack())
	{
		int yourStartCrew = you->Crew();
		int enemyStartCrew = victim->Crew();
		
		// Figure out what action the other ship will take.
		bool youAttack = (key == 'a');
		bool enemyAttacks = defenseOdds.Odds(enemyStartCrew, yourStartCrew) > .5;
		
		if(!youAttack && !enemyAttacks)
		{
			messages.push_back("You retreat to your ships. Combat ends.");
			isCapturing = false;
		}
		else
		{
			if(youAttack)
				messages.push_back("You attack. ");
			else if(enemyAttacks)
				messages.push_back("You defend. ");
			
			int rounds = max(1, yourStartCrew / 5);
			for(int round = 0; round < rounds; ++round)
			{
				int yourCrew = you->Crew();
				int enemyCrew = victim->Crew();
				if(!yourCrew || !enemyCrew)
					break;
				
				unsigned yourPower = static_cast<unsigned>(1000. * (youAttack ?
					attackOdds.AttackerPower(yourCrew) : defenseOdds.DefenderPower(yourCrew)));
				unsigned enemyPower = static_cast<unsigned>(1000. * (enemyAttacks ?
					defenseOdds.AttackerPower(enemyCrew) : attackOdds.DefenderPower(enemyCrew)));
				
				unsigned total = yourPower + enemyPower;
				if(!total)
					break;
				
				if(Random::Int(total) >= yourPower)
					you->AddCrew(-1);
				else
					victim->AddCrew(-1);
			}
			
			int yourCasualties = yourStartCrew - you->Crew();
			int enemyCasualties = enemyStartCrew - victim->Crew();
			if(yourCasualties && enemyCasualties)
				messages.back() += "You lose " + to_string(yourCasualties)
					+ " crew; they lose " + to_string(enemyCasualties) + ".";
			else if(yourCasualties)
				messages.back() += "You lose " + to_string(yourCasualties) + " crew.";
			else if(enemyCasualties)
				messages.back() += "They lose " + to_string(enemyCasualties) + " crew.";
			
			if(!you->Crew())
			{
				messages.push_back("You have been killed. Your ship is lost.");
				player.Ships().front()->WasCaptured(victim);
				player.RemoveShip(player.Ships().front());
				isCapturing = false;
			}
			else if(!victim->Crew())
			{
				messages.push_back("You have succeeded in capturing this ship.");
				victim->WasCaptured(player.Ships().front());
				player.AddShip(victim);
				you->AddEscort(victim);
				isCapturing = false;
			}
		}
		// Trim the list of status messages.
		while(messages.size() > 5)
			messages.erase(messages.begin());
	}
	
	return true;
}



bool BoardingPanel::Click(int x, int y)
{
	// Was the click inside the plunder list?
	if(x >= -330 && x < 20 && y >= -180 && y < 60)
	{
		int index = (scroll + y - -170) / 20;
		if(static_cast<unsigned>(index) < plunder.size())
			selected = index;
		return true;
	}
	
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("boarding");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key != '\0')
			return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	return true;
}



bool BoardingPanel::Drag(int dx, int dy)
{
	// The list is 240 pixels tall, and there are 10 pixels padding on the top
	// and the bottom, so:
	int maximumScroll = max(0, static_cast<int>(20 * plunder.size() - 220));
	scroll = max(0, min(maximumScroll, scroll + dy));
	
	return true;
}



bool BoardingPanel::Scroll(int dx, int dy)
{
	return Drag(dx, dy * 50);
}



bool BoardingPanel::CanExit() const
{
	return !isCapturing;
}



bool BoardingPanel::CanTake(int index) const
{
	// If you ship or the other ship has been captured:
	if(you->GetGovernment() != player.GetGovernment())
		return false;
	if(victim->GetGovernment() == player.GetGovernment())
		return false;
	
	if(index < 0)
		index = selected;
	return static_cast<unsigned>(index) < plunder.size() && player.GetShip()
		&& plunder[index].CanTake(player.GetShip()->Cargo().Free());
}



bool BoardingPanel::CanCapture() const
{
	// If you ship or the other ship has been captured:
	if(you->GetGovernment() != player.GetGovernment())
		return false;
	if(victim->GetGovernment() == player.GetGovernment())
		return false;
	
	return !isCapturing && player.GetShip()->Crew() > 1;
}



bool BoardingPanel::CanAttack() const
{
	return isCapturing;
}



BoardingPanel::Plunder::Plunder(const string &commodity, int count, int unitValue)
	: name(commodity), outfit(nullptr), count(count), unitValue(unitValue)
{
	UpdateStrings();
}



BoardingPanel::Plunder::Plunder(const Outfit *outfit, int count)
	: name(outfit->Name()), outfit(outfit), count(count), unitValue(outfit->Cost())
{
	UpdateStrings();
}



// Sort by value per ton of mass.
bool BoardingPanel::Plunder::operator<(const Plunder &other) const
{
	// This may involve infinite values when the mass is zero, but that's okay.
	return (unitValue / UnitMass() > other.unitValue / other.UnitMass());
}



// Check how many of this item are left un-plundered. Once this is zero,
// the item can be removed from the list.
int BoardingPanel::Plunder::Count() const
{
	return count;
}



// Get the name of this item. If it is a commodity, this is its name.
const string &BoardingPanel::Plunder::Name() const
{
	return name;
}



// Get the mass, in the format "<count> x <unit mass>". If this is a
// commodity, no unit mass is given (because it is 1). If the count is
// 1, only the unit mass is reported.
const string &BoardingPanel::Plunder::Size() const
{
	return size;
}



// Get the total value (unit value times count) as a string.
const string &BoardingPanel::Plunder::Value() const
{
	return value;
}



// If this is an outfit, get the outfit. Otherwise, this returns null.
const Outfit *BoardingPanel::Plunder::GetOutfit() const
{
	return outfit;
}



// Find out how many of these I can take if I have this amount of cargo
// space free.
int BoardingPanel::Plunder::CanTake(int freeSpace) const
{
	double mass = UnitMass();
	if(mass <= 0.)
		return count;
	
	return min(count, static_cast<int>(freeSpace / mass));
}



// Take some or all of this plunder item.
void BoardingPanel::Plunder::Take(int count)
{
	this->count -= count;
	UpdateStrings();
}



void BoardingPanel::Plunder::UpdateStrings()
{
	int mass = static_cast<int>(UnitMass());
	if(!outfit)
		size = to_string(count);
	else if(count == 1)
		size = to_string(mass);
	else
		size = to_string(count) + " x " + to_string(mass);
	
	value = to_string(unitValue * count);
}



double BoardingPanel::Plunder::UnitMass() const
{
	return outfit ? outfit->Get("mass") : 1.;
}
