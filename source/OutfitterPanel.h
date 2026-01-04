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
	// Define locations which items may move to and from within the outfitter.
	enum class OutfitLocation {
		Ship,
		Shop,
		Cargo,
		Storage,
	};


public:
	explicit OutfitterPanel(PlayerInfo &player, Sale<Outfit> stock);

	virtual void Step() override;


protected:
	virtual int TileSize() const override;
	virtual int VisibilityCheckboxesSize() const override;
	virtual bool HasItem(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point) override;
	virtual double DrawDetails(const Point &center) override;
	virtual TransactionResult CanMoveOutfit(OutfitLocation fromLocation, OutfitLocation toLocation,
		const std::string &actionName = "no action specified") const;
	virtual TransactionResult MoveOutfit(OutfitLocation fromLocation, OutfitLocation toLocation,
		const std::string &actionName = "no action specified") const;
	virtual bool ButtonActive(char key, bool shipRelatedOnly = false);
	virtual bool ShouldHighlight(const Ship *ship) override;
	virtual void DrawKey() override;

	// Toggles for the display filters.
	virtual void ToggleForSale();
	virtual void ToggleInstalled();
	virtual void ToggleStorage();
	virtual void ToggleCargo();

	virtual int FindItem(const std::string &text) const override;

	virtual double ButtonPanelHeight() const override;
	virtual void DrawButtons() override;
	virtual TransactionResult HandleShortcuts(SDL_Keycode key) override;


private:
	static bool ShipCanAdd(const Ship *ship, const Outfit *outfit);
	static bool ShipCanRemove(const Ship *ship, const Outfit *outfit);
	static void DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned);
	bool HasLicense(const std::string &name) const;
	void CheckRefill();
	void Refill();
	// Shared code for reducing the selected ships to those that have the
	// same quantity of the selected outfit.
	const std::vector<Ship *> GetShipsToOutfit(bool isInstall = false) const;

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
