/* ShopPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHOP_PANEL_H_
#define SHOP_PANEL_H_

#include "Panel.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Planet;
class PlayerInfo;
class Point;
class Ship;
class Outfit;



// Class representing the common elements of both the shipyard panel and the
// outfitter panel (e.g. the sidebar with the ships you own).
class ShopPanel : public Panel {
public:
	ShopPanel(PlayerInfo &player, const std::vector<std::string> &categories);
	
	virtual void Draw() const;
	
	
protected:
	void DrawSidebar() const;
	void DrawButtons() const;
	void DrawMain() const;
	
	static void DrawShip(const Ship &ship, const Point &center, bool isSelected);
	
	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int DrawPlayerShipInfo(const Point &point) const = 0;
	virtual bool DrawItem(const std::string &name, const Point &point) const = 0;
	virtual int DividerOffset() const = 0;
	virtual int DetailWidth() const = 0;
	virtual int DrawDetails(const Point &center) const = 0;
	virtual bool CanBuy() const = 0;
	virtual void Buy() = 0;
	virtual bool CanSell() const = 0;
	virtual void Sell() = 0;
	virtual bool FlightCheck() = 0;
	virtual int Modifier() const = 0;
	
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	virtual bool Scroll(int dx, int dy);
	
	
protected:
	class ClickZone {
	public:
		ClickZone(int x, int y, int rx, int ry, const Ship *ship);
		ClickZone(int x, int y, int rx, int ry, const Outfit *outfit);
		
		bool Contains(int x, int y) const;
		const Ship *GetShip() const;
		const Outfit *GetOutfit() const;
		
		int CenterX() const;
		int CenterY() const;
		
	private:
		int left;
		int top;
		int right;
		int bottom;
		
		const Ship *ship;
		const Outfit *outfit;
	};
	
	
protected:
	static const int SIDE_WIDTH = 250;
	static const int SHIP_SIZE = 250;
	static const int OUTFIT_SIZE = 180;
	
	
protected:
	PlayerInfo &player;
	const Planet *planet;
	
	Ship *playerShip;
	const Ship *selectedShip;
	const Outfit *selectedOutfit;
	
	int mainScroll;
	int sideScroll;
	mutable int maxMainScroll;
	mutable int maxSideScroll;
	bool dragMain;
	mutable int mainDetailHeight;
	mutable int sideDetailHeight;
	
	mutable std::vector<ClickZone> zones;
	
	std::map<std::string, std::set<std::string>> catalog;
	const std::vector<std::string> &categories;
	
	
private:
	void SideSelect(bool next);
	void MainLeft();
	void MainRight();
	void MainUp();
	void MainDown();
	std::vector<ClickZone>::const_iterator Selected() const;
	std::vector<ClickZone>::const_iterator MainStart() const;
};



#endif
