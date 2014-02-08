/* MapPanel.h
Michael Zahniser, 2 Nov 2013

A panel that displays the galaxy star map, with options for color-coding the
stars based on attitude towards the player, government, or commodity price.
*/

#ifndef MAP_PANEL_H_INCLUDED
#define MAP_PANEL_H_INCLUDED

#include "Panel.h"

#include "Point.h"

#include <map>

class GameData;
class Planet;
class PlayerInfo;
class System;



class MapPanel : public Panel {
public:
	MapPanel(const GameData &data, PlayerInfo &info, int commodity = 0);
	virtual ~MapPanel() {}
	
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	const GameData &data;
	const System *current;
	const System *selected;
	PlayerInfo &player;
	
	std::map<const System *, int> distance;
	
	mutable int governmentY;
	
	Point center;
	mutable int tradeY;
	int commodity;
	
	mutable std::map<const Planet *, int> planetY;
	mutable std::map<const Planet *, Point> planets;
	const Planet *selectedPlanet;
};



#endif
