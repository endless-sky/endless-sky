/* ColumnChooserPanel.cpp
Copyright (c) 2017 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef COLUMN_CHOOSER_PANEL_H_
#define COLUMN_CHOOSER_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "Color.h"
#include "text/layout.hpp"
#include "PlayerInfoPanel.h"
#include "Point.h"

#include <set>
#include <vector>

class PlayerInfo;



// This panel displays detailed information about the player and their fleet. If
// the player is landed on a planet, it also allows them to reorder the ships in
// their fleet (including changing which one is the flagship).
class ColumnChooserPanel : public Panel {
public:
	explicit ColumnChooserPanel(const std::vector<PlayerInfoPanel::SortableColumn> &columns, InfoPanelState *panelState);

	virtual void Draw() override;

	// The player info panel allow fast-forward to stay active.
	bool AllowsFastForward() const noexcept final;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	void DrawTooltip(const std::string &text, const Point &hoverPoint);

	// Handle mouse hover (also including hover during drag actions):
	bool Hover(const Point &point);
	// Adjust the scroll by the given amount. Return true if it changed.
	bool Scroll(int distance);
	// Try to scroll to the given position. Return true if position changed.
	bool ScrollAbsolute(int scroll);

private:
	std::vector<PlayerInfoPanel::SortableColumn> columns;
	InfoPanelState *panelState;

	std::vector<ClickZone<std::string>> zones;

	// Keep track of which toggle the mouse is hovering over.
	int hoverIndex = -1;

	// Initialize mouse point to something off-screen to not
	// make the game think the player is hovering on something.
	Point hoverPoint = Point(-10000, -10000);
};



#endif
