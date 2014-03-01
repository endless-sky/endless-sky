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

#include <SDL/SDL.h>

#include <memory>

class UI;



// Class representing a UI window (full screen or pop-up) which responds to user
// input and can draw itself.
class Panel {
public:
	// Constructor.
	Panel();
	// Make the destructor virtual just in case any derived class needs it.
	virtual ~Panel();
	
	// Move the state of this panel forward one game step. This will only be
	// called on the front-most panel, so if there are things like animations
	// that should work on panels behind that one, update them in Draw().
	virtual void Step(bool isActive);
	
	// Draw this panel.
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	bool IsFullScreen();
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	bool TrapAllEvents();
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool RClick(int x, int y);
	virtual bool Hover(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	void SetIsFullScreen(bool set);
	void SetTrapAllEvents(bool set);
	UI *GetUI();
	
	
private:
	void SetUI(UI *ui);
	UI *ui;
	
	bool isFullScreen;
	bool trapAllEvents;
	
	friend class UI;
};



#endif
