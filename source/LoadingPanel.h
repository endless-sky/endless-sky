/* LoadingPanel.h
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LOADING_PANEL_H_
#define LOADING_PANEL_H_

#include "Panel.h"

#include <string>
#include <functional>
#include <vector>

class PlayerInfo;
class UI;



// Class representing the loading menu, which is shown when loading resources
// (like game data and save files).
class LoadingPanel final : public Panel {
public:
	LoadingPanel(PlayerInfo &player, UI &gamePanels);

	void Step() final;
	void Draw() final;


private:
	PlayerInfo &player;
	UI &gamePanels;

	// Used to draw the loading circle.
	int progress = 0;
};



#endif
