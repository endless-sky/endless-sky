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

#include "text/alignment.hpp"
#include "CargoHold.h"
#include "Depreciation.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "ShipInfoPanel.h"
#include "System.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

using namespace std;

namespace {
	// Format the given double with one decimal place.
	string Round(double value)
	{
		int integer = round(value * 10.);
		string result = to_string(integer / 10);
		result += ".0";
		result.back() += integer % 10;
		
		return result;
	}
}



// Constructor.
BoardingPanel::BoardingPanel(PlayerInfo &player, const shared_ptr<Ship> &victim)
	: player(player), you(player.FlagshipPtr()), victim(victim),
	attackOdds(*you, *victim), defenseOdds(*victim, *you)
{
	// The escape key should close this panel rather than bringing up the main menu.
	SetInterruptible(false);
	
	// Figure out how much the victim's commodities are worth in the current
	// system and add them to the list of plunder.
	const System &system = *player.GetSystem();
	for(const auto &it : victim->Cargo().Commodities())
		if(it.second)
			plunder.emplace_back(it.first, it.second, system.Trade(it.first));
	
	// You cannot plunder hand to hand weapons, because they are kept in the
	// crew's quarters, not mounted on the exterior of the ship. Certain other
	// outfits are also unplunderable, like outfits expansions.
	auto sit = victim->Outfits().begin();
	auto cit = victim->Cargo().Outfits().begin();
	while(sit != victim->Outfits().end() || cit != victim->Cargo().Outfits().end())
	{
		const Outfit *outfit = nullptr;
		int count = 0;
		// Merge the outfit lists from the ship itself and its cargo bay. If an
		// outfit exists in both locations, combine the counts.
		bool shipIsFirst = (cit == victim->Cargo().Outfits().end() || 
			(sit != victim->Outfits().end() && sit->first <= cit->first));
		bool cargoIsFirst = (sit == victim->Outfits().end() ||
			(cit != victim->Cargo().Outfits().end() && cit->first <= sit->first));
		if(shipIsFirst)
		{
			outfit = sit->first;
			// Don't include outfits that are installed and unplunderable. But,
			// "unplunderable" outfits can still be stolen from cargo.
			if(!sit->first->Get("unplunderable"))
				count += sit->second;
			++sit;
		}
		if(cargoIsFirst)
		{
			outfit = cit->first;
			count += cit->second;
			++cit;
		}
		if(outfit && count)
			plunder.emplace_back(outfit, count);
	}
	
	// Some "ships" do not represent something the player could actually pilot.
	if(!victim->IsCapturable())
		messages.emplace_back("This is not a ship that you can capture.");
	
	// Sort the plunder by price per ton.
	sort(plunder.begin(), plunder.end());
}



