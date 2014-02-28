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
#include "shift.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <GL/glew.h>

using namespace std;

namespace {
	static const int WIDTH = 540;
	static const int TOP = 240;
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
	Color half(0., .7);
	Color back(0.125, 1.);
	FillShader::Fill(Point(0., 0.), Point(Screen::Width(), Screen::Height()), half.Get());
	FillShader::Fill(
		Point(Screen::Width() * -.5 + WIDTH * .5 + 15., 0.),
		Point(WIDTH + 30., Screen::Height()), 
		back.Get());
	
	SpriteShader::Draw(
		SpriteSet::Get("ui/right edge"),
		Point(Screen::Width() * -.5 + WIDTH + 45., 0.));
	
	SpriteShader::Draw(conversation.Scene(),
		Point(
			Screen::Width() * -.5 + WIDTH * .5 + 20.,
			Screen::Height() * -.5 + TOP * .5 + scroll));
	
	int x = (-Screen::Width() / 2) + 20;
	int y = (-Screen::Height() / 2) + TOP + scroll;
	
	const Font &font = FontSet::Get(14);
	
	Color dim(.2, 0.);
	Color grey(.5, 0.);
	for(const WrappedText &it : text)
	{
		for(const WrappedText::Word &word : it.Words())
		{
			// TODO: color your choices brighter.
			Point point(word.X() + x, word.Y() + y);
			font.Draw(word.String(), point, grey.Get());
		}
		y += it.Height();
	}
	
	Color bright(.8, 0.);
	Color selectionColor(.1, 0.);
	
	if(node < 0)
	{
		string done = "[done]";
		font.Draw(done, 
			Point(Screen::Width() / -2 + 20 + WIDTH - font.Width(done), y),
			bright.Get());
		return;
	}
	if(choices.empty())
	{
		Point center(x + (choice ? 420 : 190), y + 7);
		Point size(150, 20);
		FillShader::Fill(center, size, selectionColor.Get());
		
		font.Draw("First name:", Point(x + 40, y), dim.Get());
		font.Draw(firstName, Point(x + 120, y), (choice ? grey : bright).Get());
		
		font.Draw("Last name:", Point(x + 270, y), dim.Get());
		font.Draw(lastName, Point(x + 350, y), (choice ? bright : grey).Get());
		return;
	}
	
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
			font.Draw(word.String(), point, bright.Get());
		}
		y += it.Height();
	}
}



// Only override the ones you need; the default action is to return false.
bool ConversationPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(node < 0)
	{
		if(key == SDLK_RETURN)
			Pop(this);
		
		return true;
	}
	if(choices.empty())
	{
		string &name = (choice ? lastName : firstName);
		if(key >= ' ' && key <= '~')
			name += ((mod & KMOD_SHIFT) ? SHIFT[key] : key);
		else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && name.size())
			name.erase(name.size() - 1);
		else if(key == '\t')
			choice = !choice;
		else if(key == SDLK_RETURN && !firstName.empty() && !lastName.empty())
		{
			for(char &c : firstName)
				if(c == '~')
					c = '-';
			for(char &c : lastName)
				if(c == '~')
					c = '-';
			
			string name = "\t\tName: " + firstName + " " + lastName + ".\n";
			text.push_back(wrap);
			text.back().Wrap(name);
			
			Goto(node + 1);
		}
		return true;
	}
	
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
	
	return true;
}



bool ConversationPanel::Drag(int dx, int dy)
{
	scroll = min(0, scroll + dy);
	
	return true;
}



void ConversationPanel::Goto(int index)
{
	choices.clear();
	node = index;
	
	while(node >= 0 && !conversation.IsChoice(node))
	{
		text.push_back(wrap);
		text.back().Wrap(conversation.Text(node));
		node = conversation.NextNode(node);
	}
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		choices.push_back(wrap);
		choices.back().Wrap(conversation.Text(node, i));
	}
	choice = 0;
	
	int y = TOP + scroll;
	for(const WrappedText &it : text)
		y += it.Height();
	for(const WrappedText &it : choices)
		y += it.Height();
	if(choices.empty())
		y += 20;
	
	if(y > Screen::Height())
		scroll -= (y - Screen::Height());
}
