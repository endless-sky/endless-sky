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
#include "FillShader.h"
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
#include "image/Sprite.h"
#include "image/SpriteSet.h"
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

	constexpr int checkboxSpacing = 20;

	// Button size/placement info:
	constexpr double BUTTON_ROW_PAD = 5.;
	constexpr double BUTTON_COL_PAD = 5.;
	// These button widths need to add up to 220 with the current right panel
	// width and column padding:
	constexpr double BUTTON_1_WIDTH = 37.;
	constexpr double BUTTON_2_WIDTH = 73.;
	constexpr double BUTTON_3_WIDTH = 55.;
	constexpr double BUTTON_4_WIDTH = 55.;

	constexpr char SELL = 's';
	constexpr char UNINSTALL = 'u';

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
			if(outfit->Ammo() && !outfit->IsWeapon() && !armed.contains(outfit))
				toRefill.emplace(outfit->Ammo());
		}
		return toRefill;
	}



	bool IsLicense(const string &name)
	{
		static const string LICENSE = " License";
		if(name.length() < LICENSE.length())
			return false;
		if(name.compare(name.length() - LICENSE.length(), LICENSE.length(), LICENSE))
			return false;

		return true;
	}



	string LicenseRoot(const string &name)
	{
		static const string &LICENSE = " License";
		return name.substr(0, name.length() - LICENSE.length());
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
	// Each checkbox is 20px, 4 checkboxes in total is 80px.
	return 80;
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

	if(showInstalled)
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
	// Don't show the "in stock" amount if the outfit has an unlimited stock.
	int stock = 0;
	if(!outfitter.Has(outfit))
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



double OutfitterPanel::ButtonPanelHeight() const
{
	// The 60 is for padding the credit and cargo space information lines.
	return 60. + BUTTON_HEIGHT * 2 + BUTTON_ROW_PAD;
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
		outfitInfo.Update(*selectedOutfit, player, static_cast<bool>(CanSell()), collapsed.contains(DESCRIPTION));
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
			if(!collapsed.contains(DESCRIPTION))
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

	// Draw this string representing the selected item (if any), centered in the details side panel.
	const Color &bright = *GameData::Colors().Get("bright");
	Point selectedPoint(center.X() - INFOBAR_WIDTH / 2, center.Y());
	font.Draw({selectedItem, {INFOBAR_WIDTH, Alignment::CENTER, Truncate::MIDDLE}},
		selectedPoint, bright);

	return heightOffset;
}



bool OutfitterPanel::IsInShop() const
{
	return outfitter.Has(selectedOutfit) || player.Stock(selectedOutfit) > 0;
}



// Return true if the player is able to purchase the item from the outfitter or
// previously sold stock and can afford it and is properly licensed (availability,
// payment, licensing).
ShopPanel::TransactionResult OutfitterPanel::CanPurchase(bool checkSpecialItems = true) const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!IsInShop())
	{
		// The store doesn't have it.
		return "You cannot buy this outfit here. "
			"It is being shown in the list because you have one, "
			"but this " + planet->Noun() + " does not sell them.";
	}

	// Check special unique outfits, if you already have them.
	// Skip this if told to for sake of speed
	if(checkSpecialItems)
	{
		int mapSize = selectedOutfit->Get("map");
		if(mapSize > 0 && player.HasMapped(mapSize))
			return "You have already mapped all the systems shown by this map, "
				"so there is no reason to buy another.";

		if(HasLicense(selectedOutfit->TrueName()))
			return "You already have one of these licenses, "
				"so there is no reason to buy another.";
	}

	// Determine what you will have to pay to buy this outfit.
	int64_t cost = player.StockDepreciation().Value(selectedOutfit, day);
	int64_t credits = player.Accounts().Credits();
	if(cost > credits)
		return "You don't have enough money to buy this outfit. You need a further " +
			Format::CreditString(cost - credits);

	// Add the cost to buy the required license.
	int64_t licenseCost = LicenseCost(selectedOutfit, false);
	if(cost + licenseCost > credits)
		return "You don't have enough money to buy this outfit because you also need to buy a "
			"license for it. You need a further " +
			Format::CreditString(cost + licenseCost - credits);

	// Check that the player has any necessary licenses.
	if(licenseCost < 0)
		return "You cannot buy this outfit, because it requires a license that you don't have.";

	// The outfit is able to be purchased (available in the Outfitter, licensed and affordable).
	return true;
}