// Draw the panel.
void BoardingPanel::Draw()
{
	// Draw a translucent black scrim over everything beneath this panel.
	DrawBackdrop();
	
	// Draw the list of plunder.
	const Color &opaque = *GameData::Colors().Get("panel background");
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	FillShader::Fill(Point(-155., -60.), Point(360., 250.), opaque);
	
	int index = (scroll - 10) / 20;
	int y = -170 - scroll + 20 * index;
	int endY = 60;
	
	const Font &font = FontSet::Get(14);
	// Y offset to center the text in a 20-pixel high row.
	double fontOff = .5 * (20 - font.Height());
	for( ; y < endY && static_cast<unsigned>(index) < plunder.size(); y += 20, ++index)
	{
		const Plunder &item = plunder[index];
		
		// Check if this is the selected row.
		bool isSelected = (index == selected);
		if(isSelected)
			FillShader::Fill(Point(-155., y + 10.), Point(360., 20.), back);
		
		// Color the item based on whether you have space for it.
		const Color &color = item.CanTake(*you) ? isSelected ? bright : medium : dim;
		Point pos(-320., y + fontOff);
		font.Draw(item.Name(), pos, color);
		font.Draw({item.Value(), {260, Alignment::RIGHT}}, pos, color);
		font.Draw({item.Size(), {330, Alignment::RIGHT}}, pos, color);
	}
	
	// Set which buttons are active.
	Information info;
	if(CanExit())
		info.SetCondition("can exit");
	if(CanTake())
		info.SetCondition("can take");
	if(CanCapture())
		info.SetCondition("can capture");
	if(CanAttack() && (you->Crew() > 1 || !victim->RequiredCrew()))
		info.SetCondition("can attack");
	if(CanAttack())
		info.SetCondition("can defend");
	
	// This should always be true, but double check.
	int crew = 0;
	if(you)
	{
		crew = you->Crew();
		info.SetString("cargo space", to_string(you->Cargo().Free()));
		info.SetString("your crew", to_string(crew));
		info.SetString("your attack",
			Round(attackOdds.AttackerPower(crew)));
		info.SetString("your defense",
			Round(defenseOdds.DefenderPower(crew)));
	}
	int vCrew = victim ? victim->Crew() : 0;
	if(victim && (victim->IsCapturable() || victim->IsYours()))
	{
		info.SetString("enemy crew", to_string(vCrew));
		info.SetString("enemy attack",
			Round(defenseOdds.AttackerPower(vCrew)));
		info.SetString("enemy defense",
			Round(attackOdds.DefenderPower(vCrew)));
	}
	if(victim && victim->IsCapturable() && !victim->IsYours())
	{
		// If you haven't initiated capture yet, show the self destruct odds in
		// the attack odds. It's illogical for you to have access to that info,
		// but not knowing what your true odds are is annoying.
		double odds = attackOdds.Odds(crew, vCrew);
		if(!isCapturing)
			odds *= (1. - victim->Attributes().Get("self destruct"));
		info.SetString("attack odds",
			Round(100. * odds) + "%");
		info.SetString("attack casualties",
			Round(attackOdds.AttackerCasualties(crew, vCrew)));
		info.SetString("defense odds",
			Round(100. * (1. - defenseOdds.Odds(vCrew, crew))) + "%");
		info.SetString("defense casualties",
			Round(defenseOdds.DefenderCasualties(vCrew, crew)));
	}
	
	const Interface *interface = GameData::Interfaces().Get("boarding");
	interface->Draw(info, this);
	
	// Draw the status messages from hand to hand combat.
	Point messagePos(50., 55.);
	for(const string &message : messages)
	{
		font.Draw(message, messagePos, bright);
		messagePos.Y() += 20.;
	}
}



