/* CreditsPanel.h
Michael Zahniser, 21 Jan 2014

Class representing a panel that pops up with a message asking you to enter an
integer amount, up to a given limit.
*/

#ifndef CREDITS_PANEL_H_INCLUDED
#define CREDITS_PANEL_H_INCLUDED

#include "Panel.h"

#include <string>



class CreditsPanel : public Panel {
public:
	// Create a panel.
	CreditsPanel(const std::string &message, int *amount, int limit);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	std::string message;
	int *amount;
	int limit;
};



#endif
