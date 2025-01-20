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
#include "DelaunayTriangulation.h"
#include "Dialog.h"
#include "shader/FillShader.h"
#include "text/Format.h"
#include "GameData.h"
#include "GamePad.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "UI.h"

#include <SDL2/SDL.h>

using namespace std;



Panel::Panel() noexcept
{
	// Clear any triggered commands when starting a new panel
	Command::InjectClear();
}



Panel::~Panel()
{
}



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
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if ((*it)->IsFullScreen())
			return true;

	return isFullScreen;
}



// Return true if, when this panel is on the stack, no events should be
// passed to any panel under it. By default, all panels do this.
bool Panel::TrapAllEvents() const noexcept
{
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if ((*it)->IsFullScreen())
			return true;
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



// Add a clickable zone to the panel.
void Panel::AddZone(const Rectangle &rect, const function<void(const Event&)> &fun)
{
	// The most recently added zone will typically correspond to what was drawn
	// most recently, so it should be on top.
	zones.emplace_front(rect, fun);
}



void Panel::AddZone(const Rectangle &rect, SDL_Keycode key)
{
	AddZone(rect, [this, key](){ this->KeyDown(key, 0, Command(), true); });
}



void Panel::AddZone(const Rectangle &rect, Command command)
{
	zones.emplace_front(rect, command);
}



// Add a clickable zone to the panel.
void Panel::AddZone(const Point &center, float radius, const function<void()> &fun)
{
	// The most recently added zone will typically correspond to what was drawn
	// most recently, so it should be on top.
	zones.emplace_front(center, radius, fun);
}



// Add a clickable zone to the panel.
void Panel::AddZone(const Point &center, float radius, const function<void(const Event &)> &fun)
{
	// The most recently added zone will typically correspond to what was drawn
	// most recently, so it should be on top.
	zones.emplace_front(center, radius, fun);
}



void Panel::AddZone(const Point &center, float radius, SDL_Keycode key)
{
	AddZone(center, radius, [this, key](){ this->KeyDown(key, 0, Command(), true); });
}



void Panel::AddZone(const Point &center, float radius, Command command)
{
	zones.emplace_front(center, radius, command);
}



// Check if a click at the given coordinates triggers a clickable zone. If
// so, apply that zone's action and return true.
bool Panel::ZoneMouseDown(const Point &point, int id)
{
	for(const Zone &zone : zones)
	{
		if(zone.Contains(point))
		{
			// If the panel is in editing mode, make sure it knows that a mouse
			// click has broken it out of that mode, so it doesn't interpret a
			// button press and a text character entered.
			EndEditing();
			zone.MouseDown(point, id);
			return true;
		}
	}
	return false;
}



// Check if a click at the given coordinates triggers a clickable zone. If
// so, apply that zone's action and return true.
bool Panel::ZoneFingerDown(const Point &point, int id)
{
	for(const Zone &zone : zones)
	{
		if(zone.Contains(point))
		{
			// If the panel is in editing mode, make sure it knows that a mouse
			// click has broken it out of that mode, so it doesn't interpret a
			// button press and a text character entered.
			EndEditing();
			zone.FingerDown(point, id);
			return true;
		}
	}
	return false;
}



// Check if a click at the given coordinates are on a zone.
bool Panel::HasZone(const Point &point)
{
	for(const Zone &zone : zones)
	{
		if(zone.Contains(point))
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



bool Panel::DoKeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	return EventVisit(&Panel::KeyDown, key, mod, command, isNewPress);
}



bool Panel::DoClick(int x, int y, int clicks)
{
	return EventVisit(&Panel::Click, x, y, clicks);
}



bool Panel::DoRClick(int x, int y)
{
	return EventVisit(&Panel::RClick, x, y);
}



bool Panel::DoHover(int x, int y)
{
	return EventVisit(&Panel::Hover, x, y);
}



bool Panel::DoDrag(double dx, double dy)
{
	return EventVisit(&Panel::Drag, dx, dy);
}



bool Panel::DoRelease(int x, int y)
{
	return EventVisit(&Panel::Release, x, y);
}



bool Panel::DoScroll(double dx, double dy)
{
	return EventVisit(&Panel::Scroll, dx, dy);
}



bool Panel::DoFingerDown(int x, int y, int fid, int clicks)
{
	// Order:
	//   0. Children first.
	//   1. Zones (these will be buttons)
	//   2. Finger down events (this will be game controls)
	//      2.5 Trigger a hover as well, as some ui's use this to
	//          determine where a drag begins from.
	//   3. Clicks (fallback to mouse click)
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if((*it)->DoFingerDown(x, y, fid, clicks))
			return true;

	if(ZoneFingerDown(Point(x, y), fid))
	{
		zoneFingerId = fid;
		return true;
	}
	if(FingerDown(x, y, fid))
		return true;
	Hover(x, y);
	if(DoClick(x, y, clicks))
	{
		panelFingerId = fid;
		return true;
	}
	return false;
}



bool Panel::DoFingerMove(int x, int y, double dx, double dy, int fid)
{
	// Order:
	//   0. Children
	//   1. FingerMove events (These will be game controls)
	//   2. Drag (ui events)
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if((*it)->DoFingerMove(x, y, dx, dy, fid))
			return true;

	if(FingerMove(x, y, fid))
		return true;
	if(fid == panelFingerId)
		return Drag(dx, dy);
	return false;
}



bool Panel::DoFingerUp(int x, int y, int fid)
{
	// Order:
	//   0. Children
	//   1. Zones (these will be buttons)
	//   2. Finger down events (this will be game controls)
	//   3. Clicks (fallback to mouse click)
	for(auto it = children.rbegin(); it != children.rend(); ++it)
		if((*it)->DoFingerUp(x, y, fid))
			return true;

	if(fid == zoneFingerId)
	{
		zoneFingerId = -1;
		if (HasZone(Point(x, y)))
			return true;
	}
	if(FingerUp(x, y, fid))
		return true;
	if (fid == panelFingerId)
	{
		panelFingerId = -1;
		return Release(x, y);
	}
	return false;
}



bool Panel::DoGesture(Gesture::GestureEnum gesture)
{
	return EventVisit(&Panel::Gesture, gesture);
}



bool Panel::DoControllersChanged()
{
	return EventVisit(&Panel::ControllersChanged);
}



bool Panel::DoControllerButtonDown(SDL_GameControllerButton button)
{
	return EventVisit(&Panel::ControllerButtonDown, button);
}



bool Panel::DoControllerButtonUp(SDL_GameControllerButton button)
{
	return EventVisit(&Panel::ControllerButtonUp, button);
}



bool Panel::DoControllerAxis(SDL_GameControllerAxis axis, int position)
{
	return EventVisit(&Panel::ControllerAxis, axis, position);
}



bool Panel::DoControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive)
{
	return EventVisit(&Panel::ControllerTriggerPressed, axis, positive);
}



bool Panel::DoControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive)
{
	return EventVisit(&Panel::ControllerTriggerReleased, axis, positive);
}



void Panel::DoDraw()
{
	Draw();
	for(auto &child : children)
		child->DoDraw();
}


bool Panel::FingerDown(int x, int y, int fid)
{
	return false;
}



bool Panel::FingerMove(int x, int y, int fid)
{
	return false;
}



bool Panel::FingerUp(int x, int y, int fid)
{
	return false;
}



bool Panel::Gesture(Gesture::GestureEnum gesture)
{
	return false;
}



bool Panel::ControllersChanged()
{
	return false;
}



bool Panel::ControllerButtonDown(SDL_GameControllerButton button)
{
	return false;
}



bool Panel::ControllerButtonUp(SDL_GameControllerButton button)
{
	return false;
}



bool Panel::ControllerAxis(SDL_GameControllerAxis axis, int position)
{
	return false;
}



bool Panel::ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive)
{
	return false;
}



bool Panel::ControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive)
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



UI *Panel::GetUI() const noexcept
{
	return parent ? parent->GetUI() : ui;
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



void Panel::AddChild(const shared_ptr<Panel> &panel)
{
	panel->parent = this;
	childrenToAdd.push_back(panel);
}



void Panel::RemoveChild(Panel *panel)
{
	panel->parent = nullptr;
	childrenToRemove.push_back(panel);
}