// Handle key presses or button clicks that were mapped to key presses.
bool BoardingPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if((key == 'd' || key == 'x' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)))) && CanExit())
	{
		// When closing the panel, mark the player dead if their ship was captured.
		if(playerDied)
			player.Die();
		GetUI()->Pop(this);
	}
	else if(playerDied)
		return false;
	else if(key == 't' && CanTake())
	{
		CargoHold &cargo = you->Cargo();
		int count = plunder[selected].Count();
		
		const Outfit *outfit = plunder[selected].GetOutfit();
		if(outfit)
		{
			// Check if this outfit is ammo for one of your weapons. If so, use
			// it to refill your ammo rather than putting it in cargo.
			int available = count;
			// Keep track of how many you actually took.
			count = 0;
			for(const auto &it : you->Outfits())
				if(it.first != outfit && it.first->Ammo() == outfit)
				{
					// Figure out how many of these outfits you can install.
					count = you->Attributes().CanAdd(*outfit, available);
					you->AddOutfit(outfit, count);
					// You have now installed as many of these items as possible.
					break;
				}
			// Transfer as many as possible of these outfits to your cargo hold.
			count += cargo.Add(outfit, available - count);
			// Take outfits from cargo first, then from the ship itself.
			int remaining = count - victim->Cargo().Remove(outfit, count);
			victim->AddOutfit(outfit, -remaining);
		}
		else
			count = victim->Cargo().Transfer(plunder[selected].Name(), count, cargo);
		
		// If all of the plunder of this type was taken, remove it from the list.
		// Otherwise, just update the count in the list item.
		if(count == plunder[selected].Count())
		{
			plunder.erase(plunder.begin() + selected);
			selected = min<int>(selected, plunder.size());
		}
		else
			plunder[selected].Take(count);
	}
	else if((key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) && !isCapturing)
	{
		// Scrolling the list of plunder.
		if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
			Drag(0, 200 * ((key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP)));
		else
		{
			if(key == SDLK_UP && selected)
				--selected;
			else if(key == SDLK_DOWN && selected < static_cast<int>(plunder.size() - 1))
				++selected;
			
			// Scroll down at least far enough to view the current item.
			double minimumScroll = max(0., 20. * selected - 200.);
			double maximumScroll = 20. * selected;
			scroll = max(minimumScroll, min(maximumScroll, scroll));
		}
	}
	else if(key == 'c' && CanCapture())
	{
		// A ship that self-destructs checks once when you board it, and again
		// when you try to capture it, to see if it will self-destruct. This is
		// so that capturing will be harder than plundering.
		if(Random::Real() < victim->Attributes().Get("self destruct"))
		{
			victim->SelfDestruct();
			GetUI()->Pop(this);
			GetUI()->Push(new Dialog("The moment you blast through the airlock, a series of explosions rocks the enemy ship. They appear to have set off their self-destruct sequence..."));
			return true;
		}
		isCapturing = true;
		messages.push_back("The airlock blasts open. Combat has begun!");
		messages.push_back("(It will end if you both choose to \"defend.\")");
	}
	else if((key == 'a' || key == 'd') && CanAttack())
	{
		int yourStartCrew = you->Crew();
		int enemyStartCrew = victim->Crew();
		
		// Figure out what action the other ship will take. As a special case,
		// if you board them but immediately "defend" they will let you return
		// to your ship in peace. That is to allow the player to "cancel" if
		// they did not really mean to try to capture the ship.
		bool youAttack = (key == 'a' && (yourStartCrew > 1 || !victim->RequiredCrew()));
		bool enemyAttacks = defenseOdds.Odds(enemyStartCrew, yourStartCrew) > .5;
		if(isFirstCaptureAction && !youAttack)
			enemyAttacks = false;
		isFirstCaptureAction = false;
		
		// If neither side attacks, combat ends.
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
			
			// To speed things up, have multiple rounds of combat each time you
			// click the button, if you started with a lot of crew.
			int rounds = max(1, yourStartCrew / 5);
			for(int round = 0; round < rounds; ++round)
			{
				int yourCrew = you->Crew();
				int enemyCrew = victim->Crew();
				if(!yourCrew || !enemyCrew)
					break;
				
				// Your chance of winning this round is equal to the ratio of
				// your power to the enemy's power.
				double yourPower = (youAttack ?
					attackOdds.AttackerPower(yourCrew) : defenseOdds.DefenderPower(yourCrew));
				double enemyPower = (enemyAttacks ?
					defenseOdds.AttackerPower(enemyCrew) : attackOdds.DefenderPower(enemyCrew));
				
				double total = yourPower + enemyPower;
				if(!total)
					break;
				
				if(Random::Real() * total >= yourPower)
					you->AddCrew(-1);
				else
					victim->AddCrew(-1);
			}
			
			// Report how many casualties each side suffered.
			int yourCasualties = yourStartCrew - you->Crew();
			int enemyCasualties = enemyStartCrew - victim->Crew();
			if(yourCasualties && enemyCasualties)
				messages.back() += "You lose " + to_string(yourCasualties)
					+ " crew; they lose " + to_string(enemyCasualties) + ".";
			else if(yourCasualties)
				messages.back() += "You lose " + to_string(yourCasualties) + " crew.";
			else if(enemyCasualties)
				messages.back() += "They lose " + to_string(enemyCasualties) + " crew.";
			
			// Check if either ship has been captured.
			if(!you->Crew())
			{
				messages.push_back("You have been killed. Your ship is lost.");
				you->WasCaptured(victim);
				playerDied = true;
				isCapturing = false;
			}
			else if(!victim->Crew())
			{
				messages.push_back("You have succeeded in capturing this ship.");
				victim->GetGovernment()->Offend(ShipEvent::CAPTURE, victim->RequiredCrew());
				victim->WasCaptured(you);
				if(!victim->JumpsRemaining() && you->CanRefuel(*victim))
					you->TransferFuel(victim->JumpFuelMissing(), &*victim);
				player.AddShip(victim);
				for(const Ship::Bay &bay : victim->Bays())
					if(bay.ship)
					{
						player.AddShip(bay.ship);
						player.HandleEvent(ShipEvent(you, bay.ship, ShipEvent::CAPTURE), GetUI());
					}
				isCapturing = false;
				
				// Report this ship as captured in case any missions care.
				ShipEvent event(you, victim, ShipEvent::CAPTURE);
				player.HandleEvent(event, GetUI());
			}
		}
	}
	else if(command.Has(Command::INFO))
		GetUI()->Push(new ShipInfoPanel(player));
	
	// Trim the list of status messages.
	while(messages.size() > 5)
		messages.erase(messages.begin());
	
	return true;
}



