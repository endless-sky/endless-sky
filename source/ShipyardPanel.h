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

#include "Sale.h"

#include <string>

class PlayerInfo;
class Point;
class Ship;



// Class representing the shipyard UI panel, which allows you to buy new ships
// or to sell any of the ones you own. For certain ships, you may need to have a
// certain license to buy them, in which case the cost of the license is added
// to the cost of the ship. (This is intended to be an annoyance, representing
// a government that is particularly repressive of independent pilots.)
class ShipyardPanel : public ShopPanel {
public:
	explicit ShipyardPanel(PlayerInfo &player);
	
	
protected:
	virtual int TileSize() const override;
	virtual int DrawPlayerShipInfo(const Point &point) override;
	virtual bool HasItem(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point, int scrollY) override;
	virtual int DividerOffset() const override;
	virtual int DetailWidth() const override;
	virtual int DrawDetails(const Point &center) override;
	virtual bool CanBuy(bool checkAlreadyOwned = true) const override;
	virtual void Buy(bool alreadyOwned = false) override;
	virtual void FailBuy() const override;
	virtual bool CanSell(bool toStorage = false) const override;
	virtual void Sell(bool toStorage = false) override;
	virtual bool CanSellMultiple() const override;
	
	
private:
	void BuyShip(const std::string &name);
	void SellShip();
	
	
private:
	int modifier;
	
	Sale<Ship> shipyard;
};


#endif
