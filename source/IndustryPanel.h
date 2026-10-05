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



// Planet screen panel for building industrial facilities, supplying them with
// inputs and collecting what they produce. Drawn in the planet description
// area, like the bank. Tab switches to an overview of all the player's holdings.
class IndustryPanel : public Panel {
public:
	IndustryPanel(PlayerInfo &player, const Planet &planet);

	// Check whether the Industry button should be shown on this planet: there is
	// a facility that can be built or is owned here, or the player owns
	// facilities elsewhere that they may want to review.
	static bool IsAvailable(const PlayerInfo &player, const Planet &planet);

	virtual void Draw() override;


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	// The facility types that can be built or are owned on this planet.
	static std::vector<const Facility *> Facilities(const PlayerInfo &player, const Planet &planet);
	const Facility *Selected() const;

	void DrawPlanetView();
	void DrawOverview();

	// Actions on the selected facility.
	void Build();
	void Supply();
	void Collect();
	void ToggleAutoSell();
	bool CanAutoSell() const;


private:
	PlayerInfo &player;
	const Planet &planet;
	bool showOverview = false;
	int selectedRow = 0;
	int scroll = 0;
	std::string status;
};
