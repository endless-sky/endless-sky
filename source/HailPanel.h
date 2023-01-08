/* HailPanel.h
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

#ifndef HAIL_PANEL_H_
#define HAIL_PANEL_H_

#include "Panel.h"

#include "Angle.h"

#include <string>

class Ship;
class Sprite;



// This panel is shown when a ship hails another that involves a player's ships.
class HailPanel : public Panel {
public:
	HailPanel();


protected:
	void DrawHail();
	void DrawIcon(const Ship &ship);
	void DrawIcon(const Sprite &sprite, Angle facing);


protected:
	std::string header;
	std::string message;
};



#endif
