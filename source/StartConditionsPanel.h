/* StartConditionsPanel.h
Copyright (c) 2020-2021 by FranchuFranchu <fff999abc999@gmail.com>
Copyright (c) 2021 by Benjamin Hauch

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

#include "ClickZone.h"
#include "Color.h"
#include "Information.h"
#include "Point.h"
#include "Rectangle.h"
#include "text/WrappedText.h"

#include <vector>

class PlayerInfo;
class StartConditions;
class UI;



class StartConditionsPanel : public Panel {
	using StartConditionsList = std::vector<StartConditions>;
public:
	StartConditionsPanel(PlayerInfo &player, UI &gamePanels, const StartConditionsList &allScenarios, const Panel *parent);

	virtual void Draw() override final;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override final;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override final;
	virtual bool Hover(int x, int y) override final;
	virtual bool Drag(double dx, double dy) override final;
	virtual bool Scroll(double dx, double dy) override final;


private:
	void OnConversationEnd(int);
	void ScrollToSelected();
	void Select(StartConditionsList::iterator it);


private:
	PlayerInfo &player;
	UI &gamePanels;
	// The panel to close when a scenario is chosen.
	const Panel *parent;
	// The list of starting scenarios to pick from.
	StartConditionsList scenarios;
	// The currently selected starting scenario.
	StartConditionsList::iterator startIt;
	// Colors with which to draw text.
	const Color &bright;
	const Color &medium;
	const Color &selectedBackground;
	// The selected scenario's description.
	WrappedText description;
	// Displayed information for the selected scenario.
	Information info;

	bool hasHover = false;
	Point hoverPoint;

	double entriesScroll = 0.;
	double descriptionScroll = 0.;

	// This is a map that will let us figure out which start conditions item the user clicked on.
	std::vector<ClickZone<StartConditionsList::iterator>> startConditionsClickZones;

	// Interface-controlled positions & dimensions.
	Rectangle descriptionBox;
	Rectangle entryBox;
	Rectangle entriesContainer;
	Point entryTextPadding;
};
