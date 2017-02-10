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
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "UI.h"

using namespace std;

namespace {
	// The name entry dialog should include a "Random" button to choose a random
	// name using the civilian ship name generator.
	class NameDialog : public Dialog {
	public:
		NameDialog(ShipyardPanel *panel, void (ShipyardPanel::*fun)(const string &), const string &message)
			: Dialog(panel, fun, message) {}
		
		virtual void Draw() override
		{
			Dialog::Draw();
			
			randomPos = cancelPos - Point(80., 0.);
			SpriteShader::Draw(SpriteSet::Get("ui/dialog cancel"), randomPos);

			const Font &font = FontSet::Get(14);
			static const string label = "Random";
			Point labelPos = randomPos - .5 * Point(font.Width(label), font.Height());
			font.Draw(label, labelPos, *GameData::Colors().Get("medium"));
		}
		
	protected:
		virtual bool Click(int x, int y, int clicks) override
		{
			Point off = Point(x, y) - randomPos;
			if(fabs(off.X()) < 40. && fabs(off.Y()) < 20.)
			{
				input = GameData::Phrases().Get("civilian")->Get();
				return true;
			}
			return Dialog::Click(x, y, clicks);
		}
		
	private:
		Point randomPos;
	};
}



ShipyardPanel::ShipyardPanel(PlayerInfo &player)
	: ShopPanel(player, false), modifier(0)
{
	for(const auto &it : GameData::Ships())
		catalog[it.second.Attributes().Category()].insert(it.first);
	
	if(player.GetPlanet())
		shipyard = player.GetPlanet()->Shipyard();
}



int ShipyardPanel::TileSize() const
{
	return SHIP_SIZE;
}



int ShipyardPanel::DrawPlayerShipInfo(const Point &point)
{
	shipInfo.Update(*playerShip, player.FleetDepreciation(), player.GetDate().DaysSinceEpoch());
	shipInfo.DrawSale(point);
	shipInfo.DrawAttributes(point + Point(0, shipInfo.SaleHeight()));
	
	return shipInfo.SaleHeight() + shipInfo.AttributesHeight();
}



bool ShipyardPanel::HasItem(const string &name) const
{
	const Ship *ship = GameData::Ships().Get(name);
	return shipyard.Has(ship);
}



void ShipyardPanel::DrawItem(const string &name, const Point &point, int scrollY)
{
	const Ship *ship = GameData::Ships().Get(name);
	zones.emplace_back(point, Point(SHIP_SIZE, SHIP_SIZE), ship, scrollY);
	if(point.Y() + SHIP_SIZE / 2 < Screen::Top() || point.Y() - SHIP_SIZE / 2 > Screen::Bottom())
		return;
	
	DrawShip(*ship, point, ship == selectedShip);
}



int ShipyardPanel::DividerOffset() const
{
	return 121;
}



int ShipyardPanel::DetailWidth() const
{
	return 3 * shipInfo.PanelWidth();
}



int ShipyardPanel::DrawDetails(const Point &center)
{
	shipInfo.Update(*selectedShip, player.StockDepreciation(), player.GetDate().DaysSinceEpoch());
	Point offset(shipInfo.PanelWidth(), 0.);
	
	shipInfo.DrawDescription(center - offset * 1.5);
	shipInfo.DrawAttributes(center - offset * .5);
	shipInfo.DrawOutfits(center + offset * .5);
	
	return shipInfo.MaximumHeight();
}



bool ShipyardPanel::CanBuy() const
{
	if(!selectedShip)
		return false;
	
	int64_t cost = player.StockDepreciation().Value(*selectedShip, day);
	
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
	GetUI()->Push(new NameDialog(this, &ShipyardPanel::BuyShip, message));
}



void ShipyardPanel::FailBuy() const
{
	if(!selectedShip)
		return;
	
	int64_t cost = player.StockDepreciation().Value(*selectedShip, day);
	
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
			cost -= player.FleetDepreciation().Value(*it, day);
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
	int initialCount = count;
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
	// To allow calculating the sale price of all the ships in the list,
	// temporarily copy into a shared_ptr vector:
	vector<shared_ptr<Ship>> toSell;
	for(const auto &it : playerShips)
		toSell.push_back(it->shared_from_this());
	int64_t total = player.FleetDepreciation().Value(toSell, day);
	
	message += ((initialCount > 2) ? "\nfor " : " for ") + Format::Number(total) + " credits?";
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
	
	string shipName = name;
	if(shipName.empty())
		shipName = player.FirstName() + "'s " + selectedShip->ModelName();
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
	player.UpdateCargoCapacities();
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
