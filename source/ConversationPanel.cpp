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
	// Width of the conversation text.
	static const int WIDTH = 540;
	// Margin on either side of the text.
	static const int MARGIN = 20;
}



// Constructor.
ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system)
	: player(player), conversation(conversation), scroll(0.), system(system)
{
	// These substitutions need to be applied on the fly as each paragraph of
	// text is prepared for display.
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	if(player.Flagship())
		subs["<ship>"] = player.Flagship()->Name();
	
	// Begin at the start of the conversation.
	Goto(0);
}



// Draw this panel.
void ConversationPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();
	
	// Draw the panel itself, stretching from top to bottom of the screen on
	// the left side. The edge sprite contains 10 pixels of the margin; the rest
	// of the margin is included in the filled rectangle drawn here:
	Color back(0.125, 1.);
	double boxWidth = WIDTH + 2. * MARGIN - 10.;
	FillShader::Fill(
		Point(Screen::Left() + .5 * boxWidth, 0.),
		Point(boxWidth, Screen::Height()),
		back);
	
	const Sprite *edgeSprite = SpriteSet::Get("ui/right edge");
	if(edgeSprite->Height())
	{
		// If the screen is high enough, the edge sprite should repeat.
		double spriteHeight = edgeSprite->Height();
		Point pos(
			Screen::Left() + boxWidth + .5 * edgeSprite->Width(),
			Screen::Top() + .5 * spriteHeight);
		for( ; pos.Y() - .5 * spriteHeight < Screen::Bottom(); pos.Y() += spriteHeight)
			SpriteShader::Draw(edgeSprite, pos);
	}
	
	// Get the font and colors we'll need for drawing everything.
	const Font &font = FontSet::Get(14);
	Color selectionColor = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("dim");
	Color grey = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	// Figure out where we should start drawing.
	Point point(
		Screen::Left() + MARGIN,
		Screen::Top() + MARGIN + scroll);
	// Draw all the conversation text up to this point.
	for(const Paragraph &it : text)
		point = it.Draw(point, grey);
	
	// Draw whatever choices are being presented.
	zones.clear();
	if(node < 0)
	{
		// The conversation has already ended. Draw a "done" button.
		static const string done = "[done]";
		int width = font.Width(done);
		int height = font.Height();
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(done, off, bright);
		
		// Remember where the done button is.
		Point size(width, height);
		zones.emplace_back(off + .5 * size, size);
	}
	else if(choices.empty())
	{
		// This conversation node is prompting the player to enter their name.
		// Fill in whichever entry box is active right now.
		Point center = point + Point(choice ? 420 : 190, 7);
		Point size(150, 20);
		FillShader::Fill(center, size, selectionColor);
		// Draw the text cursor.
		int width = font.Width(choice ? lastName : firstName);
		int height = font.Height();
		center.X() += width - 67;
		FillShader::Fill(center, Point(1., 16.), dim);
		
		font.Draw("First name:", point + Point(40, 0), dim);
		font.Draw(firstName, point + Point(120, 0), choice ? grey : bright);
		
		font.Draw("Last name:", point + Point(270, 0), dim);
		font.Draw(lastName, point + Point(350, 0), choice ? bright : grey);
		
		// Draw the OK button, and remember its location.
		static const string ok = "[ok]";
		width = font.Width(ok);
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(ok, off, bright);
		size = Point(width, height);
		zones.emplace_back(off + .5 * size, size);
	}
	else
	{
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
			point = it.Draw(point, bright);
		}
	}
	// Store the total height of the text.
	maxScroll = min(0, Screen::Top() - static_cast<int>(point.Y() - scroll) + font.Height() + 15);
}



