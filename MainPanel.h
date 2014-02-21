/* MainPanel.h
Michael Zahniser, 5 Nov 2013

Class representing the main panel (i.e. the view of your ship moving around).
*/

#ifndef MAIN_PANEL_H_INCLUDED
#define MAIN_PANEL_H_INCLUDED

#include "Panel.h"

#include "Engine.h"

class GameData;
class PlayerInfo;



class MainPanel : public Panel {
public:
	MainPanel(GameData &gameData, PlayerInfo &playerInfo);
	virtual ~MainPanel() {}
	
	virtual void Step(bool isActive);
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	
	
private:
	GameData &gameData;
	PlayerInfo &playerInfo;
	
	Engine engine;
	
	mutable double load;
	mutable double loadSum;
	mutable int loadCount;
};



#endif
