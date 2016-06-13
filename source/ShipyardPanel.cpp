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
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "InfoPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "System.h"
#include "UI.h"

using namespace std;

namespace {
	string JUNKYARD_SHIP_SUFFIX = " (Empty Hull)";
}

ShipyardPanel::ShipyardPanel(PlayerInfo &player)
	: ShopPanel(player, Ship::CATEGORIES), modifier(0), used(player.UsedShips()), junkyard(player.JunkyardShips()) 
{	
	for(const auto &it : GameData::Ships())
		catalog[it.second.Attributes().Category()].insert(it.first);
	
	// Put in the junkyard category at the bottom.
	for(auto it : player.JunkyardShips())
		if (it)
			catalog[Ship::JUNKYARD_CATEGORY_NAME].insert(it->ModelName() + JUNKYARD_SHIP_SUFFIX);
	
	if(player.GetPlanet())
		shipyard = player.GetPlanet()->Shipyard();
}



int ShipyardPanel::TileSize() const
{
	return SHIP_SIZE;
}



int ShipyardPanel::DrawPlayerShipInfo(const Point &point) const
{
	Point drawPoint = point;
	shipInfo.Update(*playerShip);

	// This should look good in a side-by-side comparison with a ship from the shipyard.
	shipInfo.DrawAttributes(drawPoint);
	drawPoint.Y() += shipInfo.AttributesHeight();
	
	FillShader::Fill(drawPoint + Point(DETAILS_WIDTH/2,0), Point(DETAILS_WIDTH, 1), COLOR_DIVIDERS);
	
	shipInfo.DrawOutfits(drawPoint);
	drawPoint.Y() += shipInfo.OutfitsHeight();

	FillShader::Fill(drawPoint + Point(DETAILS_WIDTH/2,0), Point(DETAILS_WIDTH, 1), COLOR_DIVIDERS);
	
	shipInfo.DrawDescription(drawPoint);
	drawPoint.Y() += shipInfo.DescriptionHeight();
	
	return drawPoint.Y() - point.Y();
}



int ShipyardPanel::DrawCargoHoldInfo(const Point &point) const
{
	Point drawPoint = point + Point(10,0);
	
	drawPoint.Y() += InfoPanel::DrawCargoHold(player.Cargo(), drawPoint , Point(DETAILS_WIDTH - 20, 20), 0, nullptr);	
	
	return drawPoint.Y() - point.Y();
}



bool ShipyardPanel::HasItem(const string &name) const
{
	string modelName = name;
	bool isJunkyard = false;
	if (Format::EndsWith(name, Ship::JUNKYARD_SHIP_SUFFIX))
	{
		modelName = name.substr(0, name.size() - Ship::JUNKYARD_SHIP_SUFFIX.size());
		isJunkyard = true;
	}
	const Ship *newShip = GameData::Ships().Get(modelName);
	const Ship *usedShip = MostUsedModel(isJunkyard ? junkyard : used, modelName);
	
	if(!shipyard.Has(newShip) && !usedShip)
		return false;
	return true;
}



int ShipyardPanel::DrawItem(const string &name, const Point &point, int scrollY) const
{
	string modelName = name;
	bool isJunkyard = false;
	if (Format::EndsWith(name, Ship::JUNKYARD_SHIP_SUFFIX))
	{
		modelName = name.substr(0, name.size() - Ship::JUNKYARD_SHIP_SUFFIX.size());
		isJunkyard = true;
	}
	const Ship *newShip = GameData::Ships().Get(modelName);
	const Ship *usedShip = MostUsedModel(isJunkyard ? junkyard : used, modelName);
	const Ship *shipToDraw = usedShip ? usedShip : newShip;
	
	if(!shipyard.Has(newShip) && !usedShip)
		return NOT_DRAWN;
	
	// The ship we put into "zones" will become the "selectedShip" when this zone is clicked.
	zones.emplace_back(point, Point(SHIP_SIZE-2, SHIP_SIZE-2), shipToDraw, scrollY);
	// If it's the same as "selectedShip" then this tile is selected.
	int retVal = (selectedShip && shipToDraw == selectedShip) ? SELECTED : DRAWN;
	
	if(point.Y() + SHIP_SIZE / 2 < Screen::Top() || point.Y() - SHIP_SIZE / 2 > Screen::Bottom())
		return retVal;
	
	DrawShip(*shipToDraw, point, retVal == SELECTED);
	
	// If we're drawing a used ship, draw the sale tag.
	if (usedShip)
	{
		Font saleFont = FontSet::Get(18);
		const Color &bright = *GameData::Colors().Get("bright");
		string saleLabel = (isJunkyard?"[EMPTY HULL! ":"[SALE! ") + Format::Percent(1 - OutfitGroup::CostFunction(usedShip->GetWear())) + " OFF!]";
		Point pos = point + Point(-saleFont.Width(saleLabel) / 2, -OUTFIT_SIZE / 2 );
		saleFont.Draw(saleLabel, pos, bright);
	}
	
	return retVal;
}



int ShipyardPanel::DividerOffset() const
{
	return 121;
}