// Handle mouse clicks.
bool BoardingPanel::Click(int x, int y, int clicks)
{
	// Was the click inside the plunder list?
	if(x >= -330 && x < 20 && y >= -180 && y < 60)
	{
		int index = (scroll + y - -170) / 20;
		if(static_cast<unsigned>(index) < plunder.size())
			selected = index;
		return true;
	}
	
	return true;
}



// Allow dragging of the plunder list.
bool BoardingPanel::Drag(double dx, double dy)
{
	// The list is 240 pixels tall, and there are 10 pixels padding on the top
	// and the bottom, so:
	double maximumScroll = max(0., 20. * plunder.size() - 220.);
	scroll = max(0., min(maximumScroll, scroll - dy));
	
	return true;
}



// The scroll wheel can be used to scroll the plunder list.
bool BoardingPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



// You can't exit this panel if you're engaged in hand to hand combat.
bool BoardingPanel::CanExit() const
{
	return !isCapturing;
}



// Check if you can take the given plunder item.
bool BoardingPanel::CanTake() const
{
	// If you ship or the other ship has been captured:
	if(!you->IsYours())
		return false;
	if(victim->IsYours())
		return false;
	if(isCapturing || playerDied)
		return false;
	if(static_cast<unsigned>(selected) >= plunder.size())
		return false;
	
	return plunder[selected].CanTake(*you);
}



// Check if it's possible to initiate hand to hand combat.
bool BoardingPanel::CanCapture() const
{
	// You can't click the "capture" button if you're already in combat mode.
	if(isCapturing || playerDied)
		return false;
	
	// If your ship or the other ship has been captured:
	if(!you->IsYours())
		return false;
	if(victim->IsYours())
		return false;
	if(!victim->IsCapturable())
		return false;
	
	return (!victim->RequiredCrew() || you->Crew() > 1);
}



// Check if you are in the process of hand to hand combat.
bool BoardingPanel::CanAttack() const
{
	return isCapturing;
}



// Functions for BoardingPanel::Plunder:

// Constructor (commodity cargo).
BoardingPanel::Plunder::Plunder(const string &commodity, int count, int unitValue)
	: name(commodity), outfit(nullptr), count(count), unitValue(unitValue)
{
	UpdateStrings();
}



// Constructor (outfit installed in the victim ship or transported as cargo).
BoardingPanel::Plunder::Plunder(const Outfit *outfit, int count)
	: name(outfit->Name()), outfit(outfit), count(count),
	unitValue(outfit->Cost() * (outfit->Get("installable") < 0. ? 1 : Depreciation::Full()))
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



// Get the value of each unit of this plunder item.
int64_t BoardingPanel::Plunder::UnitValue() const
{
	return unitValue;
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
bool BoardingPanel::Plunder::CanTake(const Ship &ship) const
{
	// If there's cargo space for this outfit, you can take it.
	double mass = UnitMass();
	if(ship.Cargo().Free() >= mass)
		return true;
	
	// Otherwise, check if it is ammo for any of your weapons. If so, check if
	// you can install it as an outfit.
	if(outfit)
		for(const auto &it : ship.Outfits())
			if(it.first != outfit && it.first->Ammo() == outfit && ship.Attributes().CanAdd(*outfit))
				return true;
	
	return false;
}



// Take some or all of this plunder item.
void BoardingPanel::Plunder::Take(int count)
{
	this->count -= count;
	UpdateStrings();
}



// Update the text to reflect a change in the item count.
void BoardingPanel::Plunder::UpdateStrings()
{
	double mass = UnitMass();
	if(!outfit)
		size = to_string(count);
	else if(count == 1)
		size = Format::Number(mass);
	else
		size = to_string(count) + " x " + Format::Number(mass);
	
	value = Format::Credits(unitValue * count);
}



// Commodities come in units of one ton.
double BoardingPanel::Plunder::UnitMass() const
{
	return outfit ? outfit->Mass() : 1.;
}
