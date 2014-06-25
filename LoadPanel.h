/* LoadPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LOAD_PANEL_H_
#define LOAD_PANEL_H_

#include "Panel.h"

#include "PlayerInfo.h"

#include <map>
#include <string>
#include <vector>

class GameData;



// UI panel for loading and saving games.
class LoadPanel : public Panel {
public:
	LoadPanel(const GameData &data, PlayerInfo &player, UI &gamePanels);
	
	virtual void Draw() const;
	
	// New player "conversation" callback.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	
	
private:
	void UpdateLists();
	void DeletePilot();
	void DeleteSave();
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	UI &gamePanels;
	
	std::map<std::string, std::vector<std::string>> files;
	std::string selectedPilot;
	std::string selectedFile;
	
	PlayerInfo loadedInfo;
};



#endif
