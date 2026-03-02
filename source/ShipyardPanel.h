/* ShipyardPanel.h
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
	explicit ShipyardPanel(PlayerInfo &player, Sale<Ship> stock);

	virtual void Step() override;


protected:
	virtual int TileSize() const override;
	virtual bool HasItem(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point) override;
	virtual double ButtonPanelHeight() const override;
	virtual double DrawDetails(const Point &center) override;
	virtual void DrawButtons() override;
	virtual TransactionResult CanDoBuyButton() const;
	virtual void DoBuyButton();
	virtual void Sell(bool storeOutfits);
	virtual int FindItem(const std::string &text) const override;
	virtual TransactionResult HandleShortcuts(SDL_Keycode key) override;


private:
	bool BuyShip(const std::string &name);
	void SellShipAndOutfits();
	void SellShipChassis();
	void SellShip(bool toStorage);


private:
	int modifier;

	Sale<Ship> shipyard;
};
