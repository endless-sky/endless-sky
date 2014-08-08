/* ConversationPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONVERSATION_PANEL_H_
#define CONVERSATION_PANEL_H_

#include "Panel.h"

#include "Callback.h"
#include "WrappedText.h"

#include <list>
#include <string>
#include <vector>

class Conversation;
class PlayerInfo;
class System;



// User interface panel that displays a conversation, allowing you to make choices,
// and then can be closed once the conversation ends.
class ConversationPanel : public Panel {
public:
	ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system = nullptr);
	void SetCallback(const Callback &callback);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	void Goto(int index);
	
	
private:
	class ClickZone {
	public:
		ClickZone(const Point &topLeft, const Point &size);
		
		bool Contains(const Point &point);
		
	private:
		Point topLeft;
		Point size;
	};
	
	
private:
	PlayerInfo &player;
	
	const Conversation &conversation;
	int node;
	Callback callback;
	
	int scroll;
	
	WrappedText wrap;
	std::list<WrappedText> text;
	std::list<WrappedText> choices;
	int choice;
	
	std::string firstName;
	std::string lastName;
	
	mutable std::vector<ClickZone> zones;
	
	const System *system;
};



#endif
