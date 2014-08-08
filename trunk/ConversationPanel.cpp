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
#include "GameData.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "shift.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#include <GL/glew.h>
#endif

using namespace std;

namespace {
	static const int WIDTH = 540;
}



ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system)
	: player(player), conversation(conversation), scroll(0), system(system)
{
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(WIDTH);
	wrap.SetFont(FontSet::Get(14));
	
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	
	Goto(0);
}



void ConversationPanel::SetCallback(const Callback &callback)
{
	this->callback = callback;
}



// Draw this panel.
void ConversationPanel::Draw() const
{
	DrawBackdrop();
	
	Color back(0.125, 1.);
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
	
	int sceneHeight = 20;
	if(conversation.Scene() && conversation.Scene()->Height())
	{
		sceneHeight = 40 + conversation.Scene()->Height();
		SpriteShader::Draw(conversation.Scene(),
			Point(
				Screen::Width() * -.5 + WIDTH * .5 + 20.,
				Screen::Height() * -.5 + sceneHeight * .5 + scroll));
	}
	
	Point point(
		(-Screen::Width() / 2) + 20,
		(-Screen::Height() / 2) + sceneHeight + scroll);
	
	const Font &font = FontSet::Get(14);
	
	Color selectionColor = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("dim");
	Color grey = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	for(const WrappedText &it : text)
	{
		it.Draw(point, grey);
		point.Y() += it.Height();
	}
	
	zones.clear();
	if(node < 0)
	{
		string done = "[done]";
		int width = font.Width(done);
		Point off(Screen::Width() / -2 + 20 + WIDTH - font.Width(done), point.Y());
		font.Draw(done, off, bright);
		zones.emplace_back(off, Point(width, font.Height()));
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
	
	for(const WrappedText &it : choices)
	{
		Point center = point + Point(WIDTH, it.Height() - it.ParagraphBreak()) * .5;
		Point size(WIDTH, it.Height());
		
		if(zones.size() == static_cast<unsigned>(choice))
			FillShader::Fill(center, size, selectionColor);
		zones.emplace_back(point, size);
		
		it.Draw(point, bright);
		point.Y() += it.Height();
	}
}



// Only override the ones you need; the default action is to return false.
bool ConversationPanel::KeyDown(SDL_Keycode key, Uint16 mod)
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
			subs["<first>"] = player.FirstName();
			subs["<last>"] = player.LastName();
			
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
	else if(key == GameData::Keys().Get(Key::MAP))
		GetUI()->Push(new MapDetailPanel(player, -4, system));
	else
		return false;
	
	return true;
}



bool ConversationPanel::Click(int x, int y)
{
	Point point(x, y);
	if(node < 0)
	{
		if(zones.empty() || zones.front().Contains(point))
		{
			callback(node);
			GetUI()->Pop(this);
		}
	}
	else if(choices.empty() && node >= 0)
	{
		// Get the x position relative to the left side of the screen.
		x -= -Screen::Width() / 2;
		if(x > 135 && x < 285)
			choice = 0;
		else if(x > 365 && x < 515)
			choice = 1;
	}
	else
	{
		for(unsigned i = 0; i < zones.size(); ++i)
			if(zones[i].Contains(point))
			{
				Goto(conversation.NextNode(node, i));
				break;
			}
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
		string altered = Substitute(conversation.Text(node));
		text.back().Wrap(altered);
		node = conversation.NextNode(node);
	}
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		choices.push_back(wrap);
		string altered = Substitute(conversation.Text(node, i));
		choices.back().Wrap(altered);
	}
	choice = 0;
	
	int y = scroll;
	if(conversation.Scene())
		y += conversation.Scene()->Height();
	for(const WrappedText &it : text)
		y += it.Height();
	for(const WrappedText &it : choices)
		y += it.Height();
	if(choices.empty())
		y += 20;
	
	if(y > Screen::Height())
		scroll -= (y - Screen::Height());
}



string ConversationPanel::Substitute(const string &source) const
{
	string result;
	result.reserve(source.length());
	
	size_t start = 0;
	size_t search = start;
	while(search < source.length())
	{
		size_t left = source.find('<', search);
		if(left == string::npos)
			break;
		
		size_t right = source.find('>', left);
		if(right == string::npos)
			break;
		
		bool matched = false;
		++right;
		size_t length = right - left;
		for(const auto &it : subs)
			if(!source.compare(left, length, it.first))
			{
				result.append(source, start, left - start);
				result.append(it.second);
				start = right;
				search = start;
				matched = true;
				break;
			}
		
		if(!matched)
			search = left + 1;
	}
	
	result.append(source, start, source.length() - start);
	return result;
}



ConversationPanel::ClickZone::ClickZone(const Point &topLeft, const Point &size)
	: topLeft(topLeft), size(size)
{
}



bool ConversationPanel::ClickZone::Contains(const Point &point)
{
	Point r = point - topLeft;
	return (r.X() >= 0. && r.Y() >= 0. && r.X() < size.X() && r.Y() < size.Y());
}
