/* NamePanel.h
Michael Zahniser, 30 Dec 2013

Panel for typing in the name of a planet.
*/

#ifndef NAME_PANEL_H_INCLUDED
#define NAME_PANEL_H_INCLUDED

#include "Panel.h"

#include <string>



class NamePanel : public Panel {
public:
	NamePanel(std::string &name);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
public:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	
	
private:
	std::string &name;
};



#endif
