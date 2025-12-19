/* Panel.h
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

#include "MouseButton.h"
#include "Rectangle.h"

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

class Command;
class Point;
class Sprite;
class TestContext;
class UI;



// Class representing a UI window (full screen or pop-up) which responds to user
// input and can draw itself. Everything displayed in the game is drawn in a
// Panel, and panels can stack on top of each other like "real" UI windows. By
// default, a panel allows the panels under it to show through, but does not
// allow them to receive any events that it does not know how to handle.
class Panel {
public:
	// Draw a sprite repeatedly to make a vertical edge.
	static void DrawEdgeSprite(const Sprite *edgeSprite, int posX);


public:
	// Make the destructor virtual just in case any derived class needs it.
	virtual ~Panel() = default;

	// Move the state of this panel forward one game step.
	virtual void Step();

	// Draw this panel.
	virtual void Draw() = 0;

	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	bool IsFullScreen() const noexcept;
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	bool TrapAllEvents() const noexcept;
	// Check if this panel can be "interrupted" to return to the main menu.
	bool IsInterruptible() const noexcept;

	// Clear the list of clickable zones.
	void ClearZones();
	// Add a clickable zone to the panel.
	void AddZone(const Rectangle &rect, const std::function<void()> &fun);
	void AddZone(const Rectangle &rect, SDL_Keycode key);
	// Check if a click at the given coordinates triggers a clickable zone. If
	// so, apply that zone's action and return true.
	bool ZoneClick(const Point &point);

	// Is fast-forward allowed to be on when this panel is on top of the GUI stack?
	virtual bool AllowsFastForward() const noexcept;

	virtual void UpdateTooltipActivation();


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	virtual bool Click(int x, int y, MouseButton button, int clicks);
	virtual bool Hover(int x, int y);
	virtual bool Drag(double dx, double dy);
	virtual bool Release(int x, int y, MouseButton button);
	virtual bool Scroll(double dx, double dy);

	virtual void Resize();

	// If a clickable zone is clicked while editing is happening, the panel may
	// need to know to exit editing mode before handling the click.
	virtual void EndEditing() {}

	void SetIsFullScreen(bool set);
	void SetTrapAllEvents(bool set);
	void SetInterruptible(bool set);

	// Dim the background of this panel.
	void DrawBackdrop() const;

	UI *GetUI() const noexcept;
	void SetUI(UI *ui);

	// This is not for overriding, but for calling KeyDown with only one or two
	// arguments. In this form, the command is never set, so you can call this
	// with a key representing a known keyboard shortcut without worrying that a
	// user-defined command key will override it.
	bool DoKey(SDL_Keycode key, Uint16 mod = 0);

	// A lot of different UI elements allow a modifier to change the number of
	// something you are buying, so the shared function is defined here:
	static int Modifier();
	// Display the given help message if it has not yet been shown
	// (or if force is set to true). Return true if the message was displayed.
	bool DoHelp(const std::string &name, bool force = false) const;

	const std::vector<std::shared_ptr<Panel>> &GetChildren();
	// Add a child. Deferred until next frame.
	void AddChild(const std::shared_ptr<Panel> &panel);
	// Remove a child. Deferred until next frame.
	void RemoveChild(const Panel *panel);
	// Handle deferred add/remove child operations.
	void AddOrRemove();

private:
	class Zone : public Rectangle {
	public:
		Zone(const Rectangle &rect, const std::function<void()> &fun) : Rectangle(rect), fun(fun) {}

		void Click() const { fun(); }

	private:
		std::function<void()> fun;
	};

	// The UI class will not directly call the virtual methods, but will call
	// these instead. These methods will recursively allow child panels to
	// handle the event first, before calling the virtual method for the derived
	// class to handle it.
	bool DoKeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	bool DoClick(int x, int y, MouseButton button, int clicks);
	bool DoHover(int x, int y);
	bool DoDrag(double dx, double dy);
	bool DoRelease(int x, int y, MouseButton button);
	bool DoScroll(double dx, double dy);

	void DoDraw();

	void DoResize();

	// Call a method on all the children in reverse order, and then on this
	// object. Recursion stops as soon as any child returns true.
	template<typename...FARGS, typename...ARGS>
	bool EventVisit(bool(Panel::*f)(FARGS ...args), ARGS ...args);


private:
	UI *ui = nullptr;

	bool isFullScreen = false;
	bool trapAllEvents = true;
	bool isInterruptible = true;

	std::list<Zone> zones;

	std::vector<std::shared_ptr<Panel>> children;
	std::vector<std::shared_ptr<Panel>> childrenToAdd;
	std::vector<const Panel *> childrenToRemove;

	friend class UI;
};



template<typename ...FARGS, typename ...ARGS>
bool Panel::EventVisit(bool (Panel::*f)(FARGS ...), ARGS ...args)
{
	// Check if a child panel will consume this event first.
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if((*it)->EventVisit(f, args...))
			return true;

	// If none of our children handled this event, then it could be for us.
	return (this->*f)(args...);
}
