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

#include "audio/Audio.h"
#include "Command.h"
#include "Panel.h"
#include "Screen.h"

#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;



// Handle an event. The event is handed to each panel on the stack until one
// of them handles it. If none do, this returns false.
bool UI::Handle(const SDL_Event &event)
{
	bool handled = false;

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
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			if(event.button.button == SDL_BUTTON_LEFT)
				handled = (*it)->ZoneClick(Point(x, y));
			if(!handled)
				handled = (*it)->DoClick(x, y, static_cast<MouseButton>(event.button.button), event.button.clicks);
		}
		else if(event.type == SDL_MOUSEBUTTONUP)
		{
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			handled = (*it)->DoRelease(x, y, static_cast<MouseButton>(event.button.button));
		}
		else if(event.type == SDL_MOUSEWHEEL)
			handled = (*it)->DoScroll(event.wheel.x, event.wheel.y);
		else if(event.type == SDL_KEYDOWN)
		{
			Command command(event.key.keysym.sym);
			handled = (*it)->DoKeyDown(event.key.keysym.sym, event.key.keysym.mod, command, !event.key.repeat);
		}

		// If this panel does not want anything below it to receive events, do
		// not let this event trickle further down the stack.
		if((*it)->TrapAllEvents())
			break;
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
}



const vector<shared_ptr<Panel>> &UI::Stack() const
{
	return stack;
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



void UI::AdjustViewport() const
{
	for(auto &it : stack)
		it->DoResize();
}



// Get the current mouse position.
Point UI::GetMouse()
{
	int x = 0;
	int y = 0;
	SDL_GetMouseState(&x, &y);
	return Screen::TopLeft() + Point(x, y) * (100. / Screen::Zoom());
}



void UI::PlaySound(UISound sound)
{
	string name;
	switch(sound)
	{
		case UISound::NORMAL:
			name = "ui/click";
			break;
		case UISound::SOFT:
			name = "ui/click soft";
			break;
		case UISound::SOFT_BUZZ:
			name = "ui/buzz soft";
			break;
		case UISound::TARGET:
			name = "ui/target";
			break;
		case UISound::FAILURE:
			name = "ui/fail";
			break;
		default:
			return;
	}
	Audio::Play(Audio::Get(name), SoundCategory::UI);
}



// If a push or pop is queued, apply it.
void UI::PushOrPop()
{
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
