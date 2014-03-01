/* MenuPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MENU_PANEL_H_
#define MENU_PANEL_H_

#include "Panel.h"

#include <string>
#include <vector>

class GameData;
class PlayerInfo;



// Class representing the main menu, which is shown before you enter a game or
// when your player has been killed, ending the game.
class MenuPanel : public Panel {
public:
	MenuPanel(GameData &gameData, PlayerInfo &playerInfo, UI &mainUI);
	
	virtual void Step(bool isActive);
	virtual void Draw() const;
	
	// New player "conversation" callback.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	GameData &gameData;
	PlayerInfo &playerInfo;
	UI &gamePanels;
	
	std::vector<std::string> credits;
	unsigned scroll;
};



#endif
