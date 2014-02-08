/* PlanetPanel.cpp
Michael Zahniser, 2 Nov 2013

Function definitions for the PlanetPanel class.
*/

#include "PlanetPanel.h"

#include "Information.h"

#include "Color.h"
#include "ConversationPanel.h"
#include "Font.h"
#include "FontSet.h"
#include "ShipyardPanel.h"

using namespace std;



PlanetPanel::PlanetPanel(const GameData &data, PlayerInfo &player, const Planet &planet)
	: data(data), player(player), planet(planet),
	system(*player.GetShip()->GetSystem()),
	ui(*data.Interfaces().Get("planet")),
	trading(data, player), bank(player), spaceport(planet.SpaceportDescription()),
	selectedPanel(nullptr)
{
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.Description());
}



void PlanetPanel::Draw() const
{
	Information info;
	info.SetSprite("land", planet.Landscape());
	if(planet.HasSpaceport())
		info.SetCondition("has spaceport");
	if(planet.HasShipyard())
		info.SetCondition("has shipyard");
	if(planet.HasOutfitter())
		info.SetCondition("has outfitter");
	
	ui.Draw(info);
	
	if(selectedPanel)
		selectedPanel->Draw();
	else
		text.Draw(Point(-300., 80.), Color(.8));
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDLKey key, SDLMod mod)
{
	Panel *oldPanel = selectedPanel;
	
	if(key == 'd')
		Pop(this);
	else if(key == 't' && planet.HasSpaceport())
	{
		selectedPanel = &trading;
		Push(selectedPanel);
	}
	else if(key == 'b' && planet.HasSpaceport())
	{
		selectedPanel = &bank;
		Push(selectedPanel);
	}
	else if(key == 'p' && planet.HasSpaceport())
	{
		selectedPanel = &spaceport;
		Push(selectedPanel);
	}
	else if(key == 's' && planet.HasShipyard())
	{
		Push(new ShipyardPanel(data, player));
		return true;
	}
	else if(key == 'o' && planet.HasOutfitter())
	{
		// TODO: outfitter panel.
		selectedPanel = nullptr;
	}
	else if(key == 'j' || key == 'h')
		selectedPanel = nullptr;
	else
		return true;
	
	// If we are here, it is because something happened to change the selected
	// panel. So, we need to pop the old selected panel:
	if(oldPanel)
		PopWithoutDelete(oldPanel);
	
	return true;
}



bool PlanetPanel::Click(int x, int y)
{
	char key = ui.OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	return true;
}
