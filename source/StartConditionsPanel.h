/* StartConditionsPanel.h
Copyright (c) 2020 by FranchuFranchu <fff999abc999@gmail.com>

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef START_CONDITIONS_PANEL_H_
#define START_CONDITIONS_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "Color.h"
#include "text/WrappedText.h"

class PlayerInfo;
class StartConditions;
class UI;

class StartConditionsPanel : public Panel {
public:
	StartConditionsPanel(PlayerInfo &player, UI &gamePanels, Panel *parent);
	void OnCallback(int);
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	// This takes in the amount of pixels to be scrolled, and returns the amount of entries that were actually scrolled.
	double DoScrolling(int scrollAmount);


private:
	PlayerInfo &player;
	UI &gamePanels;
	
	WrappedText descriptionWrappedText;
	
	const StartConditions *chosenStart = nullptr;
	std::vector<StartConditions>::const_iterator chosenStartIterator;
	
	// Stored here so that we can remove it if the player chooses a scenario.
	Panel *parent = nullptr; 
	Point hoverPoint;
	
	double listScroll = 0;
	double descriptionScroll = 0;
	
	// This is a map that will let us figure out which start conditions item the user clicked on.
	std::vector<ClickZone<std::vector<StartConditions>::const_iterator>> startConditionsClickZones;
	
	Rectangle descriptionBox;
	Rectangle entryBox;
	Rectangle entryListBox;
	Rectangle entryInternalBox;
	
	Color bright;
	Color medium;
	Color selectedBackground;
};

#endif
