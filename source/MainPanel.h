/* MainPanel.h
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

#pragma once

#include "Panel.h"

#include "Command.h"
#include "Engine.h"

#include <list>

class PlayerInfo;
class ShipEvent;



// Class representing the main panel (i.e. the view of your ship moving around).
// The goal is that the Engine class will not need to know about displaying
// panels or handling key presses; it instead focuses just on the calculations
// needed to move the ships around and to figure out where they should be drawn.
class MainPanel : public Panel {
public:
	explicit MainPanel(PlayerInfo &player);

	virtual void Step() override;
	virtual void Draw() override;

	// The planet panel calls this when it closes.
	void OnCallback();
	// The hail panel calls this when it closes.
	void OnBribeCallback(const Government *bribed);

	// The main panel allows fast-forward.
	bool AllowsFastForward() const noexcept final;

	// Get the underlying game engine used by the game.
	Engine &GetEngine();


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	void ShowScanDialog(const ShipEvent &event);
	bool ShowHailPanel();
	bool ShowHelp(bool force);
	void StepEvents(bool &isActive);


private:
	PlayerInfo &player;

	Engine engine;

	// These are the pending ShipEvents that have yet to be processed.
	std::list<ShipEvent> eventQueue;
	bool handledFront = false;

	Command show;

	// Keep track of how long a starting player has spent drifting in deep space.
	int lostness = 0;
	int lostCount = 0;

	Point dragSource;
	Point dragPoint;
	bool isDragging = false;
	bool hasShift = false;
	bool hasControl = false;
	bool canClick = false;
	bool canDrag = false;
};