// Checking where it will go (can it fit in fleet cargo), not checking where it will
// be sourced from.
ShopPanel::TransactionResult OutfitterPanel::CanFitInCargo(bool returnReason) const
{
	// TODO: https://github.com/endless-sky/endless-sky/issues/10599

	// Check fleet cargo space vs mass.
	double mass = selectedOutfit->Mass();
	double freeCargo = player.Cargo().FreePrecise();
	if(!mass || freeCargo >= mass)
		return true;

	// Don't waste time making strings that won't get used. However, we do need
	// to make the string here while we have the values available.
	if(returnReason)
	{
		return "You cannot load this outfit into cargo, because it takes up "
			+ Format::CargoString(mass, "mass") + " and your fleet has "
			+ Format::CargoString(freeCargo, "cargo space") + " free.";
	}

	return false;
}



// Checking where it will go (whether it actually be installed into ANY selected ship),
// not checking where it will be sourced from.
ShopPanel::TransactionResult OutfitterPanel::CanBeInstalled() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!playerShip)
		return "No ship selected.";

	// Collect relevant errors.
	vector<string> errors;

	// Find if any ship can install the outfit.
	for(const Ship *ship : playerShips)
		if(ShipCanAdd(ship, selectedOutfit))
			return true;

	// If no selected ship can install the outfit, report error based on playerShip.
	double outfitNeeded = -selectedOutfit->Get("outfit space");
	double outfitSpace = playerShip->Attributes().Get("outfit space");
	if(outfitNeeded > outfitSpace)
		errors.push_back("You cannot install this outfit, because it takes up "
			+ Format::CargoString(outfitNeeded, "outfit space") + ", and this ship has "
			+ Format::MassString(outfitSpace) + " free.");

	double weaponNeeded = -selectedOutfit->Get("weapon capacity");
	double weaponSpace = playerShip->Attributes().Get("weapon capacity");
	if(weaponNeeded > weaponSpace)
		errors.push_back("Only part of your ship's outfit capacity is usable for weapons. "
			"You cannot install this outfit, because it takes up "
			+ Format::CargoString(weaponNeeded, "weapon space") + ", and this ship has "
			+ Format::MassString(weaponSpace) + " free.");

	double engineNeeded = -selectedOutfit->Get("engine capacity");
	double engineSpace = playerShip->Attributes().Get("engine capacity");
	if(engineNeeded > engineSpace)
		errors.push_back("Only part of your ship's outfit capacity is usable for engines. "
			"You cannot install this outfit, because it takes up "
			+ Format::CargoString(engineNeeded, "engine space") + ", and this ship has "
			+ Format::MassString(engineSpace) + " free.");

	if(selectedOutfit->Category() == "Ammunition")
		errors.emplace_back(!playerShip->OutfitCount(selectedOutfit) ?
			"This outfit is ammunition for a weapon. "
			"You cannot install it without first installing the appropriate weapon."
			: "You already have the maximum amount of ammunition for this weapon. "
			"If you want to install more ammunition, you must first install another of these weapons.");

	int mountsNeeded = -selectedOutfit->Get("turret mounts");
	int mountsFree = playerShip->Attributes().Get("turret mounts");
	if(mountsNeeded && !mountsFree)
		errors.emplace_back("This weapon is designed to be installed on a turret mount, "
			"but your ship does not have any unused turret mounts available.");

	int gunsNeeded = -selectedOutfit->Get("gun ports");
	int gunsFree = playerShip->Attributes().Get("gun ports");
	if(gunsNeeded && !gunsFree)
		errors.emplace_back("This weapon is designed to be installed in a gun port, "
			"but your ship does not have any unused gun ports available.");

	if(selectedOutfit->Get("installable") < 0.)
		errors.emplace_back("This item is not an outfit that can be installed in a ship.");

	// For unhandled outfit requirements, show a catch-all error message.
	if(errors.empty())
		errors.emplace_back("You cannot install this outfit in your ship, "
			"because it would reduce one of your ship's attributes to a negative amount.");

	// Return the errors in the appropriate format.
	if(errors.empty())
		return true;

	if(errors.size() == 1)
		return errors[0];

	string errorMessage = "There are several reasons why you cannot buy this outfit:\n";
	for(const auto & error : errors)
		errorMessage += "- " + error + "\n";

	return errorMessage;
}



