/* OutfitterPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFITTER_PANEL_H_
#define OUTFITTER_PANEL_H_

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
	
	virtual void Step() override;
	
protected:
	enum ViewMode
	{
		INSTALLABLE,
		PURCHASABLE,
		CARGO,
		INSTALLED
	};

	virtual int TileSize() const override;
	virtual int DrawPlayerShipInfo(const Point &point) override;
	virtual ItemStatus GetItemStatus(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point, int scrollY, bool isEnabled) override;
	virtual int DividerOffset() const override;
	virtual int DetailWidth() const override;
	virtual int DrawDetails(const Point &center) override;
	virtual bool CanBuy() const override;
	virtual bool CanBuy(const Outfit *outfit) const;
	virtual void Buy(bool fromCargo = false) override;
	virtual void FailBuy() const override;
	virtual bool CanSell(bool toCargo = false) const override;
	virtual void Sell(bool toCargo = false) override;
	virtual void FailSell(bool toCargo = false) const override;
	virtual bool ShouldHighlight(const Ship *ship) override;
	virtual void DrawKey() override;
	virtual void ToggleCargo() override;
	virtual void SelectShip(Ship *ship) override;
	virtual void SetViewMode(ViewMode value);
	
private:
	static bool ShipCanBuy(const Ship *ship, const Outfit *outfit);
	static bool ShipCanSell(const Ship *ship, const Outfit *outfit);
	static void DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned, bool isEnabled);
	bool HasMapped(int mapSize) const;
	bool IsLicense(const std::string &name) const;
	bool HasLicense(const std::string &name) const;
	std::string LicenseName(const std::string &name) const;
	void CheckRefill();
	void Refill();
	// Shared code for reducing the selected ships to those that have the
	// same quantity of the selected outfit.
	const std::vector<Ship *> GetShipsToOutfit(bool isBuy = false) const;
	
private:
	// Record whether we've checked if the player needs ammo refilled.
	bool checkedRefill = false;
	ViewMode viewMode;
	// Remember what ships are selected if the player switches to cargo.
	Ship *previousShip = nullptr;
	std::set<Ship *> previousShips;
	
	Sale<Outfit> outfitter;
};


#endif
