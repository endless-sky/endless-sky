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

#include "Color.h"
#include "Command.h"
#include "DialogPanel.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Information.h"
#include "Interface.h"
#include "OptionalInputDialogPanel.h"
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

namespace {
	// Gamerule display names.
	const string DEPRECIATION_MIN = "Minimum value";
	const string DEPRECIATION_GRACE_PERIOD = "Grace period";
	const string DEPRECIATION_MAX_AGE = "Maximum age";
	const string DEPRECIATION_DAILY = "Daily depreciation";
	const string PERSON_SPAWN_PERIOD = "Spawn attempt period";
	const string NO_PERSON_SPAWN_WEIGHT = "No spawn weight";
	const string NPC_MAX_MINING_TIME = "NPC max mining time";
	const string UNIVERSAL_FRUGAL_THRESHOLD = "Universal frugal threshold";
	const string UNIVERSAL_RAMSCOOP = "Universal ramscoop";
	const string SYSTEM_DEPARTURE_MIN = "Minimum departure distance";
	const string SYSTEM_ARRIVAL_MIN = "Minimum arrival distance";
	const string FLEET_MULTIPLIER = "Fleet multiplier";
	const string LOCK_GAMERULES = "Lock gamerules";
	const string FIGHTERS_HIT_WHEN_DISABLED = "Fighters hit when disabled";
	const string UNIVERSAL_AMMO_STOCKING = "Universal ammo stocking";

	const string AMMO_RESTOCKING_NAME = "universal ammo restocking";

	const auto DISPLAY_NAME_TO_RULE_NAME = map<string, string>{
		{DEPRECIATION_MIN, "depreciation min"},
		{DEPRECIATION_GRACE_PERIOD, "depreciation grace period"},
		{DEPRECIATION_MAX_AGE, "depreciation max age"},
		{DEPRECIATION_DAILY, "depreciation daily"},
		{PERSON_SPAWN_PERIOD, "person spawn period"},
		{NO_PERSON_SPAWN_WEIGHT, "no person spawn weight"},
		{NPC_MAX_MINING_TIME, "npc max mining time"},
		{UNIVERSAL_FRUGAL_THRESHOLD, "universal frugal threshold"},
		{UNIVERSAL_RAMSCOOP, "universal ramscoop"},
		{SYSTEM_DEPARTURE_MIN, "system departure min"},
		{SYSTEM_ARRIVAL_MIN, "system arrival min"},
		{FLEET_MULTIPLIER, "fleet multiplier"},
		{LOCK_GAMERULES, "lock gamerules"},
		{FIGHTERS_HIT_WHEN_DISABLED, "disabled fighters avoid projectiles"},
		{UNIVERSAL_AMMO_STOCKING, AMMO_RESTOCKING_NAME},
	};

	const int GAMERULES_PAGE_COUNT = 1;
}



GamerulesPanel::GamerulesPanel(Gamerules &gamerules, bool existingPilot)
	: gamerules(gamerules), existingPilot(existingPilot),
	gamerulesUi(GameData::Interfaces().Get("gamerules")),
	presetUi(GameData::Interfaces().Get("gamerules presets")),
	tooltip(270, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	selectedPreset(gamerules.Name())
{
	Rectangle presetListBox = presetUi->GetBox("preset list");
	presetListScroll.SetDisplaySize(presetListBox.Height());
	presetListScroll.SetMaxValue(GameData::GamerulesPresets().size() * 20);
	Rectangle presetDescriptionBox = presetUi->GetBox("preset description");
	presetDescriptionScroll.SetDisplaySize(presetDescriptionBox.Height());
}



GamerulesPanel::~GamerulesPanel()
{
}



// Draw this panel.
void GamerulesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	Information info;

	if(GAMERULES_PAGE_COUNT > 1)
		info.SetCondition("multiple gamerules pages");
	if(currentGamerulesPage > 0)
		info.SetCondition("show previous gamerules");
	if(currentGamerulesPage + 1 < GAMERULES_PAGE_COUNT)
		info.SetCondition("show next gamerules");

	GameData::Interfaces().Get("menu background")->Draw(info, this);
	(page == 'g' ? gamerulesUi : presetUi)->Draw(info, this);

	gameruleZones.clear();
	presetZones.clear();
	if(page == 'g')
	{
		DrawGamerules();
		DrawTooltips();
	}
	else if(page == 'p')
		DrawPresets();
}



void GamerulesPanel::UpdateTooltipActivation()
{
	tooltip.UpdateActivationCount();
}



