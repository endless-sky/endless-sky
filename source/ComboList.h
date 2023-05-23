/* ComboList.h
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef COMBO_LIST_H_
#define COMBO_LIST_H_

#include "ClickZone.h"
#include "Panel.h"
#include "text/alignment.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

class Point;
class Rectangle;

// A ComboList is a UI element that offers a list of buttons
// that each have a function when clicked.
class ComboList : public Panel {
public:
	typedef std::function<void()> callback;

public:
	// Constructor.
	// The box is the position and size of the initial label, which copies of will be drawn above, or below if space isn't available.
	// The list's elements are string and callback pair.
	// Justified alignment does nothing, defaults to centered.
	// Dim backgrounds dims the background when enabled.
	// Padding does not have any effect when center-aligned.
	ComboList(Rectangle box, const std::vector<std::pair<std::string, callback>> &listElements, Alignment alignment = Alignment::CENTER,
		bool dimBackground = false, int padding = 2, int initialIndex = 0);

	virtual void Draw() override;

	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Hover(int x, int y) override;

	inline void SetBackgroundDimming(bool dim) { dimBackground = dim; };
	inline bool IsBackgroundDimming() { return dimBackground; };

private:
	std::vector<std::pair<std::string, callback>> elements;
	std::vector<ClickZone<int>> zones;

	bool facingUp;

	int currentIndex;
	Rectangle rect;
	Alignment alignment;
	int padding;
	bool dimBackground;

private:
	void Close();

};



#endif
