/* Panel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PANEL_H_
#define PANEL_H_

#include "Rectangle.h"

#include <functional>
#include <list>
#include <string>

#include <SDL2/SDL.h>

class Command;
class Point;
class UI;



// Class representing a UI window (full screen or pop-up) which responds to user
// input and can draw itself. Everything displayed in the game is drawn in a
// Panel, and panels can stack on top of each other like "real" UI windows. By
// default, a panel allows the panels under it to show through, but does not
// allow them to receive any events that it does not know how to handle.
class Panel {
public:
	// Make the destructor virtual just in case any derived class needs it.
	virtual ~Panel() = default;
	
	// Move the state of this panel forward one game step.
	virtual void Step();
	
	// Draw this panel.
	virtual void Draw();
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	bool IsFullScreen();
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	bool TrapAllEvents();
	// Check if this panel can be "interrupted" to return to the main menu.
	bool IsInterruptible() const;
	
	// Clear the list of clickable zones.
	void ClearZones();
	// Add a clickable zone to the panel.
	void AddZone(const Rectangle &rect, const std::function<void()> &fun);
	void AddZone(const Rectangle &rect, SDL_Keycode key);
	// Check if a click at the given coordinates triggers a clickable zone. If
	// so, apply that zone's action and return true.
	bool ZoneClick(const Point &point);
	
	// Is fast-forward allowed to be on when this panel is on top of the GUI stack?
	virtual bool AllowFastForward() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	virtual bool Click(int x, int y, int clicks);
	virtual bool RClick(int x, int y);
	virtual bool Hover(int x, int y);
	virtual bool Drag(double dx, double dy);
	virtual bool Release(int x, int y);
	virtual bool Scroll(double dx, double dy);
	// If a clickable zone is clicked while editing is happening, the panel may
	// need to know to exit editing mode before handling the click.
	virtual void EndEditing() {}
	
	void SetIsFullScreen(bool set);
	void SetTrapAllEvents(bool set);
	void SetInterruptible(bool set);
	
	// Dim the background of this panel.
	void DrawBackdrop() const;
	
	UI *GetUI() const;
	
	// This is not for overriding, but for calling KeyDown with only one or two
	// arguments. In this form, the command is never set, so you can call this
	// with a key representing a known keyboard shortcut without worrying that a
	// user-defined command key will override it.
	bool DoKey(SDL_Keycode key, Uint16 mod = 0);
	
	// A lot of different UI elements allow a modifier to change the number of
	// something you are buying, so the shared function is defined here:
	static int Modifier();
	// Display the given help message if it has not yet been shown. Return true
	// if the message was displayed.
	bool DoHelp(const std::string &name) const;
	
	
private:
	class Zone : public Rectangle {
	public:
		Zone(const Rectangle &rect, const std::function<void()> &fun) : Rectangle(rect), fun(fun) {}
		
		void Click() const { fun(); }
		
	private:
		std::function<void()> fun;
	};
	
	
private:
	void SetUI(UI *ui);
	
	
private:
	UI *ui = nullptr;
	
	bool isFullScreen = false;
	bool trapAllEvents = true;
	bool isInterruptible = true;
	
	std::list<Zone> zones;
	
	friend class UI;
};



#endif
