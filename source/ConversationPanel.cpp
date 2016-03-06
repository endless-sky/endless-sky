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

#include "BoardingPanel.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "shift.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <iterator>

using namespace std;

namespace {
	static const int WIDTH = 540;
}



ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system)
	: player(player), conversation(conversation), scroll(0.), system(system)
{
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	if(player.Flagship())
		subs["<ship>"] = player.Flagship()->Name();
	
	Goto(0);
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
	
	const Font &font = FontSet::Get(14);
	Point point(
		(-Screen::Width() / 2) + 20,
		(-Screen::Height() / 2) + 20 + scroll);
	
	Color selectionColor = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("dim");
	Color grey = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	for(const Paragraph &it : text)
		it.Draw(point, grey);
	
	zones.clear();
	if(node < 0)
	{
		static const string done = "[done]";
		int width = font.Width(done);
		int height = font.Height();
		Point off(Screen::Left() + 20 + WIDTH - width, point.Y());
		font.Draw(done, off, bright);
		
		Point size(width, height);
		zones.emplace_back(off + .5 * size, size);
		maxScroll = min(0, Screen::Top() - static_cast<int>(point.Y() - scroll) + height / 2);
		return;
	}
	if(choices.empty())
	{
		Point center = point + Point(choice ? 420 : 190, 7);
		Point size(150, 20);
		FillShader::Fill(center, size, selectionColor);
		int width = font.Width(choice ? lastName : firstName);
		int height = font.Height();
		center.X() += width - 67;
		FillShader::Fill(center, Point(1., 16.), dim);
		
		font.Draw("First name:", point + Point(40, 0), dim);
		font.Draw(firstName, point + Point(120, 0), choice ? grey : bright);
		
		font.Draw("Last name:", point + Point(270, 0), dim);
		font.Draw(lastName, point + Point(350, 0), choice ? bright : grey);
		
		static const string ok = "[ok]";
		width = font.Width(ok);
		Point off(Screen::Left() + 20 + WIDTH - width, point.Y());
		font.Draw(ok, off, bright);
		size = Point(width, height);
		zones.emplace_back(off + .5 * size, size);
		maxScroll = min(0, Screen::Top() - static_cast<int>(point.Y() - scroll) + height / 2);

		return;
	}
	
	string label = "0:";
	for(const Paragraph &it : choices)
	{
		++label[0];
		
		Point center = point + it.Center();
		Point size(WIDTH, it.Height());
		
		if(zones.size() == static_cast<unsigned>(choice))
			FillShader::Fill(center + Point(-5, 0), size + Point(30, 0), selectionColor);
		zones.emplace_back(point + .5 * size, size);
		
		font.Draw(label, point + Point(-15, 0), dim);
		it.Draw(point, bright);
	}
	maxScroll = min(0, Screen::Top() - static_cast<int>(point.Y() - scroll) + font.Height() + 15);
}



// Only override the ones you need; the default action is to return false.
bool ConversationPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	// Map popup happens when you press the map key, unless the name text entry
	// fields are currently active.
	if(command.Has(Command::MAP) && !choices.empty())
		GetUI()->Push(new MapDetailPanel(player, MapPanel::SHOW_REPUTATION, system));
	if(node < 0)
	{
		if(key == SDLK_RETURN)
		{
			SelectNode();
			return true;
		}
		return false;
	}
	if(choices.empty())
	{
		string &name = (choice ? lastName : firstName);
		string &otherName = (choice ? firstName : lastName);
		if(key >= ' ' && key <= '~')
			name += ((mod & (KMOD_SHIFT | KMOD_CAPS)) ? SHIFT[key] : key);
		else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && name.size())
			name.erase(name.size() - 1);
		else if(key == '\t' || (key == SDLK_RETURN && otherName.empty()))
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
			text.emplace_back(name);
			
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
		Goto(conversation.NextNode(node, choice), choice);
	else if(key > '0' && key <= static_cast<SDL_Keycode>('0' + choices.size()))
		Goto(conversation.NextNode(node, key - '1'), key - '1');
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
			SelectNode();
		}
	}
	else if(choices.empty() && node >= 0)
	{
		if(!zones.empty() && zones.front().Contains(point))
			return DoKey(SDLK_RETURN);
		
		// Get the x position relative to the left side of the screen.
		x -= Screen::Left();
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
				Goto(conversation.NextNode(node, i), i);
				break;
			}
	}
	return true;
}



bool ConversationPanel::Drag(double dx, double dy)
{
	scroll = min(0., max(static_cast<double>(maxScroll), scroll + dy));
	
	return true;
}



bool ConversationPanel::Scroll(double dx, double dy)
{
	return Drag(50. * dx, 50. * dy);
}




void ConversationPanel::Goto(int index, int choice)
{
	if(index)
	{
		// Add the chosen option to the text.
		if(choice >= 0 && choice < static_cast<int>(choices.size()))
			text.splice(text.end(), choices, next(choices.begin(), choice));
		
		// Scroll to the start of the new text, unless the conversation ended.
		if(index >= 0)
		{
			int y = 20;
			for(const Paragraph &it : text)
				y += it.Height();
			scroll = -y + 9;
		}
	}
	
	choices.clear();
	node = index;
	
	while(node >= 0 && !conversation.IsChoice(node))
	{
		if(conversation.IsBranch(node))
		{
			int choice = !conversation.Conditions(node).Test(player.Conditions());
			node = conversation.NextNode(node, choice);
			continue;
		}
		if(conversation.IsApply(node))
		{
			conversation.Conditions(node).Apply(player.Conditions());
			node = conversation.NextNode(node);
			continue;
		}
		
		string altered = Format::Replace(conversation.Text(node), subs);
		text.emplace_back(altered, conversation.Scene(node));
		node = conversation.NextNode(node);
	}
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		string altered = Format::Replace(conversation.Text(node, i), subs);
		choices.emplace_back(altered);
	}
	this->choice = 0;
}



// Called by KeyDown or Click when a conversation node is selected (by clicking or pressing enter).  
void ConversationPanel::SelectNode()
{
	GetUI()->Pop(this);
	if(callback)
	{
		if(player.BoardingShip())
		{
			if(Conversation::LeaveImmediately(node))
				player.BoardingShip()->Destroy();
			else if(player.BoardingShip()->GetGovernment()->IsEnemy())
			{
				if(node != Conversation::ACCEPT)
					GetUI()->Push(new BoardingPanel(player, player.BoardingShip()));
			}
		}
		callback(node);
	}
}



ConversationPanel::Paragraph::Paragraph(const string &text, const Sprite *scene)
	: scene(scene)
{
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(WIDTH);
	wrap.SetFont(FontSet::Get(14));
	
	wrap.Wrap(text);
}



int ConversationPanel::Paragraph::Height() const
{
	int height = wrap.Height();
	if(scene)
		height += 40 + scene->Height();
	return height;
}



Point ConversationPanel::Paragraph::Center() const
{
	return Point(.5 * WIDTH, .5 * (Height() - wrap.ParagraphBreak()));
}



void ConversationPanel::Paragraph::Draw(Point &point, const Color &color) const
{
	if(scene)
	{
		Point offset(WIDTH / 2, 20 + scene->Height() / 2);
		SpriteShader::Draw(scene, point + offset);
		point.Y() += 40 + scene->Height();
	}
	wrap.Draw(point, color);
	point.Y() += wrap.Height();
}