// Used in both the Sell and the Uninstall contexts. Are we able to remove Outfits from
// the selected ships?
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanSellOrUninstall(const string &verb) const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!playerShip)
		return "No ship selected.";

	if(selectedOutfit->Get("map"))
		return "You cannot " + verb + " maps. Once you buy one, it is yours permanently.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You cannot " + verb + " licenses. Once you obtain one, it is yours permanently.";

	for(const Ship *ship : playerShips)
		if(ShipCanRemove(ship, selectedOutfit))
			return true;

	// Something is being blocked from being sold or we don't have any.
	// Check if we have one available or not, and then whether other outfit depend on it,
	// if so dependent outfits must be uninstalled first.
	// Collect the reasons that the outfit can't be sold or uninstalled.
	vector<string> errors;
	bool hasOutfit = false;
	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(selectedOutfit))
		{
			hasOutfit = true;
			break;
		}

	if(!hasOutfit)
		return "You don't have any of these outfits to " + verb + ".";

	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(selectedOutfit))
			for(const pair<const char *, double> &it : selectedOutfit->Attributes())
				if(ship->Attributes().Get(it.first) < it.second)
				{
					bool detailsFound = false;
					for(const auto &sit : ship->Outfits())
						if(sit.first->Get(it.first) < 0.)
						{
							errors.push_back("You cannot " + verb + " this outfit, "
								"because that would cause your ship " + ship->Name() + "'s " +
								"\"" + it.first + "\" value to be reduced to less than zero. " +
								"To " + verb + " this outfit, you must " + verb + " the " +
								sit.first->DisplayName() + " outfit first.");
							detailsFound = true;
						}
					if(!detailsFound)
					{
						errors.push_back("You cannot " + verb + " this outfit, "
							"because that would cause your ship " + ship->Name() + "'s \"" +
							it.first + "\" value to be reduced to less than zero.");
					}
				}

	// Return the errors in the appropriate format.
	if(errors.empty())
		return true;

	if(errors.size() == 1)
		return errors[0];

	string errorMessage = "There are several reasons why you cannot " + verb + " this outfit:\n";
	for(const auto & error : errors)
		errorMessage += "- " + error + "\n";
	return errorMessage;
}



// Uninstall outfits from selected ships, redeem credits if it was a sale.
// This also handles ammo dependencies.
// Note: This will remove ONE outfit from EACH selected ship which has the outfit.
void OutfitterPanel::SellOrUninstallOne(SDL_Keycode contextKey) const
{
	// Key:
	//   's' - Sell
	//		Uninstall and redeem credits, item goes to Stock if not usually available.
	//   'u' - Uninstall
	//		Uninstall from ship and keep the item in Storage.

	// Get the ships that have the most of this outfit installed.
	if(const vector<Ship *> shipsToOutfit = GetShipsToOutfit(); !shipsToOutfit.empty())
	{
		// Note: to get here, we have already confirmed that every ship in the selection
		// has the outfit and it is able to be uninstalled in the first place.
		for(Ship *ship : shipsToOutfit)
		{
			// Uninstall the outfit.
			ship->AddOutfit(selectedOutfit, -1);
			if((selectedOutfit->Get("required crew")))
				ship->AddCrew(-selectedOutfit->Get("required crew"));
			ship->Recharge();

			// If the context is sale:
			if(contextKey == 's')
			{
				// Do the sale.
				int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(price);
				player.AddStock(selectedOutfit, 1);
			}
			// If the context is uninstall, move the outfit into Storage
			else
				// Move to storage.
				player.Storage().Add(selectedOutfit, 1);

			// Since some outfits have ammo, remove any ammo that must be sold because there
			// aren't enough supporting slots for said ammo once this outfit is removed.
			const Outfit *ammo = selectedOutfit->Ammo();
			if(ammo && ship->OutfitCount(ammo))
			{
				// Determine how many of this ammo we must Uninstall to also Uninstall the launcher.
				int mustUninstall = 0;
				for(const pair<const char *, double> &it : ship->Attributes().Attributes())
					if(it.second < 0.)
						mustUninstall = max<int>(mustUninstall, it.second / ammo->Get(it.first));

				if(mustUninstall)
				{
					ship->AddOutfit(ammo, -mustUninstall);

					// If the context is sale:
					if(contextKey == 's')
					{
						// Do the sale.
						int64_t price = player.FleetDepreciation().Value(ammo, day, mustUninstall);
						player.Accounts().AddCredits(price);
						player.AddStock(ammo, mustUninstall);
					}
					// If the context is uninstall, move the outfit into Cargo, so we can take it
					// with us, if possible.
					else if(contextKey == 'u' && CanFitInCargo())
					{
						// Move to cargo.
						player.Cargo().Add(ammo, mustUninstall);
					}
					// Otherwise, the context is move to storage ('r'), or the sale/uninstall
					// couldn't make it work with Cargo so we have to leave it on the planet
					// (by definition this planet has an outfitter).
					else
						// Move to storage.
						player.Storage().Add(ammo, mustUninstall);
				}
			}
		}
	}
}



// Check if the outfit can be purchased (availability, payment, licensing) and
// if it can be installed on ANY of the selected ships.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanDoBuyButton () const
{
	if(TransactionResult result = CanPurchase(); !result)
		return result;
	return CanBeInstalled();
}



