/* HiringPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HIRING_PANEL_H_
#define HIRING_PANEL_H_

#include "Panel.h"

#include "PlayerInfo.h"



// Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
// buying and selling.
class HiringPanel : public Panel {
public:
	HiringPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	
	
private:
	static int Modifier();
	
	
private:
	const GameData &data;
	PlayerInfo &player;
};



#endif
