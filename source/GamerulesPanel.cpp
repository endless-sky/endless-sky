/* GamerulesPanel.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GamerulesPanel.h"

#include "text/Alignment.h"
#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "RenderBuffer.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "shader/StarField.h"
#include "text/Table.h"
#include "text/Truncate.h"
#include "UI.h"
#include "text/WrappedText.h"

#include "opengl.h"

#include <algorithm>

using namespace std;



GamerulesPanel::GamerulesPanel(PlayerInfo &player)
	: player(player), presetUi(GameData::Interfaces().Get("gamerules presets")), selected(0)
{
	selectedPreset = GameData::GetGamerules().Name();

	// Set the initial preset list and description scroll ranges.
	Rectangle presetListBox = presetUi->GetBox("preset list");

	presetListHeight = GameData::GamerulesPresets().size() * 20;

	presetListScroll.SetDisplaySize(presetListBox.Height());
	presetListScroll.SetMaxValue(presetListHeight);
	Rectangle presetDescriptionBox = presetUi->GetBox("preset description");
	presetDescriptionScroll.SetDisplaySize(presetDescriptionBox.Height());

	presetListClip = std::make_unique<RenderBuffer>(presetListBox.Dimensions());
	RenderPresetDescription(selectedPreset);
}



// Stub, for unique_ptr destruction to be defined in the right compilation unit.
GamerulesPanel::~GamerulesPanel()
{
}



// Draw this panel.
void GamerulesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	Information info;

	GameData::Interfaces().Get("menu background")->Draw(info, this);
	presetUi->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);

	presetZones.clear();
	DrawPresets();
}



bool GamerulesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_DOWN)
		HandleDown();
	else if(key == SDLK_UP)
		HandleUp();
	else if(key == 'c')
		GetUI()->Push(new Dialog("Gamerule customization will be added in a future update."));
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else
		return false;

	return true;
}



bool GamerulesPanel::Click(int x, int y, int clicks)
{
	Point point(x, y);

	// Don't handle clicks outside of the clipped area.
	Rectangle presetListBox = presetUi->GetBox("preset list");
	if(presetListBox.Contains(point))
	{
		int index = 0;
		for(const auto &zone : presetZones)
		{
			if(zone.Contains(point) && selectedPreset != zone.Value())
			{
				selectedPreset = zone.Value();
				selected = index;
				RenderPresetDescription(selectedPreset);
				break;
			}
			index++;
		}
	}

	return true;
}



bool GamerulesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	hoverItem.clear();

	for(const auto &zone : presetZones)
		if(zone.Contains(hoverPoint))
			hoverItem = zone.Value();

	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool GamerulesPanel::Scroll(double dx, double dy)
{
	if(!dy)
		return false;

	const Rectangle &presetBox = presetUi->GetBox("preset list");
	if(presetBox.Contains(hoverPoint))
	{
		presetListScroll.Scroll(-dy * Preferences::ScrollSpeed());
		return true;
	}

	const Rectangle &descriptionBox = presetUi->GetBox("preset description");
	if(descriptionBox.Contains(hoverPoint) && presetDescriptionBuffer)
	{
		presetDescriptionScroll.Scroll(-dy * Preferences::ScrollSpeed());
		return true;
	}
	return false;
}



bool GamerulesPanel::Drag(double dx, double dy)
{
	const Rectangle &presetBox = presetUi->GetBox("preset list");
	const Rectangle &descriptionBox = presetUi->GetBox("preset description");

	if(presetBox.Contains(hoverPoint))
	{
		// Steps is zero so that we don't animate mouse drags.
		presetListScroll.Scroll(-dy, 0);
		return true;
	}
	if(descriptionBox.Contains(hoverPoint))
	{
		// Steps is zero so that we don't animate mouse drags.
		presetDescriptionScroll.Scroll(-dy, 0);
		return true;
	}
	return false;
}



void GamerulesPanel::DrawPresets()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const Sprite *box[2] = { SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked") };

	// Animate scrolling.
	presetListScroll.Step();

	// Switch render target to presetListClip. Until target is destroyed or
	// deactivated, all opengl commands will be drawn there instead.
	auto target = presetListClip->SetTarget();
	Rectangle presetListBox = presetUi->GetBox("preset list");

	Table table;
	table.AddColumn(
		presetListClip->Left() + box[0]->Width(),
		Layout(presetListBox.Width() - box[0]->Width(), Truncate::MIDDLE)
	);
	table.SetUnderline(presetListClip->Left() + box[0]->Width(), presetListClip->Right());

	int firstY = presetListClip->Top();
	table.DrawAt(Point(0, firstY - static_cast<int>(presetListScroll.AnimatedValue())));

	for(const auto &it : GameData::GamerulesPresets())
	{
		const auto &preset = it.second;
		const string &name = preset.Name();
		if(name.empty())
			continue;

		presetZones.emplace_back(presetListBox.Center() + table.GetCenterPoint(), table.GetRowSize(), name);

		bool isSelected = (name == selectedPreset);
		if(isSelected || name == hoverItem)
			table.DrawHighlight(back);

		const Sprite *sprite = box[&preset == &GameData::GetGamerules()];
		const Point topLeft = table.GetRowBounds().TopLeft() - Point(sprite->Width(), 0.);
		Rectangle spriteBounds = Rectangle::FromCorner(topLeft, Point(sprite->Width(), sprite->Height()));
		SpriteShader::Draw(sprite, spriteBounds.Center());

		Rectangle zoneBounds = spriteBounds + presetListBox.Center();

		// Only include the zone as clickable if it's within the drawing area.
		bool displayed = table.GetPoint().Y() > presetListClip->Top() - 20 &&
			table.GetPoint().Y() < presetListClip->Bottom() - table.GetRowBounds().Height() + 20;
		if(displayed)
			AddZone(zoneBounds, [&]() { GamerulesPanel::SelectPreset(name); });
		if(isSelected)
			table.Draw(name, bright);
		else
			table.Draw(name, medium);
	}

	// Switch back to normal opengl operations.
	target.Deactivate();

	presetListClip->SetFadePadding(
		presetListScroll.IsScrollAtMin() ? 0 : 20,
		presetListScroll.IsScrollAtMax() ? 0 : 20
	);

	// Draw the scrolled and clipped preset list to the screen.
	presetListClip->Draw(presetListBox.Center());
	const Point UP{0, -1};
	const Point DOWN{0, 1};
	const Point POINTER_OFFSET{0, 5};
	if(presetListScroll.Scrollable())
	{
		// Draw up and down pointers, mostly to indicate when scrolling
		// is possible, but might as well make them clickable too.
		Rectangle topRight({presetListBox.Right(), presetListBox.Top() + POINTER_OFFSET.Y()}, {20.0, 20.0});
		PointerShader::Draw(topRight.Center(), UP,
			10.f, 10.f, 5.f, Color(presetListScroll.IsScrollAtMin() ? .2f : .8f, 0.f));
		AddZone(topRight, [&]() { presetListScroll.Scroll(-Preferences::ScrollSpeed()); });

		Rectangle bottomRight(presetListBox.BottomRight() - POINTER_OFFSET, {20.0, 20.0});
		PointerShader::Draw(bottomRight.Center(), DOWN,
			10.f, 10.f, 5.f, Color(presetListScroll.IsScrollAtMax() ? .2f : .8f, 0.f));
		AddZone(bottomRight, [&]() { presetListScroll.Scroll(Preferences::ScrollSpeed()); });
	}

	// Draw the pre-rendered preset description, if applicable.
	if(presetDescriptionBuffer)
	{
		presetDescriptionScroll.Step();

		presetDescriptionBuffer->SetFadePadding(
			presetDescriptionScroll.IsScrollAtMin() ? 0 : 20,
			presetDescriptionScroll.IsScrollAtMax() ? 0 : 20
		);

		Rectangle descriptionBox = presetUi->GetBox("preset description");
		presetDescriptionBuffer->Draw(
			descriptionBox.Center(),
			descriptionBox.Dimensions(),
			Point(0, static_cast<int>(presetDescriptionScroll.AnimatedValue()))
		);

		if(presetDescriptionScroll.Scrollable())
		{
			// Draw up and down pointers, mostly to indicate when
			// scrolling is possible, but might as well make them
			// clickable too.
			Rectangle topRight({descriptionBox.Right(), descriptionBox.Top() + POINTER_OFFSET.Y()}, {20.0, 20.0});
			PointerShader::Draw(topRight.Center(), UP,
				10.f, 10.f, 5.f, Color(presetDescriptionScroll.IsScrollAtMin() ? .2f : .8f, 0.f));
			AddZone(topRight, [&]() { presetDescriptionScroll.Scroll(-Preferences::ScrollSpeed()); });

			Rectangle bottomRight(descriptionBox.BottomRight() - POINTER_OFFSET, {20.0, 20.0});
			PointerShader::Draw(bottomRight.Center(), DOWN,
				10.f, 10.f, 5.f, Color(presetDescriptionScroll.IsScrollAtMax() ? .2f : .8f, 0.f));
			AddZone(bottomRight, [&]() { presetDescriptionScroll.Scroll(Preferences::ScrollSpeed()); });
		}
	}
}



// Render the named preset description into the presetDescriptionBuffer.
void GamerulesPanel::RenderPresetDescription(const std::string &name)
{
	const Gamerules *preset = GameData::GamerulesPresets().Find(name);
	if(preset)
		RenderPresetDescription(*preset);
	else
		presetDescriptionBuffer.reset();
}



// Render the preset description into the presetDescriptionBuffer.
void GamerulesPanel::RenderPresetDescription(const Gamerules &preset)
{
	const Color &medium = *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);
	Rectangle box = presetUi->GetBox("preset description");

	// We are resizing and redrawing the description buffer. Reset the scroll
	// back to zero.
	presetDescriptionScroll.Set(0, 0);

	int descriptionHeight = 0;

	WrappedText wrap(font);
	wrap.SetWrapWidth(box.Width());
	static const string EMPTY = "(No description given.)";
	wrap.Wrap(preset.Description().empty() ? EMPTY : preset.Description());

	descriptionHeight += wrap.Height();

	// Now that we know the size of the rendered description, resize the buffer
	// to fit, and activate it as a render target.
	if(descriptionHeight < box.Height())
		descriptionHeight = box.Height();
	presetDescriptionScroll.SetMaxValue(descriptionHeight);
	presetDescriptionBuffer = std::make_unique<RenderBuffer>(Point(box.Width(), descriptionHeight));
	// Redirect all drawing commands into the offscreen buffer.
	auto target = presetDescriptionBuffer->SetTarget();

	Point top(presetDescriptionBuffer->Left(), presetDescriptionBuffer->Top());

	wrap.Draw(top, medium);
	target.Deactivate();
}



void GamerulesPanel::HandleUp()
{
	selected = max(0, selected - 1);
	selectedPreset = presetZones.at(selected).Value();
	RenderPresetDescription(selectedPreset);
	ScrollSelectedPreset();
}



void GamerulesPanel::HandleDown()
{
	selected = min(selected + 1, static_cast<int>(presetZones.size() - 1));
	selectedPreset = presetZones.at(selected).Value();
	RenderPresetDescription(selectedPreset);
	ScrollSelectedPreset();
}



void GamerulesPanel::SelectPreset(const string &name)
{
	const Gamerules *selected = GameData::GamerulesPresets().Get(name);
	player.SetGamerules(selected);
	GameData::SetGamerules(selected);
}



void GamerulesPanel::ScrollSelectedPreset()
{
	while(selected * 20 - presetListScroll < 0)
		presetListScroll.Scroll(-Preferences::ScrollSpeed());
	while(selected * 20 - presetListScroll > presetListClip->Height())
		presetListScroll.Scroll(Preferences::ScrollSpeed());
}
