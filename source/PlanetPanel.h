/* PlanetPanel.h
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

#ifndef PLANET_PANEL_H_
#define PLANET_PANEL_H_

#include "Panel.h"

#include "Ship.h"
#include "text/WrappedText.h"

#include <functional>
#include <memory>

class Interface;
class Planet;
class PlayerInfo;
class SpaceportPanel;
class System;



// Dialog that pops up when you land on a planet. The shipyard and outfitter are
// shown in full-screen panels that pop up above this one, but the remaining views
// (trading, jobs, bank, port, and crew) are displayed within this dialog.
class PlanetPanel : public Panel {
public:
	PlanetPanel(PlayerInfo &player, std::function<void()> callback);

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;


private:
	void TakeOffIfReady();
	void CheckWarningsAndTakeOff();
	void TakeOff();


private:
	PlayerInfo &player;
	std::function<void()> callback = nullptr;
	bool requestedLaunch = false;

	const Planet &planet;
	const System &system;
	const Interface &ui;

	std::shared_ptr<Panel> trading;
	std::shared_ptr<Panel> bank;
	std::shared_ptr<SpaceportPanel> spaceport;
	std::shared_ptr<Panel> hiring;
	Panel *selectedPanel = nullptr;

	WrappedText text;

	// Out of system (absent) ships that cannot fly for some reason.
	std::vector<std::shared_ptr<Ship>> absentCannotFly;

	// Cache flight checks to not calculate them twice before each takeoff.
	std::map<const std::shared_ptr<Ship>, std::vector<std::string>> flightChecks;
};



#endif
