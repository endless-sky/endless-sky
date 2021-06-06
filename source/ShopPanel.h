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

class Outfit;
class Planet;
class PlayerInfo;
class Ship;



// Class representing the common elements of both the shipyard panel and the
// outfitter panel (e.g. the sidebar with the ships you own).
class ShopPanel : public Panel {
public:
	explicit ShopPanel(PlayerInfo &player, bool isOutfitter);
	
	virtual void Step() override;
	virtual void Draw() override;
	
protected:
	void DrawShipsSidebar();
	void DrawDetailsSidebar();
	void DrawButtons();
	void DrawMain();
	
	void DrawShip(const Ship &ship, const Point &center, bool isSelected);
	
	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int DrawPlayerShipInfo(const Point &point) = 0;
	virtual bool HasItem(const std::string &name) const = 0;
	virtual void DrawItem(const std::string &name, const Point &point, int scrollY) = 0;
	virtual int DividerOffset() const = 0;
	virtual int DetailWidth() const = 0;
	virtual int DrawDetails(const Point &center) = 0;
	virtual bool CanBuy(bool checkAlreadyOwned = true) const = 0;
	virtual void Buy(bool alreadyOwned = false) = 0;
	virtual void FailBuy() const = 0;
	virtual bool CanSell(bool toStorage = false) const = 0;
	virtual void Sell(bool toStorage = false) = 0;
	virtual void FailSell(bool toStorage = false) const;
	virtual bool CanSellMultiple() const;
	virtual bool IsAlreadyOwned() const;
	virtual bool ShouldHighlight(const Ship *ship);
	virtual void DrawKey();
	virtual void ToggleForSale();
	virtual void ToggleCargo();
	
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	
	int64_t LicenseCost(const Outfit *outfit) const;
	
	
protected:
	class Zone : public ClickZone<const Ship *> {
	public:
		explicit Zone(Point center, Point size, const Ship *ship, double scrollY = 0.);
		explicit Zone(Point center, Point size, const Outfit *outfit, double scrollY = 0.);
		
		const Ship *GetShip() const;
		const Outfit *GetOutfit() const;
		
		double ScrollY() const;
		
	private:
		double scrollY = 0.;
		const Outfit *outfit = nullptr;
	};
	
	enum class ShopPane : int {
		Main,
		Sidebar,
		Info
	};
	
	
protected:
	static const int SIDEBAR_WIDTH = 250;
	static const int INFOBAR_WIDTH = 300;
	static const int SIDE_WIDTH = SIDEBAR_WIDTH + INFOBAR_WIDTH;
	static const int BUTTON_HEIGHT = 70;
	static const int SHIP_SIZE = 250;
	static const int OUTFIT_SIZE = 180;
	
	
protected:
	PlayerInfo &player;
	// Remember the current day, for calculating depreciation.
	int day;
	const Planet *planet = nullptr;
	
	// The player-owned ship that was first selected in the sidebar (or most recently purchased).
	Ship *playerShip = nullptr;
	// The player-owned ship being reordered.
	Ship *dragShip = nullptr;
	bool isDraggingShip = false;
	Point dragPoint;
	// The group of all selected, player-owned ships.
	std::set<Ship *> playerShips;
	
	// The currently selected Ship, for the ShipyardPanel.
	const Ship *selectedShip = nullptr;
	// The currently selected Outfit, for the OutfitterPanel.
	const Outfit *selectedOutfit = nullptr;
	// (It may be worth moving the above pointers into the derived classes in the future.)
	
	double mainScroll = 0.;
	double sidebarScroll = 0.;
	double infobarScroll = 0.;
	double maxMainScroll = 0.;
	double maxSidebarScroll = 0.;
	double maxInfobarScroll = 0.;
	ShopPane activePane = ShopPane::Main;
	int mainDetailHeight = 0;
	int sideDetailHeight = 0;
	bool scrollDetailsIntoView = false;
	double selectedTopY = 0.;
	bool sameSelectedTopY = false;
	char hoverButton = '\0';
	
	std::vector<Zone> zones;
	std::vector<ClickZone<std::string>> categoryZones;
	
	std::map<std::string, std::set<std::string>> catalog;
	const std::vector<std::string> &categories;
	std::set<std::string> &collapsed;
	
	ShipInfoDisplay shipInfo;
	OutfitInfoDisplay outfitInfo;
	
	mutable Point warningPoint;
	mutable std::string warningType;
	
	
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
	// Check if the given point is within the button zone, and if so return the
	// letter of the button (or ' ' if it's not on a button).
	char CheckButton(int x, int y);
};



#endif
