/* Panel.h
Michael Zahniser, 1 Nov 2013

Class representing a UI window (full screen or pop-up) which responds to user
input and can draw itself.
*/

#ifndef PANEL_H_INCLUDED
#define PANEL_H_INCLUDED

#include <SDL/SDL.h>

#include <memory>

class UI;



class Panel {
public:
	// Constructor.
	Panel(bool isFullScreen = false, bool trapAllEvents = true);
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
	
	void Push(Panel *panel);
	void Push(const std::shared_ptr<Panel> &panel);
	void Pop(const Panel *panel);
	void Quit();
	
	
private:
	void SetUI(UI *ui);
	UI *ui;
	
	bool isFullScreen;
	bool trapAllEvents;
	
	friend class UI;
};



#endif
