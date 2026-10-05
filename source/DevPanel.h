/* DevPanel.h
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

#include <functional>
#include <string>
#include <vector>

class PlayerInfo;



// Playtesting cheat menu, opened with ` or F12 on the planet screen or in flight.
// Lets the tester skip days or add credits without playing for them.
class DevPanel : public Panel {
public:
	explicit DevPanel(PlayerInfo &player);

	virtual void Draw() override;


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Hover(int x, int y) override;


private:
	struct Entry {
		std::string label;
		std::function<void()> action;
	};

	void AdvanceDays(int days);
	void AddCredits(int64_t amount);
	void AddCustomCredits(int amount);
	void Run(size_t index);


private:
	PlayerInfo &player;
	std::vector<Entry> entries;
	int hoverIndex = -1;
	std::string lastResult;
};
