/* SystemPanel.h
Michael Zahniser, 26 Dec 2013

Class representing a panel for viewing and editing a star system.
*/

#ifndef SYSTEM_PANEL_H_INCLUDED
#define SYSTEM_PANEL_H_INCLUDED

#include "Panel.h"

#include "DrawList.h"
#include "Point.h"
#include "System.h"
#include "Set.h"
#include "Planet.h"

#include <vector>
#include <string>
#include <map>



class SystemPanel : public Panel {
public:
	SystemPanel(System &system, Set<Planet> &planets);
	
	// Move the state of this panel forward one game step. This will only be
	// called on the front-most panel, so if there are things like animations
	// that should work on panels behind that one, update them in Draw().
	virtual void Step(bool isActive);
	
	// Draw this panel.
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
public:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	void Draw(const System::Object &object, Point center = Point()) const;
	
	
private:
	System &system;
	Set<Planet> &planets;
	Point position;
	mutable DrawList draw;
	
	System::Object *selected;
	mutable std::map<const System::Object *, Point> drawn;
	
	time_t now;
	bool paused;
};

#endif
