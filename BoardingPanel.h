/* BoardingPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef BOARDING_PANEL_H_
#define BOARDING_PANEL_H_

#include "Panel.h"

#include "CaptureOdds.h"

#include <string>
#include <vector>

class Outfit;
class PlayerInfo;
class Ship;



// This panel is displayed whenever your flagship boards another ship, to give
// you a choice of what to plunder or whether to attempt to capture it.
class BoardingPanel : public Panel {
public:
	BoardingPanel(PlayerInfo &player, const std::shared_ptr<Ship> &victim);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	virtual bool Scroll(int dx, int dy);
	
	
private:
	bool CanExit() const;
	bool CanTake(int index = -1) const;
	bool CanCapture() const;
	bool CanAttack() const;
	
	
private:
	class Plunder {
	public:
		Plunder(const std::string &commodity, int count, int unitValue);
		Plunder(const Outfit *outfit, int count);
		
		// Sort by value per ton of mass.
		bool operator<(const Plunder &other) const;
		
		// Check how many of this item are left un-plundered. Once this is zero,
		// the item can be removed from the list.
		int Count() const;
		
		// Get the name of this item. If it is a commodity, this is its name.
		const std::string &Name() const;
		// Get the mass, in the format "<count> x <unit mass>". If this is a
		// commodity, no unit mass is given (because it is 1). If the count is
		// 1, only the unit mass is reported.
		const std::string &Size() const;
		// Get the total value (unit value times count) as a string.
		const std::string &Value() const;
		
		// If this is an outfit, get the outfit. Otherwise, this returns null.
		const Outfit *GetOutfit() const;
		// Find out how many of these I can take if I have this amount of cargo
		// space free.
		int CanTake(int freeSpace) const;
		// Take some or all of this plunder item.
		void Take(int count);
		
	private:
		void UpdateStrings();
		double UnitMass() const;
		
	private:
		std::string name;
		const Outfit *outfit;
		int count;
		int unitValue;
		std::string size;
		std::string value;
	};
	
private:
	PlayerInfo &player;
	std::shared_ptr<Ship> you;
	std::shared_ptr<Ship> victim;
	
	std::vector<Plunder> plunder;
	int selected;
	int scroll;
	
	bool isCapturing;
	CaptureOdds attackOdds;
	CaptureOdds defenseOdds;
	std::vector<std::string> messages;
};



#endif
