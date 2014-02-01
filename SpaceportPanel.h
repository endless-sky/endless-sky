/* SpaceportPanel.h
Michael Zahniser, 11 Jan 2014

GUI panel to be shown when you are in a spaceport.
*/

#ifndef SPACEPORT_PANEL_H_INCLUDED
#define SPACEPORT_PANEL_H_INCLUDED

#include "Panel.h"

#include "WrappedText.h"

#include <string>



class SpaceportPanel : public Panel {
public:
	SpaceportPanel(const std::string &description);
	virtual ~SpaceportPanel() {}
	
	virtual void Draw() const;
	
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	virtual bool TrapAllEvents() { return false; }
	
	
private:
	WrappedText text;
};



#endif
