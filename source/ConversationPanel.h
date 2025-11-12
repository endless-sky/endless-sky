/* ConversationPanel.h
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

#pragma once

#include "Panel.h"

#include "text/WrappedText.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Color;
class Conversation;
class Mission;
class PlayerInfo;
class Point;
class Ship;
class Sprite;
class System;



// User interface panel that displays a conversation, allowing you to make
// choices. If a callback function is given, that function will be called when
// the panel closes, to report the outcome of the conversation.
class ConversationPanel : public Panel {
public:
	ConversationPanel(PlayerInfo &player, const Conversation &conversation,
		const Mission *caller = nullptr, const System *system = nullptr,
		const std::shared_ptr<Ship> &ship = nullptr, bool useTransactions = false);

	virtual ~ConversationPanel() override;

	template<class T>
	void SetCallback(T *t, void (T::*fun)(int));
	void SetCallback(std::function<void(int)> fun);

	// Draw this panel.
	virtual void Draw() override;


protected:
	// Event handlers.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;


private:
	// Go to the given conversation node. If a choice index is given, include
	// the text of that choice in the conversation history.
	void Goto(int index, int selectedChoice = -1);
	// Exit this panel and do whatever needs to happen next, which includes
	// possibly activating a callback function and, if docked with an NPC,
	// destroying it or showing the BoardingPanel (if it is hostile).
	void Exit();
	// Handle mouse click on the "ok," "done," or a conversation choice.
	void ClickName(int side);
	void ClickChoice(int index);
	// Given an index into the list of displayed choices (i.e. not including
	// conditionally-skipped choices), return its "raw index" in the
	// conversation (i.e. including conditionally-skipped choices)
	int MapChoice(int n) const;


private:
	// Text to be displayed is broken up into chunks (paragraphs). Paragraphs
	// may also include "scene" images.
	class Paragraph {
	public:
		explicit Paragraph(const std::string &text, const Sprite *scene = nullptr, bool isFirst = false);

		// Get the height of this paragraph.
		int Height() const;
		// Get the "center point" of this paragraph. This is for drawing a
		// highlight under paragraphs that represent choices.
		Point Center() const;
		// Draw this paragraph at the given point, and return the point that the
		// next paragraph below this one should be drawn at.
		Point Draw(Point point, const Color &color) const;

	private:
		const Sprite *scene = nullptr;
		WrappedText wrap;
		// Special case: if this is the very first paragraph and it begins with
		// a "scene" image, there is no need for padding above the image.
		bool isFirst = false;
	};


private:
	// Reference to the player, to apply any changes to them.
	PlayerInfo &player;

	// A pointer to the mission that called this conversation.
	const Mission *caller = nullptr;

	// Should we use a PlayerInfo transaction to prevent save-load glitches?
	bool useTransactions = false;

	// The conversation we are displaying.
	const Conversation &conversation;
	// All conversations start with node 0.
	int node = 0;
	// This function should be called with the conversation outcome.
	std::function<void(int)> callback = nullptr;

	// Current scroll position.
	double scroll = 0.;

	// The "history" of the conversation up to this point:
	std::list<Paragraph> text;
	// The current choices being presented to you, and their indices:
	std::list<std::pair<Paragraph, int>> choices;
	int choice = 0;
	// Flicker time, set if the player enters invalid input for a pilot's name.
	int flickerTime = 0;

	// Text entry fields for changing the player's name.
	std::string firstName;
	std::string lastName;
	// Text substitutions (player's name, and ship name).
	std::map<std::string, std::string> subs;

	// Maximum scroll amount.
	double maxScroll = 0.;

	// If specified, this is a star system to display with a special big pointer
	// when the player brings up the map. (Typically a mission destination.)
	const System *system;
	// If specified, this is a pointer to the ship that this conversation
	// acts upon (e.g. the ship failing a "flight check", or the NPC you
	// have boarded).
	std::shared_ptr<Ship> ship;

	// Whether the mouse moved in the current frame.
	bool isHovering = false;
	Point hoverPoint;
};



// Allow the callback function to be a member of any class.
template<class T>
void ConversationPanel::SetCallback(T *t, void (T::*fun)(int))
{
	callback = std::bind(fun, t, std::placeholders::_1);
}
