/* MainPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAIN_PANEL_H_
#define MAIN_PANEL_H_

#include "Panel.h"

#include "Engine.h"

class PlayerInfo;
class ShipEvent;



// Class representing the main panel (i.e. the view of your ship moving around).
class MainPanel : public Panel {
public:
	MainPanel(PlayerInfo &player);
	
	virtual void Step(bool isActive);
	virtual void Draw() const;
	
	// The planet panel calls this when it closes.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	
	
private:
	void FinishNormalMissions();
	void FinishSpecialMissions();
	void ShowScanDialog(const ShipEvent &event);
	
	
private:
	PlayerInfo &player;
	
	Engine engine;
	
	mutable double load;
	mutable double loadSum;
	mutable int loadCount;
};



#endif
