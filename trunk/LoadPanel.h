/* LoadPanel.h
Michael Zahniser, 26 Feb 2014

UI panel for loading and saving games.
*/

#ifndef LOAD_PANEL_H_
#define LOAD_PANEL_H_

#include "Panel.h"

#include "PlayerInfo.h"

class GameData;



class LoadPanel : public Panel {
public:
	LoadPanel(const GameData &data, PlayerInfo &player, const Panel *parent);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const Panel *parent;
	
	PlayerInfo loadedInfo;
};



#endif
