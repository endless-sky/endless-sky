/* MapPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_PANEL_H_
#define MAP_PANEL_H_

#include "Panel.h"

#include "DistanceMap.h"
#include "Point.h"

#include <map>
#include <set>

class GameData;
class Planet;
class PlayerInfo;
class System;



// A panel that displays the galaxy star map, with options for color-coding the
// stars based on attitude towards the player, government, or commodity price.
class MapPanel : public Panel {
public:
	MapPanel(const GameData &data, PlayerInfo &info, int commodity = 0);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	const GameData &data;
	const System *current;
	const System *selected;
	PlayerInfo &player;
	
	DistanceMap distance;
	std::set<const System *> destinations;
	
	mutable int governmentY;
	
	Point center;
	mutable int tradeY;
	int commodity;
	
	mutable std::map<const Planet *, int> planetY;
	mutable std::map<const Planet *, Point> planets;
	const Planet *selectedPlanet;
};



#endif
