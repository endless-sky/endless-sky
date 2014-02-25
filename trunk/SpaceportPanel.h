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
	
	virtual void Draw() const;
	
	
private:
	WrappedText text;
};



#endif
