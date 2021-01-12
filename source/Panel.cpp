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

#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "FillShader.h"
#include "GameData.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "UI.h"

using namespace std;



// Move the state of this panel forward one game step.
void Panel::Step()
{
}



// Draw this panel.
void Panel::Draw()
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



// Check if this panel can be "interrupted" to return to the main menu.
bool Panel::IsInterruptible() const
{
	return isInterruptible;
}



// Clear the list of clickable zones.
void Panel::ClearZones()
{
	zones.clear();
}



// Add a clickable zone to the panel.
void Panel::AddZone(const Rectangle &rect, const function<void()> &fun)
{
	// The most recently added zone will typically correspond to what was drawn
	// most recently, so it should be on top.
	zones.emplace_front(rect, fun);
}



void Panel::AddZone(const Rectangle &rect, SDL_Keycode key)
{
	AddZone(rect, [this, key](){ this->KeyDown(key, 0, Command(), true); });
}



// Check if a click at the given coordinates triggers a clickable zone. If
// so, apply that zone's action and return true.
bool Panel::ZoneClick(const Point &point)
{
	for(const Zone &zone : zones)
		if(zone.Contains(point))
		{
			// If the panel is in editing mode, make sure it knows that a mouse
			// click has broken it out of that mode, so it doesn't interpret a
			// button press and a text character entered.
			EndEditing();
			zone.Click();
			return true;
		}
	return false;
}



// Panels will by default not allow fast-forward. The ones that do allow
// it will override this (virtual) function and return true.
bool Panel::AllowFastForward() const
{
	return false;
}



// Only override the ones you need; the default action is to return false.
bool Panel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	return false;
}



bool Panel::Click(int x, int y, int clicks)
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



bool Panel::Drag(double dx, double dy)
{
	return false;
}



bool Panel::Scroll(double dx, double dy)
{
	return false;
}



bool Panel::Release(int x, int y)
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



void Panel::SetInterruptible(bool set)
{
	isInterruptible = set;
}


	
// Dim the background of this panel.
void Panel::DrawBackdrop() const
{
	if(!GetUI()->IsTop(this))
		return;
	
	// Darken everything but the dialog.
	const Color &back = *GameData::Colors().Get("dialog backdrop");
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
}



UI *Panel::GetUI() const
{
	return ui;
}



// This is not for overriding, but for calling KeyDown with only one or two
// arguments. In this form, the command is never set, so you can call this
// with a key representing a known keyboard shortcut without worrying that a
// user-defined command key will override it.
bool Panel::DoKey(SDL_Keycode key, Uint16 mod)
{
	return KeyDown(key, mod, Command(), true);
}



// A lot of different UI elements allow a modifier to change the number of
// something you are buying, so the shared function is defined here:
int Panel::Modifier()
{
	SDL_Keymod mod = SDL_GetModState();
	
	int modifier = 1;
	if(mod & KMOD_ALT)
		modifier *= 500;
	if(mod & (KMOD_CTRL | KMOD_GUI))
		modifier *= 20;
	if(mod & KMOD_SHIFT)
		modifier *= 5;
	
	return modifier;
}



// Display the given help message if it has not yet been shown. Return true
// if the message was displayed.
bool Panel::DoHelp(const string &name) const
{
	string preference = "help: " + name;
	if(Preferences::Has(preference))
		return false;
	
	const string &message = GameData::HelpMessage(name);
	if(message.empty())
		return false;
	
	Preferences::Set(preference);
	ui->Push(new Dialog(message));
	
	return true;
}



void Panel::SetUI(UI *ui)
{
	this->ui = ui;
}
