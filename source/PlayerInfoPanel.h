/* PlayerInfoPanel.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PLAYER_INFO_PANEL_H_
#define PLAYER_INFO_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "InfoPanelState.h"
#include "text/layout.hpp"
#include "Point.h"

#include <set>
#include <vector>

class PlayerInfo;
class Rectangle;



// This panel displays detailed information about the player and their fleet. If
// the player is landed on a planet, it also allows them to reorder the ships in
// their fleet (including changing which one is the flagship).
class PlayerInfoPanel : public Panel {
public:
	explicit PlayerInfoPanel(PlayerInfo &player);
	explicit PlayerInfoPanel(PlayerInfo &player, InfoPanelState panelState);

	virtual void Step() override;
	virtual void Draw() override;

	// The player info panel allow fast-forward to stay active.
	bool AllowsFastForward() const noexcept final;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	// Draw the two subsections of this panel.
	void DrawPlayer(const Rectangle &bounds);
	void DrawFleet(const Rectangle &bounds);

	// Handle mouse hover (also including hover during drag actions):
	bool Hover(const Point &point);
	// Adjust the scroll by the given amount. Return true if it changed.
	bool Scroll(int distance);
	// Try to scroll to the given position. Return true if position changed.
	bool ScrollAbsolute(int scroll);

	void SortShips(InfoPanelState::ShipComparator *shipComparator);

	class SortableColumn {
	public:
		SortableColumn(std::string name, Layout layout, InfoPanelState::ShipComparator *shipSort);

		std::string name;
		Layout layout;
		InfoPanelState::ShipComparator *shipSort = nullptr;
	};

private:
	PlayerInfo &player;

	static const SortableColumn columns[];

	InfoPanelState panelState;

	// Column headers that sort ships when clicked.
	std::vector<ClickZone<InfoPanelState::ShipComparator *>> menuZones;

	// Keep track of which ship the mouse is hovering over.
	int hoverIndex = -1;

	// Initialize mouse point to something off-screen to not
	// make the game think the player is hovering on something.
	Point hoverPoint = Point(-10000, -10000);

	// When reordering ships, the names of ships being moved are displayed alongside the cursor.
	bool isDragging = false;
};



#endif
