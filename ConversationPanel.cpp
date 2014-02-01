/* ConversationPanel.cpp
Michael Zahniser, 9 Jan 2014

Function definitions for the ConversationPanel class.
*/

#include "ConversationPanel.h"

#include "Color.h"
#include "Conversation.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <GL/glew.h>

using namespace std;

namespace {
	static const int WIDTH = 460;
}



ConversationPanel::ConversationPanel(const Conversation &conversation)
	: conversation(conversation), scroll(0)
{
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(WIDTH);
	wrap.SetFont(FontSet::Get(14));
	
	Goto(0);
}



// Draw this panel.
void ConversationPanel::Draw() const
{
	const Sprite *sprite = SpriteSet::Get("ui/conversation");
	SpriteShader::Draw(sprite, Point(
		(sprite->Width() - Screen::Width()) * .5 + 30.,
		(sprite->Height() - Screen::Height()) * .5 + 30.));
	
	int x = (-Screen::Width() / 2) + 50;
	int y = (-Screen::Height() / 2) + 50;
	
	const Font &font = FontSet::Get(14);
	
	Color color(.8);
	for(const WrappedText &it : text)
	{
		for(const WrappedText::Word &word : it.Words())
		{
			// TODO: color your choices brighter.
			Point point(word.X() + x, word.Y() + y);
			font.Draw(word.String(), point, color.Get());
		}
		y += it.Height();
	}
	
	Color choiceColor(1.);
	Color selectionColor(.1, .1, .1, .1);
	int i = 0;
	for(const WrappedText &it : choices)
	{
		if(i++ == choice)
		{
			Point center(x + WIDTH / 2, y + (it.Height() - it.ParagraphBreak()) * .5);
			Point size(WIDTH, it.Height());
			
			FillShader::Fill(center, size, selectionColor.Get());
		}
		for(const WrappedText::Word &word : it.Words())
		{
			Point point(word.X() + x, word.Y() + y);
			font.Draw(word.String(), point, choiceColor.Get());
		}
		y += it.Height();
	}
}



// Only override the ones you need; the default action is to return false.
bool ConversationPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == SDLK_UP && choice > 0)
		--choice;
	else if(key == SDLK_DOWN && choice < conversation.Choices(node) - 1)
		++choice;
	else if(key == SDLK_RETURN && choice < conversation.Choices(node))
	{
		// Find the ith choice.
		auto it = choices.begin();
		for(int i = 0; i < choice; ++i)
			++it;
		text.splice(text.end(), choices, it);
		
		Goto(conversation.NextNode(node, choice));
	}
	else if(key == SDLK_ESCAPE)
		Pop(this);
	
	// Nothing under the conversation should be able to get events.
	return true;
}



void ConversationPanel::Goto(int index)
{
	choices.clear();
	node = index;
	
	int oops = 0;
	while(node >= 0 && !conversation.Choices(node) && oops++ < 6)
	{
		text.push_back(wrap);
		text.back().Wrap(conversation.Text(node));
		node = conversation.NextNode(node);
	}
	if(node < 0)
		return;
	
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		choices.push_back(wrap);
		choices.back().Wrap(conversation.Text(node, i));
	}
	choice = 0;
}
