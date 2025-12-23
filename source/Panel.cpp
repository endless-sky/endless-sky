/* Panel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Panel.h"

#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "shader/FillShader.h"
#include "text/Format.h"
#include "GameData.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "UI.h"

using namespace std;



// Draw a sprite repeatedly to make a vertical edge.
void Panel::DrawEdgeSprite(const Sprite *edgeSprite, int posX)
{
	if(edgeSprite->Height())
	{
		// If the screen is high enough, the edge sprite should repeat.
		double spriteHeight = edgeSprite->Height();
		Point pos(
			posX + .5 * edgeSprite->Width(),
			Screen::Top() + .5 * spriteHeight);
		for( ; pos.Y() - .5 * spriteHeight < Screen::Bottom(); pos.Y() += spriteHeight)
			SpriteShader::Draw(edgeSprite, pos);
	}
}



// Move the state of this panel forward one game step.
void Panel::Step()
{
	// It is ok for panels to be stateless.
}



// Return true if this is a full-screen panel, so there is no point in
// drawing any of the panels under it.
bool Panel::IsFullScreen() const noexcept
{
	return isFullScreen;
}



// Return true if, when this panel is on the stack, no events should be
// passed to any panel under it. By default, all panels do this.
bool Panel::TrapAllEvents() const noexcept
{
	return trapAllEvents;
}



// Check if this panel can be "interrupted" to return to the main menu.
bool Panel::IsInterruptible() const noexcept
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
	for(auto it = children.rbegin(); it != children.rend(); ++it)
	{
		if((*it)->ZoneClick(point))
			return true;
	}

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
bool Panel::AllowsFastForward() const noexcept
{
	return false;
}



void Panel::UpdateTooltipActivation()
{
}



void Panel::AddOrRemove()
{
	for(auto &panel : childrenToAdd)
		if(panel)
			children.emplace_back(std::move(panel));
	childrenToAdd.clear();

	for(auto *panel : childrenToRemove)
	{
		for(auto it = children.begin(); it != children.end(); ++it)
		{
			if(it->get() == panel)
			{
				children.erase(it);
				break;
			}
		}
	}
	childrenToRemove.clear();

	for(auto &child : children)
		child->AddOrRemove();
}



// Only override the ones you need; the default action is to return false.
bool Panel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	return false;
}



bool Panel::Click(int x, int y, MouseButton button, int clicks)
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



bool Panel::Release(int x, int y, MouseButton button)
{
	return false;
}



void Panel::Resize()
{
}



bool Panel::DoKeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	return EventVisit(&Panel::KeyDown, key, mod, command, isNewPress);
}



bool Panel::DoClick(int x, int y, MouseButton button, int clicks)
{
	return EventVisit(&Panel::Click, x, y, button, clicks);
}



bool Panel::DoHover(int x, int y)
{
	return EventVisit(&Panel::Hover, x, y);
}



bool Panel::DoDrag(double dx, double dy)
{
	return EventVisit(&Panel::Drag, dx, dy);
}



bool Panel::DoRelease(int x, int y, MouseButton button)
{
	return EventVisit(&Panel::Release, x, y, button);
}



bool Panel::DoScroll(double dx, double dy)
{
	return EventVisit(&Panel::Scroll, dx, dy);
}



void Panel::DoDraw()
{
	Draw();
	for(auto &child : children)
		child->DoDraw();
}



void Panel::DoResize()
{
	Resize();
	for(auto &child : children)
		child->DoResize();
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
	FillShader::Fill(Point(), Screen::Dimensions(), back);
}



UI *Panel::GetUI() const noexcept
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



// Display the given help message if it has not yet been shown
// (or if force is set to true). Return true if the message was displayed.
bool Panel::DoHelp(const string &name, bool force) const
{
	string preference = "help: " + name;
	if(!force && Preferences::Has(preference))
		return false;

	const string &message = GameData::HelpMessage(name);
	if(message.empty())
		return false;

	Preferences::Set(preference);
	ui->Push(new Dialog(Format::Capitalize(name) + ":\n\n" + message));

	return true;
}



void Panel::SetUI(UI *ui)
{
	this->ui = ui;
}



const vector<shared_ptr<Panel>> &Panel::GetChildren()
{
	return children;
}



void Panel::AddChild(const shared_ptr<Panel> &panel)
{
	childrenToAdd.push_back(panel);
}



void Panel::RemoveChild(const Panel *panel)
{
	childrenToRemove.push_back(panel);
}
