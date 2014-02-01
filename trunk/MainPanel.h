/* MainPanel.h
Michael Zahniser, 5 Nov 2013

Class representing the main panel (i.e. the view of your ship moving around).
*/

#ifndef MAIN_PANEL_H_INCLUDED
#define MAIN_PANEL_H_INCLUDED

#include "Panel.h"

#include "AsteroidField.h"
#include "Date.h"
#include "DrawList.h"
#include "Engine.h"
#include "Information.h"
#include "KeyStatus.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"

class GameData;



class MainPanel : public Panel {
public:
	MainPanel(const GameData &gameData);
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
	const GameData &gameData;
	
	Engine engine;
	
	mutable double load;
	mutable double loadSum;
	mutable int loadCount;
};



#endif