// Buy and install the <modifier> outfits into EACH selected ship.
// By definition Buy is from Outfitter (only).
void OutfitterPanel::BuyFromShopAndInstall() const
{
	// Buy the required license.
	if(int64_t licenseCost = LicenseCost(selectedOutfit, false))
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
	for(int i = 0; i < modifier && CanDoBuyButton (); ++i)
	{
		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanDoBuyButton ())
				return;

			// Pay for it and remove it from available stock.
			int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(-price);
			player.AddStock(selectedOutfit, -1);

			// Install it on this ship.
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



// Buy and install up to <modifier> outfits for EACH selected ship.
void OutfitterPanel::DoBuyButton ()
{
	BuyFromShopAndInstall();
}



// Return true if the player is able to purchase the item from the outfitter or
// previously sold stock AND can afford it AND the player is properly licensed,
// that is: availability, payment, and licensing. ALONG WITH wheter the outfit
// will fit in Cargo.
ShopPanel::TransactionResult OutfitterPanel::CanBuyToCargo() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	// Special case: maps, cannot place in Cargo.
	if(selectedOutfit->Get("map"))
		return "You place maps in to cargo.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You cannot place licenses into cargo.";

	TransactionResult canPurchase = CanPurchase(false);
	if(!canPurchase)
		return canPurchase;
	return CanFitInCargo(true);
}



// Buy up to <modifier> of the selected outfit and place them in fleet Cargo.
void OutfitterPanel::BuyIntoCargo()
{
	// Buy the required license.
	if(int64_t licenseCost = LicenseCost(selectedOutfit, false))
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedOutfit->Licenses())
			if(!player.HasLicense(licenseName))
				player.AddLicense(licenseName);
	}

	int howManyToBuy = Modifier();
	double mass = selectedOutfit->Mass();
	if(mass)
		howManyToBuy = min(howManyToBuy, static_cast<int>(player.Cargo().FreePrecise() / mass));

	// How much will it cost to buy all that we can fit?
	int64_t price = player.StockDepreciation().Value(selectedOutfit, day, howManyToBuy);

	// Adjust the number to buy to stay within the player's available credits.
	while(price > player.Accounts().Credits() && howManyToBuy)
	{
		howManyToBuy--;
		price = player.StockDepreciation().Value(selectedOutfit, day, howManyToBuy);
	}

	// Buy as many as can be paid for and will fit.
	player.Accounts().AddCredits(-price);
	player.AddStock(selectedOutfit, -howManyToBuy);
	player.Cargo().Add(selectedOutfit, howManyToBuy);
}



// Check if this outfit can be sold.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanSell() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	// Can sell things in Cargo.
	if(player.Cargo().Get(selectedOutfit))
		return true;

	// Can sell things in Storage.
	if(player.Storage().Get(selectedOutfit))
		return true;

	// There are reasons that Selling may fail related to availability of outfits
	// on ships based on ship selection.
	return CanSellOrUninstall("sell");
}



