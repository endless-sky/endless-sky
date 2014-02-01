/* PlanetPanel.h
Michael Zahniser, 30 Dec 2013

Panel for editing planets (name, landscape, description, etc.).
*/

#ifndef PLANET_PANEL_H_INCLUDED
#define PLANET_PANEL_H_INCLUDED

#include "Panel.h"

#include "Planet.h"



class PlanetPanel : public Panel {
public:
	PlanetPanel(Planet &planet);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
public:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	virtual bool RClick(int x, int y);
	
	
private:
	Planet &planet;
};



#endif