// Handle key presses.
bool ConversationPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	// Map popup happens when you press the map key, unless the name text entry
	// fields are currently active.
	if(command.Has(Command::MAP) && !choices.empty())
		GetUI()->Push(new MapDetailPanel(player, MapPanel::SHOW_REPUTATION, system));
	if(node < 0)
	{
		// If the conversation has ended, the only possible action is to exit.
		if(key == SDLK_RETURN)
		{
			Exit();
			return true;
		}
		return false;
	}
	if(choices.empty())
	{
		// Right now we're asking the player to enter their name.
		string &name = (choice ? lastName : firstName);
		string &otherName = (choice ? firstName : lastName);
		// Allow editing the text. The tab key toggles to the other entry field,
		// as does the return key if the other field is still empty.
		if(key >= ' ' && key <= '~')
		{
			// Apply the shift or caps lock key.
			char c = ((mod & (KMOD_SHIFT | KMOD_CAPS)) ? SHIFT[key] : key);
			// Don't allow characters that can't be used in a file name.
			static const string FORBIDDEN = "/\\?*:|\"<>~";
			if(FORBIDDEN.find(c) == string::npos)
				name += c;
		}
		else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && name.size())
			name.erase(name.size() - 1);
		else if(key == '\t' || (key == SDLK_RETURN && otherName.empty()))
			choice = !choice;
		else if(key == SDLK_RETURN && !firstName.empty() && !lastName.empty())
		{
			// Display the name the player entered.
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
	
	// Let the player select choices by using the arrow keys and then pressing
	// return, or by pressing a number key.
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



// Handle mouse clicks.
bool ConversationPanel::Click(int x, int y)
{
	Point point(x, y);
	if(node < 0)
	{
		// The conversation has ended. Check if the player clicked "done."
		if(zones.empty() || zones.front().Contains(point))
			Exit();
	}
	else if(choices.empty())
	{
		// We're currently showing a name entry field. Check if "OK" was clicked.
		if(!zones.empty() && zones.front().Contains(point))
			return DoKey(SDLK_RETURN);
		
		// Select whichever text entry box was clicked on.
		x -= Screen::Left();
		if(x > 135 && x < 285)
			choice = 0;
		else if(x > 365 && x < 515)
			choice = 1;
	}
	else
	{
		// This is an ordinary node. Check if one of the choices was clicked.
		for(unsigned i = 0; i < zones.size(); ++i)
			if(zones[i].Contains(point))
			{
				Goto(conversation.NextNode(node, i), i);
				break;
			}
	}
	return true;
}



// Allow scrolling by click and drag.
bool ConversationPanel::Drag(double dx, double dy)
{
	scroll = min(0., max(static_cast<double>(maxScroll), scroll + dy));
	
	return true;
}



// Handle the scroll wheel.
bool ConversationPanel::Scroll(double dx, double dy)
{
	return Drag(50. * dx, 50. * dy);
}



// The player just selected the given choice.
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
			scroll = -11;
			for(const Paragraph &it : text)
				scroll -= it.Height();
		}
	}
	
	// We'll need to reload the choices from whatever new node we arrive at.
	choices.clear();
	node = index;
	// Not every conversation node allows a choice. Move forward through the
	// nodes until we encounter one that does, or the conversation ends.
	while(node >= 0 && !conversation.IsChoice(node))
	{
		int choice = 0;
		if(conversation.IsBranch(node))
		{
			// Branch nodes change the flow of the conversation based on the
			// player's condition variables rather than player input.
			choice = !conversation.Conditions(node).Test(player.Conditions());
		}
		else if(conversation.IsApply(node))
		{
			// Apply nodes alter the player's condition variables but do not
			// display any conversation text of their own.
			conversation.Conditions(node).Apply(player.Conditions());
		}
		else
		{
			// This is an ordinary conversation node. Perform any necessary text
			// replacement, then add the text to the display.
			string altered = Format::Replace(conversation.Text(node), subs);
			text.emplace_back(altered, conversation.Scene(node), text.empty());
		}
		node = conversation.NextNode(node, choice);
	}
	// Display whatever choices are being offered to the player.
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		string altered = Format::Replace(conversation.Text(node, i), subs);
		choices.emplace_back(altered);
	}
	this->choice = 0;
}



// Exit this panel and do whatever needs to happen next.
void ConversationPanel::Exit()
{
	GetUI()->Pop(this);
	if(player.BoardingShip())
	{
		// If boarding a ship, you may plunder or destroy it depending on the
		// outcome of the conversation.
		if(Conversation::RequiresLaunch(node))
			player.BoardingShip()->Destroy();
		else if(player.BoardingShip()->GetGovernment()->IsEnemy())
		{
			if(node != Conversation::ACCEPT)
				GetUI()->Push(new BoardingPanel(player, player.BoardingShip()));
		}
	}
	if(callback)
		callback(node);
}



// Paragraph constructor.
ConversationPanel::Paragraph::Paragraph(const string &text, const Sprite *scene, bool isFirst)
	: scene(scene), isFirst(isFirst)
{
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(WIDTH);
	wrap.SetFont(FontSet::Get(14));
	
	wrap.Wrap(text);
}



// Get the height of this paragraph (including any "scene" image).
int ConversationPanel::Paragraph::Height() const
{
	int height = wrap.Height();
	if(scene)
		height += 20 * !isFirst + scene->Height() + 20;
	return height;
}



// Get the center point of this paragraph.
Point ConversationPanel::Paragraph::Center() const
{
	return Point(.5 * WIDTH, .5 * (Height() - wrap.ParagraphBreak()));
}



// Draw this paragraph, and return the point that the next paragraph below it
// should be drawn at.
Point ConversationPanel::Paragraph::Draw(Point point, const Color &color) const
{
	if(scene)
	{
		Point offset(WIDTH / 2, 20 * !isFirst + scene->Height() / 2);
		SpriteShader::Draw(scene, point + offset);
		point.Y() += 20 * !isFirst + scene->Height() + 20;
	}
	wrap.Draw(point, color);
	point.Y() += wrap.Height();
	return point;
}
