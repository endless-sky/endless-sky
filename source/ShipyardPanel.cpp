/* ShipyardPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipyardPanel.h"

#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Ship.h"
#include "ShipInfoDisplay.h"
#include "System.h"
#include "UI.h"

using namespace std;

namespace {
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
}



ShipyardPanel::ShipyardPanel(PlayerInfo &player)
	: ShopPanel(player, CATEGORIES)
{
	for(const auto &it : GameData::Ships())
		catalog[it.second.Attributes().Category()].insert(it.first);
}



int ShipyardPanel::TileSize() const
{
	return SHIP_SIZE;
}



int ShipyardPanel::DrawPlayerShipInfo(const Point &point) const
{
	ShipInfoDisplay info(*playerShip, player.GetSystem()->GetGovernment());
	info.DrawSale(point);
	
	return info.SaleHeight();
}



bool ShipyardPanel::DrawItem(const string &name, const Point &point, int scrollY) const
{
	const Ship *ship = GameData::Ships().Get(name);
	if(!planet->Shipyard().Has(ship))
		return false;
	
	DrawShip(*ship, point, ship == selectedShip);
	zones.emplace_back(point.X(), point.Y(), SHIP_SIZE / 2, SHIP_SIZE / 2, ship, scrollY);
	
	return true;
}



int ShipyardPanel::DividerOffset() const
{
	return 121;
}



int ShipyardPanel::DetailWidth() const
{
	return 3 * ShipInfoDisplay::PanelWidth();
}




int ShipyardPanel::DrawDetails(const Point &center) const
{
	ShipInfoDisplay info(*selectedShip, player.GetSystem()->GetGovernment());
	Point offset(info.PanelWidth(), 0.);
	
	info.DrawDescription(center - offset * 1.5);
	info.DrawAttributes(center - offset * .5);
	info.DrawOutfits(center + offset * .5);
	
	return info.MaximumHeight();
}



bool ShipyardPanel::CanBuy() const
{
	if(!selectedShip)
		return false;
	
	int cost = selectedShip->Cost();
	
	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost();
	if(licenseCost < 0)
		return false;
	cost += licenseCost;
	
	return (player.Accounts().Credits() >= cost);
}



void ShipyardPanel::Buy()
{
	int64_t licenseCost = LicenseCost();
	if(licenseCost < 0)
		return;
	
	modifier = Modifier();
	string message;
	if(licenseCost)
		message = "Note: you will need to pay " + Format::Number(licenseCost)
			+ " credits for the licenses required to operate this ship, in addition to its cost."
			" If that is okay with you, go ahead and enter a name for your brand new ";
	else
		message = "Enter a name for your brand new ";
	message += selectedShip->ModelName() + "!";
	GetUI()->Push(new Dialog(this, &ShipyardPanel::BuyShip, message));
}



void ShipyardPanel::FailBuy() const
{
	if(!selectedShip)
		return;
	
	int64_t cost = selectedShip->Cost();
	
	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost();
	if(licenseCost < 0)
	{
		GetUI()->Push(new Dialog("Buying this ship requires a special license. "
			"You will probably need to complete some sort of mission to get one."));
		return;
	}
	
	cost += licenseCost;
	if(player.Accounts().Credits() < cost)
	{
		for(const auto &it : player.Ships())
			cost -= it->Cost();
		if(player.Accounts().Credits() < cost)
			GetUI()->Push(new Dialog("You do not have enough credits to buy this ship. "
				"Consider checking if the bank will offer you a loan."));
		else
		{
			string ship = (player.Ships().size() == 1) ? "your current ship" : "one of your ships";
			GetUI()->Push(new Dialog("You do not have enough credits to buy this ship. "
				"If you want to buy it, you must sell " + ship + " first."));
		}
		return;
	}
}



bool ShipyardPanel::CanSell() const
{
	return playerShip;
}



void ShipyardPanel::Sell()
{
	static const int MAX_LIST = 20;
	
	int count = playerShips.size();
	string message = "Sell ";
	if(count == 1)
		message += playerShip->Name();
	else if(count <= MAX_LIST)
	{
		auto it = playerShips.begin();
		message += (*it++)->Name();
		--count;
		
		if(count == 1)
			message += " and ";
		else
		{
			while(count-- > 1)
				message += ",\n" + (*it++)->Name();
			message += ",\nand ";
		}
		message += (*it)->Name();
	}
	else
	{
		auto it = playerShips.begin();
		for(int i = 0; i < MAX_LIST - 1; ++i)
			message += (*it++)->Name() + ",\n";
		
		message += "and " + Format::Number(count - (MAX_LIST - 1)) + " other ships";
	}
	message += "?";
	GetUI()->Push(new Dialog(this, &ShipyardPanel::SellShip, message));
}



bool ShipyardPanel::FlightCheck()
{
	// The shipyard does not perform any flight checks.
	return true;
}



bool ShipyardPanel::CanSellMultiple() const
{
	return false;
}



void ShipyardPanel::BuyShip(const string &name)
{
	int64_t licenseCost = LicenseCost();
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &name : selectedShip->Licenses())
			if(player.GetCondition("license: " + name) <= 0)
				player.Conditions()["license: " + name] = true;
	}
	
	string shipName = name.empty() ? "Unnamed Ship" : name;
	if(modifier > 1)
		shipName += ' ';
	
	for(int i = 1; i <= modifier; ++i)
	{
		if(modifier > 1)
			player.BuyShip(selectedShip, shipName + to_string(i));
		else
			player.BuyShip(selectedShip, shipName);
	}
	
	playerShip = &*player.Ships().back();
	playerShips.clear();
	playerShips.insert(playerShip);
}



void ShipyardPanel::SellShip()
{
	for(Ship *ship : playerShips)
		player.SellShip(ship);
	playerShips.clear();
	playerShip = nullptr;
	for(shared_ptr<Ship> ship : player.Ships())
		if(ship->GetSystem() == player.GetSystem() && !ship->IsDisabled())
		{
			playerShip = ship.get();
			break;
		}
	if(playerShip)
		playerShips.insert(playerShip);
}



int64_t ShipyardPanel::LicenseCost() const
{
	int64_t cost = 0;
	for(const string &name : selectedShip->Licenses())
		if(player.GetCondition("license: " + name) <= 0)
		{
			const Outfit *outfit = GameData::Outfits().Get(name + " License");
			if(!outfit->Cost())
				return -1;
			cost += outfit->Cost();
		}
	return cost;
}
