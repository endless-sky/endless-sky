/* LoadPanel.cpp
Michael Zahniser, 26 Feb 2014

Function definitions for the LoadPanel class.
*/

#include "LoadPanel.h"

#include "GameData.h"
#include "Information.h"
#include "Interface.h"

using namespace std;



LoadPanel::LoadPanel(const GameData &data, PlayerInfo &player, const Panel *parent)
	: data(data), player(player), parent(parent)
{
}



void LoadPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	data.Background().Draw(Point(), Point());
	
	Information info;
	info.SetString("pilot", player.FirstName() + " " + player.LastName());
	if(player.GetShip())
	{
		const Ship &ship = *player.GetShip();
		info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
		info.SetString("ship", ship.Name());
		if(ship.GetPlanet())
			info.SetString("planet", ship.GetPlanet()->Name());
		else
			info.SetString("planet", "");
		info.SetString("system", ship.GetSystem()->Name());
	}
	info.SetString("credits", to_string(player.Accounts().Credits()));
	info.SetString("date", player.GetDate().ToString());
	
	const Interface *menu = data.Interfaces().Get("load menu");
	menu->Draw(info);
}



bool LoadPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'b' || key == data.Keys().Get(Key::MENU))
		Pop(this);
	
	return true;
}



bool LoadPanel::Click(int x, int y)
{
	char key = data.Interfaces().Get("load menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	// TODO: handle clicks on lists of pilots / saves.
	
	return true;
}
