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
	MainPanel(const GameData &gameData, PlayerInfo &playerInfo);
	
	virtual void Step(bool isActive);
	virtual void Draw() const;
	
	// The planet panel calls this when it closes.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	
	
private:
	const GameData &gameData;
	PlayerInfo &playerInfo;
	
	Engine engine;
	
	mutable double load;
	mutable double loadSum;
	mutable int loadCount;
};



#endif
