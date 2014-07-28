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

#include "ShipInfoDisplay.h"

#include <map>
#include <string>
#include <vector>

class Outfit;
class PlayerInfo;



// This panel displays detailed information about the player's fleet and each of
// the ships in it.
class InfoPanel : public Panel {
public:
	InfoPanel(PlayerInfo &player);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod) override;
	virtual bool Click(int x, int y) override;
	
	
private:
	void UpdateInfo();
	
	
private:
	PlayerInfo &player;
	std::vector<std::shared_ptr<Ship>>::const_iterator shipIt;
	
	ShipInfoDisplay info;
	std::map<std::string, std::vector<const Outfit *>> outfits;
};



#endif
