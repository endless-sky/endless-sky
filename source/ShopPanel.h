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

#include "ClickZone.h"
#include "OutfitInfoDisplay.h"
#include "Point.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Planet;
class PlayerInfo;
class Ship;
class Outfit;



// Class representing the common elements of both the shipyard panel and the
// outfitter panel (e.g. the sidebar with the ships you own).
class ShopPanel : public Panel {
public:
	ShopPanel(PlayerInfo &player, const std::vector<std::string> &categories);
	
	virtual void Step() override;
	virtual void Draw() override;
	
protected:
	void DrawSidebar() const;
	void DrawButtons() const;
	void DrawMain() const;
	
	void DrawShip(const Ship &ship, const Point &center, bool isSelected) const;
	
	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int DrawPlayerShipInfo(const Point &point) const = 0;
	virtual bool HasItem(const std::string &name) const = 0;
	virtual void DrawItem(const std::string &name, const Point &point, int scrollY) const = 0;
	virtual int DividerOffset() const = 0;
	virtual int DetailWidth() const = 0;
	virtual int DrawDetails(const Point &center) const = 0;
	virtual bool CanBuy() const = 0;
	virtual void Buy() = 0;
	virtual void FailBuy() const = 0;
	virtual bool CanSell() const = 0;
	virtual void Sell() = 0;
	virtual void FailSell() const;
	virtual bool FlightCheck() = 0;
	virtual bool CanSellMultiple() const;
	
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
protected:
	class Zone : public ClickZone<const Ship *> {
	public:
		Zone(Point center, Point size, const Ship *ship, double scrollY = 0.);
		Zone(Point center, Point size, const Outfit *outfit, double scrollY = 0.);
		
		const Ship *GetShip() const;
		const Outfit *GetOutfit() const;
		
		double ScrollY() const;
		
	private:
		double scrollY = 0.;
		const Outfit *outfit = nullptr;
	};
	
	
protected:
	static const int SIDE_WIDTH = 250;
	static const int BUTTON_HEIGHT = 70;
	static const int SHIP_SIZE = 250;
	static const int OUTFIT_SIZE = 180;
	
	
protected:
	PlayerInfo &player;
	const Planet *planet = nullptr;
	
	Ship *playerShip = nullptr;
	Ship *dragShip = nullptr;
	Point dragPoint;
	std::set<Ship *> playerShips;
	const Ship *selectedShip = nullptr;
	const Outfit *selectedOutfit = nullptr;
	
	double mainScroll = 0;
	double sideScroll = 0;
	mutable int maxMainScroll = 0;
	mutable int maxSideScroll = 0;
	bool dragMain = true;
	mutable int mainDetailHeight = 0;
	mutable int sideDetailHeight = 0;
	bool scrollDetailsIntoView = false;
	mutable double selectedBottomY = 0.;
	
	mutable std::vector<Zone> zones;
	mutable std::vector<ClickZone<std::string>> categoryZones;
	
	std::map<std::string, std::set<std::string>> catalog;
	const std::vector<std::string> &categories;
	std::set<std::string> collapsed;
	
	mutable ShipInfoDisplay shipInfo;
	mutable OutfitInfoDisplay outfitInfo;
	
	
private:
	bool DoScroll(double dy);
	void SideSelect(int count);
	void SideSelect(Ship *ship);
	void MainLeft();
	void MainRight();
	void MainUp();
	void MainDown();
	std::vector<Zone>::const_iterator Selected() const;
	std::vector<Zone>::const_iterator MainStart() const;
};



#endif
