/* TradingPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TRADING_PANEL_H_
#define TRADING_PANEL_H_

#include "Panel.h"

class PlayerInfo;
class System;



// Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
// buying and selling. This also lets you sell any plundered outfits you are
// carrying, although if you want to sell only certain ones and not others you
// will need to select them individually int he outfitter panel.
class TradingPanel : public Panel {
public:
	TradingPanel(PlayerInfo &player);
	
	virtual void Step() override;
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	
	
private:
	void Buy(int64_t amount);
	static int Modifier();
	
	
private:
	PlayerInfo &player;
	const System &system;
	
	int selectedRow;
};



#endif
