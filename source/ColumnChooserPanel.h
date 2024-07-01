/* ColumnChooserPanel.h
Copyright (c) 2024 by Endless Sky contributors

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

#include "PlayerInfoPanel.h"

#include <vector>

class PlayerInfo;



// This panel represents a pop-up menu containing checkboxes to show/hide table columns.
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


private:
	std::vector<PlayerInfoPanel::SortableColumn> columns;
	InfoPanelState *panelState;

	std::vector<ClickZone<std::string>> zones;
	Point hoverPoint = Point(-10000, -10000);
};



#endif
