/* MapDetailPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_DETAIL_PANEL_H_
#define MAP_DETAIL_PANEL_H_

#include "MapPanel.h"

#include "Point.h"

#include <map>

class Planet;
class PlayerInfo;



// A panel that displays the galaxy star map, with options for color-coding the
// stars based on attitude towards the player, government, or commodity price.
// This panel also lets you view what planets are in each system, and you can
// click on a planet to view its description.
class MapDetailPanel : public MapPanel {
public:
	MapDetailPanel(PlayerInfo &player, int commodity = SHOW_REPUTATION, const System *system = nullptr);
	MapDetailPanel(PlayerInfo &player, int *commodity);
	MapDetailPanel(const MapPanel &panel);
	
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	
	
private:
	void DoFind(const std::string &text);
	void DrawKey() const;
	void DrawInfo();
	void DrawOrbits() const;
	
	void ListShips() const;
	void ListOutfits() const;
	
	
private:
	mutable int governmentY;
	mutable int tradeY;
	
	mutable std::map<const Planet *, int> planetY;
	mutable std::map<const Planet *, Point> planets;
	const Planet *selectedPlanet;
};



#endif
