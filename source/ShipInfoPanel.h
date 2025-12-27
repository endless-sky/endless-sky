/* ShipInfoPanel.h
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

#pragma once

#include "Panel.h"

#include "ClickZone.h"
#include "InfoPanelState.h"
#include "Point.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Color;
class Outfit;
class PlayerInfo;
class Rectangle;



// This panel displays detailed information about one of the player's ships. If
// they are landed on a planet, it also allows the player to change weapon
// hardpoints. In flight, this panel allows them to jettison cargo.
class ShipInfoPanel : public Panel {
public:
	explicit ShipInfoPanel(PlayerInfo &player);
	explicit ShipInfoPanel(PlayerInfo &player, InfoPanelState state);
	virtual ~ShipInfoPanel() override;

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;


private:
	// Handle a change to what ship is shown.
	void UpdateInfo();
	void ClearZones();

	// Draw the ship tab (and its subsections).
	void DrawShipStats(const Rectangle &bounds);
	void DrawOutfits(const Rectangle &bounds, Rectangle &cargoBounds);
	void DrawWeapons(const Rectangle &bounds);
	void DrawCargo(const Rectangle &bounds);

	// Helper functions.
	void DrawLine(const Point &from, const Point &to, const Color &color) const;
	bool Hover(const Point &point);
	bool Rename(const std::string &name);
	bool CanDump() const;
	void Dump();
	void DumpPlunder(int count);
	void DumpCommodities(int count);
	void Disown();


private:
	PlayerInfo &player;
	// This is the currently selected ship.
	std::vector<std::shared_ptr<Ship>>::const_iterator shipIt;

	// Information about the currently selected ship.
	ShipInfoDisplay info;
	std::map<std::string, std::vector<const Outfit *>> outfits;

	// Track all the clickable parts of the UI (other than the buttons).
	std::vector<ClickZone<int>> zones;
	std::vector<ClickZone<std::string>> commodityZones;
	std::vector<ClickZone<const Outfit *>> plunderZones;
	// Keep track of which item the mouse is hovering over and which item is
	// currently being dragged.
	int hoverIndex = -1;
	int draggingIndex = -1;

	InfoPanelState panelState;

	// Track the current mouse location.
	Point hoverPoint;
	// Track whether a commodity or plundered outfit is selected to jettison.
	std::string selectedCommodity;
	const Outfit *selectedPlunder = nullptr;
};
