/* ShipyardPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIPYARD_PANEL_H_
#define SHIPYARD_PANEL_H_

#include "ShopPanel.h"

#include <string>

class PlayerInfo;
class Point;



// Class representing the shipyard UI panel, which allows you to buy new ships
// or to sell any of the ones you own. For certain ships, you may need to have a
// certain license to buy them, in which case the cost of the license is added
// to the cost of the ship. (This is intended to be an annoyance, representing
// a government that is particularly repressive of independent pilots.)
class ShipyardPanel : public ShopPanel {
public:
	ShipyardPanel(PlayerInfo &player);
	
	
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
	virtual bool FlightCheck() override;
	virtual bool CanSellMultiple() const override;
	
	
private:
	void BuyShip(const std::string &name);
	void SellShip();
	int64_t LicenseCost() const;
	
	
private:
	int modifier;
};


#endif
