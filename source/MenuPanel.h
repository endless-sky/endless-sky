/* MenuPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Panel.h"

#include <string>
#include <vector>

class Interface;
class PlayerInfo;
class UI;



// Class representing the main menu, which is shown before you enter a game or
// when you hit "escape" to return here. This includes a scrolling list of
// credits and basic information on the currently loaded player.
class MenuPanel : public Panel {
public:
	MenuPanel(PlayerInfo &player, UI &gamePanels);
	virtual ~MenuPanel();

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;


private:
	void DrawCredits() const;


private:
	PlayerInfo &player;
	UI &gamePanels;

	const Interface *mainMenuUi;

	// When the menu panel is closed, return the starfield to this position.
	Point returnPos;
	double animation = 0.;
	double xSpeed = 0.;
	double ySpeed = 0.;
	double yAmplitude = 0.;

	std::vector<std::string> credits;
	long long int scroll = 0;
	bool scrollingPaused = false;
};
