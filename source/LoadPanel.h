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

#include "Point.h"
#include "SavedGame.h"

#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

class PlayerInfo;
class UI;



// UI panel for loading and saving games. The game is automatically saved when
// the player takes off from any planet, so if they want to be able to go back
// to a previous game state they must save a "snapshot" of that state.
class LoadPanel : public Panel {
public:
	LoadPanel(PlayerInfo &player, UI &gamePanels);
	
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	void UpdateLists();
	
	// Snapshot name callback.
	void SnapshotCallback(const std::string &name);
	void WriteSnapshot(const std::string &sourceFile, const std::string &snapshotName);
	// Load snapshot callback.
	void LoadCallback();
	// Delete callbacks.
	void DeletePilot();
	void DeleteSave();
	
	
private:
	PlayerInfo &player;
	SavedGame loadedInfo;
	UI &gamePanels;
	
	std::map<std::string, std::vector<std::pair<std::string, std::time_t>>> files;
	std::string selectedPilot;
	std::string selectedFile;
	// If the player enters a filename that exists, prompt before overwriting it.
	std::string nameToConfirm;
	
	Point hoverPoint;
	int hoverCount = 0;
	bool hasHover = false;
	bool sideHasFocus = false;
	double sideScroll = 0;
	double centerScroll = 0;
};



#endif
