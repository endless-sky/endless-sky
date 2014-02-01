/* TradingPanel.h
Michael Zahniser, 14 Dec 2013

Overlay on the PlanetPanel showing commodity prices and inventory, and allowing
buying and selling.
*/

#ifndef TRADING_PANEL_H_INCLUDED
#define TRADING_PANEL_H_INCLUDED

#include "Panel.h"

#include "GameData.h"
#include "PlayerInfo.h"
#include "System.h"



class TradingPanel : public Panel {
public:
	TradingPanel(const GameData &data, PlayerInfo &player);
	virtual ~TradingPanel() {}
	
	virtual void Draw() const;
	
	// Return true if, when this panel is on the stack, no events should be
	// passed to any panel under it. By default, all panels do this.
	virtual bool TrapAllEvents() { return false; }
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	static int Modifier();
	void Buy(int amount = 1);
	void Sell(int amount = 1);
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const System &system;
	
	int selectedRow;
};



#endif
