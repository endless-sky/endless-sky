/* OutfitterPanel.h
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

#pragma once

#include "ShopPanel.h"

#include "Sale.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Outfit;
class PlayerInfo;
class Point;
class Ship;



// Class representing the Outfitter UI panel, which allows you to buy new
// outfits to install in your ship or to sell the ones you own. Any outfit you
// sell is available to be bought again until you close this panel, even if it
// is not normally sold here. You can also directly install any outfit that you
// have plundered from another ship and are storing in your cargo bay. This
// panel makes an attempt to ensure that you do not leave with a ship that is
// configured in such a way that it cannot fly (e.g. no engines or steering).
class OutfitterPanel : public ShopPanel {
public:
	explicit OutfitterPanel(PlayerInfo &player);

	void Step() override;


protected:
	int TileSize() const override;
	int VisibilityCheckboxesSize() const override;
	bool HasItem(const std::string &name) const override;
	void DrawItem(const std::string &name, const Point &point) override;
	double ButtonPanelHeight() const override;
	int DetailWidth() const override;
	double DrawDetails(const Point &center) override;
	TransactionResult CanBuyToCargo() const override;
	void BuyIntoCargo() override;
	TransactionResult CanDoBuyButton () const override;
	void DoBuyButton () override;
	TransactionResult CanSell() const override;
	void Sell() override;
	TransactionResult CanInstall() const override;
	void Install() override;
	TransactionResult CanUninstall() const override;
	void Uninstall() override;
	bool CanMoveToCargoFromStorage() const override;
	void MoveToCargoFromStorage() override;
	TransactionResult CanMoveToStorage() const override;
	void RetainInStorage() override;
	bool ShouldHighlight(const Ship *ship) override;
	void DrawKey() override;
	char CheckButton(int x, int y) override;
	void DrawButtons() override;
	int FindItem(const std::string &text) const override;


private:
	static bool ShipCanAdd(const Ship *ship, const Outfit *outfit);
	static bool ShipCanRemove(const Ship *ship, const Outfit *outfit);
	static void DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned);
	bool HasLicense(const std::string &name) const;
	void CheckRefill();
	void Refill();
	// Shared code for reducing the selected ships to those that have the
	// same quantity of the selected outfit.
	std::vector<Ship *> GetShipsToOutfit(bool isBuy = false) const;

	// Helper functions to make the cargo management code more readable.
	TransactionResult CanPurchase(bool checkSpecialItems) const;
	TransactionResult CanBeInstalled() const;
	TransactionResult CanFitInCargo(bool returnReason = false) const;
	TransactionResult CanSellOrUninstall(const std::string &verb) const;
	bool IsInShop() const;
	void BuyFromShopAndInstall() const;
	void SellOrUninstallOne(SDL_Keycode contextKey) const;

	// The visibility filter key is only displayed in the OutfitterPanel.
	void ToggleForSale();
	void ToggleInstalled();
	void ToggleStorage();
	void ToggleCargo();

private:
	// Record whether we've checked if the player needs ammo refilled.
	bool checkedRefill = false;
	// Allow toggling whether outfits that are for sale are shown.
	bool showForSale = true;
	// Allow toggling whether installed outfits are shown.
	bool showInstalled = true;
	// Allow toggling whether stored outfits are shown.
	bool showStorage = true;
	// Allow toggling whether outfits in cargo are shown.
	bool showCargo = true;

	Sale<Outfit> outfitter;

	// Keep track of whether the outfitter help screens have been shown.
	bool checkedHelp = false;

	int shipsHere = 0;
};
