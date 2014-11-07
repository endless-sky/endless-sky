/* PreferencesPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PREFERENCES_PANEL_H_
#define PREFERENCES_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "Key.h"

#include <string>
#include <vector>



// UI panel for editing preferences, especially the key mappings.
class PreferencesPanel : public Panel {
public:
	PreferencesPanel();
	
	// Draw this panel.
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	
	
private:
	void Exit();
	
	
private:
	int editing;
	int selected;
	
	mutable std::vector<ClickZone<Key::Command>> zones;
	mutable std::vector<ClickZone<std::string>> prefZones;
};



#endif
