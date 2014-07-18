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



// Class representing the shipyard UI panel, which allows you to buy new ships or
// sell any of the ones you own.
class ShipyardPanel : public ShopPanel {
public:
	ShipyardPanel(PlayerInfo &player);
	
	
protected:
	virtual int TileSize() const;
	virtual int DrawPlayerShipInfo(const Point &point) const;
	virtual bool DrawItem(const std::string &name, const Point &point) const;
	virtual int DividerOffset() const;
	virtual int DetailWidth() const;
	virtual int DrawDetails(const Point &center) const;
	virtual bool CanBuy() const;
	virtual void Buy();
	virtual bool CanSell() const;
	virtual void Sell();
	virtual bool FlightCheck();
	virtual int Modifier() const;
	
	
private:
	void BuyShip(const std::string &name);
	void SellShip();
};


#endif
