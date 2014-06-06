/* ConversationPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConversationPanel.h"

#include "Color.h"
#include "Conversation.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "shift.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <GL/glew.h>

using namespace std;

namespace {
	static const int WIDTH = 540;
	static const int TOP = 240;
}



ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation)
	: player(player), conversation(conversation), scroll(0)
{
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(WIDTH);
	wrap.SetFont(FontSet::Get(14));
	
	Goto(0);
}



void ConversationPanel::SetCallback(const Callback &callback)
{
	this->callback = callback;
}



// Draw this panel.
void ConversationPanel::Draw() const
{
	Color half(0., .7);
	Color back(0.125, 1.);
	FillShader::Fill(Point(0., 0.), Point(Screen::Width(), Screen::Height()), half);
	FillShader::Fill(
		Point(Screen::Width() * -.5 + WIDTH * .5 + 15., 0.),
		Point(WIDTH + 30., Screen::Height()), 
		back);
	
	const Sprite *edgeSprite = SpriteSet::Get("ui/right edge");
	if(edgeSprite->Height())
	{
		int steps = Screen::Height() / edgeSprite->Height();
		for(int y = -steps; y <= steps; ++y)
		{
			Point pos(Screen::Width() * -.5 + WIDTH + 45., y * 1000.);
			SpriteShader::Draw(edgeSprite, pos);
		}
	}
	
	SpriteShader::Draw(conversation.Scene(),
		Point(
			Screen::Width() * -.5 + WIDTH * .5 + 20.,
			Screen::Height() * -.5 + TOP * .5 + scroll));
	
	Point point(
		(-Screen::Width() / 2) + 20,
		(-Screen::Height() / 2) + TOP + scroll);
	
	const Font &font = FontSet::Get(14);
	
	Color dim(.2, 0.);
	Color grey(.5, 0.);
	for(const WrappedText &it : text)
	{
		it.Draw(point, grey);
		point.Y() += it.Height();
	}
	
	Color bright(.8, 0.);
	Color selectionColor(.1, 0.);
	
	if(node < 0)
	{
		string done = "[done]";
		Point off(Screen::Width() / -2 + 20 + WIDTH - font.Width(done), point.Y());
		font.Draw(done, off, bright);
		return;
	}
	if(choices.empty())
	{
		Point center = point + Point(choice ? 420 : 190, 7);
		Point size(150, 20);
		FillShader::Fill(center, size, selectionColor);
		int width = font.Width(choice ? lastName : firstName);
		center.X() += width - 67;
		FillShader::Fill(center, Point(1., 16.), dim);
		
		font.Draw("First name:", point + Point(40, 0), dim);
		font.Draw(firstName, point + Point(120, 0), choice ? grey : bright);
		
		font.Draw("Last name:", point + Point(270, 0), dim);
		font.Draw(lastName, point + Point(350, 0), choice ? bright : grey);
		return;
	}
	
	int i = 0;
	for(const WrappedText &it : choices)
	{
		if(i++ == choice)
		{
			Point center = point + Point(WIDTH, it.Height() - it.ParagraphBreak()) * .5;
			Point size(WIDTH, it.Height());
			
			FillShader::Fill(center, size, selectionColor);
		}
		it.Draw(point, bright);
		point.Y() += it.Height();
	}
}



// Only override the ones you need; the default action is to return false.
bool ConversationPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(node < 0)
	{
		if(key == SDLK_RETURN)
		{
			callback(node);
			GetUI()->Pop(this);
		}
		
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
			
			player.SetName(firstName, lastName);
			
			Goto(node + 1);
		}
		else
			return false;
		
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
	else
		return false;
	
	return true;
}



bool ConversationPanel::Click(int x, int y)
{
	if(choices.empty() && node >= 0)
	{
		// Get the x position relative to the left side of the screen.
		x -= -Screen::Width() / 2;
		if(x > 135 && x < 285)
			choice = 0;
		else if(x > 365 && x < 515)
			choice = 1;
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
