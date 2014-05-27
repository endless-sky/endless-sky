/* OutfitterPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFITTER_PANEL_H_
#define OUTFITTER_PANEL_H_

#include "Panel.h"
#include "OutfitInfoDisplay.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class GameData;
class Outfit;
class Planet;
class PlayerInfo;
class Ship;



// Class representing the Outfitter UI panel, which allows you to buy new
// outfits to install in your ship or to sell the ones you own.
class OutfitterPanel : public Panel {
public:
	OutfitterPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	virtual bool Scroll(int x, int y, int dy);
	
	
private:
	class ClickZone {
	public:
		ClickZone(int x, int y, int rx, int ry, Ship *ship);
		ClickZone(int x, int y, int rx, int ry, const Outfit *outfit);
		
		bool Contains(int x, int y) const;
		Ship *GetShip() const;
		const Outfit *GetOutfit() const;
		
	private:
		int left;
		int top;
		int right;
		int bottom;
		
		Ship *ship;
		const Outfit *outfit;
	};
	
	
private:
	bool FlightCheck();
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const Planet *planet;
	
	Ship *playerShip;
	const Outfit *selectedOutfit;
	
	OutfitInfoDisplay outfitInfo;
	ShipInfoDisplay shipInfo;
	
	int mainScroll;
	int sideScroll;
	mutable int maxMainScroll;
	mutable int maxSideScroll;
	bool dragMain;
	
	mutable std::vector<ClickZone> zones;
	
	std::map<std::string, std::set<std::string>> catalog;
	// This is how many of each outfit we have sold to this particular outfitter
	// in this particular shopping session (so that you can buy back things this
	// outfitter does not normally carry that you sold by accident).
	mutable std::map<const Outfit *, int> available;
};


#endif
