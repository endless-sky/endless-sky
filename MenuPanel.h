/* MenuPanel.h
Michael Zahniser, 5 Nov 2013

Class representing the main menu, which is shown before you enter a game or
when your player has been killed, ending the game.

TODO: There should be an option for a separate MenuPanel that is not full screen
and that can be called up, say, to change keyboard settings or to revert to a
previous saved game. There is no reason that could not be a separate instance of
this class, however.

The other option is to have the MainPanel be the bottom-most one, but that does
not make sense because the MenuPanel must tell the MainPanel what game to load.
*/

#ifndef MENU_PANEL_H_INCLUDED
#define MENU_PANEL_H_INCLUDED

#include "Panel.h"

#include "Interface.h"

class GameData;
class PlayerInfo;



class MenuPanel : public Panel {
public:
	MenuPanel(GameData &gameData, PlayerInfo &playerInfo);
	
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	GameData &gameData;
	PlayerInfo &playerInfo;
};



#endif
