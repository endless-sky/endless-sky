/* InfoPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef INFO_PANEL_H_
#define INFO_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Outfit;
class PlayerInfo;



// This panel displays detailed information about the player's fleet and each of
// the ships in it. If the player is landed on a planet, this panel also allows
// them to reorder the ships in their fleet (including changing which one is the
// flagship) and to shift the weapons on any of their ships to different
// hardpoints.
class InfoPanel : public Panel {
public:
	InfoPanel(PlayerInfo &player, bool showFlagship = false);
	
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	void UpdateInfo();
	void DrawInfo() const;
	void DrawShip() const;
	void DrawWeapon(int index, const Point &pos, const Point &hardpoint) const;
	void Rename(const std::string &name);
	bool CanDump() const;
	void Dump();
	void DumpPlunder(int count);
	bool Hover(double x, double y);
	
	
private:
	PlayerInfo &player;
	std::vector<std::shared_ptr<Ship>>::const_iterator shipIt;
	
	ShipInfoDisplay info;
	std::map<std::string, std::vector<const Outfit *>> outfits;
	
	mutable std::vector<ClickZone<int>> zones;
	mutable std::vector<ClickZone<std::string>> commodityZones;
	mutable std::vector<ClickZone<const Outfit *>> plunderZones;
	int selected = -1;
	int previousSelected = -1;
	std::set<int> allSelected;
	int hover = -1;
	double scroll = 0.;
	Point hoverPoint;
	Point dragStart;
	bool showShip = false;
	bool canEdit = false;
	bool didDrag = false;
	std::string selectedCommodity;
	const Outfit *selectedPlunder = nullptr;
};



#endif