bool GamerulesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_DOWN)
		HandleDown();
	else if(key == SDLK_UP)
		HandleUp();
	else if(key == SDLK_RETURN)
		HandleConfirm();
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		if(existingPilot && gamerules.LockGamerules())
			GetUI().Push(DialogPanel::CallFunctionIfOk([this]() -> void { GetUI().Pop(this); },
				"You have set \"Lock Gamerules\" to true, which means that you will not be able "
				"to return to this panel to make further edits after leaving. Continue anyway?", false));
		else
			GetUI().Pop(this);
	}
	else if(key == 'g' || key == 'p')
	{
		page = key;
		hoverItem.clear();
		selectedIndex = 0;

		Resize();
	}
	else if((key == 'n' || key == SDLK_PAGEUP) && (page == 'g' && currentGamerulesPage < GAMERULES_PAGE_COUNT - 1))
	{
		++currentGamerulesPage;
		selectedIndex = 0;
		selectedItem.clear();
	}
	else if((key == 'r' || key == SDLK_PAGEDOWN) && (page == 'g' && currentGamerulesPage > 0))
	{
		--currentGamerulesPage;
		selectedIndex = 0;
		selectedItem.clear();
	}
	else if((key == 'x' || key == SDLK_DELETE) && (page == 'g') && latestIndex >= 0)
	{
		const string &rule = DISPLAY_NAME_TO_RULE_NAME.at(gameruleZones[latestIndex].Value());
		gamerules.Reset(rule, *GameData::GamerulesPresets().Get(gamerules.Name()));
	}
	else
		return false;

	return true;
}



bool GamerulesPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	Point point(x, y);

	for(const auto &zone : gameruleZones)
		if(zone.Contains(point))
		{
			HandleGamerulesString(zone.Value());
			break;
		}

	if(page == 'p')
	{
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
					selectedIndex = index;
					RenderPresetDescription(selectedPreset);
					break;
				}
				index++;
			}
		}
	}

	return true;
}



bool GamerulesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);

	hoverItem.clear();
	tooltip.Clear();

	hoverIndex = -1;
	for(unsigned index = 0; index < gameruleZones.size(); ++index)
	{
		const auto &zone = gameruleZones[index];
		if(zone.Contains(hoverPoint))
		{
			hoverIndex = index;
			hoverItem = zone.Value();
			tooltip.SetZone(zone);
		}
	}

	for(const auto &zone : presetZones)
		if(zone.Contains(hoverPoint))
		{
			hoverItem = zone.Value();
			tooltip.SetZone(zone);
		}

	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool GamerulesPanel::Scroll(double dx, double dy)
{
	if(!dy)
		return false;

	if(page == 'p')
	{
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
	}
	return false;
}



bool GamerulesPanel::Drag(double dx, double dy)
{
	if(page == 'p')
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
	}
	return false;
}



void GamerulesPanel::Resize()
{
	if(page == 'p')
	{
		Rectangle presetListBox = presetUi->GetBox("preset list");
		presetListClip = make_unique<RenderBuffer>(presetListBox.Dimensions());
		RenderPresetDescription(selectedPreset);
	}
}



