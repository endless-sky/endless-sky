/* ConversationPanel.h
Michael Zahniser, 9 Jan 2014

User interface panel that displays a conversation, allowing you to make choices,
and then can be closed once the conversation ends.
*/

#ifndef CONVERSATION_PANEL_H_INCLUDED
#define CONVERSATION_PANEL_H_INCLUDED

#include "Panel.h"

#include "Callback.h"
#include "WrappedText.h"

#include <list>
#include <string>

class Conversation;
class PlayerInfo;



class ConversationPanel : public Panel {
public:
	ConversationPanel(PlayerInfo &player, const Conversation &conversation);
	void SetCallback(const Callback &callback);
	
	// Draw this panel.
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDLKey key, SDLMod mod);
	virtual bool Drag(int dx, int dy);
	
	
private:
	void Goto(int index);
	
	
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
};



#endif
