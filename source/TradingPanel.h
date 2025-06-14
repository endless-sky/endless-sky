/* TradingPanel.h
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

#include "Panel.h"

#include <map>
#include <string>

class PlayerInfo;
class System;



// Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
// buying and selling. This also lets you sell any plundered outfits you are
// carrying, although if you want to sell only certain ones and not others you
// will need to select them individually in the outfitter panel.
class TradingPanel : public Panel {
public:
	explicit TradingPanel(PlayerInfo &player);
	~TradingPanel();

	// Set the initial commodities that the player has with them. This must be
	// called after the player has pooled their cargo from their fleet.
	void SetInitialCommodities(const std::map<std::string, int> &initial);
	// Determine how many commodities the player sold from their initial cargo.
	// This must be called before the player's pooled cargo is moved back
	// onto their fleet.
	void CalculateCommoditiesSold(const std::map<std::string, int> &current);

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;


private:
	void Buy(int64_t amount);


private:
	PlayerInfo &player;
	const System &system;
	const int COMMODITY_COUNT;

	// Remember whether the "sell all" button will sell all outfits, or sell
	// everything except outfits.
	bool sellOutfits = false;

	// Keep track of how much we sold and how much profit was made.
	int commoditiesSold = 0;
	int outfitTonsSold = 0;
	int64_t profit = 0;
	// The initial number of tons of each commodity that the player had when
	// they landed. This is compared against how many of each commodity the
	// player has when they depart to determine how many tons of commodities
	// were sold for profit/loss.
	std::map<std::string, int> initial;
};
