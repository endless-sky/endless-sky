/* HailPanel.h
Copyright (c) 2025 by xobes

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

class Color;
class InfoTag;
class TextArea;



// This panel is shown when you hail a ship or planet. It allows you to ask for
// assistance from friendly ships, to bribe hostile ships to leave you alone, or
// to bribe a planet to allow you to land there.
class HelpOverlay : public Panel {
public:
	HelpOverlay(const std::string &name);

	virtual ~HelpOverlay() override;

	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;


private:
	std::string name;
};
