/* UI.h
Michael Zahniser, 24 Feb 2014

Class representing a UI (i.e. a series of Panels stacked on top of each other,
with Panels added or removed in response to user events).
*/

#ifndef UI_H_
#define UI_H_

#include <memory>
#include <vector>

#include <SDL/SDL_events.h>

class Panel;



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
	void Push(const std::shared_ptr<Panel> &panel);
	// Remove the given panel from the stack (if it is in it). The panel will be
	// deleted at the start of the next time Step() is called, so it is safe for
	// a panel to Pop() itself.
	void Pop(const Panel *panel);
	
	// Delete all the panels and clear the "done" flag.
	void Reset();
	
	// Tell the UI to quit.
	void Quit();
	// Check if it is time to quit.
	bool IsDone() const;
	// Check if there are no panels left.
	bool IsEmpty() const;
	
	
private:
	std::vector<std::shared_ptr<Panel>> stack;
	
	bool isDone;
	std::vector<std::shared_ptr<Panel>> toPush;
	std::vector<const Panel *> toPop;
};



#endif