void GamerulesPanel::DrawGamerules()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	if(selectedIndex != oldSelectedIndex)
		latestIndex = selectedIndex;
	if(hoverIndex != oldHoverIndex)
		latestIndex = hoverIndex;

	oldSelectedIndex = selectedIndex;
	oldHoverIndex = hoverIndex;

	Table table;
	table.AddColumn(-115, {230, Alignment::LEFT});
	table.AddColumn(115, {230, Alignment::RIGHT});
	table.SetUnderline(-120, 120);

	int firstY = -248;
	table.DrawAt(Point(-130, firstY));

	// About GAMERULES pagination
	// * An empty string indicates that a category has ended.
	// * A '\t' character indicates that the first column on this page has
	//   ended, and the next line should be drawn at the start of the next
	//   column.
	// * A '\n' character indicates that this page is complete, no further lines
	//   should be drawn on this page.
	// * In all three cases, the first non-special string will be considered the
	//   category heading and will be drawn differently to normal gamerule
	//   entries.
	// * The namespace variable GAMERULES_PAGE_COUNT should be updated to the max
	//   page count (count of '\n' characters plus one).
	static const string GAMERULES[] = {
		"Depreciation",
		DEPRECIATION_MIN,
		DEPRECIATION_GRACE_PERIOD,
		DEPRECIATION_MAX_AGE,
		DEPRECIATION_DAILY,
		"",
		"Person Ships",
		PERSON_SPAWN_PERIOD,
		NO_PERSON_SPAWN_WEIGHT,
		"",
		"NPC Behavior",
		NPC_MAX_MINING_TIME,
		UNIVERSAL_FRUGAL_THRESHOLD,
		"\t",
		"System Behavior",
		UNIVERSAL_RAMSCOOP,
		SYSTEM_ARRIVAL_MIN,
		SYSTEM_DEPARTURE_MIN,
		FLEET_MULTIPLIER,
		"",
		"Miscellaneous",
		LOCK_GAMERULES,
		FIGHTERS_HIT_WHEN_DISABLED,
		UNIVERSAL_AMMO_STOCKING
	};

	bool isCategory = true;
	int page = 0;
	for(const string &gamerule : GAMERULES)
	{
		// Check if this is a page break.
		if(gamerule == "\n")
		{
			++page;
			continue;
		}
		// Check if this gamerule is on the page being displayed.
		// If this gamerule isn't on the page being displayed, check if it is on an earlier page.
		// If it is, continue to the next gamerule.
		// Otherwise, this gamerule is on a later page,
		// do not continue as no further gamerules are to be displayed.
		if(page < currentGamerulesPage)
			continue;
		else if(page > currentGamerulesPage)
			break;
		// Check if this is a category break or column break.
		if(gamerule.empty() || gamerule == "\t")
		{
			isCategory = true;
			if(!gamerule.empty())
				table.DrawAt(Point(130, firstY));
			continue;
		}

		if(isCategory)
		{
			isCategory = false;
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(gamerule, bright);
			table.Advance();
			table.DrawGap(5);
			continue;
		}

		// Record where this gamerule is displayed, so the user can click on it.
		// Temporarily reset the row's size so the click zone can cover the entire gamerule.
		table.SetHighlight(-120, 120);
		gameruleZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), gamerule);

		// Setting "isOn" draws the gamerule "bright" (i.e. the gamerule is active).
		bool isOn = true;
		string text;
		if(gamerule == DEPRECIATION_MIN)
			text = Format::Percentage(gamerules.DepreciationMin(), 2);
		else if(gamerule == DEPRECIATION_GRACE_PERIOD)
			text = Format::AbbreviatedNumber(gamerules.DepreciationGracePeriod());
		else if(gamerule == DEPRECIATION_MAX_AGE)
			text = Format::AbbreviatedNumber(gamerules.DepreciationMaxAge());
		else if(gamerule == DEPRECIATION_DAILY)
			text = Format::Percentage(gamerules.DepreciationDaily(), 2);
		else if(gamerule == PERSON_SPAWN_PERIOD)
			text = Format::AbbreviatedNumber(gamerules.PersonSpawnPeriod());
		else if(gamerule == NO_PERSON_SPAWN_WEIGHT)
			text = Format::AbbreviatedNumber(gamerules.NoPersonSpawnWeight());
		else if(gamerule == NPC_MAX_MINING_TIME)
			text = Format::AbbreviatedNumber(gamerules.NPCMaxMiningTime());
		else if(gamerule == UNIVERSAL_FRUGAL_THRESHOLD)
			text = Format::Percentage(gamerules.UniversalFrugalThreshold(), 2);
		else if(gamerule == UNIVERSAL_RAMSCOOP)
			text = gamerules.UniversalRamscoopActive() ? "true" : "false";
		else if(gamerule == SYSTEM_ARRIVAL_MIN)
		{
			if(gamerules.SystemArrivalMin().has_value())
				text = Format::AbbreviatedNumber(*gamerules.SystemArrivalMin(), std::nullopt);
			else
				text = "(unset)";
		}
		else if(gamerule == SYSTEM_DEPARTURE_MIN)
			text = Format::AbbreviatedNumber(gamerules.SystemDepartureMin(), std::nullopt);
		else if(gamerule == FLEET_MULTIPLIER)
			text = Format::Percentage(gamerules.FleetMultiplier(), 2);
		else if(gamerule == LOCK_GAMERULES)
			text = gamerules.LockGamerules() ? "true" : "false";
		else if(gamerule == FIGHTERS_HIT_WHEN_DISABLED)
		{
			switch(gamerules.FightersHitWhenDisabled())
			{
			case Gamerules::FighterDodgePolicy::ALL:
				text = "all";
				break;
			case Gamerules::FighterDodgePolicy::ONLY_PLAYER:
				text = "player";
				break;
			case Gamerules::FighterDodgePolicy::NONE:
				text = "none";
				break;
			}
		}
		else if(gamerule == UNIVERSAL_AMMO_STOCKING)
			text = gamerules.GetValue(AMMO_RESTOCKING_NAME) ? "true" : "false";

		if(gamerule == hoverItem)
		{
			table.SetHighlight(-120, 120);
			table.DrawHighlight(back);
		}
		else if(gamerule == selectedItem)
		{
			auto width = FontSet::Get(14).Width(gamerule);
			table.SetHighlight(-120, width - 110);
			table.DrawHighlight(back);
		}

		table.Draw(gamerule, isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}

	// Sync the currently selected item after the gamerules map has been populated.
	if(selectedItem.empty())
		selectedItem = gameruleZones.at(selectedIndex).Value();
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

		// If the player's current gamerules are an exact copy of a preset, then check that preset's box
		// to show that it is what is currently active.
		const Sprite *sprite = box[preset == gamerules];
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
void GamerulesPanel::RenderPresetDescription(const string &name)
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

	// Compute the height before drawing, so that we know the scroll bounds.
	// Start at a height of 10 to account for padding at the top of the description.
	int descriptionHeight = 10;

	const Sprite *sprite = preset.Thumbnail();
	if(sprite)
		descriptionHeight += sprite->Height();

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
	presetDescriptionBuffer = make_unique<RenderBuffer>(Point(box.Width(), descriptionHeight));
	// Redirect all drawing commands into the offscreen buffer.
	auto target = presetDescriptionBuffer->SetTarget();

	Point top(presetDescriptionBuffer->Left(), presetDescriptionBuffer->Top());
	if(sprite)
	{
		Point center(0., top.Y() + .5 * sprite->Height());
		SpriteShader::Draw(sprite, center);
		top.Y() += sprite->Height();
	}
	// Pad the top of the text.
	top.Y() += 10.;

	wrap.Draw(top, medium);
	target.Deactivate();
}



