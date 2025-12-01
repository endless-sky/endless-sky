/* ConversationPanel.cpp
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

#include "ConversationPanel.h"

#include "text/Alignment.h"
#include "audio/Audio.h"
#include "BoardingPanel.h"
#include "text/Clipboard.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "shift.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "UI.h"

#if defined _WIN32
#include "Files.h"
#endif

#include <array>
#include <iterator>

using namespace std;

namespace {
#if defined _WIN32
	size_t PATH_LENGTH;
#endif
	// Width of the conversation text.
	const int WIDTH = 540;
	// Margin on either side of the text.
	const int MARGIN = 20;
}



// Constructor.
ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation,
	const Mission *caller, const System *system, const shared_ptr<Ship> &ship, bool useTransactions)
	: player(player), caller(caller), useTransactions(useTransactions), conversation(conversation),
	scroll(0.), system(system), ship(ship)
{
#if defined _WIN32
	PATH_LENGTH = Files::Saves().string().size();
#endif
	Audio::Pause();
	// These substitutions need to be applied on the fly as each paragraph of
	// text is prepared for display. Some substitutions already in the map
	// should not be overwritten.
	static const array<string, 3> subsToSave = {"<system>", "<date>", "<day>"};
	map<string, string> savedSubs;
	for(const auto &sub : subsToSave)
	{
		const auto it = subs.find(sub);
		if(it != subs.end() && !it->second.empty())
			savedSubs.emplace(*it);
	}
	player.AddPlayerSubstitutions(subs);
	// Restore substitutions that need to be preserved.
	for(auto &it : savedSubs)
		subs[it.first].swap(it.second);
	if(ship)
	{
		subs["<ship>"] = ship->GivenName();
		subs["<model>"] = ship->DisplayModelName();
	}

	// Start a PlayerInfo transaction to prevent saves during the conversation
	// from recording partial results.
	if(useTransactions)
		player.StartTransaction();

	// Begin at the start of the conversation.
	Goto(0);
}



ConversationPanel::~ConversationPanel()
{
	Audio::Resume();
}



void ConversationPanel::SetCallback(function<void(int)> fun)
{
	callback = std::move(fun);
}



// Draw this panel.
void ConversationPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();

	// Draw the panel itself, stretching from top to bottom of the screen on
	// the left side. The edge sprite contains 10 pixels of the margin; the rest
	// of the margin is included in the filled rectangle drawn here:
	const Color &back = *GameData::Colors().Get("conversation background");
	double boxWidth = WIDTH + 2. * MARGIN - 10.;
	FillShader::Fill(
		Point(Screen::Left() + .5 * boxWidth, 0.),
		Point(boxWidth, Screen::Height()),
		back);

	Panel::DrawEdgeSprite(SpriteSet::Get("ui/right edge"), Screen::Left() + boxWidth);

	// Get the font and colors we'll need for drawing everything.
	const Font &font = FontSet::Get(14);
	const Color &selectionColor = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &gray = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dark = *GameData::Colors().Get("dark");

	// Figure out where we should start drawing.
	Point point(
		Screen::Left() + MARGIN,
		Screen::Top() + MARGIN + scroll);
	// Draw all the conversation text up to this point.
	for(const Paragraph &it : text)
		point = it.Draw(point, gray);

	// Draw whatever choices are being presented.
	if(node < 0)
	{
		// The conversation has already ended. Draw a "done" button.
		static const string done = "[done]";
		int width = font.Width(done);
		int height = font.Height();
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(done, off, bright);

		// Handle clicks on this button.
		AddZone(Rectangle::FromCorner(off, Point(width, height)), [this](){ this->Exit(); });
	}
	else if(choices.empty())
	{
		// This conversation node is prompting the player to enter their name.
		Point fieldSize(150, 20);
		const auto layout = Layout(fieldSize.X() - 10, Truncate::FRONT);
		for(int side = 0; side < 2; ++side)
		{
			Point center = point + Point(side ? 420 : 190, 7);
			Point unselected = point + Point(side ? 190 : 420, 7);
			// Handle mouse clicks in whatever field is not selected.
			if(side != choice)
			{
				AddZone(Rectangle(center, fieldSize), [this, side](){ this->ClickName(side); });
				continue;
			}

			// Color selected text box, or flicker if user attempts an error.
			FillShader::Fill(center, fieldSize, (flickerTime % 6 > 3) ? dim : selectionColor);
			if(flickerTime)
				--flickerTime;
			// Fill non-selected text box with dimmer color.
			FillShader::Fill(unselected, fieldSize, dark);
			// Draw the text cursor.
			center.X() += font.FormattedWidth({choice ? lastName : firstName, layout}) - 67;
			FillShader::Fill(center, Point(1., 16.), dim);
		}

		font.Draw("First name:", point + Point(40, 0), dim);
		font.Draw({firstName, layout}, point + Point(120, 0), choice ? gray : bright);

		font.Draw("Last name:", point + Point(270, 0), dim);
		font.Draw({lastName, layout}, point + Point(350, 0), choice ? bright : gray);

		// Draw the OK button, and remember its location.
		static const string ok = "[ok]";
		int width = font.Width(ok);
		int height = font.Height();
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(ok, off, bright);

		// Handle clicks on this button.
		AddZone(Rectangle::FromCorner(off, Point(width, height)), SDLK_RETURN);
	}
	else
	{
		string label = "0:";
		int index = 0;
		for(const auto &it : choices)
		{
			++label[0];

			const auto &paragraph = it.first;

			Point center = point + paragraph.Center();
			Point size(WIDTH, paragraph.Height());

			auto zone = Rectangle::FromCorner(point, size);
			// If the mouse is hovering over this choice then we need to highlight it.
			if(isHovering && zone.Contains(hoverPoint))
				choice = index;

			if(index == choice)
				FillShader::Fill(center + Point(-5, 0), size + Point(30, 0), selectionColor);
			AddZone(zone, [this, index](){ this->ClickChoice(index); });

			font.Draw(label, point + Point(-15, 0), dim);
			point = paragraph.Draw(point, conversation.ChoiceIsActive(node, MapChoice(index)) ? bright : dim);

			++index;
		}
	}
	// Store the total height of the text.
	maxScroll = min(0., Screen::Top() - (point.Y() - scroll) + font.Height() + 15.);

	// Reset the hover flag. If the mouse is still moving than the flag will be set in the next frame.
	isHovering = false;
}



// Handle key presses.
bool ConversationPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;
	// Map popup happens when you press the map key, unless the name text entry
	// fields are currently active. The name text entry fields are active if
	// choices is empty and we aren't at the end of the conversation.
	if(command.Has(Command::MAP) && (!choices.empty() || node < 0))
	{
		sound = UI::UISound::NONE;
		GetUI()->Push(new MapDetailPanel(player, system, true));
	}
	if(node < 0)
	{
		// If the conversation has ended, the only possible action is to exit.
		if(isNewPress && (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == 'd'))
		{
			Exit();
			return true;
		}
		return false;
	}
	if(choices.empty())
	{
		// Don't allow characters that can't be used in a file name.
		static const string FORBIDDEN = "/\\?*:|\"<>~";
		// Prevent the name from being so large that it cannot be saved.
		// Most path components can be at most 255 bytes.
		size_t MAX_NAME_LENGTH = 250;
#if defined _WIN32
		MAX_NAME_LENGTH -= PATH_LENGTH;
#endif

		// Right now we're asking the player to enter their name.
		string &name = (choice ? lastName : firstName);
		string &otherName = (choice ? firstName : lastName);
		// Allow editing the text. The tab key toggles to the other entry field,
		// as does the return key if the other field is still empty.
		if(Clipboard::KeyDown(name, key, mod, MAX_NAME_LENGTH, FORBIDDEN))
		{
			// Input handled by Clipboard.
		}
		else if(key >= ' ' && key <= '~')
		{
			// Apply the shift or caps lock key.
			char c = ((mod & KMOD_SHIFT) ? SHIFT[key] : key);
			// Caps lock should shift letters, but not any other keys.
			if((mod & KMOD_CAPS) && c >= 'a' && c <= 'z')
				c += 'A' - 'a';
			if(FORBIDDEN.find(c) == string::npos && (name.size() + otherName.size()) < MAX_NAME_LENGTH)
				name += c;
			else
				flickerTime = 18;
		}
		else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && !name.empty())
			name.erase(name.size() - 1);
		else if(key == '\t' || ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && otherName.empty()))
			choice = !choice;
		else if((key == SDLK_RETURN || key == SDLK_KP_ENTER) && (firstName.empty() || lastName.empty()))
			flickerTime = 18;
		else if((key == SDLK_RETURN || key == SDLK_KP_ENTER) && !firstName.empty() && !lastName.empty())
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
	else if(key == SDLK_DOWN && choice + 1 < static_cast<int>(choices.size()))
		++choice;
	else if((key == SDLK_RETURN || key == SDLK_KP_ENTER) && isNewPress && choice < static_cast<int>(choices.size()))
	{
		if(conversation.ChoiceIsActive(node, MapChoice(choice)))
			Goto(conversation.NextNodeForChoice(node, MapChoice(choice)), choice);
		else
			sound = UI::UISound::FAILURE;
	}
	else if(key >= '1' && key < static_cast<SDL_Keycode>('1' + choices.size()))
	{
		if(conversation.ChoiceIsActive(node, MapChoice(key - '1')))
			Goto(conversation.NextNodeForChoice(node, MapChoice(key - '1')), key - '1');
		else
			sound = UI::UISound::FAILURE;
	}
	else if(key >= SDLK_KP_1 && key < static_cast<SDL_Keycode>(SDLK_KP_1 + choices.size()))
	{
		if(conversation.ChoiceIsActive(node, MapChoice(key - SDLK_KP_1)))
			Goto(conversation.NextNodeForChoice(node, MapChoice(key - SDLK_KP_1)), key - SDLK_KP_1);
		else
			sound = UI::UISound::FAILURE;
	}
	else
		return false;

	UI::PlaySound(sound);
	return true;
}



// Allow scrolling by click and drag.
bool ConversationPanel::Drag(double dx, double dy)
{
	scroll = min(0., max(maxScroll, scroll + dy));

	return true;
}



// Handle the scroll wheel.
bool ConversationPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



// Handle selecting choices by hovering with the mouse.
bool ConversationPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	isHovering = true;
	return true;
}



// The player just selected the given choice.
void ConversationPanel::Goto(int index, int selectedChoice)
{
	const ConditionsStore &conditions = player.Conditions();
	Format::ConditionGetter getter = [&conditions](const string &str, size_t start, size_t length) -> int64_t
	{
		return conditions.Get(str.substr(start, length));
	};

	if(index)
	{
		// Add the chosen option to the text.
		if(selectedChoice >= 0 && selectedChoice < static_cast<int>(choices.size()))
			text.emplace_back(next(choices.begin(), selectedChoice)->first);

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
	while(node >= 0 && !conversation.HasAnyChoices(node))
	{
		int choice = 0;

		// Skip empty choices.
		if(conversation.IsChoice(node))
		{
			node = conversation.StepToNextNode(node);
			continue;
		}

		if(conversation.IsBranch(node))
		{
			// Branch nodes change the flow of the conversation based on the
			// player's condition variables rather than player input.
			choice = !conversation.Conditions(node).Test();
		}
		else if(conversation.IsAction(node))
		{
			// Action nodes are able to perform various actions, e.g. changing
			// the player's conditions, granting payments, triggering events,
			// and more. They are not allowed to spawn additional UI elements.
			conversation.GetAction(node).Do(player, nullptr, caller);
		}
		else if(conversation.ShouldDisplayNode(node))
		{
			// This is an ordinary conversation node which should be displayed.
			// Perform any necessary text replacement, and add the text to the display.
			string altered = Format::ExpandConditions(Format::Replace(conversation.Text(node), subs), getter);
			text.emplace_back(altered, conversation.Scene(node), text.empty());
		}
		else
		{
			// This conversation node should not be displayed, so skip its goto.
			node = conversation.StepToNextNode(node);
			continue;
		}

		node = conversation.NextNodeForChoice(node, choice);
	}
	// Display whatever choices are being offered to the player.
	for(int i = 0; i < conversation.Choices(node); ++i)
		if(conversation.ShouldDisplayNode(node, i))
		{
			string altered = Format::ExpandConditions(Format::Replace(conversation.Text(node, i), subs), getter);
			choices.emplace_back(Paragraph(altered), i);
		}
	// This is a safeguard in case of logic errors, to ensure we don't set the player name.
	if(choices.empty() && conversation.Choices(node) != 0)
	{
		node = Conversation::DECLINE;
	}
	this->choice = 0;
}



// Exit this panel and do whatever needs to happen next.
void ConversationPanel::Exit()
{
	// Finish the PlayerInfo transaction so any changes get saved again.
	if(useTransactions)
		player.FinishTransaction();

	GetUI()->Pop(this);
	// Some conversations may be offered from an NPC, e.g. an assisting or
	// boarding mission's `on offer`, or from completing a mission's NPC
	// block (e.g. scanning or boarding or killing all required targets).
	if(node == Conversation::DIE || node == Conversation::EXPLODE)
		player.Die(node, ship);
	else if(ship)
	{
		// A forced-launch ending (LAUNCH, FLEE, or DEPART) destroys any NPC.
		if(Conversation::RequiresLaunch(node))
			ship->Destroy();
		// Only show the BoardingPanel for a hostile NPC that is being boarded.
		// (NPC completion conversations can result from non-boarding events.)
		// TODO: Is there a better / more robust boarding check than relative position?
		else if((node != Conversation::ACCEPT || player.CaptureOverriden(ship)) && ship->GetGovernment()->IsEnemy()
				&& !ship->IsDestroyed() && ship->IsDisabled()
				&& ship->Position().Distance(player.Flagship()->Position()) <= 1.)
			GetUI()->Push(new BoardingPanel(player, ship));
	}
	// Call the exit response handler to manage the conversation's effect
	// on the player's missions, or force takeoff from a planet.
	if(callback)
		callback(node);
}



// The player just clicked one of the two name entry text fields.
void ConversationPanel::ClickName(int side)
{
	choice = side;
}



// The player just clicked on a conversation choice.
void ConversationPanel::ClickChoice(int index)
{
	if(conversation.ChoiceIsActive(node, MapChoice(index)))
		Goto(conversation.NextNodeForChoice(node, MapChoice(index)), index);
	else
		UI::PlaySound(UI::UISound::FAILURE);
}


// Given an index into the list of displayed choices (i.e. not including
// conditionally-skipped choices), return its "raw index" in the conversation
// (i.e. including conditionally-skipped choices)
int ConversationPanel::MapChoice(int n) const
{
	if(n < 0 || n >= static_cast<int>(choices.size()))
		return 0;
	else
		return next(choices.cbegin(), n)->second;
}



// Paragraph constructor.
ConversationPanel::Paragraph::Paragraph(const string &text, const Sprite *scene, bool isFirst)
	: scene(scene), isFirst(isFirst)
{
	wrap.SetAlignment(Alignment::JUSTIFIED);
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
