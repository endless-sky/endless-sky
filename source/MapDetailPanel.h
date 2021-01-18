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
class System;



// A panel that displays the galaxy star map, with options for color-coding the
// stars based on attitude towards the player, government, or commodity price.
// This panel also lets you view what planets are in each system, and you can
// click on a planet to view its description.
class MapDetailPanel : public MapPanel {
public:
	explicit MapDetailPanel(PlayerInfo &player, const System *system = nullptr);
	explicit MapDetailPanel(const MapPanel &panel);
	
	virtual void Step() override;
	virtual void Draw() override;
	
	
protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	// Handle single & double-clicks on commodities, planet information, or objects in the "orbits" display.
	virtual bool Click(int x, int y, int clicks) override;
	// Handle right-clicks within the "orbits" display.
	virtual bool RClick(int x, int y) override;
	
	
private:
	void DrawKey();
	void DrawInfo();
	void DrawOrbits();
	
	// Set the commodity coloring, and update the player info as well.
	void SetCommodity(int index);
	
	
private:
	int governmentY = 0;
	int tradeY = 0;
	
	// Default display scaling for orbits within the currently displayed system.
	double scale = .03;
	
	// Y-indices of the selected system's "info displays" that feature its planets' names and basic information.
	std::map<const Planet *, int> planetY;
	// Vector offsets from the center of the "orbits" UI.
	std::map<const Planet *, Point> planets;
};



#endif
