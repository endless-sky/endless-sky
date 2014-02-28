/* Panel.cpp
Michael Zahniser, 1 Nov 2013

Function definitions for the Panel class.
*/

#include "Panel.h"

#include "UI.h"

using namespace std;



// Constructor.
Panel::Panel()
	: ui(nullptr), isFullScreen(false), trapAllEvents(true)
{
}



// Make the destructor just in case any derived class needs it.
Panel::~Panel()
{
}



// Move the state of this panel forward one game step. This will only be
// called on the front-most panel, so if there are things like animations
// that should work on panels behind that one, update them in Draw().
void Panel::Step(bool)
{
}



// Draw this panel.
void Panel::Draw() const
{
}



// Return true if this is a full-screen panel, so there is no point in
// drawing any of the panels under it.
bool Panel::IsFullScreen()
{
	return isFullScreen;
}



// Return true if, when this panel is on the stack, no events should be
// passed to any panel under it. By default, all panels do this.
bool Panel::TrapAllEvents()
{
	return trapAllEvents;
}



// Only override the ones you need; the default action is to return false.
bool Panel::KeyDown(SDLKey key, SDLMod mod)
{
	return false;
}



bool Panel::Click(int x, int y)
{
	return false;
}



bool Panel::RClick(int x, int y)
{
	return false;
}



bool Panel::Hover(int x, int y)
{
	return false;
}



bool Panel::Drag(int dx, int dy)
{
	return false;
}


	
void Panel::SetIsFullScreen(bool set)
{
	isFullScreen = set;
}



void Panel::SetTrapAllEvents(bool set)
{
	trapAllEvents = set;
}



UI *Panel::GetUI()
{
	return ui;
}



void Panel::SetUI(UI *ui)
{
	this->ui = ui;
}
