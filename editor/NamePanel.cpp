/* NamePanel.h
Michael Zahniser, 30 Dec 2013

Function definitions for the NamePanel GUI.
*/

#include "NamePanel.h"

#include "Font.h"
#include "FontSet.h"

#include "shift.h"

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
	
	static const float white[4] = {4., 4., 4., .25};
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
		Pop(this);
	else if(key == SDLK_ESCAPE)
	{
		name.clear();
		Pop(this);
	}
	
	return true;
}