int ShipyardPanel::DetailWidth() const
{
	return (detailsInWithMain ? 3 : 1) * ShipInfoDisplay::PanelWidth();
}



int ShipyardPanel::DrawDetails(const Point &center) const
{
	shipInfo.Update(*selectedShip);
	
	Point offset(shipInfo.PanelWidth(), 0.);

	if (detailsInWithMain)
	{
		Point offset(shipInfo.PanelWidth(), 0.);
		shipInfo.DrawDescription(center - offset * 1.5);
		shipInfo.DrawAttributes(center - offset * .5);
		shipInfo.DrawOutfits(center + offset * .5);
	}
	else
	{
		Point startPoint = Point(Screen::Right() - SideWidth() - PlayerShipWidth() - shipInfo.PanelWidth(), Screen::Top() + 10. - detailsScroll);
		Point drawPoint = startPoint;
		
		const Font &font = FontSet::Get(14);
		Color bright = *GameData::Colors().Get("bright");
		static const string label = "Details:";
		font.Draw(label, drawPoint + Point(DetailWidth()/2 - font.Width(label)/2 ,0) , bright);
		drawPoint.Y() += 30;
		
		DrawShip(*selectedShip, drawPoint + Point(DetailsWidth()/2, TileSize()/2), true);
		drawPoint.Y() += TileSize();
		
		shipInfo.DrawAttributes(drawPoint);
		drawPoint.Y() += shipInfo.AttributesHeight() + 10;
		
		FillShader::Fill(drawPoint + Point(DETAILS_WIDTH/2,0), Point(DETAILS_WIDTH, 1), COLOR_DIVIDERS);		
		
		shipInfo.DrawOutfits(drawPoint);
		drawPoint.Y() += shipInfo.OutfitsHeight() + 10;
		
		FillShader::Fill(drawPoint + Point(DETAILS_WIDTH/2,0), Point(DETAILS_WIDTH, 1), COLOR_DIVIDERS);
		
		shipInfo.DrawDescription(drawPoint);
		drawPoint.Y() += shipInfo.DescriptionHeight() + 10;
		
		maxDetailsScroll = max(0, (int)drawPoint.Y() - (int)startPoint.Y() - Screen::Height());
	}
	
	return detailsInWithMain ? shipInfo.MaximumHeight() : 0;
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
		message = "Enter a name for your brand new "; //TODO: Don't call it "brand new" if it's not.
	message += selectedShip->ModelName() + "!";
	GetUI()->Push(new Dialog(this, &ShipyardPanel::BuyShip, message));
}



void ShipyardPanel::FailBuy()
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
	
	double totalCost = 0;
	for (auto it : playerShips)
		totalCost += it->Cost();
	
	message += "\nYou will receive " + Format::Number(totalCost) + " credits.";
	
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
	
	string modelName = selectedShip->ModelName();
	bool isJunkyard = selectedShip == MostUsedModel(junkyard, modelName);
	bool parkedShipMessage = false;
	for(int i = 1; i <= modifier; ++i)
	{
		auto shipToBuy = MostUsedModel(isJunkyard ? junkyard : used, modelName);
		if (!shipToBuy && isJunkyard)
			break; // No more empty hulls of this type available.
		if (!shipToBuy)
			shipToBuy = GameData::Ships().Get(modelName); // No more used available - buy new.
			
		if(modifier > 1)
			player.BuyShip(shipToBuy, shipName + to_string(i));
		else
			player.BuyShip(shipToBuy, shipName);
		
		if(!shipToBuy->PassesFlightCheck())
			parkedShipMessage = true;
		
		used.remove(shipToBuy);			
		junkyard.remove(shipToBuy);
	}
	
	UpdateJunkyardCatalog();
	
	playerShip = &*player.Ships().back();
	playerShips.clear();
	playerShips.insert(playerShip);
	
	if (parkedShipMessage)
		GetUI()->Push(new Dialog("The \"ship\" you just bought doesn't even have the hardware it needs to pass a basic flight check. You decide to park it until you can give it some attention at the outfitter."));
}



void ShipyardPanel::SellShip()
{
	for(Ship *ship : playerShips)
		player.SellShip(ship);

	playerShips.clear();
	playerShip = nullptr;
	for(shared_ptr<Ship> ship : player.Ships())
		if(ShipIsHere(ship))
		{
			playerShip = ship.get();
			break;
		}
	if(playerShip)
		playerShips.insert(playerShip);
		
	UpdateJunkyardCatalog();
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



const Ship* ShipyardPanel::MostUsedModel(std::list<const Ship*> listToSearch, const string& modelName) const 
{
	const Ship* retVal = nullptr;
	for (auto it : listToSearch)
	{
		if(it && it->ModelName() == modelName && (!retVal || it->GetWear() > retVal->GetWear()))
			retVal = it;
	}
	return retVal;
}



void ShipyardPanel::UpdateJunkyardCatalog() 
{
	// Put in the junkyard category at the bottom.
	catalog[Ship::JUNKYARD_CATEGORY_NAME].clear();
	for(auto it : junkyard)
		if (it)
			catalog[Ship::JUNKYARD_CATEGORY_NAME].insert(it->ModelName() + JUNKYARD_SHIP_SUFFIX);	
}
