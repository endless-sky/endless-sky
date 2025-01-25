/* UI.cpp
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

#include "UI.h"

#include "Angle.h"
#include "Color.h"
#include "Command.h"
#include "DelaunayTriangulation.h"
#include "GameData.h"
#include "GamePad.h"
#include "GamepadCursor.h"
#include "Gesture.h"
#include "shader/LineShader.h"
#include "Panel.h"
#include "shader/PointerShader.h"
#include "Screen.h"
#include "TouchScreen.h"

#include <SDL2/SDL.h>

#include <SDL2/SDL_log.h>
#include <algorithm>

using namespace std;

namespace
{
	Angle g_pointer_angle;
}



// Handle an event. The event is handed to each panel on the stack until one
// of them handles it. If none do, this returns false.
bool UI::Handle(const SDL_Event &event)
{
	bool handled = false;
	SDL_GameControllerAxis axisTriggered = SDL_CONTROLLER_AXIS_INVALID;
	SDL_GameControllerAxis axisUnTriggered = SDL_CONTROLLER_AXIS_INVALID;

	vector<shared_ptr<Panel>>::iterator it = stack.end();
	while(it != stack.begin() && !handled)
	{
		--it;
		// Panels that are about to be popped cannot handle any other events.
		if(count(toPop.begin(), toPop.end(), it->get()))
			continue;

		if(event.type == SDL_MOUSEMOTION)
		{
			if(event.motion.state & SDL_BUTTON(1))
				handled = (*it)->DoDrag(
					event.motion.xrel * 100. / Screen::Zoom(),
					event.motion.yrel * 100. / Screen::Zoom());
			else
				handled = (*it)->DoHover(
					Screen::Left() + event.motion.x * 100 / Screen::Zoom(),
					Screen::Top() + event.motion.y * 100 / Screen::Zoom());
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			// TODO: move this zone then click logic into DoClick()
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			if(event.button.button == 1)
			{
				handled = (*it)->ZoneMouseDown(Point(x, y), event.button.which);
				if(!handled)
					handled = (*it)->DoClick(x, y, event.button.clicks);
			}
			else if(event.button.button == 3)
				handled = (*it)->DoRClick(x, y);
		}
		else if(event.type == SDL_MOUSEBUTTONUP)
		{
			// TODO: move this zone then click logic into DoRelease()
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			handled = (*it)->HasZone(Point(x, y));
			if(!handled)
				handled = (*it)->DoRelease(x, y);
		}
		else if(event.type == SDL_MOUSEWHEEL)
			handled = (*it)->DoScroll(event.wheel.x, event.wheel.y);
		else if(event.type == SDL_FINGERDOWN)
		{
			// finger coordinates are 0 to 1, normalize to screen coordinates
			int x = (event.tfinger.x - .5) * Screen::Width();
			int y = (event.tfinger.y - .5) * Screen::Height();

			uint32_t now = SDL_GetTicks();
			handled = (*it)->DoFingerDown(x, y, event.tfinger.fingerId, (now - lastTap) > 500 ? 1 : 2);
			lastTap = now;
		}
		else if(event.type == SDL_FINGERMOTION)
		{
			// finger coordinates are 0 to 1, normalize to screen coordinates
			int x = (event.tfinger.x - .5) * Screen::Width();
			int y = (event.tfinger.y - .5) * Screen::Height();
			double dx = (event.tfinger.dx) * Screen::Width();
			double dy = (event.tfinger.dy) * Screen::Height();

			handled = (*it)->DoFingerMove(x, y, dx, dy, event.tfinger.fingerId);
		}
		else if(event.type == SDL_FINGERUP)
		{
			// finger coordinates are 0 to 1, normalize to screen coordinates
			int x = (event.tfinger.x - .5) * Screen::Width();
			int y = (event.tfinger.y - .5) * Screen::Height();

			handled = (*it)->DoFingerUp(x, y, event.tfinger.fingerId);
		}
		else if(event.type == SDL_KEYDOWN)
		{
			Command command(event.key.keysym.sym);
			handled = (*it)->DoKeyDown(event.key.keysym.sym, event.key.keysym.mod, command, !event.key.repeat);
		}
		else if(event.type == Command::EventID())
		{
			Command command(event);
			if(reinterpret_cast<const CommandEvent&>(event).pressed == SDL_PRESSED)
				handled = (*it)->DoKeyDown(0, 0, command, true);
		}
		else if(event.type == Gesture::EventID())
		{
			auto gesture_type = static_cast<Gesture::GestureEnum>(event.user.code);

			// if the panel doesn't want the gesture, convert it to a
			// command, and try again
			if(!(handled = (*it)->DoGesture(gesture_type)))
			{
				Command command(gesture_type);
				Command::InjectOnce(command);
				handled = (*it)->DoKeyDown(0, 0, command, true);
			}
		}
		else if(event.type == SDL_CONTROLLERDEVICEADDED ||
		        event.type == SDL_CONTROLLERDEVICEREMOVED)
		{
			handled = (*it)->ControllersChanged();
		}
		else if(event.type == SDL_CONTROLLERAXISMOTION)
		{
			// Try a raw axis event first. If nobody handles it, then convert it
			// to an axis trigger event, and try again.
			if(!(handled = (*it)->ControllerAxis(static_cast<SDL_GameControllerAxis>(event.caxis.axis), event.caxis.value)))
			{
				// Not handled. Convert it to a trigger event
				if(activeAxis == SDL_CONTROLLER_AXIS_INVALID)
				{
					if(SDL_abs(event.caxis.value) > GamePad::AxisIsButtonPressThreshold())
					{
						// Trigger/axis has moved far enough that we are certain the
						// user meant to press it.
						activeAxisIsPositive = event.caxis.value > 0;
						activeAxis = static_cast<SDL_GameControllerAxis>(event.caxis.axis);
						handled = (*it)->ControllerTriggerPressed(activeAxis, activeAxisIsPositive);
						if(!handled)
						{
							Command command = Command::FromTrigger(event.caxis.axis, activeAxisIsPositive);
							if(!(command == Command()))
								handled = (*it)->DoKeyDown(0, 0, command, true);
							if(!handled)
							{
								// By default, convert the right joystick into a scroll event.
								// for mouse wheel, positive is up, so flip the direction
								if(activeAxis == SDL_CONTROLLER_AXIS_RIGHTY)
									handled = (*it)->Scroll(0, activeAxisIsPositive ? -1 : 1);
								else if(activeAxis == SDL_CONTROLLER_AXIS_RIGHTX)
									handled = (*it)->Scroll(activeAxisIsPositive ? -1 : 1, 0);
							}
						}
						axisTriggered = activeAxis;
					}
				}
				else if(activeAxis == event.caxis.axis && SDL_abs(event.caxis.value) < GamePad::DeadZone())
				{
					// Trigger returned to zero-ish position.
					handled = (*it)->ControllerTriggerReleased(activeAxis, activeAxisIsPositive);
					axisUnTriggered = activeAxis;
					activeAxis = SDL_CONTROLLER_AXIS_INVALID;
				}
			}
		}
		else if(event.type == SDL_CONTROLLERBUTTONDOWN)
		{
			handled = (*it)->ControllerButtonDown(static_cast<SDL_GameControllerButton>(event.cbutton.button));
			if(!handled)
			{
				Command command = Command::FromButton(event.cbutton.button);
				handled = (*it)->DoKeyDown(0, 0, command, true);
			}
		}
		else if(event.type == SDL_CONTROLLERBUTTONUP)
		{
			handled = (*it)->DoControllerButtonUp(static_cast<SDL_GameControllerButton>(event.cbutton.button));
		}

		// If this panel does not want anything below it to receive events, do
		// not let this event trickle further down the stack.
		if((*it)->TrapAllEvents())
			break;
	}

	// If game controller events are not explicitly handled by the panels, then
	// handle them here. We do this outside of the panel loop, because in order
	// to pick and choose buttons to navigate to, we need to consider all of the
	// panels, not just the top one.
	if(!handled)
	{
		if(axisTriggered != SDL_CONTROLLER_AXIS_INVALID)
			handled = DefaultControllerTriggerPressed(axisTriggered, activeAxisIsPositive);
		else if(axisUnTriggered != SDL_CONTROLLER_AXIS_INVALID)
			handled = DefaultControllerTriggerReleased(axisTriggered, activeAxisIsPositive);
		else if(event.type == SDL_CONTROLLERBUTTONDOWN)
			handled = DefaultControllerButtonDown(static_cast<SDL_GameControllerButton>(event.cbutton.button));
		else if(event.type == SDL_CONTROLLERBUTTONUP)
			handled = DefaultControllerButtonUp(static_cast<SDL_GameControllerButton>(event.cbutton.button));
	}

	// Handle any queued push or pop commands.
	PushOrPop();

	return handled;
}



// Step all the panels forward (advance animations, move objects, etc.).
void UI::StepAll()
{
	// Handle any queued push or pop commands.
	PushOrPop();

	// Step all the panels.
	for(shared_ptr<Panel> &panel : stack)
		panel->Step();
}



// Draw all the panels.
void UI::DrawAll()
{
	// First, clear all the clickable zones. New ones will be added in the
	// course of drawing the screen.
	for(const shared_ptr<Panel> &it : stack)
		it->ClearZones();

	// Find the topmost full-screen panel. Nothing below that needs to be drawn.
	vector<shared_ptr<Panel>>::const_iterator it = stack.end();
	while(it != stack.begin())
		if((*--it)->IsFullScreen())
			break;

	for( ; it != stack.end(); ++it)
		(*it)->DoDraw();

	// If the panel has a valid ui element selected, draw a rotating indicator
	// around it
	GamepadCursor::Draw();
}



// Add the given panel to the stack. UI is responsible for deleting it.
void UI::Push(Panel *panel)
{
	Push(shared_ptr<Panel>(panel));
}



void UI::Push(const shared_ptr<Panel> &panel)
{
	toPush.push_back(panel);
	panel->SetUI(this);
}



// Remove the given panel from the stack (if it is in it). The panel will be
// deleted at the start of the next time Step() is called, so it is safe for
// a panel to Pop() itself.
void UI::Pop(const Panel *panel)
{
	toPop.push_back(panel);
}



// Remove the given panel and every panel that is higher in the stack.
void UI::PopThrough(const Panel *panel)
{
	for(auto it = stack.rbegin(); it != stack.rend(); ++it)
	{
		toPop.push_back(it->get());
		if(it->get() == panel)
			break;
	}
}



// Check whether the given panel is on top of the existing panels, i.e. is the
// active one, on this Step. Any panels that have been pushed this Step are not
// considered.
bool UI::IsTop(const Panel *panel) const
{
	return (!stack.empty() && stack.back().get() == panel);
}



// Get the absolute top panel, even if it is not yet drawn (i.e. was pushed on
// this Step).
shared_ptr<Panel> UI::Top() const
{
	if(!toPush.empty())
		return toPush.back();

	if(!stack.empty())
		return stack.back();

	return shared_ptr<Panel>();
}



// Delete all the panels and clear the "done" flag.
void UI::Reset()
{
	stack.clear();
	toPush.clear();
	toPop.clear();
	isDone = false;
}



// Get the lower-most panel.
shared_ptr<Panel> UI::Root() const
{
	if(stack.empty())
	{
		if(toPush.empty())
			return shared_ptr<Panel>();

		return toPush.front();
	}

	return stack.front();
}



// If the player enters the game, enable saving the loaded file.
void UI::CanSave(bool canSave)
{
	this->canSave = canSave;
}



bool UI::CanSave() const
{
	return canSave;
}



// Tell the UI to quit.
void UI::Quit()
{
	isDone = true;
}



// Check if it is time to quit.
bool UI::IsDone() const
{
	return isDone;
}



// Check if there are no panels left. No panels left on the gamePanels-
// stack usually means that it is time for the game to quit, while no
// panels left on the menuPanels-stack is a normal state for a running
// game.
bool UI::IsEmpty() const
{
	return stack.empty() && toPush.empty();
}



// Get the current mouse position.
Point UI::GetMouse()
{
	int x = 0;
	int y = 0;
	SDL_GetMouseState(&x, &y);
	return Screen::TopLeft() + Point(x, y) * (100. / Screen::Zoom());
}



// Return the positions of every active zone. Used for joystick navigation.
std::vector<Point> UI::ZonePositions() const
{
	std::vector<Point> options;
	for (const Panel::Zone* zone: GetZones())
		options.push_back(zone->Center());
	return options;
}



// Return the commands for every activated zone. This function will get polled
// as part of the engine processing
Command UI::ZoneCommands() const
{
	Command ret;
	// include touchscreen and mouse pointers when considering zones
	auto pointers = TouchScreen::Points();
	int mousePosX;
	int mousePosY;
	if((SDL_GetMouseState(&mousePosX, &mousePosY) & SDL_BUTTON_LMASK) != 0)
	{
		// Convert from window to game screen coordinates
		double relX = mousePosX / static_cast<double>(Screen::RawWidth()) - .5;
		double relY = mousePosY / static_cast<double>(Screen::RawHeight()) - .5;
		pointers.push_back(Point(relX * Screen::Width(), relY * Screen::Height()));
	}

	auto zones = GetZones();
	for(const Point& p: pointers)
	{
		for(const Panel::Zone* zone: zones)
		{
			if(zone->Contains(p))
			{
				if(!(zone->ZoneCommand() == Command()))
					ret.Set(zone->ZoneCommand());

				// Don't process gestures if we have crossed into a zone
				TouchScreen::CancelGesture();

				// even if this zone isn't associated with a command, don't
				// consider any other zones (this one is being drawn on top of
				// them)
				break;
			}
		}
	}
	return ret;
}




// If a push or pop is queued, apply it.
void UI::PushOrPop()
{
	// If panel state is changing, reset the controller cursor state
	if(!toPush.empty() || !toPop.empty())
		GamepadCursor::SetEnabled(false);

	// Handle any panels that should be added.
	for(shared_ptr<Panel> &panel : toPush)
		if(panel)
			stack.push_back(panel);
	toPush.clear();

	// These panels should be popped but not deleted (because someone else
	// owns them and is managing their creation and deletion).
	for(const Panel *panel : toPop)
	{
		for(auto it = stack.begin(); it != stack.end(); ++it)
			if(it->get() == panel)
			{
				stack.erase(it);
				break;
			}
	}
	toPop.clear();

	// Each panel potentially has its own children, which could be modified.
	for(auto &panel : stack)
		panel->AddOrRemove();
}



// Handle panel button navigation. This is done in the UI class instead of the
// panel class because we need to know where all the buttons on all the panels
// are in order to correctly handle navigation.
bool UI::DefaultControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive)
{
	// By default, treat the left joystick like a zone selector.
	if(axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY)
	{
		std::vector<Point> options;
		for (const Panel::Zone* zone: GetZones())
			options.push_back(zone->Center());
		if(!options.empty())
		{
			if(!GamepadCursor::Enabled())
				GamepadCursor::SetPosition(options.front());
			else
				GamepadCursor::MoveDir(GamePad::LeftStick(), options);
			return true;
		}
	}
	return false;
}



// Handle panel button navigation. This is done in the UI class instead of the
// panel class because we need to know where all the buttons on all the panels
// are in order to correctly handle navigation.
bool UI::DefaultControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive)
{
	return false;
}



// Handle panel button navigation. This is done in the UI class instead of the
// panel class because the button press might apply to a different panel than
// the top one.
bool UI::DefaultControllerButtonUp(SDL_GameControllerButton button)
{
	if(button == SDL_CONTROLLER_BUTTON_A && GamepadCursor::Enabled())
	{
		for(Panel::Zone* zone: GetZones())
		{
			// floating point equality comparision is safe here, since we haven't
			// done math on these, just assignments.
			if(zone->Center().X() == GamepadCursor::Position().X() &&
				zone->Center().Y() == GamepadCursor::Position().Y())
			{
				return true;
			}
		}
	}
	return false;
}



// Handle panel button navigation. This is done in the UI class instead of the
// panel class because the button press might apply to a different panel than
// the top one.
bool UI::DefaultControllerButtonDown(SDL_GameControllerButton button)
{
	if(button == SDL_CONTROLLER_BUTTON_A && GamepadCursor::Enabled())
	{
		for(Panel::Zone* zone: GetZones())
		{
			// floating point equality comparision is safe here, since we haven't
			// done math on these, just assignments.
			if(zone->Center().X() == GamepadCursor::Position().X() &&
				zone->Center().Y() == GamepadCursor::Position().Y())
			{
				zone->ButtonDown(static_cast<int>(button));
				return true;
			}
		}
	}
	return false;
}



// Get all the zones from currently active panels
std::vector<Panel::Zone*> UI::GetZones() const
{
	std::vector<Panel::Zone*> zones;
	for(auto it = stack.rbegin(); it != stack.rend(); ++it)
	{
		// Don't handle panels destined for death.
		if(count(toPop.begin(), toPop.end(), it->get()))
			continue;

		for(Panel::Zone& zone: (*it)->zones)
			zones.push_back(&zone);

		// Don't consider anything beneath a modal panel
		if((*it)->TrapAllEvents())
			break;
	}
	return zones;
}
