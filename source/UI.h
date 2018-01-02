/* UI.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef UI_H_
#define UI_H_

#include "Point.h"

#include <memory>
#include <vector>

#include <SDL2/SDL_events.h>

class Panel;



// Class representing a UI (i.e. a series of Panels stacked on top of each other,
// with Panels added or removed in response to user events). This handles events
// and passing them on to whatever panel is on top, and drawing of the panels
// starting with whichever one is on the bottom.
class UI {
public:
	// Default constructor.
	UI();
	
	// Handle an event. The event is handed to each panel on the stack until one
	// of them handles it. If none do, this returns false.
	bool Handle(const SDL_Event &event);
	
	// Step all the panels forward (advance animations, move objects, etc.).
	void StepAll();
	// Draw all the panels.
	void DrawAll();
	
	// Add the given panel to the stack. If you do not want a panel to be
	// deleted when it is popped, save a copy of its shared pointer elsewhere.
	void Push(Panel *panel);
	void Push(const std::shared_ptr<Panel> &panel);
	// Remove the given panel from the stack (if it is in it). The panel will be
	// deleted at the start of the next time Step() is called, so it is safe for
	// a panel to Pop() itself.
	void Pop(const Panel *panel);
	
	// Check whether the given panel is on top, i.e. is the active one, out of
	// all panels that are already drawn on this step.
	bool IsTop(const Panel *panel) const;
	// Get the top panel, out of all possible panels, including ones not yet drawn.
	std::shared_ptr<Panel> Top() const;
	
	// Delete all the panels and clear the "done" flag.
	void Reset();
	// Get the lower-most panel.
	std::shared_ptr<Panel> Root() const;
	
	// Tell the UI to quit.
	void Quit();
	// Check if it is time to quit.
	bool IsDone() const;
	// Check if there are no panels left.
	bool IsEmpty() const;
	
	// Get the current mouse position.
	static Point GetMouse();
	
	
private:
	// If a push or pop is queued, apply it.
	void PushOrPop();
	
	
private:
	std::vector<std::shared_ptr<Panel>> stack;
	
	bool isDone;
	std::vector<std::shared_ptr<Panel>> toPush;
	std::vector<const Panel *> toPop;
};



#endif
