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

#include <memory>
#include <string>
#include <vector>

class Outfit;
class PlayerInfo;
class Ship;



// This panel is displayed whenever your flagship boards another ship, to give
// you a choice of what to plunder or whether to attempt to capture it. The
// items you can plunder are shown in a list sorted by value per ton. This also
// handles the crew "bonus" that must be paid if you try to capture a ship, to
// compensate for them risking their lives. (The bonus is needed to keep ship
// capture from being so lucrative that the player can almost immediately have
// ridiculous amounts of money as soon as they get a ship that is capable of
// easily capturing other ships. It also serves as a penalty for capturing ships
// and then failing to protect them.
class BoardingPanel : public Panel {
public:
	BoardingPanel(PlayerInfo &player, const std::shared_ptr<Ship> &victim);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	bool CanExit() const;
	bool CanTake(int index = -1) const;
	bool CanCapture() const;
	bool CanAttack() const;
	
	
private:
	class Plunder {
	public:
		Plunder(const std::string &commodity, int count, int unitValue);
		Plunder(const Outfit *outfit, int count, int age);
		
		// Sort by value per ton of mass.
		bool operator<(const Plunder &other) const;
		
		// Check how many of this item are left un-plundered. Once this is zero,
		// the item can be removed from the list.
		int Count() const;
		// Get the value of each unit of this plunder item.
		int64_t UnitValue() const;
		
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
		int64_t unitValue;
		std::string size;
		std::string value;
	};
	
private:
	PlayerInfo &player;
	std::shared_ptr<Ship> you;
	std::shared_ptr<Ship> victim;
	
	std::vector<Plunder> plunder;
	int selected = 0;
	double scroll = 0.;
	
	bool playerDied = false;
	bool isCapturing = false;
	CaptureOdds attackOdds;
	CaptureOdds defenseOdds;
	std::vector<std::string> messages;
	
	int64_t initialCrew;
	int64_t casualties = 0;
	int64_t crewBonus = 0;
};



#endif
