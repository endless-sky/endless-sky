/* Panel.h
Michael Zahniser, 1 Nov 2013

Class representing a UI window (full screen or pop-up) which responds to user
input and can draw itself.
*/

#ifndef PANEL_H_INCLUDED
#define PANEL_H_INCLUDED

#include <SDL/SDL.h>



class Panel {
public:
	// Handle an event. The event is handed to each panel on the stack until one
	// of them handles it. If none do, this returns false.
	static bool Handle(const SDL_Event &event);
	
	// Step all the panels forward (advance animations, move objects, etc.).
	static void StepAll();
	
	// Draw all the panels.
	static void DrawAll();
	
	// Add the given panel to the stack. Panel is responsible for deleting it.
	static void Push(Panel *panel);
	
	// Remove the given panel from the stack (if it is in it). The panel will be
	// deleted at the start of the next time Step() is called, so it is safe for
	// a panel to Pop() itself.
	static void Pop(Panel *panel);
	// Remove the given panel from the stack, but do not delete it.
	static void PopWithoutDelete(Panel *panel);
	
	// Delete all the panels.
	static void FreeAll();
	
	// Tell the game to quit.
	static void Quit();
	// Check if it is time to quit.
	static bool IsDone();
	
	
public:
	// Make the destructor virtual just in case any derived class needs it.
	virtual ~Panel() {}
	
	// Move the state of this panel forward one game step. This will only be
	// called on the front-most panel, so if there are things like animations
	// that should work on panels behind that one, update them in Draw().
	virtual void Step(bool isActive) {}
	
	// Draw this panel.
	virtual void Draw() const {}
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return false; }
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	virtual bool TrapAllEvents() { return true; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod) { return false; }
	virtual bool Click(int x, int y) { return false; }
	virtual bool RClick(int x, int y) { return false; }
	virtual bool Hover(int x, int y) { return false; }
	virtual bool Drag(int dx, int dy) { return false; }
};



#endif
