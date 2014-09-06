/* HailPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HAIL_PANEL_H_
#define HAIL_PANEL_H_

#include "Panel.h"

#include <string>

class PlayerInfo;
class Ship;
class StellarObject;



// This panel is shown when you hail a ship or planet.
class HailPanel : public Panel {
public:
	HailPanel(PlayerInfo &player, const Ship *ship);
	HailPanel(PlayerInfo &player, const StellarObject *planet);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod) override;
	virtual bool Click(int x, int y) override;
	
	
private:
	PlayerInfo &player;
	const Ship *ship = nullptr;
	const StellarObject *planet = nullptr;
	
	std::string header;
	std::string message;
	
	int bribe = 0;
	bool playerNeedsHelp = false;
	bool canGiveFuel = false;
	bool canRepair = false;
};



#endif
