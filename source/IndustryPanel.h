/* IndustryPanel.h
Copyright (c) 2026 by Endless Sky Expanded contributors

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

class Facility;
class Planet;
class PlayerInfo;



// Planet screen panel for building industrial facilities and collecting the
// goods they have produced. Drawn in the planet description area, like the bank.
class IndustryPanel : public Panel {
public:
	IndustryPanel(PlayerInfo &player, const Planet &planet);

	// Check whether there is anything to show on this planet: either a facility
	// that can be built here, or one that the player already owns here.
	static bool HasIndustry(const PlayerInfo &player, const Planet &planet);

	virtual void Draw() override;


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;


private:
	// The facility types shown on this planet, in display order.
	static std::vector<const Facility *> Facilities(const PlayerInfo &player, const Planet &planet);
	// Build the facility if it isn't owned yet, otherwise collect its stockpile.
	void DoAction(const Facility &facility);


private:
	PlayerInfo &player;
	const Planet &planet;
	int selectedRow = 0;
	std::string status;
};
