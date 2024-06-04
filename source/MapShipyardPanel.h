/* MapShipyardPanel.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MAP_SHIPYARD_PANEL_H_
#define MAP_SHIPYARD_PANEL_H_

#include "MapSalesPanel.h"

#include "ShipInfoDisplay.h"

#include <vector>

class PlayerInfo;
class Ship;
class Sprite;



// A panel that displays the galaxy star map, along with a side panel showing
// all ships that are for sale in known systems. You can click on one of them
// to see which systems it is available in.
class MapShipyardPanel : public MapSalesPanel {
public:
	explicit MapShipyardPanel(PlayerInfo &player);
	explicit MapShipyardPanel(const MapPanel &panel, bool onlyHere = false);


protected:
	virtual const Sprite *SelectedSprite() const override;
	virtual const Sprite *CompareSprite() const override;
	virtual int SelectedSpriteSwizzle() const override;
	virtual int CompareSpriteSwizzle() const override;
	virtual const ItemInfoDisplay &SelectedInfo() const override;
	virtual const ItemInfoDisplay &CompareInfo() const override;
	virtual const std::string &KeyLabel(int index) const override;

	virtual void Select(int index) override;
	virtual void Compare(int index) override;
	virtual double SystemValue(const System *system) const override;
	virtual int FindItem(const std::string &text) const override;

	virtual void DrawKey(Information &info) const override;
	virtual void DrawItems() override;


private:
	void Init();


private:
	std::map<std::string, std::vector<const Ship *>> catalog;
	std::vector<const Ship *> list;
	std::map<const System *, std::map<const Ship *, int>> parkedShips;

	const Ship *selected = nullptr;
	const Ship *compare = nullptr;

	ShipInfoDisplay selectedInfo;
	ShipInfoDisplay compareInfo;
};



#endif
