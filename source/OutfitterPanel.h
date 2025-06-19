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
	explicit OutfitterPanel(PlayerInfo &player, Sale<Outfit> stock);

	virtual void Step() override;


protected:
	virtual int TileSize() const override;
	virtual int VisibilityCheckboxesSize() const override;
	virtual bool HasItem(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point) override;
	virtual double ButtonPanelHeight() const override;
	virtual double DrawDetails(const Point &center) override;
	virtual TransactionResult CanBuyToCargo() const override;
	virtual void BuyIntoCargo() override;
	virtual TransactionResult CanDoBuyButton() const override;
	virtual void DoBuyButton() override;
	virtual TransactionResult CanUninstall(ShopPanel::UninstallAction action) const override;
	virtual void Sell(bool storeOutfits) override;
	virtual TransactionResult CanInstall() const override;
	virtual void Install() override;
	virtual void Uninstall() override;
	virtual bool CanMoveToCargoFromStorage() const override;
	virtual void MoveToCargoFromStorage() override;
	virtual void RetainInStorage() override;
	virtual bool ShouldHighlight(const Ship *ship) override;
	virtual void DrawKey() override;
	virtual char CheckButton(int x, int y) override;
	virtual void DrawButtons() override;
	virtual int FindItem(const std::string &text) const override;


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
	TransactionResult CanPurchase(bool checkSpecialItems = true) const;
	TransactionResult CanBeInstalled() const;
	TransactionResult CanFitInCargo(bool returnReason = false) const;
	void BuyFromShopAndInstall() const;
	void Uninstall(bool sell) const;

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
