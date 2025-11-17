/* LogbookPanel.h
Copyright (c) 2017 by Michael Zahniser

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

#include "BookEntry.h"
#include "Date.h"

#include <map>
#include <string>
#include <vector>

class PlayerInfo;



// User interface panel that displays a conversation, allowing you to make
// choices. If a callback function is given, that function will be called when
// the panel closes, to report the outcome of the conversation.
class LogbookPanel : public Panel {
public:
	explicit LogbookPanel(PlayerInfo &player);

	// Draw this panel.
	virtual void Draw() override;


protected:
	// Event handlers.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;


private:
	void Update(bool selectLast = true);


private:
	// Reference to the player, to apply any changes to them.
	PlayerInfo &player;

	// Current month being displayed:
	Date selectedDate;
	std::string selectedName;
	std::map<Date, BookEntry>::const_iterator begin;
	std::map<Date, BookEntry>::const_iterator end;
	// Other months available for display:
	std::vector<std::string> contents;
	std::vector<Date> dates;

	Point hoverPoint;

	// Current scroll:
	double categoryScroll = 0.;
	double scroll = 0.;
	mutable double maxCategoryScroll = 0.;
	mutable double maxScroll = 0.;
};
