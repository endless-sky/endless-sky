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
#include "Screen.h"
#include "Ship.h"
#include "System.h"
#include "UI.h"

#include <algorithm>

using namespace std;



BoardingPanel::BoardingPanel(const GameData &data, PlayerInfo &player, Ship &victim)
	: data(data), player(player), victim(victim), selected(0), scroll(0)
{
	TrapAllEvents();
	
	const System &system = *player.GetSystem();
	for(const auto &it : victim.Cargo().Commodities())
		plunder.emplace_back(it.first, it.second, system.Trade(it.first));
	
	for(const auto &it : victim.Outfits())
		plunder.emplace_back(it.first, it.second);
	
	sort(plunder.begin(), plunder.end());
}


	
void BoardingPanel::Draw() const
{
	// Darken everything but the dialog.
	Color back(0., .7);
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
	
	// Draw the list of plunder.
	Color opaque(.1, 1.);
	FillShader::Fill(Point(-155., -60.), Point(360., 250.), opaque);
	
	int index = (scroll - 10) / 20;
	int y = -170 - scroll + 20 * index;
	int endY = 60;
	
	const Font &font = FontSet::Get(14);
	double fontOff = .5 * (20 - font.Height());
	int freeSpace = player.GetShip() ? player.GetShip()->Cargo().Free() : 0;
	for( ; y < endY && static_cast<unsigned>(index) < plunder.size(); y += 20, ++index)
	{
		const Plunder &item = plunder[index];
		
		bool isSelected = (index == selected);
		if(isSelected)
			FillShader::Fill(Point(-155., y + 10.), Point(360., 20.), Color(.1, .1));
		
		Color color(item.CanTake(freeSpace) ? isSelected ? .8 : .5 : .2, 0.);
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
	if(player.GetShip())
	{
		// TODO: Tabulate attack and odds for each number of crew.
		const Ship &ship = *player.GetShip();
		info.SetString("cargo space", to_string(freeSpace));
		info.SetString("your crew", to_string(ship.Crew()));
		info.SetString("your attack", to_string(ship.Crew()));
		info.SetString("your defense", to_string(2 * ship.Crew()));
	}
	info.SetString("enemy crew", to_string(victim.Crew()));
	info.SetString("enemy attack", to_string(victim.Crew()));
	info.SetString("enemy defense", to_string(2 * victim.Crew()));
	
	info.SetString("attack odds", "0%");
	info.SetString("attack casualties", "0");
	info.SetString("defense odds", "0%");
	info.SetString("defense casualties", "0");
	
	const Interface *interface = data.Interfaces().Get("boarding");
	interface->Draw(info);
}



bool BoardingPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if((key == 'd' || key == 'x') && CanExit())
		GetUI()->Pop(this);
	else if(key == 't' && CanTake())
	{
		CargoHold &cargo = player.GetShip()->Cargo();
		int count = plunder[selected].CanTake(cargo.Free());
		
		const Outfit *outfit = plunder[selected].GetOutfit();
		if(outfit)
		{
			cargo.Transfer(outfit, -count);
			victim.AddOutfit(outfit, -count);
		}
		else
			victim.Cargo().Transfer(plunder[selected].Name(), count, &cargo);
		
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
	}
	else if(key == 'a' && CanAttack())
	{
	}
	else if(key == 'd' && CanAttack())
	{
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
	const Interface *interface = data.Interfaces().Get("boarding");
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
	return true;
}



bool BoardingPanel::CanTake(int index) const
{
	if(index < 0)
		index = selected;
	return static_cast<unsigned>(index) < plunder.size() && player.GetShip()
		&& plunder[index].CanTake(player.GetShip()->Cargo().Free());
}



bool BoardingPanel::CanCapture() const
{
	return false;
}




bool BoardingPanel::CanAttack() const
{
	return false;
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
