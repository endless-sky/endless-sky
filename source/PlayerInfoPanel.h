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
	
	
private:
	PlayerInfo &player;
	
	std::vector<ClickZone<int>> zones;
	// Keep track of which ship the mouse is hovering over, which ship was most
	// recently selected, which ship is currently being dragged, and all ships
	// that are currently selected.
	int hoverIndex = -1;
	int selectedIndex = -1;
	std::set<int> allSelected;
	// This is the index of the ship at the top of the fleet listing.
	int scroll = 0;
	Point hoverPoint;
	// When the player is landed, they are able to change their flagship and reorder their fleet.
	bool canEdit = false;
	// When reordering ships, the names of ships being moved are displayed alongside the cursor.
	bool isDragging = false;
};



#endif
