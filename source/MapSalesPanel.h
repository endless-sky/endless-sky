/* MapSalesPanel.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_SALES_PANEL_H_
#define MAP_SALES_PANEL_H_

#include "MapPanel.h"

#include "ClickZone.h"

#include <map>
#include <string>
#include <vector>

class ItemInfoDisplay;
class PlayerInfo;
class Point;
class Sprite;



// Base class for the maps os shipyards and outfitters.
class MapSalesPanel : public MapPanel {
public:
	MapSalesPanel(PlayerInfo &player, bool isOutfitters);
	MapSalesPanel(const MapPanel &panel, bool isOutfitters);
	
	void Draw();
	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command);
	bool Click(int x, int y);
	bool Hover(int x, int y);
	bool Drag(double dx, double dy);
	bool Scroll(double dx, double dy);
	double SystemValue(const System *system) const;
	
	
protected:
	virtual const Sprite *SelectedSprite() const = 0;
	virtual const Sprite *CompareSprite() const = 0;
	virtual const ItemInfoDisplay &SelectedInfo() const = 0;
	virtual const ItemInfoDisplay &CompareInfo() const = 0;

	virtual void Select(int index) = 0;
	virtual void Compare(int index) = 0;
	virtual bool HasAny(const Planet *planet) const = 0;
	virtual bool HasThis(const Planet *planet) const = 0;
	virtual int FindItem(const std::string &text) const = 0;
	
	virtual void DrawItems() const = 0;
	
	void DrawKey() const;
	void DrawPanel() const;
	void DrawButtons();
	void DrawInfo() const;

	bool DrawHeader(Point &corner, const std::string &category) const;
	void DrawSprite(const Point &corner, const Sprite *sprite) const;
	void Draw(Point &corner, const Sprite *sprite, bool isForSale, bool isSelected,
		const std::string &name, const std::string &price, const std::string &info) const;
	
	void DoFind(const std::string &text);
	void ScrollTo(int index);
	
	
protected:
	static const double ICON_HEIGHT;
	static const double PAD;
	static const int WIDTH;
	
	
protected:
	double scroll = 0.;
	mutable double maxScroll = 0.;

	const std::vector<std::string> &categories;
	
	
private:
	bool isDragging = false;
	bool isOutfitters = false;
	
	mutable bool hidPrevious = true;
	std::map<std::string, bool> hideCategory;
	mutable std::vector<ClickZone<std::string>> categoryZones;
	
	mutable std::vector<ClickZone<int>> zones;
	int selected = -1;
	int compare = -1;
	
	int swizzle = 0;
};



#endif
