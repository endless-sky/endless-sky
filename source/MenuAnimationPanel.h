/* MenuAnimationPanel.h
Copyright (c) 2022 by Michael Zahniser

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



// Class representing the menu animation including sound effects and music
// that appears when the game is started and everything is loaded.
class MenuAnimationPanel final : public Panel {
public:
	MenuAnimationPanel();

	void Step() final;
	void Draw() final;


private:
	float alpha = 1.f;
};
