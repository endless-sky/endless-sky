/* GamepadPanel.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GAMEPAD_PANEL_H_
#define GAMEPAD_PANEL_H_

#include "Dropdown.h"
#include "Panel.h"

#include "ClickZone.h"
#include "InfoPanelState.h"
#include "Point.h"
#include "ShipInfoDisplay.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Color;
class Outfit;
class PlayerInfo;
class Rectangle;



// This panel allows the user to configure their gamepads
class GamepadPanel : public Panel {
public:
	explicit GamepadPanel();

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool ControllersChanged() override;
	virtual bool ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive) override;
	virtual bool ControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive) override;
	virtual bool ControllerButtonDown(SDL_GameControllerButton button) override;
	virtual bool ControllerButtonUp(SDL_GameControllerButton button) override;

	void UpdateUserMessage();

private:
	std::shared_ptr<Dropdown> gamepadList;
	std::shared_ptr<Dropdown> deadZoneList;
	std::shared_ptr<Dropdown> triggerThresholdList;

	bool reloadGamepad = true;
	bool startRemap = false;

	// If this is -1, then we are processing input normally. Otherwise, this
	// represents the index of the input that we want to change on the
	// controller
	int remapIdx = -1;

	class Interface const * const ui;

	std::string userMessage;
	bool mapping_saved = false;
};



#endif
