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

#include "ClickZone.h"
#include "WrappedText.h"

#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

class Color;
class Conversation;
class PlayerInfo;
class Point;
class Sprite;
class System;



// User interface panel that displays a conversation, allowing you to make
// choices. If a callback function is given, that function will be called when
// the panel closes, to report the outcome of the conversation.
class ConversationPanel : public Panel {
public:
	ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system = nullptr);
	
template <class T>
	void SetCallback(T *t, void (T::*fun)(int));
	
	// Draw this panel.
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	void Goto(int index, int choice = -1);
	void SelectNode();
	
	
private:
	class Paragraph {
	public:
		Paragraph(const std::string &text, const Sprite *scene = nullptr);
		
		int Height() const;
		Point Center() const;
		void Draw(Point &point, const Color &color) const;
		
	private:
		const Sprite *scene = nullptr;
		WrappedText wrap;
	};
	
	
private:
	PlayerInfo &player;
	
	const Conversation &conversation;
	int node;
	std::function<void(int)> callback = nullptr;
	
	double scroll;
	
	std::list<Paragraph> text;
	std::list<Paragraph> choices;
	int choice;
	
	std::string firstName;
	std::string lastName;
	std::map<std::string, std::string> subs;
	
	mutable std::vector<ClickZone<int>> zones;
	mutable int maxScroll = 0.;
	
	const System *system;
};



template <class T>
void ConversationPanel::SetCallback(T *t, void (T::*fun)(int))
{
	callback = std::bind(fun, t, std::placeholders::_1);
}



#endif