// Sell <modifier> outfits, sourcing from Cargo (first) OR Storage (second) OR else the
// selected Ships.
// Note: When selling from Cargo or Storage, up to <modifier> outfit will be sold in total,
// but when selling from the selected ships, up to <modifier> outfit will be sold from EACH ship.
void OutfitterPanel::Sell()
{
	int modifier = Modifier();
	// Now figure out where to sell it from:
	// Sell from Cargo if we have any there:
	if(player.Cargo().Get(selectedOutfit))
		for(int i = 0; i < modifier && player.Cargo().Get(selectedOutfit); ++i)
		{
			// Do the sale from cargo.
			player.Cargo().Remove(selectedOutfit);
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
	// Otherwise, sell from Storage if there are any available.
	else if(player.Storage().Get(selectedOutfit))
		for(int i = 0; i < modifier && player.Storage().Get(selectedOutfit); ++i)
		{
			// Do the sale from storage.
			player.Storage().Remove(selectedOutfit);
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
	// And lastly, sell from the selected Ships, if any.
	// Get the ships that have the most of this outfit installed.
	else if(const vector<Ship *> shipsToOutfit = GetShipsToOutfit(); !shipsToOutfit.empty())
		for(int i = 0; i < modifier && CanSellOrUninstall("sell"); ++i)
			SellOrUninstallOne(SELL);
}



// Check if the outfit is available to Install (from already owned stores) as well as
// if it can actually be installed on ANY of the selected ships.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanInstall() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!player.Storage().Get(selectedOutfit) && !player.Cargo().Get(selectedOutfit))
		return "You don't have any of this outfit to install, use \"B\" to buy (and install) it.";

	// Then check if installation requirements are met by at least one selected ship.
	return CanBeInstalled();
}



// Install from already owned stores: Cargo or Storage.
// Note: Up to <modifier> for EACH selected ship will be installed.
void OutfitterPanel::Install()
{
	int modifier = Modifier();
	for(int i = 0; i < modifier && CanBeInstalled(); ++i)
	{
		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanBeInstalled())
				return;

			// Use cargo first:
			if(player.Cargo().Get(selectedOutfit))
				player.Cargo().Remove(selectedOutfit);

			// Use storage second:
			else if(player.Storage().Get(selectedOutfit))
				player.Storage().Remove(selectedOutfit);

			// None were found in Cargo or Storage, bail out.
			else
				return;

			// Install the selected outfit to this ship.
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



// Check if the outfit is able to be uninstalled.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanUninstall() const
{
	return CanSellOrUninstall("uninstall");
}



// Uninstall up to <modifier> outfits from EACH of the selected ships and move them to Storage,
// OR, if none are installed, move up to <modifier> of the selected outfit from fleet Cargo to
// Storage.
void OutfitterPanel::Uninstall()
{
	int modifier = Modifier();
	// If intsalled on ship
	if(CanSellOrUninstall("uninstall"))
	{
		for(int i = 0; i < modifier && CanSellOrUninstall("move to storage"); ++i)
			SellOrUninstallOne(UNINSTALL);
	}
	// otherwise, move up to <modifier> of the selected outfit from fleet Cargo to Storage.
	else if(int cargoCount = player.Cargo().Get(selectedOutfit); cargoCount)
	{
		// Transfer <cargoCount> outfits from Cargo to Storage.
		int howManyToMove = min(cargoCount, modifier);
		player.Cargo().Remove(selectedOutfit, howManyToMove);
		player.Storage().Add(selectedOutfit, howManyToMove);
	}
}



// Check if the outfit is able to be moved to Cargo from Storage.
// If not, return the reasons why not.
bool OutfitterPanel::CanMoveToCargoFromStorage() const
{
	if(!planet || !selectedOutfit)
		// No outfit selected.
		return false;

	if(selectedOutfit->Get("map"))
		// You cannot move maps around. Once you buy one, it is yours permanently.
		return false;

	if(HasLicense(selectedOutfit->TrueName()))
		// You cannot move licenses around. Once you obtain one, it is yours permanently.
		return false;

	if(!player.Storage().Get(selectedOutfit))
		// You don't have any of these outfits in storage to move to your cargo hold.
		return false;

	// The return value must be a bool.
	return static_cast<bool>(CanFitInCargo());
}



// Move up to <modifier> of selected outfit to Cargo from Storage.
void OutfitterPanel::MoveToCargoFromStorage()
{
	int modifier = Modifier();
	for(int i = 0; i < modifier && player.Storage().Get(selectedOutfit) && CanFitInCargo(); ++i)
	{
		player.Cargo().Add(selectedOutfit);
		player.Storage().Remove(selectedOutfit);
	}
}



// Check if the outfit is able to be moved to Storage from Cargo or Ship.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanMoveToStorage() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(selectedOutfit->Get("map"))
		return "You cannot move maps around. Once you buy one, it is yours permanently.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You cannot move licenses around. Once you obtain one, it is yours permanently.";

	// If we have one in cargo, we'll move that one.
	if(player.Cargo().Get(selectedOutfit))
		return true;

	// Otherwise it depends on if it can actually be uninstalled.
	return CanSellOrUninstall("move to storage");
}



// Move up to <modifier> of the selected outfit from fleet Cargo to Storage, or,
// if none are available in Cargo, uninstall up to <modifier> outfits from EACH of the
// selected ships and move them to Storage.
void OutfitterPanel::RetainInStorage()
{
	int modifier = Modifier();
	// If possible, move up to <modifier> of the selected outfit from fleet cargo to Storage.
	int cargoCount = player.Cargo().Get(selectedOutfit);
	if(cargoCount)
	{
		// Transfer <cargoCount> outfits from Cargo to Storage.
		int howManyToMove = min(cargoCount, modifier);
		player.Cargo().Remove(selectedOutfit, howManyToMove);
		player.Storage().Add(selectedOutfit, howManyToMove);
	}
	// Otherwise, uninstall and move up to <modifier> selected outfits to storage from EACH
	// selected ship:
	else
		for(int i = 0; i < modifier && CanSellOrUninstall("move to storage"); ++i)
			SellOrUninstallOne(UNINSTALL);
}



bool OutfitterPanel::ShouldHighlight(const Ship *ship)
{
	if(!selectedOutfit)
		return false;

	// If we're hovering above a button that can modify ship outfits, highlight
	// the ship.
	if(hoverButton == 'b')
		return CanDoBuyButton () && ShipCanAdd(ship, selectedOutfit);
	if(hoverButton == 'i')
		return CanInstall() && ShipCanAdd(ship, selectedOutfit);
	if(hoverButton == 's')
		return CanSell() && ShipCanRemove(ship, selectedOutfit);
	if(hoverButton == 'u')
		return CanUninstall() && ShipCanRemove(ship, selectedOutfit);

	return false;
}



// Draw the display filter selection checkboxes in the lower left of the outfit panel.
void OutfitterPanel::DrawKey()
{
	const Sprite *back = SpriteSet::Get("ui/outfitter key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));

	const Font &font = FontSet::Get(14);
	Color color[2] = {*GameData::Colors().Get("medium"), *GameData::Colors().Get("bright")};
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};

	Point pos = Screen::BottomLeft() + Point(10., -VisibilityCheckboxesSize() + checkboxSpacing / 2.);
	const Point off = Point(10., -.5 * font.Height());
	const Point checkboxSize = Point(180., checkboxSpacing);
	const Point checkboxOffset = Point(80., 0.);

	SpriteShader::Draw(box[showForSale], pos);
	font.Draw("Show outfits for sale", pos + off, color[showForSale]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleForSale(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showInstalled], pos);
	// The text color will be "medium" when no ships are selected, regardless of checkmark state,
	// indicating that the selection is invalid (invalid context).
	font.Draw("Show installed outfits", pos + off, color[showInstalled && playerShip]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this]() { ToggleInstalled(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showCargo], pos);
	font.Draw("Show outfits in cargo", pos + off, color[showCargo]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleCargo(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showStorage], pos);
	font.Draw("Show outfits in storage", pos + off, color[showStorage]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleStorage(); });
}



