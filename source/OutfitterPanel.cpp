/* OutfitterPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "OutfitterPanel.h"

#include "text/alignment.hpp"
#include "comparators/BySeriesAndIndex.h"
#include "Color.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Hardpoint.h"
#include "Mission.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>
#include <limits>
#include <memory>

using namespace std;

namespace {
	// Label for the description field of the detail pane.
	const string DESCRIPTION = "description";

	// Determine the refillable ammunition a particular ship consumes or stores.
	set<const Outfit *> GetRefillableAmmunition(const Ship &ship) noexcept
	{
		auto toRefill = set<const Outfit *>{};
		auto armed = set<const Outfit *>{};
		for(auto &&it : ship.Weapons())
			if(it.GetOutfit())
			{
				const Outfit *weapon = it.GetOutfit();
				armed.emplace(weapon);
				if(weapon->Ammo() && weapon->AmmoUsage() > 0)
					toRefill.emplace(weapon->Ammo());
			}

		// Carriers may be configured to supply ammunition for carried ships found
		// within the fleet. Since a particular ammunition outfit is not bound to
		// any particular weapon (i.e. one weapon may consume it, while another may
		// only require it be installed), we always want to restock these outfits.
		for(auto &&it : ship.Outfits())
		{
			const Outfit *outfit = it.first;
			if(outfit->Ammo() && !outfit->IsWeapon() && !armed.count(outfit))
				toRefill.emplace(outfit->Ammo());
		}
		return toRefill;
	}
}



OutfitterPanel::OutfitterPanel(PlayerInfo &player)
	: ShopPanel(player, true)
{
	for(const pair<const string, Outfit> &it : GameData::Outfits())
		catalog[it.second.Category()].push_back(it.first);

	for(pair<const string, vector<string>> &it : catalog)
		sort(it.second.begin(), it.second.end(), BySeriesAndIndex<Outfit>());

	if(player.GetPlanet())
		outfitter = player.GetPlanet()->Outfitter();

	for(auto &ship : player.Ships())
		if(ship->GetPlanet() == planet)
			++shipsHere;
}



void OutfitterPanel::Step()
{
	CheckRefill();
	ShopPanel::Step();
	ShopPanel::CheckForMissions(Mission::OUTFITTER);
	if(GetUI()->IsTop(this) && !checkedHelp)
		// Use short-circuiting to only display one of them at a time.
		// (The first valid condition encountered will make us skip the others.)
		if(DoHelp("outfitter") || DoHelp("cargo management") || DoHelp("uninstalling and storage")
				|| (shipsHere > 1 && DoHelp("outfitter with multiple ships")) || true)
			// Either a help message was freshly displayed, or all of them have already been seen.
			checkedHelp = true;
}



int OutfitterPanel::TileSize() const
{
	return OUTFIT_SIZE;
}



int OutfitterPanel::VisibilityCheckboxesSize() const
{
	return 30;
}



bool OutfitterPanel::HasItem(const string &name) const
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	if(showForSale && (outfitter.Has(outfit) || player.Stock(outfit) > 0))
		return true;

	if(showCargo && player.Cargo().Get(outfit))
		return true;

	if(showStorage && player.Storage().Get(outfit))
		return true;

	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(outfit))
			return true;

	if(showForSale && HasLicense(name))
		return true;

	return false;
}



void OutfitterPanel::DrawItem(const string &name, const Point &point)
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	zones.emplace_back(point, Point(OUTFIT_SIZE, OUTFIT_SIZE), outfit);
	if(point.Y() + OUTFIT_SIZE / 2 < Screen::Top() || point.Y() - OUTFIT_SIZE / 2 > Screen::Bottom())
		return;

	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);

	// Check if this outfit is a "license".
	bool isLicense = IsLicense(name);
	int mapSize = outfit->Get("map");

	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	if(playerShip || isLicense || mapSize)
	{
		int minCount = numeric_limits<int>::max();
		int maxCount = 0;
		if(isLicense)
			minCount = maxCount = player.HasLicense(LicenseRoot(name));
		else if(mapSize)
			minCount = maxCount = player.HasMapped(mapSize);
		else
		{
			for(const Ship *ship : playerShips)
			{
				int count = ship->OutfitCount(outfit);
				minCount = min(minCount, count);
				maxCount = max(maxCount, count);
			}
		}

		if(maxCount)
		{
			string label = "installed: " + to_string(minCount);
			if(maxCount > minCount)
				label += " - " + to_string(maxCount);

			Point labelPos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 38);
			font.Draw(label, labelPos, bright);
		}
	}
	// Don't show the "in stock" amount if the outfit has an unlimited stock or
	// if it is not something that you can buy.
	int stock = 0;
	if(!outfitter.Has(outfit) && (outfit->Get("installable") >= 0. || outfit->Get("minable") >= 0.))
		stock = max(0, player.Stock(outfit));
	int cargo = player.Cargo().Get(outfit);
	int storage = player.Storage().Get(outfit);

	string message;
	if(cargo && storage && stock)
		message = "cargo+stored: " + to_string(cargo + storage) + ", in stock: " + to_string(stock);
	else if(cargo && storage)
		message = "in cargo: " + to_string(cargo) + ", in storage: " + to_string(storage);
	else if(cargo && stock)
		message = "in cargo: " + to_string(cargo) + ", in stock: " + to_string(stock);
	else if(storage && stock)
		message = "in storage: " + to_string(storage) + ", in stock: " + to_string(stock);
	else if(cargo)
		message = "in cargo: " + to_string(cargo);
	else if(storage)
		message = "in storage: " + to_string(storage);
	else if(stock)
		message = "in stock: " + to_string(stock);
	else if(!outfitter.Has(outfit))
		message = "(not sold here)";
	if(!message.empty())
	{
		Point pos = point + Point(
			OUTFIT_SIZE / 2 - 20 - font.Width(message),
			OUTFIT_SIZE / 2 - 24);
		font.Draw(message, pos, bright);
	}
}



int OutfitterPanel::DividerOffset() const
{
	return 80;
}



int OutfitterPanel::DetailWidth() const
{
	return 3 * ItemInfoDisplay::PanelWidth();
}



double OutfitterPanel::DrawDetails(const Point &center)
{
	string selectedItem = "Nothing Selected";
	const Font &font = FontSet::Get(14);

	double heightOffset = 20.;

	if(selectedOutfit)
	{
		outfitInfo.Update(*selectedOutfit, player, CanSell(), collapsed.count(DESCRIPTION));
		selectedItem = selectedOutfit->DisplayName();

		const Sprite *thumbnail = selectedOutfit->Thumbnail();
		const float tileSize = thumbnail
			? max(thumbnail->Height(), static_cast<float>(TileSize()))
			: static_cast<float>(TileSize());
		const Point thumbnailCenter(center.X(), center.Y() + 20 + static_cast<int>(tileSize / 2));
		const Point startPoint(center.X() - INFOBAR_WIDTH / 2 + 20, center.Y() + 20 + tileSize);

		const Sprite *background = SpriteSet::Get("ui/outfitter selected");
		SpriteShader::Draw(background, thumbnailCenter);
		if(thumbnail)
			SpriteShader::Draw(thumbnail, thumbnailCenter);

		const bool hasDescription = outfitInfo.DescriptionHeight();

		double descriptionOffset = hasDescription ? 40. : 0.;

		if(hasDescription)
		{
			// Maintenance note: This can be replaced with collapsed.contains() in C++20
			if(!collapsed.count(DESCRIPTION))
			{
				descriptionOffset = outfitInfo.DescriptionHeight();
				outfitInfo.DrawDescription(startPoint);
			}
			else
			{
				const Color &dim = *GameData::Colors().Get("medium");
				font.Draw(DESCRIPTION, startPoint + Point(35., 12.), dim);
				const Sprite *collapsedArrow = SpriteSet::Get("ui/collapsed");
				SpriteShader::Draw(collapsedArrow, startPoint + Point(20., 20.));
			}

			// Calculate the ClickZone for the description and add it.
			const Point descriptionDimensions(INFOBAR_WIDTH, descriptionOffset);
			const Point descriptionCenter(center.X(), startPoint.Y() + descriptionOffset / 2);
			ClickZone<string> collapseDescription = ClickZone<string>(
				descriptionCenter, descriptionDimensions, DESCRIPTION);
			categoryZones.emplace_back(collapseDescription);
		}

		const Point requirementsPoint(startPoint.X(), startPoint.Y() + descriptionOffset);
		const Point attributesPoint(startPoint.X(), requirementsPoint.Y() + outfitInfo.RequirementsHeight());
		outfitInfo.DrawRequirements(requirementsPoint);
		outfitInfo.DrawAttributes(attributesPoint);

		heightOffset = attributesPoint.Y() + outfitInfo.AttributesHeight();
	}

	// Draw this string representing the selected item (if any), centered in the details side panel
	const Color &bright = *GameData::Colors().Get("bright");
	Point selectedPoint(center.X() - INFOBAR_WIDTH / 2, center.Y());
	font.Draw({selectedItem, {INFOBAR_WIDTH, Alignment::CENTER, Truncate::MIDDLE}},
		selectedPoint, bright);

	return heightOffset;
}



ShopPanel::BuyResult OutfitterPanel::CanBuy(bool onlyOwned) const
{
	if(!planet || !selectedOutfit)
		return false;

	// Check special unique outfits, if you already have them.
	int mapSize = selectedOutfit->Get("map");
	if(mapSize > 0 && player.HasMapped(mapSize))
		return "You have already mapped all the systems shown by this map, "
			"so there is no reason to buy another.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You already have one of these licenses, "
			"so there is no reason to buy another.";

	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost(selectedOutfit, onlyOwned);
	if(licenseCost < 0)
		return "You cannot buy this outfit, because it requires a license that you don't have.";

	// Check if the outfit is available to get at all.
	bool isInCargo = player.Cargo().Get(selectedOutfit);
	bool isInStorage = player.Storage().Get(selectedOutfit);
	bool isInStore = outfitter.Has(selectedOutfit) || player.Stock(selectedOutfit) > 0;
	if(isInStorage && (onlyOwned || isInStore || playerShip))
	{
		// In storage, the outfit is certainly available to get,
		// except for this one case: 'b' does not move storage to cargo.
	}
	else if(isInCargo && playerShip)
	{
		// Installing to a ship will work from cargo.
	}
	else if(onlyOwned)
	{
		// Not using the store, there's nowhere to get this outfit.
		if(isInStore)
		{
			// Player hit 'i' or 'c' when they should've hit 'b'.
			if(playerShip)
				return "You'll need to buy this outfit to install it (using \"b\").";
			else
				return "You'll need to buy this outfit to put it in your cargo hold (using \"b\").";
		}
		else
		{
			// Player hit 'i' or 'c' to install, with no outfit to use.
			if(playerShip)
				return "You do not have any of these outfits available to install.";
			else
				return "You do not have any of these outfits in storage to move to your cargo hold.";
		}
	}
	else if(!isInStore)
	{
		// The store doesn't have it.
		return "You cannot buy this outfit here. "
			"It is being shown in the list because you have one, "
			"but this " + planet->Noun() + " does not sell them.";
	}

	// Check if you need to pay, and can't afford it.
	if(!onlyOwned)
	{
		// Determine what you will have to pay to buy this outfit.
		int64_t cost = player.StockDepreciation().Value(selectedOutfit, day);
		int64_t credits = player.Accounts().Credits();

		if(cost > credits)
			return "You cannot buy this outfit, because it costs "
				+ Format::CreditString(cost) + ", and you only have "
				+ Format::Credits(credits) + ".";

		// Add the cost to buy the required license.
		if(cost + licenseCost > credits)
			return "You don't have enough money to buy this outfit, because it will cost you an extra "
				+ Format::CreditString(licenseCost) + " to buy the necessary licenses.";
	}

	// Check if the outfit will fit
	if(!playerShip)
	{
		// Buying into cargo, so check cargo space vs mass.
		double mass = selectedOutfit->Mass();
		double freeCargo = player.Cargo().FreePrecise();
		if(!mass || freeCargo >= mass)
			return true;

		return "You cannot " + string(onlyOwned ? "load" : "buy") + " this outfit, because it takes up "
			+ Format::CargoString(mass, "mass") + " and your fleet has "
			+ Format::CargoString(freeCargo, "cargo space") + " free.";
	}
	else
	{
		// Find if any ship can install the outfit.
		for(const Ship *ship : playerShips)
			if(ShipCanBuy(ship, selectedOutfit))
				return true;

		// If no selected ship can install the outfit,
		// report error based on playerShip.
		double outfitNeeded = -selectedOutfit->Get("outfit space");
		double outfitSpace = playerShip->Attributes().Get("outfit space");
		if(outfitNeeded > outfitSpace)
			return "You cannot install this outfit, because it takes up "
				+ Format::CargoString(outfitNeeded, "outfit space") + ", and this ship has "
				+ Format::MassString(outfitSpace) + " free.";

		double weaponNeeded = -selectedOutfit->Get("weapon capacity");
		double weaponSpace = playerShip->Attributes().Get("weapon capacity");
		if(weaponNeeded > weaponSpace)
			return "Only part of your ship's outfit capacity is usable for weapons. "
				"You cannot install this outfit, because it takes up "
				+ Format::CargoString(weaponNeeded, "weapon space") + ", and this ship has "
				+ Format::MassString(weaponSpace) + " free.";

		double engineNeeded = -selectedOutfit->Get("engine capacity");
		double engineSpace = playerShip->Attributes().Get("engine capacity");
		if(engineNeeded > engineSpace)
			return "Only part of your ship's outfit capacity is usable for engines. "
				"You cannot install this outfit, because it takes up "
				+ Format::CargoString(engineNeeded, "engine space") + ", and this ship has "
				+ Format::MassString(engineSpace) + " free.";

		if(selectedOutfit->Category() == "Ammunition")
			return !playerShip->OutfitCount(selectedOutfit) ?
				"This outfit is ammunition for a weapon. "
				"You cannot install it without first installing the appropriate weapon."
				: "You already have the maximum amount of ammunition for this weapon. "
				"If you want to install more ammunition, you must first install another of these weapons.";

		int mountsNeeded = -selectedOutfit->Get("turret mounts");
		int mountsFree = playerShip->Attributes().Get("turret mounts");
		if(mountsNeeded && !mountsFree)
			return "This weapon is designed to be installed on a turret mount, "
				"but your ship does not have any unused turret mounts available.";

		int gunsNeeded = -selectedOutfit->Get("gun ports");
		int gunsFree = playerShip->Attributes().Get("gun ports");
		if(gunsNeeded && !gunsFree)
			return "This weapon is designed to be installed in a gun port, "
				"but your ship does not have any unused gun ports available.";

		if(selectedOutfit->Get("installable") < 0.)
			return "This item is not an outfit that can be installed in a ship.";

		// For unhandled outfit requirements, show a catch-all error message.
		return "You cannot install this outfit in your ship, "
			"because it would reduce one of your ship's attributes to a negative amount. "
			"For example, it may use up more cargo space than you have left.";
	}
}



void OutfitterPanel::Buy(bool onlyOwned)
{
	int64_t licenseCost = LicenseCost(selectedOutfit, onlyOwned);
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedOutfit->Licenses())
			if(!player.HasLicense(licenseName))
				player.AddLicense(licenseName);
	}

	// Special case: maps.
	int mapSize = selectedOutfit->Get("map");
	if(mapSize)
	{
		player.Map(mapSize);
		player.Accounts().AddCredits(-selectedOutfit->Cost());
		return;
	}

	// Special case: licenses.
	if(IsLicense(selectedOutfit->TrueName()))
	{
		player.AddLicense(LicenseRoot(selectedOutfit->TrueName()));
		player.Accounts().AddCredits(-selectedOutfit->Cost());
		return;
	}

	int modifier = Modifier();
	for(int i = 0; i < modifier && CanBuy(onlyOwned); ++i)
	{
		// Buying into cargo, either from storage or from stock/supply.
		if(!playerShip)
		{
			if(onlyOwned)
			{
				if(!player.Storage().Get(selectedOutfit))
					continue;
				player.Cargo().Add(selectedOutfit);
				player.Storage().Remove(selectedOutfit);
			}
			else
			{
				// Check if the outfit is for sale or in stock so that we can actually buy it.
				if(!outfitter.Has(selectedOutfit) && player.Stock(selectedOutfit) <= 0)
					continue;
				player.Cargo().Add(selectedOutfit);
				int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(-price);
				player.AddStock(selectedOutfit, -1);
				continue;
			}
		}

		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);

		for(Ship *ship : shipsToOutfit)
		{
			if(!CanBuy(onlyOwned))
				return;

			if(player.Cargo().Get(selectedOutfit))
				player.Cargo().Remove(selectedOutfit);
			else if(player.Storage().Get(selectedOutfit))
				player.Storage().Remove(selectedOutfit);
			else if(onlyOwned || !(player.Stock(selectedOutfit) > 0 || outfitter.Has(selectedOutfit)))
				break;
			else
			{
				int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(-price);
				player.AddStock(selectedOutfit, -1);
			}
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



bool OutfitterPanel::CanSell(bool toStorage) const
{
	if(!planet || !selectedOutfit)
		return false;

	if(player.Cargo().Get(selectedOutfit))
		return true;

	if(!toStorage && player.Storage().Get(selectedOutfit))
		return true;

	for(const Ship *ship : playerShips)
		if(ShipCanSell(ship, selectedOutfit))
			return true;

	return false;
}



void OutfitterPanel::Sell(bool toStorage)
{
	CargoHold &storage = player.Storage();

	if(player.Cargo().Get(selectedOutfit))
	{
		player.Cargo().Remove(selectedOutfit);
		if(toStorage && storage.Add(selectedOutfit))
		{
			// Transfer to planetary storage completed.
			// The storage->Add() function should never fail as long as
			// planetary storage has unlimited size.
		}
		else
		{
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
		return;
	}

	// Get the ships that have the most of this outfit installed.
	// If there are no ships that have this outfit, then sell from storage.
	const vector<Ship *> shipsToOutfit = GetShipsToOutfit();

	if(!shipsToOutfit.empty())
	{
		for(Ship *ship : shipsToOutfit)
		{
			ship->AddOutfit(selectedOutfit, -1);
			if(selectedOutfit->Get("required crew"))
				ship->AddCrew(-selectedOutfit->Get("required crew"));
			ship->Recharge();

			if(toStorage && storage.Add(selectedOutfit))
			{
				// Transfer to planetary storage completed.
			}
			else if(toStorage)
			{
				// No storage available; transfer to cargo even if it
				// would exceed the cargo capacity.
				int size = player.Cargo().Size();
				player.Cargo().SetSize(-1);
				player.Cargo().Add(selectedOutfit);
				player.Cargo().SetSize(size);
			}
			else
			{
				int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(price);
				player.AddStock(selectedOutfit, 1);
			}

			const Outfit *ammo = selectedOutfit->Ammo();
			if(ammo && ship->OutfitCount(ammo))
			{
				// Determine how many of this ammo I must sell to also sell the launcher.
				int mustSell = 0;
				for(const pair<const char *, double> &it : ship->Attributes().Attributes())
					if(it.second < 0.)
						mustSell = max<int>(mustSell, it.second / ammo->Get(it.first));

				if(mustSell)
				{
					ship->AddOutfit(ammo, -mustSell);
					if(toStorage)
						mustSell -= storage.Add(ammo, mustSell);
					if(mustSell)
					{
						int64_t price = player.FleetDepreciation().Value(ammo, day, mustSell);
						player.Accounts().AddCredits(price);
						player.AddStock(ammo, mustSell);
					}
				}
			}
		}
		return;
	}

	if(!toStorage && storage.Get(selectedOutfit))
	{
		storage.Remove(selectedOutfit);
		int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
		player.Accounts().AddCredits(price);
		player.AddStock(selectedOutfit, 1);
	}
}



void OutfitterPanel::FailSell(bool toStorage) const
{
	const string &verb = toStorage ? "uninstall" : "sell";
	if(!planet || !selectedOutfit)
		return;
	else if(selectedOutfit->Get("map"))
		GetUI()->Push(new Dialog("You cannot " + verb + " maps. Once you buy one, it is yours permanently."));
	else if(HasLicense(selectedOutfit->TrueName()))
		GetUI()->Push(new Dialog("You cannot " + verb + " licenses. Once you obtain one, it is yours permanently."));
	else
	{
		bool hasOutfit = player.Cargo().Get(selectedOutfit);
		hasOutfit = hasOutfit || (!toStorage && player.Storage().Get(selectedOutfit));
		for(const Ship *ship : playerShips)
			if(ship->OutfitCount(selectedOutfit))
			{
				hasOutfit = true;
				break;
			}
		if(!hasOutfit)
			GetUI()->Push(new Dialog("You do not have any of these outfits to " + verb + "."));
		else
		{
			for(const Ship *ship : playerShips)
				for(const pair<const char *, double> &it : selectedOutfit->Attributes())
					if(ship->Attributes().Get(it.first) < it.second)
					{
						for(const auto &sit : ship->Outfits())
							if(sit.first->Get(it.first) < 0.)
							{
								GetUI()->Push(new Dialog("You cannot " + verb + " this outfit, "
									"because that would cause your ship's \"" + it.first +
									"\" value to be reduced to less than zero. "
									"To " + verb + " this outfit, you must " + verb + " the " +
									sit.first->DisplayName() + " outfit first."));
								return;
							}
						GetUI()->Push(new Dialog("You cannot " + verb + " this outfit, "
							"because that would cause your ship's \"" + it.first +
							"\" value to be reduced to less than zero."));
						return;
					}
			GetUI()->Push(new Dialog("You cannot " + verb + " this outfit, "
				"because something else in your ship depends on it."));
		}
	}
}



bool OutfitterPanel::ShouldHighlight(const Ship *ship)
{
	if(!selectedOutfit)
		return false;

	if(hoverButton == 'b')
		return CanBuy() && ShipCanBuy(ship, selectedOutfit);
	else if(hoverButton == 's')
		return CanSell() && ShipCanSell(ship, selectedOutfit);

	return false;
}



void OutfitterPanel::DrawKey()
{
	const Sprite *back = SpriteSet::Get("ui/outfitter key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));

	const Font &font = FontSet::Get(14);
	Color color[2] = {*GameData::Colors().Get("medium"), *GameData::Colors().Get("bright")};
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};

	Point pos = Screen::BottomLeft() + Point(10., -VisibilityCheckboxesSize() - 20.);
	Point off = Point(10., -.5 * font.Height());
	SpriteShader::Draw(box[showForSale], pos);
	font.Draw("Show outfits for sale", pos + off, color[showForSale]);
	AddZone(Rectangle(pos + Point(80., 0.), Point(180., 20.)), [this](){ ToggleForSale(); });

	pos.Y() += 20.;
	SpriteShader::Draw(box[showCargo], pos);
	font.Draw("Show outfits in cargo", pos + off, color[showCargo]);
	AddZone(Rectangle(pos + Point(80., 0.), Point(180., 20.)), [this](){ ToggleCargo(); });

	pos.Y() += 20.;
	SpriteShader::Draw(box[showStorage], pos);
	font.Draw("Show outfits in storage", pos + off, color[showStorage]);
	AddZone(Rectangle(pos + Point(80., 0.), Point(180., 20.)), [this](){ ToggleStorage(); });
}



void OutfitterPanel::ToggleForSale()
{
	showForSale = !showForSale;
	ShopPanel::ToggleForSale();
}



void OutfitterPanel::ToggleStorage()
{
	showStorage = !showStorage;
	ShopPanel::ToggleStorage();
}



void OutfitterPanel::ToggleCargo()
{
	showCargo = !showCargo;

	if(playerShip)
	{
		playerShip = nullptr;
		playerShips.clear();
	}
	else
	{
		playerShip = player.Flagship();
		if(playerShip)
			playerShips.insert(playerShip);
	}

	ShopPanel::ToggleCargo();
}



bool OutfitterPanel::ShipCanBuy(const Ship *ship, const Outfit *outfit)
{
	return (ship->Attributes().CanAdd(*outfit, 1) > 0);
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
	const string &name = outfit.DisplayName();
	const Font &font = FontSet::Get(14);
	Point offset(-.5 * OUTFIT_SIZE, -.5 * OUTFIT_SIZE + 10.);
	font.Draw({name, {OUTFIT_SIZE, Alignment::CENTER, Truncate::MIDDLE}},
		center + offset, Color((isSelected | isOwned) ? .8 : .5, 0.));
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
	return (IsLicense(name) && player.HasLicense(LicenseRoot(name)));
}



string OutfitterPanel::LicenseRoot(const string &name) const
{
	static const string &LICENSE = " License";
	return name.substr(0, name.length() - LICENSE.length());
}



void OutfitterPanel::CheckRefill()
{
	if(checkedRefill)
		return;
	checkedRefill = true;

	int count = 0;
	map<const Outfit *, int> needed;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip ships in other systems and those that were unable to land in-system.
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;

		++count;
		auto toRefill = GetRefillableAmmunition(*ship);
		for(const Outfit *outfit : toRefill)
		{
			int amount = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(amount > 0)
			{
				bool available = outfitter.Has(outfit) || player.Stock(outfit) > 0;
				available = available || player.Cargo().Get(outfit) || player.Storage().Get(outfit);
				if(available)
					needed[outfit] += amount;
			}
		}
	}

	int64_t cost = 0;
	for(auto &it : needed)
	{
		// Don't count cost of anything installed from cargo or storage.
		it.second = max(0, it.second - player.Cargo().Get(it.first) - player.Storage().Get(it.first));
		if(!outfitter.Has(it.first))
			it.second = min(it.second, max(0, player.Stock(it.first)));
		cost += player.StockDepreciation().Value(it.first, day, it.second);
	}
	if(!needed.empty() && cost < player.Accounts().Credits())
	{
		string message = "Do you want to reload all the ammunition for your ship";
		message += (count == 1) ? "?" : "s?";
		if(cost)
			message += " It will cost " + Format::CreditString(cost) + ".";
		GetUI()->Push(new Dialog(this, &OutfitterPanel::Refill, message));
	}
}



void OutfitterPanel::Refill()
{
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip ships in other systems and those that were unable to land in-system.
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;

		auto toRefill = GetRefillableAmmunition(*ship);
		for(const Outfit *outfit : toRefill)
		{
			int neededAmmo = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(neededAmmo > 0)
			{
				// Fill first from any stockpiles in storage.
				const int fromStorage = player.Storage().Remove(outfit, neededAmmo);
				neededAmmo -= fromStorage;
				// Then from cargo.
				const int fromCargo = player.Cargo().Remove(outfit, neededAmmo);
				neededAmmo -= fromCargo;
				// Then, buy at reduced (or full) price.
				int available = outfitter.Has(outfit) ? neededAmmo : min<int>(neededAmmo, max<int>(0, player.Stock(outfit)));
				if(neededAmmo && available > 0)
				{
					int64_t price = player.StockDepreciation().Value(outfit, day, available);
					player.Accounts().AddCredits(-price);
					player.AddStock(outfit, -available);
				}
				ship->AddOutfit(outfit, available + fromStorage + fromCargo);
			}
		}
	}
}



// Determine which ships of the selected ships should be referenced in this
// iteration of Buy / Sell.
const vector<Ship *> OutfitterPanel::GetShipsToOutfit(bool isBuy) const
{
	vector<Ship *> shipsToOutfit;
	int compareValue = isBuy ? numeric_limits<int>::max() : 0;
	int compareMod = 2 * isBuy - 1;
	for(Ship *ship : playerShips)
	{
		if((isBuy && !ShipCanBuy(ship, selectedOutfit))
				|| (!isBuy && !ShipCanSell(ship, selectedOutfit)))
			continue;

		int count = ship->OutfitCount(selectedOutfit);
		if(compareMod * count < compareMod * compareValue)
		{
			shipsToOutfit.clear();
			compareValue = count;
		}
		if(count == compareValue)
			shipsToOutfit.push_back(ship);
	}

	return shipsToOutfit;
}



int OutfitterPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	auto it = zones.begin();
	for(unsigned int i = 0; i < zones.size(); ++i, ++it)
	{
		const Outfit *outfit = it->GetOutfit();
		int index = Format::Search(outfit->DisplayName(), text);
		if(index >= 0 && index < bestIndex)
		{
			bestIndex = index;
			bestItem = i;
			if(!index)
				return i;
		}
	}
	return bestItem;
}
