/* Panel.cpp
Michael Zahniser, 1 Nov 2013

Function definitions for the Panel class.
*/

#include "Panel.h"

#include "Screen.h"

#include <algorithm>
#include <thread>
#include <vector>

using namespace std;

namespace {
	vector<Panel *> stack;
	mutex stackMutex;
	
	vector<Panel *> toPush;
	vector<Panel *> toDelete;
	vector<Panel *> toPop;
	mutex addMutex;
}



// Handle an event. The event is handed to each panel on the stack until one
// of them handles it. If none do, this returns false.
bool Panel::Handle(const SDL_Event &event)
{
	bool handled = false;
	
	lock_guard<mutex> lock(stackMutex);
	vector<Panel *>::iterator it = stack.end();
	while(it != stack.begin() && !handled)
	{
		--it;
		if(event.type == SDL_MOUSEMOTION)
		{
			if(event.motion.state & SDL_BUTTON(1))
				handled = (*it)->Drag(event.motion.xrel, event.motion.yrel);
			else
				handled = (*it)->Hover(
					event.motion.x - Screen::Width() / 2,
					event.motion.y - Screen::Height() / 2);
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			int x = event.button.x - Screen::Width() / 2;
			int y = event.button.y - Screen::Height() / 2;
			if(event.button.button == 1)
				handled = (*it)->Click(x, y);
			else if(event.button.button == 3)
				handled = (*it)->RClick(x, y);
		}
		else if(event.type == SDL_KEYDOWN)
			handled = (*it)->KeyDown(event.key.keysym.sym, event.key.keysym.mod);
		
		// If this panel does not want anything below it to receive events, do
		// not let this event trickle further down the stack.
		if((*it)->TrapAllEvents())
			break;
	}
	
	return handled;
}



// Step all the panels forward (advance animations, move objects, etc.).
void Panel::StepAll()
{
	lock_guard<mutex> lock(stackMutex);
	{
		// Handle any panels that should be added or deleted.
		lock_guard<mutex> lock(addMutex);
		stack.insert(stack.end(), toPush.begin(), toPush.end());
		toPush.clear();
		
		// These panels should be popped but not deleted (because someone else
		// owns them and is managing their creation and deletion).
		for(Panel *panel : toPop)
		{
			vector<Panel *>::iterator it = find(stack.begin(), stack.end(), panel);
			if(it != stack.end())
				stack.erase(it);
		}
		toPop.clear();
		
		// These panels should be popped _and_ deleted.
		for(Panel *panel : toDelete)
		{
			vector<Panel *>::iterator it = find(stack.begin(), stack.end(), panel);
			if(it != stack.end())
			{
				// Only delete the given panel if Panel is managing it. (Handing
				// any other pointer to this function is a user error.)
				stack.erase(it);
				delete panel;
			}
		}
		toDelete.clear();
	}
	
	// Step all the panels.
	for(Panel *panel : stack)
		panel->Step(panel == stack.back());
}



// Draw all the panels.
void Panel::DrawAll()
{
	lock_guard<mutex> lock(stackMutex);
	
	// Find the topmost full-screen panel. Nothing below that needs to be drawn.
	vector<Panel *>::const_iterator it = stack.end();
	while(it != stack.begin())
		if((*--it)->IsFullScreen())
			break;
	
	for( ; it != stack.end(); ++it)
		(*it)->Draw();
}



// Delete all the panels.
void Panel::FreeAll()
{
	while(!stack.empty())
	{
		delete stack.back();
		stack.pop_back();
	}
}



// Add the given panel to the stack. Panel is responsible for deleting it.
void Panel::Push(Panel *panel)
{
	lock_guard<mutex> lock(addMutex);
	toPush.push_back(panel);
}



// Remove the given panel from the stack (if it is in it). The panel will be
// deleted at the start of the next time Step() is called, so it is safe for
// a panel to Pop() itself.
void Panel::Pop(Panel *panel)
{
	lock_guard<mutex> lock(addMutex);
	toDelete.push_back(panel);
}



// Remove the given panel from the stack, but do not delete it.
void Panel::PopWithoutDelete(Panel *panel)
{
	lock_guard<mutex> lock(addMutex);
	toPop.push_back(panel);
}
