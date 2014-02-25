/* BankPanel.h
Michael Zahniser, 14 Dec 2013

Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
buying and selling.
*/

#ifndef BANK_PANEL_H_INCLUDED
#define BANK_PANEL_H_INCLUDED

#include "Panel.h"

#include "PlayerInfo.h"



class BankPanel : public Panel {
public:
	BankPanel(PlayerInfo &player);
	
	virtual void Step(bool isActive);
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	PlayerInfo &player;
	int qualify;
	int selectedRow;
	
	int amount;
};



#endif
