/* NamePanel.h
Michael Zahniser, 30 Dec 2013

Function definitions for the NamePanel GUI.
*/

#include "NamePanel.h"

#include "Color.h"
#include "Font.h"
#include "FontSet.h"
#include "Point.h"
#include "shift.h"
#include "UI.h"

using namespace std;



NamePanel::NamePanel(std::string &name)
	: name(name)
{
}



// Draw this panel.
void NamePanel::Draw() const
{
	const Font &font = FontSet::Get(14);
	
	const string &label = "Planet name: ";
	int x = font.Width(label);
	
	static const Color white(1., 1., 1., .25);
	font.Draw(label, Point(-x, 0), white);
	font.Draw(name, Point(), white);
}



bool NamePanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key >= ' ' && key <= '~')
	{
		bool isShifted = (mod & KMOD_SHIFT);
		name += isShifted ? SHIFT[key] : key;
	}
	else if(!name.empty() && (key == SDLK_DELETE || key == SDLK_BACKSPACE))
		name.erase(name.length() - 1);
	else if(key == SDLK_RETURN)
		GetUI()->Pop(this);
	else if(key == SDLK_ESCAPE)
	{
		name.clear();
		GetUI()->Pop(this);
	}
	
	return true;
}
