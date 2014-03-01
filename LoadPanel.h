/* LoadPanel.h
Michael Zahniser, 26 Feb 2014

UI panel for loading and saving games.
*/

#ifndef LOAD_PANEL_H_
#define LOAD_PANEL_H_

#include "Panel.h"

#include "PlayerInfo.h"

#include <map>
#include <string>
#include <vector>

class GameData;



class LoadPanel : public Panel {
public:
	LoadPanel(const GameData &data, PlayerInfo &player, UI &gamePanels);
	
	virtual void Draw() const;
	
	// New player "conversation" callback.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
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
