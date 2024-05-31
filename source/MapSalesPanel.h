/* MapSalesPanel.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MAP_SALES_PANEL_H_
#define MAP_SALES_PANEL_H_

#include "MapPanel.h"

#include "CategoryList.h"
#include "ClickZone.h"
#include "Information.h"

#include <set>
#include <string>
#include <vector>

class ItemInfoDisplay;
class PlayerInfo;
class Point;
class Sprite;



// Base class for the maps of shipyards and outfitters.
class MapSalesPanel : public MapPanel {
public:
	MapSalesPanel(PlayerInfo &player, bool isOutfitters);
	MapSalesPanel(const MapPanel &panel, bool isOutfitters);

	virtual void Draw() override;


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;

	virtual const Sprite *SelectedSprite() const = 0;
	virtual const Sprite *CompareSprite() const = 0;
	virtual int SelectedSpriteSwizzle() const;
	virtual int CompareSpriteSwizzle() const;
	virtual const ItemInfoDisplay &SelectedInfo() const = 0;
	virtual const ItemInfoDisplay &CompareInfo() const = 0;
	virtual const std::string &KeyLabel(int index) const = 0;

	virtual void Select(int index) = 0;
	virtual void Compare(int index) = 0;
	virtual double SystemValue(const System *system) const override = 0;
	virtual int FindItem(const std::string &text) const = 0;

	virtual void DrawItems() = 0;

	void DrawKey(const std::string &buttonCondition) const;
	void DrawPanel() const;
	void DrawInfo() const;

	bool DrawHeader(Point &corner, const std::string &category);
	void DrawSprite(const Point &corner, const Sprite *sprite, int swizzle) const;
	void Draw(Point &corner, const Sprite *sprite, int swizzle, bool isForSale, bool isSelected,
		const std::string &name, const std::string &price, const std::string &info,
		const std::string &storage = "");

	void DoFind(const std::string &text);
	void ScrollTo(int index);
	void ClickCategory(const std::string &name);


protected:
	static const double ICON_HEIGHT;
	static const double PAD;
	static const int WIDTH;


protected:
	double scroll = 0.;
	double maxScroll = 0.;

	const CategoryList &categories;
	bool onlyShowSoldHere = false;
	bool onlyShowStorageHere = false;


private:
	bool isDragging = false;
	bool isOutfitters = false;

	bool hidPrevious = true;
	std::set<std::string> &collapsed;

	std::vector<ClickZone<int>> zones;
	int selected = -1;
	int compare = -1;
};



#endif
