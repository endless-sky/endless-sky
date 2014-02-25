/* ShipyardPanel.h
Michael Zahniser, 24 Jan 2014

Class representing the shipyard UI panel, which allows you to buy new ships or
sell any of the ones you own.
*/

#ifndef SHIPYARD_PANEL_H_INCLUDED
#define SHIPYARD_PANEL_H_INCLUDED

#include "Panel.h"
#include "ShipInfoDisplay.h"

class GameData;
class PlayerInfo;
class Ship;

#include <vector>



class ShipyardPanel : public Panel {
public:
	ShipyardPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
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
	const GameData &data;
	PlayerInfo &player;
	
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
};


#endif
