/* MapOutfitterPanel.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_OUTFITTER_PANEL_H_
#define MAP_OUTFITTER_PANEL_H_

#include "MapPanel.h"

#include "ClickZone.h"

#include <map>
#include <string>
#include <vector>

class Outfit;
class PlayerInfo;



// A panel that displays the galaxy star map, along with a side panel showing
// all outfits that are for sale in known systems. You can click on one of them
// to see which systems it is available in.
class MapOutfitterPanel : public MapPanel {
public:
	MapOutfitterPanel(PlayerInfo &player);
	MapOutfitterPanel(const MapPanel &panel);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(int dx, int dy) override;
	virtual bool Scroll(int dx, int dy) override;

	virtual double SystemValue(const System *system) const override;	
	
	
private:
	void Init();
	void DrawPanel() const;
	void DrawItems() const;
	
	
private:
	std::map<std::string, std::vector<const Outfit *>> catalog;
	const Outfit *selected = nullptr;
	
	mutable std::vector<ClickZone<const Outfit *>> zones;
	
	int scroll = 0;
	mutable int maxScroll = 0;
	bool isDragging = false;
};



#endif