void GamerulesPanel::DrawTooltips()
{
	if(!GetUI().IsTop(this))
		return;
	if(hoverItem.empty())
	{
		tooltip.DecrementCount();
		return;
	}
	tooltip.IncrementCount();
	if(!tooltip.ShouldDraw())
		return;

	if(!tooltip.HasText())
		tooltip.SetText(GameData::Tooltip(hoverItem));

	tooltip.Draw();
}



void GamerulesPanel::HandleGamerulesString(const string &str)
{
	if(str == DEPRECIATION_MIN)
	{
		string message = "Set the minimum deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		GetUI().Push(DialogPanel::RequestDoubleWithValidation(&gamerules, &Gamerules::SetDepreciationMin, validate,
			message, gamerules.DepreciationMin()));
	}
	else if(str == DEPRECIATION_GRACE_PERIOD)
	{
		string message = "Set the depreciation grace period. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		GetUI().Push(DialogPanel::RequestIntegerWithValidation(&gamerules, &Gamerules::SetDepreciationGracePeriod,
			validate, message, gamerules.DepreciationGracePeriod()));
	}
	else if(str == DEPRECIATION_MAX_AGE)
	{
		string message = "Set the depreciation maximum age. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		GetUI().Push(DialogPanel::RequestIntegerWithValidation(&gamerules, &Gamerules::SetDepreciationMaxAge, validate,
			message, gamerules.DepreciationMaxAge()));
	}
	else if(str == DEPRECIATION_DAILY)
	{
		string message = "Set the daily deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		GetUI().Push(DialogPanel::RequestDoubleWithValidation(&gamerules, &Gamerules::SetDepreciationDaily, validate,
			message, gamerules.DepreciationDaily()));
	}
	else if(str == PERSON_SPAWN_PERIOD)
	{
		string message = "Set the person ship spawn attempt period. (Integer value greater than or equal to 1.)";
		auto validate = [](int value) -> bool { return value >= 1; };
		GetUI().Push(DialogPanel::RequestIntegerWithValidation(&gamerules, &Gamerules::SetPersonSpawnPeriod, validate,
			message, gamerules.PersonSpawnPeriod()));
	}
	else if(str == NO_PERSON_SPAWN_WEIGHT)
	{
		string message = "Set the no person ship spawn weight. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		GetUI().Push(DialogPanel::RequestIntegerWithValidation(&gamerules, &Gamerules::SetNoPersonSpawnWeight, validate,
			message, gamerules.NoPersonSpawnWeight()));
	}
	else if(str == NPC_MAX_MINING_TIME)
	{
		string message = "Set the NPC max mining time. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		GetUI().Push(DialogPanel::RequestIntegerWithValidation(&gamerules, &Gamerules::SetNPCMaxMiningTime, validate,
			message, gamerules.NPCMaxMiningTime()));
	}
	else if(str == UNIVERSAL_FRUGAL_THRESHOLD)
	{
		string message = "Set the universal frugal threshold. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		GetUI().Push(DialogPanel::RequestDoubleWithValidation(&gamerules, &Gamerules::SetUniversalFrugalThreshold,
			validate, message, gamerules.UniversalFrugalThreshold()));
	}
	else if(str == UNIVERSAL_RAMSCOOP)
		gamerules.SetUniversalRamscoopActive(!gamerules.UniversalRamscoopActive());
	else if(str == SYSTEM_DEPARTURE_MIN)
	{
		string message = "Set the minimum system departure distance. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.0; };
		GetUI().Push(DialogPanel::RequestDoubleWithValidation(&gamerules, &Gamerules::SetSystemDepartureMin, validate,
			message, gamerules.SystemDepartureMin()));
	}
	else if(str == SYSTEM_ARRIVAL_MIN)
	{
		string message = "Set the minimum system arrival distance. (Any decimal value.)";
		GetUI().Push(OptionalInputDialogPanel::RequestDouble(&gamerules, &Gamerules::SetSystemArrivalMin, message,
			gamerules.SystemArrivalMin()));
	}
	else if(str == FLEET_MULTIPLIER)
	{
		string message = "Set the fleet spawn multiplier. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.0; };
		GetUI().Push(DialogPanel::RequestDoubleWithValidation(&gamerules, &Gamerules::SetFleetMultiplier, validate,
			message, gamerules.FleetMultiplier()));
	}
	else if(str == LOCK_GAMERULES)
		gamerules.SetLockGamerules(!gamerules.LockGamerules());
	else if(str == FIGHTERS_HIT_WHEN_DISABLED)
	{
		Gamerules::FighterDodgePolicy value = gamerules.FightersHitWhenDisabled();
		if(value == Gamerules::FighterDodgePolicy::ALL)
			value = Gamerules::FighterDodgePolicy::NONE;
		else if(value == Gamerules::FighterDodgePolicy::NONE)
			value = Gamerules::FighterDodgePolicy::ONLY_PLAYER;
		else if(value == Gamerules::FighterDodgePolicy::ONLY_PLAYER)
			value = Gamerules::FighterDodgePolicy::ALL;
		gamerules.SetFighterDodgePolicy(value);
	}
	else if(str == UNIVERSAL_AMMO_STOCKING)
		gamerules.SetMiscValue(AMMO_RESTOCKING_NAME, !gamerules.GetValue(AMMO_RESTOCKING_NAME));
}



