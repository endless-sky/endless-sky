/* Panel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
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
bool Panel::KeyDown(SDL_Keycode key, Uint16 mod)
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



bool Panel::Scroll(int dx, int dy)
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
