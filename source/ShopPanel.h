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

#include "Color.h"
#include "Panel.h"

#include "ClickZone.h"
#include "OutfitInfoDisplay.h"
#include "Point.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <memory>
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
	// The possible outcomes of a DrawItem call:
	enum {
		NOT_DRAWN = 0,
		DRAWN = 1,
		SELECTED = 2
	};

public:
	ShopPanel(PlayerInfo &player, const std::vector<std::string> &categories);
	
	virtual void Step() override;
	virtual void Draw() override;
	
protected:
	void DrawSidebars() const;
	void DrawButtons() const;
	void DrawMain() const;
	
	void DrawShip(const Ship &ship, const Point &center, bool isSelected) const;
	
	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int DrawPlayerShipInfo(const Point &point) const = 0;
	virtual int DrawCargoHoldInfo(const Point &point) const = 0;	
	virtual bool HasItem(const std::string &name) const = 0;
	virtual int DrawItem(const std::string &name, const Point &point, int scrollY) const = 0;
	virtual int DividerOffset() const = 0;
	virtual int DetailWidth() const = 0;
	virtual int DrawDetails(const Point &center) const = 0;
	virtual bool CanBuy() const = 0;
	virtual void Buy() = 0;
	virtual void FailBuy() = 0;
	virtual bool CanSell() const = 0;
	virtual void Sell() = 0;
	virtual void FailSell() const;
	virtual bool FlightCheck() = 0;
	virtual bool CanSellMultiple() const;
	virtual void ToggleParked() const;
	
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
	
	bool ShipIsHere(std::shared_ptr<Ship> ship) const;
	
	// These can change based on configuration or resolution.
	int DetailsWidth() const;
	int PlayerShipWidth() const;
	int SideWidth() const;
	int IconCols() const;
	
protected:
	static const int BUTTON_HEIGHT = 70;

	static const int SHIP_SIZE = 250;
	static const int DETAILS_WIDTH = 250;
	static const int OUTFIT_SIZE = 180;

	static const int ICON_TILE = 62;
	static const int ICON_SIZE = ICON_TILE - 8;
	
	const Color COLOR_DETAILS_BG = Color(.1, 1.);
	const Color COLOR_DIVIDERS = Color(.2, 1.);
	const Color COLOR_BUTTONS_BG = Color(.3, 1.);
	
protected:
	PlayerInfo &player;
	const Planet *planet = nullptr;
	
	// The selected player ship
	Ship *playerShip = nullptr;
	// Used when holding shift to select multiple ships.
	Ship *shiftSelectAnchorShip = nullptr;
	
	Ship *dragShip = nullptr;
	Point dragPoint;
	// Selected ships in side panel
	std::set<Ship *> playerShips; 
	// Total number of player ships in the shop
	mutable int shipsHere = 0; 
	// Selected ship or outfit in the main panel
	const Ship *selectedShip = nullptr;
	const Outfit *selectedOutfit = nullptr;
	
	double mainScroll = 0;
	double sideScroll = 0;
	double playerShipScroll = 0;
	double detailsScroll = 0;
	
	mutable int maxMainScroll = 0;
	mutable int maxSideScroll = 0;
	mutable int maxPlayerShipScroll = 0;
	mutable int maxDetailsScroll = 0;
	
	bool dragMain = true;
	bool dragDetails = true;
	bool dragPlayerShip = true;
	mutable int mainDetailHeight = 0;
	mutable int sideDetailHeight = 0;
	bool scrollDetailsIntoView = false;
	bool detailsInWithMain = false;
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
