/* StartConditionsPanel.h
Copyright (c) 2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef START_CONDITIONS_PANEL_H_
#define START_CONDITIONS_PANEL_H_

#include "LoadPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"

class StartConditionsPanel : public Panel {
public:
    StartConditionsPanel(PlayerInfo &player, UI &gamePanels, LoadPanel *LoadPanel);
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
private:
    PlayerInfo &player;
    UI &gamePanels;
    StartConditions *chosenStart;

    // Stored here  that we can remove it if the player chooses a scenario
    LoadPanel *loadPanel; 
    Point hoverPoint;

    double listScroll;
    double descriptionScroll;
};

#endif