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
	explicit ShipyardPanel(PlayerInfo &player);

	void Step() override;


protected:
	int TileSize() const override;
	bool HasItem(const std::string &name) const override;
	void DrawItem(const std::string &name, const Point &point) override;
	double ButtonPanelHeight() const override;
	double DrawDetails(const Point &center) override;
	TransactionResult CanDoBuyButton() const override;
	void DoBuyButton() override;
	void Sell(bool storeOutfits) override;
	char CheckButton(int x, int y) override;
	void DrawButtons() override;
	int FindItem(const std::string &text) const override;


private:
	void BuyShip(const std::string &name);
	void SellShipAndOutfits();
	void SellShipChassis();
	void SellShip(bool toStorage);


private:
	int modifier;

	Sale<Ship> shipyard;
};
