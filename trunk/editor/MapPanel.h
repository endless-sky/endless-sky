/* MapPanel.h
Michael Zahniser, 28 Dec 2013

GUI panel displaying a star map and allowing editing of it (moving, renaming,
adding, and deleting stars).
*/

#ifndef MAP_PANEL_H_INCLUDED
#define MAP_PANEL_H_INCLUDED

#include "Panel.h"

#include "Set.h"
#include "System.h"
#include "Planet.h"

#include <list>
#include <string>



class MapPanel : public Panel {
public:
	MapPanel(const char *path);
	~MapPanel();
	
	// Draw this panel.
	virtual void Draw() const;
	
	// Return true if this is a full-screen panel, so there is no point in
	// drawing any of the panels under it.
	virtual bool IsFullScreen() { return true; }
	
	
public:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool RClick(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	std::string mapPath;
	
	Point position;
	System *selected;
	std::string commodity;
	
	Set<System> systems;
	Set<Planet> planets;
	std::list<DataFile::Node> unrecognized;
};



#endif
