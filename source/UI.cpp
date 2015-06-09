/* UI.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "UI.h"

#include "Command.h"
#include "Panel.h"
#include "Screen.h"

#include <SDL2/SDL.h>

using namespace std;



// Default constructor.
UI::UI()
	: isDone(false)
{
}



// Handle an event. The event is handed to each panel on the stack until one
// of them handles it. If none do, this returns false.
bool UI::Handle(const SDL_Event &event)
{
	bool handled = false;
	
	vector<shared_ptr<Panel>>::iterator it = stack.end();
	while(it != stack.begin() && !handled)
	{
		--it;
		if(event.type == SDL_MOUSEMOTION)
		{
			if(event.motion.state & SDL_BUTTON(1))
				handled = (*it)->Drag(
					event.motion.xrel * 100 / Screen::Zoom(),
					event.motion.yrel * 100 / Screen::Zoom());
			else
				handled = (*it)->Hover(
					Screen::Left() + event.motion.x * 100 / Screen::Zoom(),
					Screen::Top() + event.motion.y * 100 / Screen::Zoom());
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			if(event.button.button == 1)
				handled = (*it)->Click(x, y);
			else if(event.button.button == 3)
				handled = (*it)->RClick(x, y);
		}
		else if(event.type == SDL_MOUSEBUTTONUP)
		{
			int x = Screen::Left() + event.button.x * 100 / Screen::Zoom();
			int y = Screen::Top() + event.button.y * 100 / Screen::Zoom();
			handled = (*it)->Release(x, y);
		}
		else if(event.type == SDL_MOUSEWHEEL)
			handled = (*it)->Scroll(event.wheel.x, event.wheel.y);
		else if(event.type == SDL_KEYDOWN)
		{
			Command command(event.key.keysym.sym);
			handled = (*it)->KeyDown(event.key.keysym.sym, event.key.keysym.mod, command);
		}
		
		// If this panel does not want anything below it to receive events, do
		// not let this event trickle further down the stack.
		if((*it)->TrapAllEvents())
			break;
	}
	
	return handled;
}



// Step all the panels forward (advance animations, move objects, etc.).
void UI::StepAll()
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
				it = stack.erase(it);
				break;
			}
	}
	toPop.clear();
	
	// Step all the panels.
	for(shared_ptr<Panel> &panel : stack)
		panel->Step();
}



// Draw all the panels.
void UI::DrawAll()
{
	// Find the topmost full-screen panel. Nothing below that needs to be drawn.
	vector<shared_ptr<Panel>>::const_iterator it = stack.end();
	while(it != stack.begin())
		if((*--it)->IsFullScreen())
			break;
	
	for( ; it != stack.end(); ++it)
		(*it)->Draw();
}



// Add the given panel to the stack. UI is responsible for deleting it.
void UI::Push(Panel *panel)
{
	Push(shared_ptr<Panel>(panel));
}



void UI::Push(const shared_ptr<Panel> &panel)
{
	panel->SetUI(this);
	toPush.push_back(panel);
}



// Remove the given panel from the stack (if it is in it). The panel will be
// deleted at the start of the next time Step() is called, so it is safe for
// a panel to Pop() itself.
void UI::Pop(const Panel *panel)
{
	toPop.push_back(panel);
}



// Check whether the given panel is on top, i.e. is the active one.
bool UI::IsTop(const Panel *panel) const
{
	return (!stack.empty() && stack.back().get() == panel);
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



// Check if it is time to quit.
bool UI::IsEmpty() const
{
	return stack.empty() && toPush.empty();
}
