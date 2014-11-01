/* BankPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef BANK_PANEL_H_
#define BANK_PANEL_H_

#include "Panel.h"

#include "PlayerInfo.h"



// This is an overlay drawn on top of the PlanetPanel when the player clicks on
// the "bank" button. It shows the player's mortgages and other expenses, and
// allows them to apply for new mortgages or pay extra on existing debts.
class BankPanel : public Panel {
public:
	BankPanel(PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	
	
private:
	void PayExtra(int amount);
	void NewMortgage(int amount);
	
	
private:
	PlayerInfo &player;
	int64_t qualify;
	int selectedRow;
};



#endif
