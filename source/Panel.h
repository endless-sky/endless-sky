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

#include "Rectangle.h"
#include "Command.h"

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

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
	struct Event
	{
		Point pos;
		int id;
		enum {MOUSE, TOUCH, BUTTON, AXIS} type;
	};

	Panel() noexcept;

	// Make the destructor virtual just in case any derived class needs it.
	virtual ~Panel();

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
	void AddZone(const Rectangle &rect, const std::function<void(const Event &)> &fun);
	void AddZone(const Rectangle &rect, SDL_Keycode key);
	void AddZone(const Rectangle &rect, Command command);
	void AddZone(const Point& pos, float radius, const std::function<void()> &fun);
	void AddZone(const Point& pos, float radius, const std::function<void(const Event &)> &fun);
	void AddZone(const Point& pos, float radius, SDL_Keycode key);
	void AddZone(const Point& pos, float radius, Command command);
	// Check if a click at the given coordinates triggers a clickable zone. If
	// so, forward the event and return true.
	bool ZoneMouseDown(const Point &point, int id);
	bool ZoneFingerDown(const Point &point, int id);
	bool HasZone(const Point &point);

	// Is fast-forward allowed to be on when this panel is on top of the GUI stack?
	virtual bool AllowsFastForward() const noexcept;

	// TODO: delete this when DropDown gets rewritten
	// Return UI associated with this panel
	UI *GetUI() const noexcept;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	virtual bool Click(int x, int y, int clicks);
	virtual bool RClick(int x, int y);
	virtual bool Hover(int x, int y);
	virtual bool Drag(double dx, double dy);
	virtual bool Release(int x, int y);
	virtual bool Scroll(double dx, double dy);
	virtual bool FingerDown(int x, int y, int fid);
	virtual bool FingerMove(int x, int y, int fid);
	virtual bool FingerUp(int x, int y, int fid);
	virtual bool Gesture(Gesture::GestureEnum gesture);
	virtual bool ControllersChanged();
	virtual bool ControllerButtonDown(SDL_GameControllerButton button);
	virtual bool ControllerButtonUp(SDL_GameControllerButton button);
	virtual bool ControllerAxis(SDL_GameControllerAxis axis, int position);
	virtual bool ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive);
	virtual bool ControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive);
	// If a clickable zone is clicked while editing is happening, the panel may
	// need to know to exit editing mode before handling the click.
	virtual void EndEditing() {}

	void SetIsFullScreen(bool set);
	void SetTrapAllEvents(bool set);
	void SetInterruptible(bool set);

	// Dim the background of this panel.
	void DrawBackdrop() const;

	// TODO: put this back after DropDown gets rewritten
	//UI *GetUI() const noexcept;
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

	// Add a child. Deferred until next frame.
	void AddChild(const std::shared_ptr<Panel> &panel);
	// Remove a child. Deferred until next frame.
	void RemoveChild(const Panel *panel);
	// Handle deferred add/remove child operations.
	void AddOrRemove();

private:
	class Zone : public Rectangle {
	public:
		Zone(const Rectangle &rect, const std::function<void()> &fun_down):
			Rectangle(rect),
			fun_down(fun_down),
			radius(0)
		{}
		Zone(const Point &pos, float radius, const std::function<void()> &fun_down):
			Rectangle(pos, Point()),
			fun_down(fun_down),
			radius(radius)
		{}
		Zone(const Rectangle &rect, const std::function<void(const Event&)> &fun):
			Rectangle(rect),
			fun_down_event(fun),
			radius(0)
		{}
		Zone(const Point &pos, float radius, const std::function<void(const Event&)> &fun):
			Rectangle(pos, Point()),
			fun_down_event(fun),
			radius(radius)
		{}
		Zone(const Rectangle &rect, Command command):
			Rectangle(rect),
			command(command),
			radius(0)
		{
			fun_down = [command]() { Command::InjectOnce(command); };
		}
		Zone(const Point &pos, float radius, Command command):
			Rectangle(pos, Point()),
			command(command),
			radius(radius)
		{
			fun_down = [command]() { Command::InjectOnce(command); };
		}

		void MouseDown(const Point& pos, int id) const
		{
			if (fun_down_event)
			{
				Event e{pos, id, Event::MOUSE};
				fun_down_event(e);
			}
			else
				fun_down();
		}
		void FingerDown(const Point& pos, int id) const
		{
			if (fun_down_event)
			{
				Event e{pos, id, Event::TOUCH};
				fun_down_event(e);
			}
			else
				fun_down();
		}
		void ButtonDown(int id) const
		{
			if (fun_down_event)
			{
				Event e{Center(), id, Event::BUTTON};
				fun_down_event(e);
			}
			else
				fun_down();
		}
		void AxisDown(int id) const
		{
			if (fun_down_event)
			{
				Event e{Center(), id, Event::AXIS};
				fun_down_event(e);
			}
			else
				fun_down();
		}

		bool Contains(const Point& p) const
		{
			if(radius)
				return Center().DistanceSquared(p) < radius * radius;
			else
				return Rectangle::Contains(p);
		}

		const Command& ZoneCommand() const { return command; }

	private:
		std::function<void()> fun_down;
		std::function<void(const Event&)> fun_down_event;
		Command command;
		float radius = 0;
	};

	// The UI class will not directly call the virtual methods, but will call
	// these instead. These methods will recursively allow child panels to
	// handle the event first, before calling the virtual method for the derived
	// class to handle it.
	bool DoKeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	bool DoClick(int x, int y, int clicks);
	bool DoRClick(int x, int y);
	bool DoHover(int x, int y);
	bool DoDrag(double dx, double dy);
	bool DoRelease(int x, int y);
	bool DoScroll(double dx, double dy);
	bool DoFingerDown(int x, int y, int fid, int clicks);
	bool DoFingerMove(int x, int y, double dx, double dy, int fid);
	bool DoFingerUp(int x, int y, int fid);
	bool DoGesture(Gesture::GestureEnum gesture);
	bool DoControllersChanged();
	bool DoControllerButtonDown(SDL_GameControllerButton button);
	bool DoControllerButtonUp(SDL_GameControllerButton button);
	bool DoControllerAxis(SDL_GameControllerAxis axis, int position);
	bool DoControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive);
	bool DoControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive);

	void DoDraw();

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

	static int zoneFingerId;
	static int panelFingerId;

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
