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

#include "GameData.h"
#include "PlayerInfo.h"
#include "System.h"



// Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
// buying and selling.
class TradingPanel : public Panel {
public:
	TradingPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	void Buy(int amount);
	static int Modifier();
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const System &system;
	
	int selectedRow;
};



#endif
