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

#include "Panel.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class GameData;
class Planet;
class PlayerInfo;
class Ship;



// Class representing the shipyard UI panel, which allows you to buy new ships or
// sell any of the ones you own.
class ShipyardPanel : public Panel {
public:
	ShipyardPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	virtual bool Scroll(int x, int y, int dy);
	
	
private:
	class ClickZone {
	public:
		ClickZone(int x, int y, int rx, int ry, const Ship *ship);
		
		bool Contains(int x, int y) const;
		const Ship *GetShip() const;
		
	private:
		int left;
		int top;
		int right;
		int bottom;
		
		const Ship *ship;
	};
	
	
private:
	void BuyShip(const std::string &name);
	void SellShip();
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const Planet *planet;
	
	const Ship *playerShip;
	const Ship *selectedShip;
	
	ShipInfoDisplay playerShipInfo;
	ShipInfoDisplay selectedShipInfo;
	
	int mainScroll;
	int sideScroll;
	mutable int maxMainScroll;
	mutable int maxSideScroll;
	bool dragMain;
	
	mutable std::vector<ClickZone> zones;
	
	std::map<std::string, std::set<std::string>> catalog;
};


#endif