void GamerulesPanel::SelectPreset(const string &name)
{
	gamerules.Replace(*GameData::GamerulesPresets().Get(name));
}



void GamerulesPanel::HandleUp()
{
	selectedIndex = max(0, selectedIndex - 1);
	switch(page)
	{
	case 'g':
		selectedItem = gameruleZones.at(selectedIndex).Value();
		break;
	case 'p':
		selectedPreset = presetZones.at(selectedIndex).Value();
		RenderPresetDescription(selectedPreset);
		ScrollSelectedPreset();
		break;
	default:
		break;
	}
}



void GamerulesPanel::HandleDown()
{
	switch(page)
	{
	case 'g':
		selectedIndex = min(selectedIndex + 1, static_cast<int>(gameruleZones.size() - 1));
		selectedItem = gameruleZones.at(selectedIndex).Value();
		break;
	case 'p':
		selectedIndex = min(selectedIndex + 1, static_cast<int>(presetZones.size() - 1));
		selectedPreset = presetZones.at(selectedIndex).Value();
		RenderPresetDescription(selectedPreset);
		ScrollSelectedPreset();
		break;
	default:
		break;
	}
}



void GamerulesPanel::HandleConfirm()
{
	switch(page)
	{
	case 'g':
		HandleGamerulesString(selectedItem);
		break;
	case 'p':
		SelectPreset(selectedPreset);
		break;
	default:
		break;
	}
}



void GamerulesPanel::ScrollSelectedPreset()
{
	while(selectedIndex * 20 - presetListScroll < 0)
		presetListScroll.Scroll(-Preferences::ScrollSpeed());
	while(selectedIndex * 20 - presetListScroll > presetListClip->Height())
		presetListScroll.Scroll(Preferences::ScrollSpeed());
}
