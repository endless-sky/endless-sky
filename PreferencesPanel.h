/* PreferencesPanel.h
Michael Zahniser, 20 Feb 2014

UI panel for editing preferences, especially the key mappings.
*/

#ifndef PREFERENCES_PANEL_H_INCLUDED
#define PREFERENCES_PANEL_H_INCLUDED

#include "Panel.h"

class GameData;



class PreferencesPanel : public Panel {
public:
	PreferencesPanel(GameData &data);
	
	// Draw this panel.
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	void Exit();
	
	
private:
	GameData &data;
	
	int editing;
	int selected;
	mutable int firstY;
	mutable int buttonX;
	mutable int buttonY;
};



#endif
