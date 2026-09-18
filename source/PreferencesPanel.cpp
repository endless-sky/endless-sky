/* PreferencesPanel.cpp
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

#include "PreferencesPanel.h"

#include "text/Alignment.h"
#include "audio/Audio.h"
#include "Color.h"
#include "DialogPanel.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Plugin.h"
#include "PluginManager.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "RenderBuffer.h"
#include "Screen.h"
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
	// How many pages of controls and settings there are.
	const int CONTROLS_PAGE_COUNT = 2;
	const int SETTINGS_PAGE_COUNT = 3;

	const map<string, SoundCategory> volumeBars = {
		{"volume", SoundCategory::MASTER},
		{"music volume", SoundCategory::MUSIC},
		{"ui volume", SoundCategory::UI},
		{"anti-missile volume", SoundCategory::ANTI_MISSILE},
		{"weapon volume", SoundCategory::WEAPON},
		{"engine volume", SoundCategory::ENGINE},
		{"afterburner volume", SoundCategory::AFTERBURNER},
		{"jump volume", SoundCategory::JUMP},
		{"explosion volume", SoundCategory::EXPLOSION},
		{"scan volume", SoundCategory::SCAN},
		{"environment volume", SoundCategory::ENVIRONMENT},
		{"alert volume", SoundCategory::ALERT}
	};
}



PreferencesPanel::PreferencesPanel(PlayerInfo &player)
	: player(player),
	tooltip(270, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
{
	// Select the first valid plugin.
	for(const auto &plugin : PluginManager::Get())
		if(plugin.second.IsValid())
		{
			selectedPlugin = plugin.first;
			break;
		}

	SetIsFullScreen(true);

	// Set the initial plugin list and description scroll ranges.
	const Interface *pluginUi = GameData::Interfaces().Get("plugins");
	Rectangle pluginListBox = pluginUi->GetBox("plugin list");

	int pluginListHeight = 0;
	for(const auto &plugin : PluginManager::Get())
		if(plugin.second.IsValid())
			pluginListHeight += 20;

	pluginListScroll.SetDisplaySize(pluginListBox.Height());
	pluginListScroll.SetMaxValue(pluginListHeight);
	Rectangle pluginDescriptionBox = pluginUi->GetBox("plugin description");
	pluginDescriptionScroll.SetDisplaySize(pluginDescriptionBox.Height());
}



// Stub, for unique_ptr destruction to be defined in the right compilation unit.
PreferencesPanel::~PreferencesPanel()
{
}



// Draw this panel.
void PreferencesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	Information info;

	for(const auto &[bar, category] : volumeBars)
	{
		double volume = Audio::Volume(category);
		info.SetBar(bar, volume);
		if(volume > .75)
			info.SetCondition(bar + " max");
		else if(volume > .5)
			info.SetCondition(bar + " medium");
		else if(volume > .25)
			info.SetCondition(bar + " low");
		else
			info.SetCondition(bar + " none");
	}

	if(PluginManager::HasChanged())
		info.SetCondition("show plugins changed");
	if(CONTROLS_PAGE_COUNT > 1)
		info.SetCondition("multiple controls pages");
	if(currentControlsPage > 0)
		info.SetCondition("show previous controls");
	if(currentControlsPage + 1 < CONTROLS_PAGE_COUNT)
		info.SetCondition("show next controls");
	if(SETTINGS_PAGE_COUNT > 1)
		info.SetCondition("multiple settings pages");
	if(currentSettingsPage > 0)
		info.SetCondition("show previous settings");
	if(currentSettingsPage + 1 < SETTINGS_PAGE_COUNT)
		info.SetCondition("show next settings");
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : page == 'p' ? "plugins" : "audio");
	GameData::Interfaces().Get(pageName)->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);

	zones.clear();
	prefZones.clear();
	pluginZones.clear();
	if(page == 'c')
	{
		DrawControls();
		DrawTooltips();
	}
	else if(page == 's')
	{
		DrawSettings();
		DrawTooltips();
	}
	else if(page == 'p')
		DrawPlugins();
	else if(page == 'a')
	{
		// The entire audio panel is defined in interfaces, so this is a dummy.
	}
}



void PreferencesPanel::UpdateTooltipActivation()
{
	tooltip.UpdateActivationCount();
}



void PreferencesPanel::UpdateTextDisplay()
{
	tooltip.UpdateFontSize();
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		Command::SetKey(zones[editing].Value(), key);
		EndEditing();
		return true;
	}

	if(key == SDLK_DOWN)
		HandleDown();
	else if(key == SDLK_UP)
		HandleUp();
	else if(key == SDLK_RETURN)
		HandleConfirm();
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		Exit();
	else if(key == 'c' || key == 's' || key == 'p' || key == 'a')
	{
		page = key;
		hoverItem.clear();
		selected = 0;

		// Make sure the render buffers are initialized and are aware of the current UI scale.
		Resize();
	}
	else if(key == 'o' && page == 'p')
		Files::OpenUserPluginFolder();
	else if((key == 'n' || key == SDLK_PAGEUP)
		&& ((page == 'c' && currentControlsPage < CONTROLS_PAGE_COUNT - 1)
		|| (page == 's' && currentSettingsPage < SETTINGS_PAGE_COUNT - 1)))
	{
		if(page == 'c')
			++currentControlsPage;
		else
			++currentSettingsPage;
		selected = 0;
		selectedItem.clear();
	}
	else if((key == 'r' || key == SDLK_PAGEDOWN)
		&& ((page == 'c' && currentControlsPage > 0) || (page == 's' && currentSettingsPage > 0)))
	{
		if(page == 'c')
			--currentControlsPage;
		else
			--currentSettingsPage;
		selected = 0;
		selectedItem.clear();
	}
	else if((key == 'x' || key == SDLK_DELETE) && (page == 'c'))
	{
		if(!zones[latest].Value().Has(Command::MENU))
			Command::SetKey(zones[latest].Value(), 0);
	}
	else
		return false;

	return true;
}



bool PreferencesPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;
	EndEditing();

	Point point(x, y);
	const Interface *preferencesUI = GameData::Interfaces().Get("preferences");
	Rectangle volumeBox = preferencesUI->GetBox("volume box");
	if(volumeBox.Contains(point))
	{
		double barSize = preferencesUI->GetValue("master volume bar size");
		double volume = (volumeBox.Center().Y() - point.Y()) / barSize + .5;

		Audio::SetVolume(volume, SoundCategory::MASTER);
		Audio::Play(Audio::Get("warder"), SoundCategory::MASTER);
		return true;
	}

	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
		{
			if(zones[index].Value().Has(Command::MENU))
				GetUI().Push(DialogPanel::CallFunctionIfOk([this, index]()
					{
						this->editing = this->selected = index;
					},
					"Rebinding this key will change the keypress you need to access this menu. "
					"You really shouldn't rebind this unless needed.", true));
			else
				editing = selected = index;
		}

	for(const auto &zone : prefZones)
		if(zone.Contains(point))
		{
			HandleSettingsString(zone.Value(), point);
			break;
		}

	if(page == 'p')
	{
		// Don't handle clicks outside of the clipped area.
		const Interface *pluginUi = GameData::Interfaces().Get("plugins");
		Rectangle pluginListBox = pluginUi->GetBox("plugin list");
		if(pluginListBox.Contains(point))
		{
			int index = 0;
			for(const auto &zone : pluginZones)
			{
				if(zone.Contains(point) && selectedPlugin != zone.Value())
				{
					selectedPlugin = zone.Value();
					selected = index;
					RenderPluginDescription(selectedPlugin);
					break;
				}
				index++;
			}
		}
	}
	else if(page == 'a')
	{
		const Interface *audioUI = GameData::Interfaces().Get("audio");
		double barSize = audioUI->GetValue("volume bar size");
		for(const auto &[name, category] : volumeBars)
		{
			if(category != SoundCategory::MASTER)
			{
				Rectangle barZone = audioUI->GetBox(name + " box");
				if(barZone.Contains(point))
				{
					double volume = (point.X() - barZone.Center().X()) / barSize + .5;
					Audio::SetVolume(volume, category);
					Audio::Play(Audio::Get("warder"), category);
					return true;
				}
			}
		}
	}

	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);

	hoverItem.clear();
	tooltip.Clear();

	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
	{
		const auto &zone = zones[index];
		if(zone.Contains(hoverPoint))
		{
			hover = index;
			tooltip.SetZone(zone);
		}
	}

	for(const auto &zone : prefZones)
		if(zone.Contains(hoverPoint))
		{
			hoverItem = zone.Value();
			tooltip.SetZone(zone);
		}

	for(const auto &zone : pluginZones)
		if(zone.Contains(hoverPoint))
		{
			hoverItem = zone.Value();
			tooltip.SetZone(zone);
		}

	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool PreferencesPanel::Scroll(double dx, double dy)
{
	if(!dy)
		return false;

	if(page == 's' && !hoverItem.empty())
	{
		Preferences::Scroll(hoverItem, dy);
		return true;
	}
	else if(page == 'p')
	{
		auto ui = GameData::Interfaces().Get("plugins");
		const Rectangle &pluginBox = ui->GetBox("plugin list");
		const Rectangle &descriptionBox = ui->GetBox("plugin description");

		if(pluginBox.Contains(hoverPoint))
		{
			pluginListScroll.Scroll(-dy * Preferences::ScrollSpeed());
			return true;
		}
		else if(descriptionBox.Contains(hoverPoint) && pluginDescriptionBuffer)
		{
			pluginDescriptionScroll.Scroll(-dy * Preferences::ScrollSpeed());
			return true;
		}
	}
	return false;
}



bool PreferencesPanel::Drag(double dx, double dy)
{
	if(page == 'p')
	{
		auto ui = GameData::Interfaces().Get("plugins");
		const Rectangle &pluginBox = ui->GetBox("plugin list");
		const Rectangle &descriptionBox = ui->GetBox("plugin description");

		if(pluginBox.Contains(hoverPoint))
		{
			// Steps is zero so that we don't animate mouse drags.
			pluginListScroll.Scroll(-dy, 0);
			return true;
		}
		else if(descriptionBox.Contains(hoverPoint))
		{
			// Steps is zero so that we don't animate mouse drags.
			pluginDescriptionScroll.Scroll(-dy, 0);
			return true;
		}
	}
	return false;
}



void PreferencesPanel::Resize()
{
	if(page == 'p')
	{
		const Interface *pluginUi = GameData::Interfaces().Get("plugins");
		Rectangle pluginListBox = pluginUi->GetBox("plugin list");
		pluginListClip = make_unique<RenderBuffer>(pluginListBox.Dimensions());
		RenderPluginDescription(selectedPlugin);
	}
}



void PreferencesPanel::EndEditing()
{
	editing = -1;
}



void PreferencesPanel::DrawControls()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	// Colors for highlighting.
	const Color &warning = *GameData::Colors().Get("warning conflict");
	const Color &noCommand = *GameData::Colors().Get("warning no command");

	if(selected != oldSelected)
		latest = selected;
	if(hover != oldHover)
		latest = hover;

	oldSelected = selected;
	oldHover = hover;

	Table table;
	table.AddColumn(-115, {230, Alignment::LEFT});
	table.AddColumn(115, {230, Alignment::RIGHT});
	table.SetUnderline(-120, 120);

	int firstY = -248;
	table.DrawAt(Point(-130, firstY));

	// About CONTROLS pagination
	// * A NONE command means that a string from CATEGORIES should be drawn
	//   instead of a command.
	// * A '\t' category string indicates that the first column on this page has
	//   ended, and the next line should be drawn at the start of the next
	//   column.
	// * A '\n' category string indicates that this page is complete, no further
	//   lines should be drawn on this page.
	// * The namespace variable CONTROLS_PAGE_COUNT should be updated to the max
	//   page count (count of '\n' characters plus one).
	static const string CATEGORIES[] = {
		"Keyboard Navigation",
		"Fleet",
		"\t",
		"Targeting",
		"Weapons",
		"\n",
		"Interface"
	};
	const string *category = CATEGORIES;
	static const Command COMMANDS[] = {
		Command::NONE,
		Command::FORWARD,
		Command::LEFT,
		Command::RIGHT,
		Command::BACK,
		Command::AFTERBURNER,
		Command::AUTOSTEER,
		Command::LAND,
		Command::JUMP,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::HOLD_FIRE,
		Command::GATHER,
		Command::HOLD_POSITION,
		Command::AMMO,
		Command::HARVEST,
		Command::SCAN_ORDER,
		Command::NONE,
		Command::NONE,
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::NEAREST_ASTEROID,
		Command::SCAN,
		Command::NONE,
		Command::PRIMARY,
		Command::TURRET_TRACKING,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::MOUSE_TURNING_HOLD,
		Command::AIM_TURRET_HOLD,
		Command::NONE,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::FASTFORWARD,
		Command::PAUSE,
		Command::HELP,
		Command::MESSAGE_LOG,
		Command::PERFORMANCE_DISPLAY
	};

	int page = 0;
	for(const Command &command : COMMANDS)
	{
		string categoryString;
		if(!command)
		{
			if(category != end(CATEGORIES))
				categoryString = *category++;
			else
				table.Advance();
			// Check if this is a page break.
			if(categoryString == "\n")
			{
				++page;
				continue;
			}
		}
		// Check if this command is on the page being displayed.
		// If this command isn't on the page being displayed, check if it is on an earlier page.
		// If it is, continue to the next command.
		// Otherwise, this command is on a later page,
		// do not continue as no further commands are to be displayed.
		if(page < currentControlsPage)
			continue;
		else if(page > currentControlsPage)
			break;
		if(!command)
		{
			// Check if this is a column break.
			if(categoryString == "\t")
			{
				table.DrawAt(Point(130, firstY));
				continue;
			}
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(categoryString, bright);
			table.Draw("Key", bright);
			table.DrawGap(5);
		}
		else
		{
			int index = zones.size();
			// Mark conflicts.
			bool isFastForwardSyncToCapsLock = command.Has(Command::FASTFORWARD)
				&& Preferences::GetFastForwardCapsLockSync() == Preferences::FastForwardCapsLockSync::ALWAYS;
			bool isConflicted = command.HasConflict() && !isFastForwardSyncToCapsLock;
			bool isEmpty = !command.HasBinding() && !isFastForwardSyncToCapsLock;
			bool isEditing = (index == editing);
			if(isConflicted || isEditing || isEmpty)
			{
				table.SetHighlight(56, 120);
				table.DrawHighlight(isEditing ? dim : isEmpty ? noCommand : warning);
			}

			// Mark the selected row.
			bool isHovering = (index == hover && !isEditing);
			if(!isHovering && index == selected)
			{
				auto textWidth = FontSet::Get(14).Width(command.Description());
				table.SetHighlight(-120, textWidth - 110);
				table.DrawHighlight(back);
			}

			// Highlight whichever row the mouse hovers over.
			table.SetHighlight(-120, 120);
			if(isHovering)
			{
				table.DrawHighlight(back);
				hoverItem = command.Description();
			}

			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);

			const Color &keyColor = isFastForwardSyncToCapsLock ? dim : medium;
			const Color &descColor = isFastForwardSyncToCapsLock ? dim : medium;
			table.Draw(command.Description(), descColor);
			table.Draw(command.KeyName(), isEditing ? bright : keyColor);
		}
	}
}



void PreferencesPanel::DrawSettings()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	Table table;
	table.AddColumn(-115, {230, Alignment::LEFT});
	table.AddColumn(115, {230, Alignment::RIGHT});
	table.SetUnderline(-120, 120);

	int firstY = -248;
	table.DrawAt(Point(-130, firstY));

	// About SETTINGS pagination
	// * An empty string indicates that a category has ended.
	// * A '\t' character indicates that the first column on this page has
	//   ended, and the next line should be drawn at the start of the next
	//   column.
	// * A '\n' character indicates that this page is complete, no further lines
	//   should be drawn on this page.
	// * In all three cases, the first non-special string will be considered the
	//   category heading and will be drawn differently to normal setting
	//   entries.
	// * The namespace variable SETTINGS_PAGE_COUNT should be updated to the max
	//   page count (count of '\n' characters plus one).
	static const string SETTINGS[] = {
		"Display",
		Preferences::ZOOM_FACTOR_MAIN,
		Preferences::ZOOM_FACTOR_VIEW,
		Preferences::FONT_SIZE,
		Preferences::TEXT_ALIGNMENT,
		Preferences::SCREEN_MODE,
		Preferences::BLOCK_SCREEN_SAVER,
		Preferences::VSYNC,
		"",
		"Graphics",
		Preferences::CAMERA_ACCELERATION,
		Preferences::MOTION_BLUR,
		Preferences::DRAW_BACKGROUND_HAZE,
		Preferences::DRAW_STARFIELD,
		Preferences::FIXED_STARFIELD_ZOOM,
		Preferences::PARALLAX,
		Preferences::ANIMATE_MENU_BACKGROUND,
		Preferences::SHOW_HYPERSPACE_FLASH,
		Preferences::EXTENDED_JUMP_EFFECTS,
		Preferences::SHIP_OUTLINES_CLOAKED,
		Preferences::TEXTURE_FILTERING,
		"\t",
		"Performance",
		Preferences::SHOW_PERFORMANCE_METRICS,
		Preferences::REDUCE_LARGE_GRAPHICS,
		Preferences::DEFER_LOADING_IMAGES,
		Preferences::SHIP_OUTLINES_SHOP,
		Preferences::SHIP_OUTLINES_HUD,
		"",
		"Map",
		Preferences::MAP_DEADLINE_BLINK_BY_DISTANCE,
		Preferences::MAP_HIDE_UNEXPLORED,
		Preferences::MAP_SHOW_ESCORTS,
		Preferences::MAP_SHOW_OUTFITS,
		Preferences::MAP_PARENTHESIZE_PROFIT,
		"",
		"Trading",
		Preferences::TRADE_SELL_OUTFITS_WITHOUT_SHOP,
		Preferences::TRADE_CONFIRM_OUTFITS,
		Preferences::TRADE_CONFIRM_MINABLES,
		"",
		"Gameplay",
		Preferences::TRIBUTE_CONFIRMATION,
		"\n",
		"Flagship Behavior",
		Preferences::MOUSE_CONTROL_FLAGSHIP,
		Preferences::MOUSE_CONTROL_TURRETS,
		Preferences::AUTO_AIM,
		Preferences::AUTO_FIRE,
		Preferences::ASTEROID_TARGETING,
		Preferences::BOARDING_TARGET_PRIORITY,
		Preferences::REHIRE_LOST_CREW,
		Preferences::AUTO_UNPARK_FLAGSHIP,
		Preferences::FLAGSHIP_SPACE_PRIORITY,
		"",
		"Fleet Behavior",
		Preferences::TURRETS_FOCUS_FIRE,
		Preferences::ESCORT_AMMO_USAGE,
		Preferences::FLOTSAM_COLLECTION,
		Preferences::FIGHTERS_REPAIR_IN,
		Preferences::DAMAGED_FIGHTERS_RETREAT,
		Preferences::FIGHTERS_TRANSFER_CARGO,
		Preferences::AMMO_REFILL,
		"\t",
		"HUD",
		Preferences::STATUS_OVERLAYS_ALL,
		Preferences::STATUS_OVERLAYS_FLAGSHIP,
		Preferences::STATUS_OVERLAYS_ESCORT,
		Preferences::STATUS_OVERLAYS_ENEMY,
		Preferences::STATUS_OVERLAYS_NEUTRAL,
		Preferences::HUD_MISSILE_OVERLAY,
		Preferences::HUD_TURRET_OVERLAY,
		Preferences::HUD_ASTEROID_OVERLAY,
		Preferences::SHIP_HIGHLIGHTS,
		Preferences::HUD_ROTATE_FLAGSHIP,
		Preferences::HUD_PLANET_LABELS,
		Preferences::MINIMAP_DISPLAY,
		Preferences::HUD_CLICKABLE_RADAR,
		Preferences::ALERT_INDICATOR,
		Preferences::HUD_EXTRA_STATUS_MSGS,
		"\n",
		"Other",
		Preferences::UNDERLINE_SHORTCUTS,
		Preferences::REACTIVATE_HELP,
		Preferences::INTERRUPT_FAST_FORWARD,
		Preferences::FF_CAPSLOCK_SYNC,
		Preferences::LANDING_ZOOM,
		Preferences::SCROLL_SPEED,
		Preferences::TOOLTIP_ACTIVATION_TIME,
		Preferences::DATE_FORMAT,
		Preferences::DESTINATION_NOTIFICATION,
		Preferences::SAVE_MSG_LOGS,
#ifdef _WIN32
		"\t",
		"Windows Options",
		Preferences::TITLE_BAR_THEME,
		Preferences::WINDOW_ROUNDING
#endif
	};

	bool isCategory = true;
	int page = 0;
	for(const string &setting : SETTINGS)
	{
		// Check if this is a page break.
		if(setting == "\n")
		{
			++page;
			continue;
		}
		// Check if this setting is on the page being displayed.
		// If this setting isn't on the page being displayed, check if it is on an earlier page.
		// If it is, continue to the next setting.
		// Otherwise, this setting is on a later page,
		// do not continue as no further settings are to be displayed.
		if(page < currentSettingsPage)
			continue;
		if(page > currentSettingsPage)
			break;
		// Check if this is a category break or column break.
		if(setting.empty() || setting == "\t")
		{
			isCategory = true;
			if(!setting.empty())
				table.DrawAt(Point(130, firstY));
			continue;
		}

		if(isCategory)
		{
			isCategory = false;
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(setting, bright);
			table.Advance();
			table.DrawGap(5);
			continue;
		}

		// Record where this setting is displayed, so the user can click on it.
		// Temporarily reset the row's size so the clickzone can cover the entire preference.
		table.SetHighlight(-120, 120);
		prefZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), setting);

		if(setting == hoverItem)
		{
			table.SetHighlight(-120, 120);
			table.DrawHighlight(back);
		}
		else if(setting == selectedItem)
		{
			auto width = FontSet::Get(14).Width(setting);
			table.SetHighlight(-120, width - 110);
			table.DrawHighlight(back);
		}

		// Get the text for this setting. Setting "isOn"
		// draws the setting "bright" (i.e. the setting is active).
		auto [text, isOn] = Preferences::DisplayValue(setting);
		table.Draw(Preferences::DisplayName(setting), isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}

	// Sync the currently selected item after the preferences map has been populated.
	if(selectedItem.empty())
		selectedItem = prefZones.at(selected).Value();
}



void PreferencesPanel::DrawPlugins()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Interface *pluginUI = GameData::Interfaces().Get("plugins");

	const Sprite *box[2] = { SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked") };

	// Animate scrolling.
	pluginListScroll.Step();

	// Switch render target to pluginListClip. Until target is destroyed or
	// deactivated, all opengl commands will be drawn there instead.
	auto target = pluginListClip->SetTarget();
	Rectangle pluginListBox = pluginUI->GetBox("plugin list");

	Table table;
	table.AddColumn(
		pluginListClip->Left() + box[0]->Width(),
		Layout(pluginListBox.Width() - box[0]->Width(), Truncate::MIDDLE)
	);
	table.SetUnderline(pluginListClip->Left() + box[0]->Width(), pluginListClip->Right());

	int firstY = pluginListClip->Top();
	table.DrawAt(Point(0, firstY - static_cast<int>(pluginListScroll.AnimatedValue())));

	for(const auto &it : PluginManager::Get())
	{
		const auto &plugin = it.second;
		if(!plugin.IsValid())
			continue;

		pluginZones.emplace_back(pluginListBox.Center() + table.GetCenterPoint(), table.GetRowSize(), plugin.name);

		bool isSelected = (plugin.name == selectedPlugin);
		if(isSelected || plugin.name == hoverItem)
			table.DrawHighlight(back);

		const Sprite *sprite = box[plugin.currentState];
		const Point topLeft = table.GetRowBounds().TopLeft() - Point(sprite->Width(), 0.);
		Rectangle spriteBounds = Rectangle::FromCorner(topLeft, Point(sprite->Width(), sprite->Height()));
		SpriteShader::Draw(sprite, spriteBounds.Center());

		Rectangle zoneBounds = spriteBounds + pluginListBox.Center();

		// Only include the zone as clickable if it's within the drawing area.
		bool displayed = table.GetPoint().Y() > pluginListClip->Top() - 20 &&
			table.GetPoint().Y() < pluginListClip->Bottom() - table.GetRowBounds().Height() + 20;
		if(displayed)
			AddZone(zoneBounds, [&]() { PluginManager::TogglePlugin(plugin.name); });
		if(isSelected)
			table.Draw(plugin.name, bright);
		else
			table.Draw(plugin.name, plugin.enabled ? medium : dim);
	}

	// Switch back to normal opengl operations.
	target.Deactivate();

	pluginListClip->SetFadePadding(
		pluginListScroll.IsScrollAtMin() ? 0 : 20,
		pluginListScroll.IsScrollAtMax() ? 0 : 20
	);

	// Draw the scrolled and clipped plugin list to the screen.
	pluginListClip->Draw(pluginListBox.Center());
	const Point UP{0, -1};
	const Point DOWN{0, 1};
	const Point POINTER_OFFSET{0, 5};
	if(pluginListScroll.Scrollable())
	{
		// Draw up and down pointers, mostly to indicate when scrolling
		// is possible, but might as well make them clickable too.
		Rectangle topRight({pluginListBox.Right(), pluginListBox.Top() + POINTER_OFFSET.Y()}, {20.0, 20.0});
		PointerShader::Draw(topRight.Center(), UP,
			10.f, 10.f, 5.f, Color(pluginListScroll.IsScrollAtMin() ? .2f : .8f, 0.f));
		AddZone(topRight, [&]() { pluginListScroll.Scroll(-Preferences::ScrollSpeed()); });

		Rectangle bottomRight(pluginListBox.BottomRight() - POINTER_OFFSET, {20.0, 20.0});
		PointerShader::Draw(bottomRight.Center(), DOWN,
			10.f, 10.f, 5.f, Color(pluginListScroll.IsScrollAtMax() ? .2f : .8f, 0.f));
		AddZone(bottomRight, [&]() { pluginListScroll.Scroll(Preferences::ScrollSpeed()); });
	}

	// Draw the pre-rendered plugin description, if applicable.
	if(pluginDescriptionBuffer)
	{
		pluginDescriptionScroll.Step();

		pluginDescriptionBuffer->SetFadePadding(
			pluginDescriptionScroll.IsScrollAtMin() ? 0 : 20,
			pluginDescriptionScroll.IsScrollAtMax() ? 0 : 20
		);

		Rectangle descriptionBox = pluginUI->GetBox("plugin description");
		pluginDescriptionBuffer->Draw(
			descriptionBox.Center(),
			descriptionBox.Dimensions(),
			Point(0, static_cast<int>(pluginDescriptionScroll.AnimatedValue()))
		);

		if(pluginDescriptionScroll.Scrollable())
		{
			// Draw up and down pointers, mostly to indicate when
			// scrolling is possible, but might as well make them
			// clickable too.
			Rectangle topRight({descriptionBox.Right(), descriptionBox.Top() + POINTER_OFFSET.Y()}, {20.0, 20.0});
			PointerShader::Draw(topRight.Center(), UP,
				10.f, 10.f, 5.f, Color(pluginDescriptionScroll.IsScrollAtMin() ? .2f : .8f, 0.f));
			AddZone(topRight, [&]() { pluginDescriptionScroll.Scroll(-Preferences::ScrollSpeed()); });

			Rectangle bottomRight(descriptionBox.BottomRight() - POINTER_OFFSET, {20.0, 20.0});
			PointerShader::Draw(bottomRight.Center(), DOWN,
				10.f, 10.f, 5.f, Color(pluginDescriptionScroll.IsScrollAtMax() ? .2f : .8f, 0.f));
			AddZone(bottomRight, [&]() { pluginDescriptionScroll.Scroll(Preferences::ScrollSpeed()); });
		}
	}
}



// Render the named plugin description into the pluginDescriptionBuffer.
void PreferencesPanel::RenderPluginDescription(const string &pluginName)
{
	const Plugin *plugin = PluginManager::Get().Find(pluginName);
	if(plugin)
		RenderPluginDescription(*plugin);
	else
		pluginDescriptionBuffer.reset();
}



// Render the plugin description into the pluginDescriptionBuffer.
void PreferencesPanel::RenderPluginDescription(const Plugin &plugin)
{
	const Color &medium = *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(Preferences::GetFontSize());
	Rectangle box = GameData::Interfaces().Get("plugins")->GetBox("plugin description");

	// We are resizing and redrawing the description buffer. Reset the scroll
	// back to zero.
	pluginDescriptionScroll.Set(0, 0);

	// Compute the height before drawing, so that we know the scroll bounds.
	const Sprite *sprite = SpriteSet::Get(plugin.name);
	int descriptionHeight = 0;
	if(sprite)
		descriptionHeight += sprite->Height() + 10;

	WrappedText wrap(font);
	wrap.SetWrapWidth(box.Width());
	static const string EMPTY = "(No description given.)";
	wrap.Wrap(plugin.aboutText.empty() ? EMPTY : plugin.CreateDescription());

	descriptionHeight += wrap.Height();

	// Now that we know the size of the rendered description, resize the buffer
	// to fit, and activate it as a render target.
	if(descriptionHeight < box.Height())
		descriptionHeight = box.Height();
	pluginDescriptionScroll.SetMaxValue(descriptionHeight);
	pluginDescriptionBuffer = make_unique<RenderBuffer>(Point(box.Width(), descriptionHeight));
	// Redirect all drawing commands into the offscreen buffer.
	auto target = pluginDescriptionBuffer->SetTarget();

	Point top(pluginDescriptionBuffer->Left(), pluginDescriptionBuffer->Top());
	if(sprite)
	{
		Point center(0., top.Y() + .5 * sprite->Height());
		SpriteShader::Draw(sprite, center);
		top.Y() += sprite->Height() + 10.;
	}

	wrap.Draw(top, medium);
	target.Deactivate();
}



void PreferencesPanel::DrawTooltips()
{
	if(hoverItem.empty())
	{
		tooltip.DecrementCount();
		return;
	}
	tooltip.IncrementCount();
	if(!tooltip.ShouldDraw())
		return;

	if(!tooltip.HasText())
		tooltip.SetText(GameData::Tooltip(page == 'p' ? Preferences::DisplayName(hoverItem) : hoverItem));

	tooltip.Draw();
}



void PreferencesPanel::Exit()
{
	if(Command::MENU.HasConflict() || !Command::MENU.HasBinding())
	{
		GetUI().Push(DialogPanel::Info("Menu keybind is not bound or has conflicts."));
		return;
	}

	Command::SaveSettings(Files::Config() / "keys.txt");

	if(recacheDeadlines)
		player.CacheMissionInformation(true);

	GetUI().Pop(this);
}



void PreferencesPanel::HandleSettingsString(const string &str, Point cursorPosition)
{
	Preferences::Toggle(str, &GetUI());

	// If the deadline blink preference was toggled and the player is in flight,
	// then we need to recache the remaining mission deadlines. This doesn't need
	// to be done when the player is landed since the MapPanel already recalculates
	// the remaining deadlines when it is opened in that case.
	if(str == Preferences::MAP_DEADLINE_BLINK_BY_DISTANCE && !player.GetPlanet())
		recacheDeadlines = !recacheDeadlines;
}



void PreferencesPanel::HandleUp()
{
	selected = max(0, selected - 1);
	switch(page)
	{
	case 's':
		selectedItem = prefZones.at(selected).Value();
		break;
	case 'p':
		selectedPlugin = pluginZones.at(selected).Value();
		RenderPluginDescription(selectedPlugin);
		ScrollSelectedPlugin();
		break;
	default:
		break;
	}
}



void PreferencesPanel::HandleDown()
{
	switch(page)
	{
	case 'c':
		if(selected + 1 < static_cast<int>(zones.size()))
			selected++;
		break;
	case 's':
		selected = min(selected + 1, static_cast<int>(prefZones.size() - 1));
		selectedItem = prefZones.at(selected).Value();
		break;
	case 'p':
		selected = min(selected + 1, static_cast<int>(pluginZones.size() - 1));
		selectedPlugin = pluginZones.at(selected).Value();
		RenderPluginDescription(selectedPlugin);
		ScrollSelectedPlugin();
		break;
	default:
		break;
	}
}



void PreferencesPanel::HandleConfirm()
{
	switch(page)
	{
	case 'c':
		editing = selected;
		break;
	case 's':
		HandleSettingsString(selectedItem, Screen::Dimensions() / 2.);
		break;
	case 'p':
		PluginManager::TogglePlugin(selectedPlugin);
		break;
	default:
		break;
	}
}



void PreferencesPanel::ScrollSelectedPlugin()
{
	while(selected * 20 - pluginListScroll < 0)
		pluginListScroll.Scroll(-Preferences::ScrollSpeed());
	while(selected * 20 - pluginListScroll > pluginListClip->Height())
		pluginListScroll.Scroll(Preferences::ScrollSpeed());
}