void OutfitterPanel::ToggleForSale()
{
	showForSale = !showForSale;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleInstalled()
{
	showInstalled = !showInstalled;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleStorage()
{
	showStorage = !showStorage;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleCargo()
{
	showCargo = !showCargo;

	CheckSelection();
	delayedAutoScroll = true;
}



// Returns true if this ship can Install the selected Outfit.
bool OutfitterPanel::ShipCanAdd(const Ship *ship, const Outfit *outfit)
{
	return (ship->Attributes().CanAdd(*outfit, 1) > 0);
}



// Returns true if this ship can Uninstall the selected Outfit.
bool OutfitterPanel::ShipCanRemove(const Ship *ship, const Outfit *outfit)
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

	// Ammo is not a factor, check whether this ship can uninstall this outfit.
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



bool OutfitterPanel::HasLicense(const string &name) const
{
	return (IsLicense(name) && player.HasLicense(LicenseRoot(name)));
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
vector<Ship *> OutfitterPanel::GetShipsToOutfit(bool isBuy) const
{
	vector<Ship *> shipsToOutfit;
	int compareValue = isBuy ? numeric_limits<int>::max() : 0;
	int compareMod = 2 * isBuy - 1;
	for(Ship *ship : playerShips)
	{
		if((isBuy && !ShipCanAdd(ship, selectedOutfit))
				|| (!isBuy && !ShipCanRemove(ship, selectedOutfit)))
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



void OutfitterPanel::DrawButtons()
{
	// There will be two rows of buttons:
	//  [ Buy  ] [  Install  ] [ Cargo ]
	//  [ Sell ] [ Uninstall ] [ Keep  ] [ Leave ]
	// Calculate row locations from bottom to top:
	const double rowTwoY = Screen::BottomRight().Y() - .5 * BUTTON_HEIGHT - 2. * BUTTON_ROW_PAD;
	const double rowOneY = rowTwoY - BUTTON_HEIGHT - BUTTON_ROW_PAD;
	// Calculate button positions from right to left:
	const double buttonFourX = Screen::BottomRight().X() - .5 * BUTTON_4_WIDTH - 2. * BUTTON_COL_PAD;
	const double buttonThreeX = buttonFourX - (.5 * BUTTON_4_WIDTH + .5 * BUTTON_3_WIDTH) - BUTTON_COL_PAD;
	const double buttonTwoX = buttonThreeX - (.5 * BUTTON_3_WIDTH + .5 * BUTTON_2_WIDTH) - BUTTON_COL_PAD;
	const double buttonOneX = buttonTwoX - (.5 * BUTTON_2_WIDTH + .5 * BUTTON_1_WIDTH) - BUTTON_COL_PAD;

	// Draw the button panel (shop side panel footer).
	const Point buttonPanelSize(SIDEBAR_WIDTH, ButtonPanelHeight());
	FillShader::Fill(Screen::BottomRight() - .5 * buttonPanelSize, buttonPanelSize,
		*GameData::Colors().Get("shop side panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, Screen::Bottom() - ButtonPanelHeight()),
		Point(SIDEBAR_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));

	// Set up font size and colors for the credits.
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("panel background");

	// Draw the row for credits display.
	const Point creditsPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - ButtonPanelHeight() + 5);
	font.Draw("You have:", creditsPoint, dim);
	const auto credits = Format::CreditString(player.Accounts().Credits());
	font.Draw({ credits, {SIDEBAR_WIDTH - 20, Alignment::RIGHT} }, creditsPoint, bright);

	// Draw the row for Fleet Cargo Space free.
	const Point cargoPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - ButtonPanelHeight() + 25);
	font.Draw("Cargo Free:", cargoPoint, dim);
	string space = Format::Number(player.Cargo().Free()) + " / " + Format::Number(player.Cargo().Size());
	font.Draw({ space, {SIDEBAR_WIDTH - 20, Alignment::RIGHT} }, cargoPoint, bright);

	// Define the button text colors.
	const Font &bigFont = FontSet::Get(18);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");
	const Color *textColor = &inactive;
	const Point buttonOneSize = Point(BUTTON_1_WIDTH, BUTTON_HEIGHT);
	const Point buttonTwoSize = Point(BUTTON_2_WIDTH, BUTTON_HEIGHT);
	const Point buttonThreeSize = Point(BUTTON_3_WIDTH, BUTTON_HEIGHT);
	const Point buttonFourSize = Point(BUTTON_4_WIDTH, BUTTON_HEIGHT);

	// Draw the first row of buttons.
	static const string BUY = "_Buy";
	const Point buyCenter = Point(buttonOneX, rowOneY);
	FillShader::Fill(buyCenter, buttonOneSize, back);
	textColor = !CanDoBuyButton() ? &inactive : (hoverButton == 'b') ? &hover : &active;
	bigFont.Draw(BUY,
		buyCenter - .5 * Point(bigFont.Width(BUY), bigFont.Height()),
		*textColor);

	static const string INSTALL = "_Install";
	const Point installCenter = Point(buttonTwoX, rowOneY);
	FillShader::Fill(installCenter, buttonTwoSize, back);
	textColor = !CanInstall() ? &inactive : (hoverButton == 'i') ? &hover : &active;
	bigFont.Draw(INSTALL,
		installCenter - .5 * Point(bigFont.Width(INSTALL), bigFont.Height()),
		*textColor);

	static const string CARGO = "_Cargo";
	const Point cargoCenter = Point(buttonThreeX, rowOneY);
	FillShader::Fill(cargoCenter, buttonThreeSize, back);
	textColor = !(CanMoveToCargoFromStorage() || CanBuyToCargo()) ? &inactive : (hoverButton == 'c') ? &hover : &active;
	bigFont.Draw(CARGO,
		cargoCenter - .5 * Point(bigFont.Width(CARGO), bigFont.Height()),
		*textColor);

	// Draw the second row of buttons.
	static const string SELL = "_Sell";
	const Point sellCenter = Point(buttonOneX, rowTwoY);
	FillShader::Fill(sellCenter, buttonOneSize, back);
	textColor = !CanSell() ? &inactive : hoverButton == 's' ? &hover : &active;
	// The `Sell` text was too far right, hence the adjustment.
	bigFont.Draw(SELL,
		sellCenter - .5 * Point(bigFont.Width(SELL) + 2, bigFont.Height()),
		*textColor);

	static const string UNINSTALL = "_Uninstall";
	const Point uninstallCenter = Point(buttonTwoX, rowTwoY);
	FillShader::Fill(uninstallCenter, buttonTwoSize, back);
	// CanMoveToStorage is here intentionally to support U moving items from Cargo to Storage.
	textColor = !(CanUninstall() || CanMoveToStorage()) ? &inactive : (hoverButton == 'u') ? &hover : &active;
	bigFont.Draw(UNINSTALL,
		uninstallCenter - .5 * Point(bigFont.Width(UNINSTALL), bigFont.Height()),
		*textColor);

	static const string STORE = "Sto_re";
	const Point storageCenter = Point(buttonThreeX, rowTwoY);
	FillShader::Fill(storageCenter, buttonThreeSize, back);
	textColor = !CanMoveToStorage() ? &inactive : (hoverButton == 'r') ? &hover : &active;
	// The `Sto_re` text was too far right, hence the adjustment.
	bigFont.Draw(STORE,
		storageCenter - .5 * Point(bigFont.Width(STORE) + 1, bigFont.Height()),
		*textColor);

	static const string LEAVE = "_Leave";
	const Point leaveCenter = Point(buttonFourX, rowTwoY);
	FillShader::Fill(leaveCenter, buttonFourSize, back);
	bigFont.Draw(LEAVE,
		leaveCenter - .5 * Point(bigFont.Width(LEAVE), bigFont.Height()),
		hoverButton == 'l' ? hover : active);

	// Draw the Find button.
	const Point findCenter = Screen::BottomRight() - Point(580, 20);
	const Sprite *findIcon =
		hoverButton == 'f' ? SpriteSet::Get("ui/find selected") : SpriteSet::Get("ui/find unselected");
	SpriteShader::Draw(findIcon, findCenter);
	static const string FIND = "_Find";

	// Draw the Modifier hover text that appears below the buttons when a modifier
	// is being applied.
	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		font.Draw(mod, buyCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, sellCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, installCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, uninstallCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, cargoCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, storageCenter + Point(-.5 * modWidth, 10.), dim);
	}

	// Draw tooltips for the button being hovered over:
	string tooltip;
	tooltip = GameData::Tooltip(string("outfitter: ") + hoverButton);
	if(!tooltip.empty())
		// Note: there is an offset between the cursor and tooltips in this case so that other
		// buttons can be seen as the mouse moves around.
		DrawTooltip(tooltip, hoverPoint + Point(-40, -60), dim, *GameData::Colors().Get("tooltip background"));

	// Draw the tooltip for your full number of credits and free cargo space
	const Rectangle creditsBox = Rectangle::FromCorner(creditsPoint, Point(SIDEBAR_WIDTH - 20, 30));
	if(creditsBox.Contains(hoverPoint))
		ShopPanel::hoverCount += ShopPanel::hoverCount < ShopPanel::HOVER_TIME;
	else if(ShopPanel::hoverCount)
		--ShopPanel::hoverCount;

	if(ShopPanel::hoverCount == ShopPanel::HOVER_TIME)
	{
		tooltip = Format::Number(player.Accounts().Credits()) + " credits" + "\n" +
			Format::Number(player.Cargo().Free()) + " tons free out of " +
			Format::Number(player.Cargo().Size()) + " tons total capacity";
		DrawTooltip(tooltip, hoverPoint, dim, *GameData::Colors().Get("tooltip background"));
	}
}



// Check if the given point is within the button zone, and if so return the
// letter of the button (or ' ' if it's not on a button).
char OutfitterPanel::CheckButton(int x, int y)
{
	// Check the Find button.
	if(x > Screen::Right() - SIDEBAR_WIDTH - 342 && x < Screen::Right() - SIDEBAR_WIDTH - 316 &&
		y > Screen::Bottom() - 31 && y < Screen::Bottom() - 4)
		return 'f';

	if(x < Screen::Right() - SIDEBAR_WIDTH || y < Screen::Bottom() - ButtonPanelHeight())
		return '\0';

	// Calculate the tops of the button rows, from bottom to top.
	const double rowTwoTop = Screen::Bottom() - BUTTON_HEIGHT - 2. * BUTTON_ROW_PAD;
	const double rowOneTop = rowTwoTop - BUTTON_HEIGHT - BUTTON_ROW_PAD;
	// Calculate the left side of the buttons, from right to left.
	const double buttonFourLeft = Screen::Right() - BUTTON_4_WIDTH - 2. * BUTTON_COL_PAD;
	const double buttonThreeLeft = buttonFourLeft - (.5 * BUTTON_4_WIDTH + .5 * BUTTON_3_WIDTH) - BUTTON_COL_PAD;
	const double buttonTwoLeft = buttonThreeLeft - (.5 * BUTTON_3_WIDTH + .5 * BUTTON_2_WIDTH) - BUTTON_COL_PAD;
	const double buttonOneLeft = buttonTwoLeft - (.5 * BUTTON_2_WIDTH + .5 * BUTTON_1_WIDTH) - BUTTON_COL_PAD;

	if(rowOneTop < y && y <= rowOneTop + BUTTON_HEIGHT)
	{
		// Check if it's the _Buy button.
		if(buttonOneLeft <= x && x < buttonOneLeft + BUTTON_1_WIDTH)
			return 'b';
		// Check if it's the _Install button.
		if(buttonTwoLeft <= x && x < buttonTwoLeft + BUTTON_2_WIDTH)
			return 'i';
		// Check if it's the _Cargo button.
		if(buttonThreeLeft <= x && x < buttonThreeLeft + BUTTON_3_WIDTH)
			return 'c';
	}

	if(rowTwoTop < y && y <= rowTwoTop + BUTTON_HEIGHT)
	{
		// Check if it's the _Sell button:
		if(buttonOneLeft <= x && x < buttonOneLeft + BUTTON_1_WIDTH)
			return 's';
		// Check if it's the _Uninstall button.
		if(buttonTwoLeft <= x && x < buttonTwoLeft + BUTTON_2_WIDTH)
			return 'u';
		// Check if it's the Sto_re button.
		if(buttonThreeLeft <= x && x < buttonThreeLeft + BUTTON_3_WIDTH)
			return 'r';
		// Check if it's the _Leave button.
		if(buttonFourLeft <= x && x < buttonFourLeft + BUTTON_4_WIDTH)
			return 'l';
	}

	return ' ';
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
