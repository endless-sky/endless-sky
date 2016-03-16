/* OutfitterPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutfitterPanel.h"

#include "Color.h"
#include "ConversationPanel.h"
#include "Dialog.h"
#include "DistanceMap.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "OutfitInfoDisplay.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipInfoDisplay.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <limits>

class Sprite;

using namespace std;

namespace {
	string Tons(int tons)
	{
		return to_string(tons) + (tons == 1 ? " ton" : " tons");
	}
}



OutfitterPanel::OutfitterPanel(PlayerInfo &player)
	: ShopPanel(player, Outfit::CATEGORIES), available(player.SoldOutfits())
{
	for(const pair<string, Outfit> &it : GameData::Outfits())
		catalog[it.second.Category()].insert(it.first);
	
	if(player.GetPlanet())
		outfitter = player.GetPlanet()->Outfitter();
}


	
void OutfitterPanel::Step()
{
	CheckRefill();
	if(!Preferences::Has("help: outfitter"))
	{
		Preferences::Set("help: outfitter");
		GetUI()->Push(new Dialog(
			"Here, you can buy new equipment for your ship. "
			"Your ship has a limited amount of \"outfit space,\" "
			"and most outfits use up some of that space.\n"
			"\tSome types of outfits have other requirements as well. "
			"For example, only some of your outfit space can be used for engines or weapons; "
			"this is your ship's \"engine capacity\" and \"weapon capacity.\" "
			"Guns and missile launchers also require a free \"gun port,\" "
			"and turrets require a free \"turret mount.\" "
			"Also, missiles can only be bought if you have the right launcher installed.\n"
			"\tUse your scroll wheel, or click and drag, to scroll the view.\n"
			"\tAs in the trading panel, you can hold down Shift or Control "
			"to buy 5 or 20 of an outfit at once, or both keys to buy 100."));
	}
	ShopPanel::Step();
}



int OutfitterPanel::TileSize() const
{
	return OUTFIT_SIZE;
}



int OutfitterPanel::DrawPlayerShipInfo(const Point &point) const
{
	ShipInfoDisplay info(*playerShip);
	info.DrawAttributes(point);
	
	return info.AttributesHeight();
}



bool OutfitterPanel::DrawItem(const string &name, const Point &point, int scrollY) const
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	bool hasOutfit = (outfitter.Has(outfit) || available.Find(outfit) || player.Cargo().GetOutfitCount(outfit));
	if(!hasOutfit)
		for(const Ship *ship : playerShips)
			if(ship->OutfitCount(outfit))
			{
				hasOutfit = true;
				break;
			}
	if(!hasOutfit)
		return false;
	
	zones.emplace_back(point.X(), point.Y(), OUTFIT_SIZE / 2, OUTFIT_SIZE / 2, outfit, scrollY);
	if(point.Y() + OUTFIT_SIZE / 2 < Screen::Top() || point.Y() - OUTFIT_SIZE / 2 > Screen::Bottom())
		return true;
	
	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);
	
	// Check if this outfit is a "license".
	bool isLicense = IsLicense(name);
	int mapSize = outfit->Get("map");
	
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	if(playerShip || isLicense || mapSize || player.Cargo().GetOutfitCount(outfit))
	{
		int minCount = numeric_limits<int>::max();
		int maxCount = 0;
		int64_t minPrice = 0;
		int64_t maxPrice = 0;

		if(isLicense)
			minCount = maxCount = player.GetCondition(LicenseName(name));
		else if(mapSize)
			minCount = maxCount = HasMapped(mapSize);
		else
		{
			for(const Ship *ship : playerShips)
			{
				int count = ship->OutfitCount(outfit);
				int64_t highprice = ship->Outfits().GetCost(outfit, 1, false);
				int64_t lowprice = ship->Outfits().GetCost(outfit, 1, true);
				
				minCount = min(minCount, count);
				maxCount = max(maxCount, count);
				minPrice = minPrice > 0 ? min(minPrice, lowprice) : lowprice;
				maxPrice = max(maxPrice, highprice);
			}
		}
				
		if(player.Cargo().GetOutfitCount(outfit))
		{
			int64_t highprice = player.Cargo().Outfits().GetCost(outfit, 1, false);
			int64_t lowprice = player.Cargo().Outfits().GetCost(outfit, 1, true);
			minPrice = minPrice > 0 ? min(minPrice, lowprice) : lowprice;
			maxPrice = max(maxPrice, highprice);
			
			string label =  "in cargo: " + to_string(player.Cargo().GetOutfitCount(outfit));		
			Point pos = point + Point(
				-OUTFIT_SIZE / 2 + 20,
				OUTFIT_SIZE / 2 - 66);
			font.Draw(label, pos, bright);
		}
		
		if(maxCount)
		{
			// Installed label.
			string label = "installed: " + to_string(minCount);
			if(maxCount > minCount)
				label += " - " + to_string(maxCount);
			Point labelPos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 52);
			font.Draw(label, labelPos, bright);
		}
		
		if (maxPrice)
		{
			// Sell price label (includes cargo prices)
			string sellLabel =  "sell value: " + Format::Percent(minPrice, outfit->Cost());
			if (minPrice != maxPrice)
				sellLabel += "-" + Format::Percent(maxPrice, outfit->Cost());
			Point pos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 38);
			font.Draw(sellLabel, pos, bright);
		}
	}
	


	// Buy price label.
	string buyLabel;
	if (available.GetTotalCount(outfit))
	{
		int64_t cost = available.GetCost(outfit, 1, true);
		buyLabel =  + "buy used("+to_string(available.GetTotalCount(outfit))+"): " + Format::Number(cost);
		
		if (95 * outfit->Cost() >= 100 * cost)
		{
			Font saleFont = FontSet::Get(18);
			std::string saleLabel = "[SALE! "+Format::Percent(outfit->Cost()-cost, outfit->Cost())+" OFF!]";
			Point pos = point + Point(-saleFont.Width(saleLabel) / 2, -OUTFIT_SIZE / 2 + 26);
			saleFont.Draw(saleLabel, pos, bright);
		}
	}
	else if (outfitter.Has(outfit))
		buyLabel = "buy new: " + Format::Number(outfit->Cost());
	else 
		buyLabel = "(not sold here)";
	
	Point pos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 24);
	font.Draw(buyLabel, pos, bright);
	
	return true;
}



int OutfitterPanel::DividerOffset() const
{
	return 80;
}



int OutfitterPanel::DetailWidth() const
{
	return 3 * OutfitInfoDisplay::PanelWidth();
}



int OutfitterPanel::DrawDetails(const Point &center) const
{
	OutfitInfoDisplay info(*selectedOutfit);
	Point offset(info.PanelWidth(), 0.);
	
	info.DrawDescription(center - offset * 1.5 - Point(0., 10.));
	info.DrawRequirements(center - offset * .5 - Point(0., 10.));
	info.DrawAttributes(center + offset * .5 - Point(0., 10.));
	
	return info.MaximumHeight();
}



bool OutfitterPanel::CanBuy() const
{
	if(!planet || !selectedOutfit || !playerShip)
		return false;
	
	if(!(outfitter.Has(selectedOutfit) 
		|| available.Find(selectedOutfit)
		|| player.Cargo().GetOutfitCount(selectedOutfit)))
		return false;
	
	int mapSize = selectedOutfit->Get("map");
	if(mapSize > 0 && HasMapped(mapSize))
		return false;
	
	if(HasLicense(selectedOutfit->Name()))
		return false;
	
	// If you have this in your cargo hold, installing it is free.
	if(selectedOutfit->Cost() > player.Accounts().Credits() 
		&& !player.Cargo().GetOutfitCount(selectedOutfit))
		return false;
	
	for(const Ship *ship : playerShips)
		if(ShipCanBuy(ship, selectedOutfit))
			return true;
	
	return false;
}



void OutfitterPanel::Buy()
{
	int modifier = Modifier();
	for(int i = 0; i < modifier && CanBuy(); ++i)
	{
		// Special case: maps.
		int mapSize = selectedOutfit->Get("map");
		if(mapSize > 0)
		{
			if(!HasMapped(mapSize))
			{
				DistanceMap distance(player.GetSystem(), mapSize);
				for(const auto &it : distance.Distances())
					if(!player.HasVisited(it.first))
						player.Visit(it.first);
				player.Accounts().AddCredits(-selectedOutfit->Cost());
			}
			return;
		}
	
		// Special case: licenses.
		if(IsLicense(selectedOutfit->Name()))
		{
			int &entry = player.Conditions()[LicenseName(selectedOutfit->Name())];
			if(entry <= 0)
			{
				entry = true;
				player.Accounts().AddCredits(-selectedOutfit->Cost());
			}
			return;
		}
	
		// Find the ships with the fewest number of these outfits.
		vector<Ship *> shipsToOutfit;
		int fewest = numeric_limits<int>::max();
		for(Ship *ship : playerShips)
		{
			if(!ShipCanBuy(ship, selectedOutfit))
				continue;
		
			int count = ship->OutfitCount(selectedOutfit);
			if(count < fewest)
			{
				shipsToOutfit.clear();
				fewest = count;
			}
			if(count == fewest)
				shipsToOutfit.push_back(ship);
		}
	
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanBuy())
				return;
		
			if(player.Cargo().GetOutfitCount(selectedOutfit))
				ship->TransferOutfitToCargo(selectedOutfit, -1, player.Cargo(), true, 0);
			else if(!available.Find(selectedOutfit) && !outfitter.Has(selectedOutfit))
				break;
			else if (available.Find(selectedOutfit))
			{	// Buy lowest price used outfits first.
				player.Accounts().AddCredits(-available.GetCost(selectedOutfit, 1, true));
				ship->TransferOutfit(selectedOutfit, -1, &available, true, 0);
			}
			else
			{	// Buy new
				player.Accounts().AddCredits(-selectedOutfit->Cost());
				ship->AddOutfit(selectedOutfit, 1, 0);				
			}
			
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge(true);
		}
	}
}



void OutfitterPanel::FailBuy()
{
	if(!selectedOutfit || !playerShip)
		return;
	
	int64_t cost = selectedOutfit->Cost();
	int64_t credits = player.Accounts().Credits();
	bool isInCargo = player.Cargo().GetOutfitCount(selectedOutfit);
	if(!isInCargo && cost > credits)
	{
		GetUI()->Push(new Dialog("You cannot buy this outfit, because it costs "
			+ to_string(cost) + " credits, and you only have "
			+ to_string(credits) + "."));
		return;
	}
	
	if(!(outfitter.Has(selectedOutfit) || available.GetTotalCount(selectedOutfit) || isInCargo))
	{
		GetUI()->Push(new Dialog("You cannot buy this outfit here. "
			"It is being shown in the list because you have one installed in your ship, "
			"but this " + planet->Noun() + " does not sell them."));
		return;
	}
	
	double outfitNeeded = -selectedOutfit->Get("outfit space");
	double outfitSpace = playerShip->Attributes().Get("outfit space");
	if(outfitNeeded > outfitSpace)
	{
		string need =  to_string(outfitNeeded) + (outfitNeeded != 1. ? "tons" : "ton");
		BuyAsCargoPrompt("You cannot install this outfit, because it takes up "
			+ Tons(outfitNeeded) + " of outfit space, and this ship has "
			+ Tons(outfitSpace) + " free.");
		return;
	}
	
	double weaponNeeded = -selectedOutfit->Get("weapon capacity");
	double weaponSpace = playerShip->Attributes().Get("weapon capacity");
	if(weaponNeeded > weaponSpace)
	{
		BuyAsCargoPrompt("Only part of your ship's outfit capacity is usable for weapons. "
			"You cannot install this outfit, because it takes up "
			+ Tons(weaponNeeded) + " of weapon space, and this ship has "
			+ Tons(weaponSpace) + " free.");
		return;
	}
	
	double engineNeeded = -selectedOutfit->Get("engine capacity");
	double engineSpace = playerShip->Attributes().Get("engine capacity");
	if(engineNeeded > engineSpace)
	{
		BuyAsCargoPrompt("Only part of your ship's outfit capacity is usable for engines. "
			"You cannot install this outfit, because it takes up "
			+ Tons(engineNeeded) + " of engine space, and this ship has "
			+ Tons(engineSpace) + " free.");
		return;
	}
	
	if(selectedOutfit->Category() == "Ammunition")
	{
		// Since ammunition has zero mass, don't allow buying it as cargo.
		if(!playerShip->OutfitCount(selectedOutfit))
			GetUI()->Push(new Dialog("This outfit is ammunition for a weapon. "
				"You cannot install it without first installing the appropriate weapon."));
		else
			GetUI()->Push(new Dialog("You already have the maximum amount of ammunition for this weapon. "
				"If you want to install more ammunition, you must first install another of these weapons."));
		return;
	}
	
	int mountsNeeded = -selectedOutfit->Get("turret mounts");
	int mountsFree = playerShip->Attributes().Get("turret mounts");
	if(mountsNeeded && !mountsFree)
	{
		BuyAsCargoPrompt("This weapon is designed to be installed on a turret mount, "
			"but your ship does not have any unused turret mounts available.");
		return;
	}
	
	int gunsNeeded = -selectedOutfit->Get("gun ports");
	int gunsFree = playerShip->Attributes().Get("gun ports");
	if(gunsNeeded && !gunsFree)
	{
		BuyAsCargoPrompt("This weapon is designed to be installed in a gun port, "
			"but your ship does not have any unused gun ports available.");
		return;
	}
	
	if(!playerShip->Attributes().CanAdd(*selectedOutfit, 1))
	{
		BuyAsCargoPrompt("You cannot install this outfit in your ship, "
			"because it would reduce one of your ship's attributes to a negative amount. "
			"For example, perhaps it uses up more cargo space than you have left, "
			"or has a constant energy draw greater than what your ship produces.");
		return;
	}
	
	if(selectedOutfit->Get("map"))
	{
		GetUI()->Push(new Dialog("You have already mapped all the systems shown by this map, "
			"so there is no reason to buy another."));
		return;
	}
	
	if(HasLicense(selectedOutfit->Name()))
	{
		GetUI()->Push(new Dialog("You already have one of these licenses, "
			"so there is no reason to buy another."));
		return;
	}
}



bool OutfitterPanel::CanSell() const
{
	if(!planet || !selectedOutfit)
		return false;
	
	if(player.Cargo().GetOutfitCount(selectedOutfit))
		return true;
	
	for(const Ship *ship : playerShips)
		if(ShipCanSell(ship, selectedOutfit))
			return true;
	
	return false;
}



void OutfitterPanel::Sell()
{
	if(player.Cargo().GetOutfitCount(selectedOutfit))
	{
		// Sell the newest (most valuable) first.
		int64_t cost = player.Cargo().Outfits().GetCost(selectedOutfit, 1, false);
		bool makeAvailable = (!outfitter.Has(selectedOutfit) || cost < selectedOutfit->Cost());
		player.Accounts().AddCredits(cost);
		player.Cargo().Transfer(selectedOutfit, 1, makeAvailable ? &available : nullptr, false, 0);
	}
	else
	{
		int most = 0;
		vector<Ship *> shipsToOutfit;
		for(Ship *ship : playerShips)
		{
			if(!ShipCanSell(ship, selectedOutfit))
				continue;
			
			int count = ship->OutfitCount(selectedOutfit);
			if(count > most)
			{
				shipsToOutfit.clear();
				most = count;
			}
			
			if(count == most)
				shipsToOutfit.push_back(ship);
		}
		
		for(Ship *ship : shipsToOutfit)
		{
			// sell newest first; transfer from ship to available.
			int64_t cost = ship->Outfits().GetCost(selectedOutfit, 1, false);
			bool makeAvailable = (!outfitter.Has(selectedOutfit) || cost < selectedOutfit->Cost());
			ship->TransferOutfit(selectedOutfit, 1, makeAvailable ? &available : nullptr, false, 0);
			player.Accounts().AddCredits(cost);
			
			if(selectedOutfit->Get("required crew"))
				ship->AddCrew(-selectedOutfit->Get("required crew"));
			ship->Recharge(true);			
			
			const Outfit *ammo = selectedOutfit->Ammo();
			if(ammo && ship->OutfitCount(ammo))
			{
				// Determine how many of this ammo I must sell to also sell the launcher.
				int mustSell = 0;
				for(const auto &it : ship->Attributes().Attributes())
					if(it.second < 0.)
						mustSell = max(mustSell, static_cast<int>(it.second / ammo->Get(it.first)));
				
				if(mustSell)
				{
					int64_t cost = ship->Outfits().GetCost(ammo, mustSell, false);
					bool makeAvailable = (!outfitter.Has(selectedOutfit) || cost < selectedOutfit->Cost());
					ship->TransferOutfit(ammo, mustSell, makeAvailable ? &available : nullptr, false, 0);
					player.Accounts().AddCredits(cost);
				}
			}
		}
	}
}



void OutfitterPanel::FailSell() const
{
	if(!planet || !selectedOutfit)
		return;
	else if(selectedOutfit->Get("map"))
		GetUI()->Push(new Dialog("You cannot sell maps. Once you buy one, it is yours permanently."));
	else if(HasLicense(selectedOutfit->Name()))
		GetUI()->Push(new Dialog("You cannot sell licenses. Once you buy one, it is yours permanently."));
	else
	{
		bool hasOutfit = false;
		for(const Ship *ship : playerShips)
			if(ship->OutfitCount(selectedOutfit))
			{
				hasOutfit = true;
				break;
			}
		if(!hasOutfit)
			GetUI()->Push(new Dialog("You do not have any of these outfits to sell."));
		else
		{
			for(const Ship *ship : playerShips)
				for(const auto &it : selectedOutfit->Attributes())
				{
					// Don't worry about negative cost; it won't happen.
					if (it.first == "cost")  
						continue; 
					if(ship->Attributes().Get(it.first) < it.second)
					{
						for(const auto &sit : ship->Outfits())
							if(sit.GetOutfit()->Get(it.first) < 0.)
							{
								GetUI()->Push(new Dialog("You cannot sell this outfit, "
									"because that would cause your ship's \"" + it.first +
									"\" value to be reduced to less than zero. "
									"To sell this outfit, you must sell the " +
									sit.GetOutfit()->Name() + " outfit first."));
								return;
							}
						GetUI()->Push(new Dialog("You cannot sell this outfit, "
							"because that would cause your ship's \"" + it.first +
							"\" value to be reduced to less than zero."));
						return;
					}
				}
			GetUI()->Push(new Dialog("You cannot sell this outfit, "
				"because something else in your ship depends on it."));
		}
	}
}



bool OutfitterPanel::FlightCheck()
{
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(ship->GetSystem() != player.GetSystem())
			continue;
		
		const Outfit &attributes = ship->Attributes();
		double energy = attributes.Get("energy generation") + attributes.Get("energy capacity");
		if(!attributes.Get("thrust"))
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no thrusters")));
			return false;
		}
		if(attributes.Get("thrusting energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no thruster energy")));
			return false;
		}
		if(!attributes.Get("turn"))
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no steering")));
			return false;
		}
		if(attributes.Get("turning energy") > energy)
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: no steering energy")));
			return false;
		}
		double maxHeat = .1 * ship->Mass() * attributes.Get("heat dissipation");
		if(attributes.Get("heat generation") - attributes.Get("cooling") > maxHeat)
		{
			GetUI()->Push(new ConversationPanel(player,
				*GameData::Conversations().Get("flight check: overheating")));
			return false;
		}
	}
	return true;
}



bool OutfitterPanel::ShipCanBuy(const Ship *ship, const Outfit *outfit)
{
	return ship->Attributes().CanAdd(*outfit, 1);
}



bool OutfitterPanel::ShipCanSell(const Ship *ship, const Outfit *outfit)
{
	if(!ship->OutfitCount(outfit))
		return false;
	
	// If this outfit requires ammo, check if we could sell it if we sold all
	// the ammo for it first.
	const Outfit *ammo = outfit->Ammo();
	if(ammo && ship->OutfitCount(ammo))
	{
		Outfit attributes = ship->Attributes();
		attributes.Add(*ammo, -ship->OutfitCount(ammo));
		return attributes.CanAdd(*outfit, -1);
	}
	
	// Now, check whether this ship can sell this outfit.
	return ship->Attributes().CanAdd(*outfit, -1);
}


	
void OutfitterPanel::DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned)
{
	const Sprite *thumbnail = outfit.Thumbnail();
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/outfitter selected" : "ui/outfitter unselected");
	SpriteShader::Draw(back, center);
	SpriteShader::Draw(thumbnail, center);
	
	// Draw the outfit name.
	const string &name = outfit.Name();
	const Font &font = FontSet::Get(14);
	Point offset(-.5f * font.Width(name), -.5f * OUTFIT_SIZE + 10.f);
	font.Draw(name, center + offset, Color((isSelected | isOwned) ? .8 : .5, 0.));
}



bool OutfitterPanel::HasMapped(int mapSize) const
{
	DistanceMap distance(player.GetSystem(), mapSize);
	for(const auto &it : distance.Distances())
		if(!player.HasVisited(it.first))
			return false;
	
	return true;
}



bool OutfitterPanel::IsLicense(const string &name) const
{
	static const string &LICENSE = " License";
	if(name.length() < LICENSE.length())
		return false;
	if(name.compare(name.length() - LICENSE.length(), LICENSE.length(), LICENSE))
		return false;
	
	return true;
}



bool OutfitterPanel::HasLicense(const string &name) const
{
	return (IsLicense(name) && player.GetCondition(LicenseName(name)) > 0);
}



string OutfitterPanel::LicenseName(const string &name) const
{
	static const string &LICENSE = " License";
	return "license: " + name.substr(0, name.length() - LICENSE.length());
}



void OutfitterPanel::CheckRefill()
{
	if(checkedRefill)
		return;
	checkedRefill = true;
	
	int64_t cost = 0;
	int count = 0;
	for(const auto &ship : player.Ships())
	{
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;
		
		++count;
		set<const Outfit *> toRefill;
		for(const auto &it : ship->Weapons())
			if(it.GetOutfit() && it.GetOutfit()->Ammo())
				toRefill.insert(it.GetOutfit()->Ammo());
		
		for(const Outfit *outfit : toRefill)
		{
			int needed = ship->Attributes().CanAdd(*outfit, 1000000);
			if(!outfitter.Has(outfit))
				needed = min(needed, player.Cargo().GetOutfitCount(outfit) + available.GetTotalCount(selectedOutfit));
			cost += needed * outfit->Cost();
		}
	}
	
	if(cost && cost < player.Accounts().Credits())
	{
		string message = (count == 1) ?
			"Do you want to reload all the ammunition for your ship? It will cost " :
			"Do you want to reload all the ammunition for your ships? It will cost ";
		message += Format::Number(cost) + " credits.";
		GetUI()->Push(new Dialog(this, &OutfitterPanel::Refill, message));
	}
}



void OutfitterPanel::Refill()
{
	for(const auto &ship : player.Ships())
	{
		if(ship->GetSystem() != player.GetSystem())
			continue;
		
		set<const Outfit *> toRefill;
		for(const auto &it : ship->Weapons())
			if(it.GetOutfit() && it.GetOutfit()->Ammo())
				toRefill.insert(it.GetOutfit()->Ammo());
		
		// This is slower than just calculating the proper number to add, but
		// that does not matter because this is not so time-consuming anyways.
		for(const Outfit *outfit : toRefill)
			while(ship->Attributes().CanAdd(*outfit))
			{
				if(player.Cargo().GetOutfitCount(outfit))
					player.Cargo().Transfer(outfit, 1);
				else if(!available.GetTotalCount(outfit) && !outfitter.Has(outfit))
					break;
				else if (available.GetTotalCount(outfit))
				{	// Buy lowest price used outfits first.
					player.Accounts().AddCredits(-available.GetCost(outfit, 1, true));
					ship->TransferOutfit(outfit, -1, &available, true, 0);
				}
				else
				{	// Buy new
					player.Accounts().AddCredits(-outfit->Cost());
					ship->AddOutfit(outfit, 1, 0);				
				}
			}
	}
}



// If the player tried to buy an outfit that cannot be installed ask whether he'd 
// like to buy it and put it in his flagship's cargo hold. 
void OutfitterPanel::BuyAsCargoPrompt(std::string message)
{
	 // Don't allow anything with zero/negative mass in cargo.
	int mass = selectedOutfit->Get("mass");
	if (mass > 0 && player.Cargo().Free() >= mass 
		&& (outfitter.Has(selectedOutfit) || available.GetTotalCount(selectedOutfit)))
	{
		GetUI()->Push(new Dialog(this, &OutfitterPanel::BuyAsCargoCallback, message +
			"\n\nHowever, you do have some room in your cargo hold! Would you like to buy this outfit and place it in cargo?"));
	}
	else 
	{
		if (mass <= 0)
			GetUI()->Push(new Dialog(message + "\n\nSince this item has no mass, it also can't be kept in your cargo hold."));
		else if (player.Cargo().Free() < mass)
			GetUI()->Push(new Dialog(message + "\n\nYou also don't have enough free cargo space to buy this outfit as cargo." ));
		else if (!(outfitter.Has(selectedOutfit) || available.GetTotalCount(selectedOutfit)))
			GetUI()->Push(new Dialog(message + "\n\nYou also can't buy this outfit as cargo because there are none available for sale here." ));
	}
}



void OutfitterPanel::BuyAsCargoCallback()
{
	// Buy the selected outfit and put it in cargo.
	int modifier = Modifier();
	for(int i = 0; i < modifier && player.Cargo().Free() >= selectedOutfit->Get("mass"); ++i)
	{
		if(!available.Find(selectedOutfit) && !outfitter.Has(selectedOutfit))
			break;
		else if (available.Find(selectedOutfit))
		{	// Buy lowest price used outfits first.
			player.Accounts().AddCredits(-available.GetCost(selectedOutfit, 1, true));
			player.Cargo().Transfer(selectedOutfit, -1, &available, true, 0);
		}
		else
		{	// Buy new
			player.Accounts().AddCredits(-selectedOutfit->Cost());
			player.Cargo().Transfer(selectedOutfit, -1, nullptr, true, 0);				
		}

	}
}
