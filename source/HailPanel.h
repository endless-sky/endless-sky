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

#include "Angle.h"
#include "Point.h"

#include <cstdint>
#include <memory>
#include <string>

class Planet;
class PlayerInfo;
class Ship;
class Sprite;
class StellarObject;



// This panel is shown when you hail a ship or planet. It allows you to ask for
// assistance from friendly ships, to bribe hostile ships to leave you alone, or
// to bribe a planet to allow you to land there.
class HailPanel : public Panel {
public:
	HailPanel(PlayerInfo &player, const std::shared_ptr<Ship> &ship);
	HailPanel(PlayerInfo &player, const StellarObject *object);
	
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	
	
private:
	void SetBribe(double scale);
	
	
private:
	PlayerInfo &player;
	std::shared_ptr<Ship> ship = nullptr;
	const Planet *planet = nullptr;
	const Sprite *sprite = nullptr;
	Angle facing;
	
	std::string header;
	std::string message;
	
	int64_t bribe = 0;
	bool playerNeedsHelp = false;
	bool canGiveFuel = false;
	bool canRepair = false;
	bool hasLanguage = true;
};



#endif
