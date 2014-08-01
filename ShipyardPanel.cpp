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

#include "Color.h"
#include "Dialog.h"
#include "FillShader.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "ShipInfoDisplay.h"
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
	ShipInfoDisplay info(*playerShip);
	info.DrawSale(point);
	
	return info.SaleHeight();
}



bool ShipyardPanel::DrawItem(const string &name, const Point &point) const
{
	const Ship *ship = GameData::Ships().Get(name);
	if(!planet->Shipyard().Has(ship))
		return false;
	
	DrawShip(*ship, point, ship == selectedShip);
	zones.emplace_back(point.X(), point.Y(), SHIP_SIZE / 2, SHIP_SIZE / 2, ship);
	
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
	ShipInfoDisplay info(*selectedShip);
	Point offset(info.PanelWidth(), 0.);
	
	info.DrawDescription(center - offset * 1.5);
	info.DrawAttributes(center - offset * .5);
	info.DrawOutfits(center + offset * .5);
	
	return info.MaximumHeight();
}



bool ShipyardPanel::CanBuy() const
{
	return (selectedShip && player.Accounts().Credits() >= selectedShip->Cost());
}



void ShipyardPanel::Buy()
{
	GetUI()->Push(new Dialog(this, &ShipyardPanel::BuyShip,
		"Enter a name for your brand new " + selectedShip->ModelName() + "!"));
}



bool ShipyardPanel::CanSell() const
{
	return playerShip;
}



void ShipyardPanel::Sell()
{
	GetUI()->Push(new Dialog(this, &ShipyardPanel::SellShip,
		"Sell ''" + playerShip->Name() + "''?"));
}



bool ShipyardPanel::FlightCheck()
{
	// TODO: Check, e.g. that all your fighters have a ship to carry them?
	// Or, should that be a limit placed on buying fighters / selling carriers?
	return true;
}



int ShipyardPanel::Modifier() const
{
	// Never allow buying ships in bulk?
	return 1;
}



void ShipyardPanel::BuyShip(const string &name)
{
	if(name.empty())
		player.BuyShip(selectedShip, "Unnamed Ship");
	else
		player.BuyShip(selectedShip, name);
}



void ShipyardPanel::SellShip()
{
	player.SellShip(playerShip);
	playerShip = nullptr;
}
