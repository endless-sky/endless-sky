/* PlayerInfoPanel.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PLAYER_INFO_PANEL_H_
#define PLAYER_INFO_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "GameData.h"
#include "InfoPanelState.h"
#include "Point.h"
#include "text/Table.h"

#include <map>
#include <memory>
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
	virtual bool AllowFastForward() const override;
	
	
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
	
	void SortShips(InfoPanelState::ShipComparator &shipComparator);
	
	class SortableColumn {
	public:
		SortableColumn(std::string name, double offset, double endX, Layout layout, InfoPanelState::ShipComparator *shipSort);
		
		std::string name;
		double offset = 0.;
		double endX = 0.;
		Layout layout;
		InfoPanelState::ShipComparator *shipSort = nullptr;
	};
	
private:
	PlayerInfo &player;
	
	static const SortableColumn columns[];
	
	InfoPanelState panelState;
	
	// Keep track of which column header is under the mouse.
	InfoPanelState::ShipComparator *hoverMenuPtr = nullptr;
	
	// Column headers that sort ships when clicked.
	std::vector<ClickZone<InfoPanelState::ShipComparator*>> menuZones;
	
	// Ships in the table.
	std::vector<ClickZone<int>> shipZones;
	
	// Keep track of which ship the mouse is hovering over.
	int hoverIndex = -1;
	int tableHoverIndex = -1; // TODO: track which line in current table view is hovered over with this
	Point hoverPoint;
	
	// When reordering ships, the names of ships being moved are displayed alongside the cursor.
	bool isDragging = false;
	
	// Colors of various table elements.
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &elsewhere = *GameData::Colors().Get("dim");
	const Color &dead = *GameData::Colors().Get("dead");
	const Color &flagship = *GameData::Colors().Get("flagship");
	const Color &disabled = *GameData::Colors().Get("disabled");
};



#endif
