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

#include <map>
#include <string>

class Outfit;
class PlayerInfo;
class Point;



// Class representing the Outfitter UI panel, which allows you to buy new
// outfits to install in your ship or to sell the ones you own. Any outfit you
// sell is available to be bought again until you close this panel, even if it
// is not normally sold here. You can also directly install any outfit that you
// have plundered from another ship and are storing in your cargo bay. This
// panel makes an attempt to ensure that you do not leave with a ship that is
// configured in such a way that it cannot fly (e.g. no engines or steering).
class OutfitterPanel : public ShopPanel {
public:
	OutfitterPanel(PlayerInfo &player);
	
	virtual void Step() override;
	
	
protected:
	virtual int TileSize() const override;
	virtual int DrawPlayerShipInfo(const Point &point) const override;
	virtual bool DrawItem(const std::string &name, const Point &point, int scrollY) const override;
	virtual int DividerOffset() const override;
	virtual int DetailWidth() const override;
	virtual int DrawDetails(const Point &center) const override;
	virtual bool CanBuy() const override;
	virtual void Buy() override;
	virtual void FailBuy() const override;
	virtual bool CanSell() const override;
	virtual void Sell() override;
	virtual void FailSell() const override;
	virtual bool FlightCheck() override;
	
	
private:
	static bool ShipCanBuy(const Ship *ship, const Outfit *outfit);
	static bool ShipCanSell(const Ship *ship, const Outfit *outfit);
	static void DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned);
	bool HasMapped(int mapSize) const;
	bool IsLicense(const std::string &name) const;
	bool HasLicense(const std::string &name) const;
	std::string LicenseName(const std::string &name) const;
	void CheckRefill();
	void Refill();
	
	
private:
	// This is how many of each outfit we have sold to this particular outfitter
	// in this particular shopping session (so that you can buy back things this
	// outfitter does not normally carry that you sold by accident).
	std::map<const Outfit *, int> &available;
	bool checkedRefill = false;
};


#endif
