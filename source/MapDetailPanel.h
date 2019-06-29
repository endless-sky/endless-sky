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
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual void Select(const System *system) override;
	
	
private:
	void DrawKey();
	void DrawInfo();
	void DrawOrbits();
	// Draw the quickest trade route when comparing commodity prices.
	void DrawTradePlan();
	
	// Set the commodity coloring, and update the player info as well.
	void SetCommodity(int index);
	
	
private:
	// Keep track of UI click zones
	int governmentY = 0;
	int tradeY = 0;
	bool wideCommodity = false;
	
	std::map<const Planet *, int> planetY;
	std::map<const Planet *, Point> planets;
	
	bool dimTravelPlan = false;
	// A system pointer used to compare commodity prices.
	const System *compareSystem = nullptr;
	// Every two systems form a link, each pair is separate from the next pair allowing a disjointed path.
	std::vector<const System *> tradeRoute;
	int tradeRouteJumps = 0;
};



#endif
