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

#pragma once

#include "Panel.h"

#include "ClickZone.h"
#include "Mission.h"
#include "OutfitInfoDisplay.h"
#include "Point.h"
#include "Rectangle.h"
#include "ScrollBar.h"
#include "ScrollVar.h"
#include "ShipInfoDisplay.h"
#include "Tooltip.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class CategoryList;
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

	virtual void UpdateTooltipActivation() override;


protected:
	// TransactionResult holds the result of an attempt to do a transaction. It is implicitly
	// created from a string or boolean in code. Any string indicates failure.
	// True indicates success, of course, while false (without a string)
	// indicates failure, but no need to pop up a message about it.
	class TransactionResult {
	public:
		TransactionResult(const char *error) : success(false), message(error) {}
		TransactionResult(std::string error) : success(false), message(std::move(error)) {}
		TransactionResult(bool result) : success(result), message() {}

		explicit operator bool() const noexcept { return success; }

		bool HasMessage() const noexcept { return !message.empty(); }
		const std::string &Message() const noexcept { return message; }


	private:
		bool success = true;
		std::string message;
	};


protected:
	void DrawShip(const Ship &ship, const Point &center, bool isSelected);

	void CheckForMissions(Mission::Location location) const;

	// These are for the individual shop panels to override.
	virtual int TileSize() const = 0;
	virtual int VisibilityCheckboxesSize() const;
	virtual bool HasItem(const std::string &name) const = 0;
	virtual void DrawItem(const std::string &name, const Point &point) = 0;
	virtual double ButtonPanelHeight() const = 0;
	virtual double DrawDetails(const Point &center) = 0;
	virtual void DrawButtons() = 0;
	virtual TransactionResult HandleShortcuts(SDL_Keycode key) = 0;

	virtual bool ShouldHighlight(const Ship *ship);
	virtual void DrawKey() {};

	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool Scroll(double dx, double dy) override;

	void DoFind(const std::string &text);
	virtual int FindItem(const std::string &text) const = 0;

	int64_t LicenseCost(const Outfit *outfit, bool onlyOwned = false) const;

	void DrawButton(const std::string &name, const Rectangle &buttonShape, bool isActive, bool hovering, char keyCode);
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
	static constexpr int SIDEBAR_PADDING = 5;
	static constexpr int SIDEBAR_CONTENT = 250;
	static constexpr int SIDEBAR_WIDTH = SIDEBAR_CONTENT + SIDEBAR_PADDING;
	static constexpr int INFOBAR_WIDTH = 300;
	static constexpr int SIDE_WIDTH = SIDEBAR_WIDTH + INFOBAR_WIDTH;
	static constexpr int SHIP_SIZE = 250;
	static constexpr int OUTFIT_SIZE = 183;
	// Button size/placement info:
	static constexpr double BUTTON_ROW_START_PAD = 10;
	static constexpr double BUTTON_ROW_PAD = 9.;
	static constexpr double BUTTON_COL_PAD = 9.;
	static constexpr double BUTTON_HEIGHT = 30.;
	static constexpr double BUTTON_WIDTH = 73.;

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

	ScrollBar mainScrollbar;
	ScrollBar sidebarScrollbar;
	ScrollBar infobarScrollbar;

	double previousX = 0.;

	std::vector<Zone> zones;
	std::vector<ClickZone<char>> buttonZones;
	std::vector<ClickZone<const Ship *>> shipZones;
	std::vector<ClickZone<std::string>> categoryZones;

	std::map<std::string, std::vector<std::string>> catalog;
	const CategoryList &categories;
	std::set<std::string> &collapsed;

	ShipInfoDisplay shipInfo;
	OutfitInfoDisplay outfitInfo;

	bool delayedAutoScroll = false;
	Point hoverPoint;

	Tooltip shipsTooltip;
	Tooltip creditsTooltip;
	Tooltip buttonsTooltip;


private:
	void DrawShipsSidebar();
	void DrawDetailsSidebar();
	void DrawMain();

	int DrawPlayerShipInfo(const Point &point);

	bool DoScroll(double dy, int steps = 5);
	bool SetScrollToTop();
	bool SetScrollToBottom();
	void SideSelect(int count);
	void SideSelect(Ship *ship, int clicks = 1);
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


private:
	std::string shipName;
	std::string warningType;

	// Define the colors used by DrawButton, implemented at the class level to avoid repeat lookups from GameData.
	const Color &hover;
	const Color &active;
	const Color &inactive;
	const Color &back;

	bool checkedHelp = false;
};
