/* PlanetPanel.h
Michael Zahniser 2 Nov 2013

Dialog that pops up when you land on a planet. The shipyard and outfitter are
shown in full-screen panels that pop up above this one, but the remaining views
(trading, jobs, bank, port, and crew) are displayed within this dialog.
*/

#ifndef PLANET_PANEL_H_INCLUDED
#define PLANET_PANEL_H_INCLUDED

#include "Panel.h"

#include "BankPanel.h"
#include "GameData.h"
#include "Interface.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "SpaceportPanel.h"
#include "System.h"
#include "TradingPanel.h"
#include "WrappedText.h"



class PlanetPanel : public Panel {
public:
	PlanetPanel(const GameData &data, PlayerInfo &player, const Planet &planet);
	virtual ~PlanetPanel() {}
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Click(int x, int y);
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const Planet &planet;
	const System &system;
	const Interface &ui;
	
	TradingPanel trading;
	BankPanel bank;
	SpaceportPanel spaceport;
	Panel *selectedPanel;
	
	WrappedText text;
};



#endif
