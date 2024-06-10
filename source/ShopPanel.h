/* ShopPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SHOP_PANEL_H_
#define SHOP_PANEL_H_

#include "Panel.h"

#include "CategoryList.h"
#include "ClickZone.h"
#include "Mission.h"
#include "OutfitInfoDisplay.h"
#include "Point.h"
#include "ScrollVar.h"
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
	// BuyResult holds the result of an attempt to buy. It is implicitly
	// created from a string or boolean in code. Any string indicates failure.
	// True indicates success, of course, while false (without a string)
	// indicates failure, but no need to pop up a message about it.
	class BuyResult {
	public:
		BuyResult(const char *error) : success(false), message(error) {}
		BuyResult(std::string error) : success(false), message(std::move(error)) {}
		BuyResult(bool result) : success(result), message() {}

		explicit operator bool() const noexcept { return success; }

		bool HasMessage() const noexcept { return !message.empty(); }
		const std::string &Message() const noexcept { return message; }


	private:
		bool success = true;
		std::string message;
	};


protected:
	void DrawShip(const Ship &ship, const Point &center, bool isSelected);

	void CheckForMissions(Mission::Location location);

	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int VisibilityCheckboxesSize() const;
	virtual bool HasItem(const std::string &name) const = 0;
	virtual void DrawItem(const std::string &name, const Point &point) = 0;
	virtual int DividerOffset() const = 0;
	virtual int DetailWidth() const = 0;
	virtual double DrawDetails(const Point &center) = 0;
	virtual BuyResult CanBuy(bool onlyOwned = false) const = 0;
	virtual void Buy(bool onlyOwned = false) = 0;
	virtual bool CanSell(bool toStorage = false) const = 0;
	virtual void Sell(bool toStorage = false) = 0;
	virtual void FailSell(bool toStorage = false) const;
	virtual bool CanSellMultiple() const;
	virtual bool IsAlreadyOwned() const;
	virtual bool ShouldHighlight(const Ship *ship);
	virtual void DrawKey();
	virtual void ToggleForSale();
	virtual void ToggleStorage();
	virtual void ToggleCargo();

	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;

	void DoFind(const std::string &text);
	virtual int FindItem(const std::string &text) const = 0;

	int64_t LicenseCost(const Outfit *outfit, bool onlyOwned = false) const;

	void CheckSelection();


protected:
	class Zone : public ClickZone<const Ship *> {
	public:
		explicit Zone(Point center, Point size, const Ship *ship);
		explicit Zone(Point center, Point size, const Outfit *outfit);

		const Ship *GetShip() const;
		const Outfit *GetOutfit() const;

	private:
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
	static const int OUTFIT_SIZE = 183;


protected:
	PlayerInfo &player;
	// Remember the current day, for calculating depreciation.
	int day;
	const Planet *planet = nullptr;
	const bool isOutfitter;

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

	ScrollVar<double> mainScroll;
	ScrollVar<double> sidebarScroll;
	ScrollVar<double> infobarScroll;
	ShopPane activePane = ShopPane::Main;
	char hoverButton = '\0';

	double previousX = 0.;

	std::vector<Zone> zones;
	std::vector<ClickZone<const Ship *>> shipZones;
	std::vector<ClickZone<std::string>> categoryZones;

	std::map<std::string, std::vector<std::string>> catalog;
	const CategoryList &categories;
	std::set<std::string> &collapsed;

	ShipInfoDisplay shipInfo;
	OutfitInfoDisplay outfitInfo;


private:
	void DrawShipsSidebar();
	void DrawDetailsSidebar();
	void DrawButtons();
	void DrawMain();

	int DrawPlayerShipInfo(const Point &point);

	bool DoScroll(double dy, int steps = 5);
	bool SetScrollToTop();
	bool SetScrollToBottom();
	void SideSelect(int count);
	void SideSelect(Ship *ship);
	void MainAutoScroll(const std::vector<Zone>::const_iterator &selected);
	void MainLeft();
	void MainRight();
	void MainUp();
	void MainDown();
	void CategoryAdvance(const std::string &category);
	std::vector<Zone>::const_iterator Selected() const;
	// Check if the given point is within the button zone, and if so return the
	// letter of the button (or ' ' if it's not on a button).
	char CheckButton(int x, int y);

	bool EscortSelected();
	bool CanPark();
	bool CanUnpark();


private:
	bool delayedAutoScroll = false;

	Point hoverPoint;
	std::string shipName;
	std::string warningType;
	int hoverCount = 0;
};



#endif
