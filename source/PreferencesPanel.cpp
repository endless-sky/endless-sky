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

#include "text/alignment.hpp"
#include "Audio.h"
#include "Color.h"
#include "Dialog.h"
#include "Files.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Plugins.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "RenderBuffer.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"
#include "text/WrappedText.h"

#include "opengl.h"

#include <algorithm>

using namespace std;

namespace {
	// Settings that require special handling.
	const string ZOOM_FACTOR = "Main zoom factor";
	const int ZOOM_FACTOR_MIN = 100;
	const int ZOOM_FACTOR_MAX = 200;
	const int ZOOM_FACTOR_INCREMENT = 10;
	const string VIEW_ZOOM_FACTOR = "View zoom factor";
	const string AUTO_AIM_SETTING = "Automatic aiming";
	const string AUTO_FIRE_SETTING = "Automatic firing";
	const string SCREEN_MODE_SETTING = "Screen mode";
	const string VSYNC_SETTING = "VSync";
	const string CAMERA_ACCELERATION = "Camera acceleration";
	const string CLOAK_OUTLINE = "Cloaked ship outlines";
	const string STATUS_OVERLAYS_ALL = "Show status overlays";
	const string STATUS_OVERLAYS_FLAGSHIP = "   Show flagship overlay";
	const string STATUS_OVERLAYS_ESCORT = "   Show escort overlays";
	const string STATUS_OVERLAYS_ENEMY = "   Show enemy overlays";
	const string STATUS_OVERLAYS_NEUTRAL = "   Show neutral overlays";
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string FLOTSAM_SETTING = "Flotsam collection";
	const string TURRET_TRACKING = "Turret tracking";
	const string FOCUS_PREFERENCE = "Turrets focus fire";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";
	const string REACTIVATE_HELP = "Reactivate first-time help";
	const string SCROLL_SPEED = "Scroll speed";
	const string FIGHTER_REPAIR = "Repair fighters in";
	const string SHIP_OUTLINES = "Ship outlines in shops";
	const string DATE_FORMAT = "Date format";
	const string BOARDING_PRIORITY = "Boarding target priority";
	const string TARGET_ASTEROIDS_BASED_ON = "Target asteroid based on";
	const string BACKGROUND_PARALLAX = "Parallax background";
	const string EXTENDED_JUMP_EFFECTS = "Extended jump effects";
	const string ALERT_INDICATOR = "Alert indicator";
	const string HUD_SHIP_OUTLINES = "Ship outlines in HUD";

	// How many pages of controls and settings there are.
	const int CONTROLS_PAGE_COUNT = 2;
	const int SETTINGS_PAGE_COUNT = 2;
	// Hovering a preference for this many frames activates the tooltip.
	const int HOVER_TIME = 60;
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0), hover(-1)
{
	// Select the first valid plugin.
	for(const auto &plugin : Plugins::Get())
		if(plugin.second.IsValid())
		{
			selectedPlugin = plugin.first;
			break;
		}

	SetIsFullScreen(true);

	// Initialize a centered tooltip.
	hoverText.SetFont(FontSet::Get(14));
	hoverText.SetWrapWidth(250);
	hoverText.SetAlignment(Alignment::LEFT);

	// Set the initial plugin list and description scroll ranges.
	const Interface *pluginUi = GameData::Interfaces().Get("plugins");
	Rectangle pluginListBox = pluginUi->GetBox("plugin list");

	pluginListHeight = 0;
	for(const auto &plugin : Plugins::Get())
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
	GameData::Background().Draw(Point(), Point());

	Information info;
	info.SetBar("volume", Audio::Volume());
	if(Plugins::HasChanged())
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
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : "plugins");
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
	else if(key == 'c' || key == 's' || key == 'p')
	{
		page = key;
		hoverItem.clear();
		selected = 0;

		if(page == 'p')
		{
			// Reset the render buffers in case the UI scale has changed.
			const Interface *pluginUi = GameData::Interfaces().Get("plugins");
			Rectangle pluginListBox = pluginUi->GetBox("plugin list");
			pluginListClip = std::make_unique<RenderBuffer>(pluginListBox.Dimensions());
			RenderPluginDescription(selectedPlugin);
		}
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
		if(zones[latest].Value().KeyName() != Command::MENU.KeyName())
			Command::SetKey(zones[latest].Value(), 0);
	}
	else
		return false;

	return true;
}



bool PreferencesPanel::Click(int x, int y, int clicks)
{
	EndEditing();

	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		Audio::Play(Audio::Get("warder"));
		return true;
	}

	Point point(x, y);
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;

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

	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);

	hoverItem.clear();
	tooltip.clear();

	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(hoverPoint))
			hover = index;

	for(const auto &zone : prefZones)
		if(zone.Contains(hoverPoint))
			hoverItem = zone.Value();

	for(const auto &zone : pluginZones)
		if(zone.Contains(hoverPoint))
			hoverItem = zone.Value();

	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool PreferencesPanel::Scroll(double dx, double dy)
{
	if(!dy)
		return false;

	if(page == 's' && !hoverItem.empty())
	{
		if(hoverItem == ZOOM_FACTOR)
		{
			int zoom = Screen::UserZoom();
			if(dy < 0. && zoom > ZOOM_FACTOR_MIN)
				zoom -= ZOOM_FACTOR_INCREMENT;
			if(dy > 0. && zoom < ZOOM_FACTOR_MAX)
				zoom += ZOOM_FACTOR_INCREMENT;

			Screen::SetZoom(zoom);
			if(Screen::Zoom() != zoom)
				Screen::SetZoom(Screen::Zoom());

			// Convert to raw window coordinates, at the new zoom level.
			Point point = hoverPoint * (Screen::Zoom() / 100.);
			point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
			SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
		}
		else if(hoverItem == VIEW_ZOOM_FACTOR)
		{
			if(dy < 0.)
				Preferences::ZoomViewOut();
			else
				Preferences::ZoomViewIn();
		}
		else if(hoverItem == SCROLL_SPEED)
		{
			int speed = Preferences::ScrollSpeed();
			if(dy < 0.)
				speed = max(10, speed - 10);
			else
				speed = min(60, speed + 10);
			Preferences::SetScrollSpeed(speed);
		}
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
		Command::GATHER,
		Command::HOLD,
		Command::AMMO,
		Command::HARVEST,
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
		Command::NONE,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::FASTFORWARD,
		Command::HELP,
		Command::MESSAGE_LOG
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
			bool isConflicted = command.HasConflict();
			bool isEmpty = !command.HasBinding();
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

			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}

	Table infoTable;
	infoTable.AddColumn(125, {150, Alignment::RIGHT});
	infoTable.SetUnderline(0, 130);
	infoTable.DrawAt(Point(-400, 32));

	infoTable.DrawUnderline(medium);
	infoTable.Draw("Additional info", bright);
	infoTable.DrawGap(5);
	infoTable.Draw("Press '_x' over controls", medium);
	infoTable.Draw("to unbind them.", medium);
	infoTable.Draw("Controls can share", medium);
	infoTable.Draw("the same keybind.", medium);
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
		ZOOM_FACTOR,
		VIEW_ZOOM_FACTOR,
		SCREEN_MODE_SETTING,
		VSYNC_SETTING,
		CAMERA_ACCELERATION,
		"",
		"Performance",
		"Show CPU / GPU load",
		"Render motion blur",
		"Reduce large graphics",
		"Draw background haze",
		"Draw starfield",
		BACKGROUND_PARALLAX,
		"Show hyperspace flash",
		EXTENDED_JUMP_EFFECTS,
		SHIP_OUTLINES,
		HUD_SHIP_OUTLINES,
		CLOAK_OUTLINE,
		"\t",
		"HUD",
		STATUS_OVERLAYS_ALL,
		STATUS_OVERLAYS_FLAGSHIP,
		STATUS_OVERLAYS_ESCORT,
		STATUS_OVERLAYS_ENEMY,
		STATUS_OVERLAYS_NEUTRAL,
		"Show missile overlays",
		"Show asteroid scanner overlay",
		"Highlight player's flagship",
		"Rotate flagship in HUD",
		"Show planet labels",
		"Show mini-map",
		"Clickable radar display",
		ALERT_INDICATOR,
		"Extra fleet status messages",
		"\n",
		"Gameplay",
		"Control ship with mouse",
		AUTO_AIM_SETTING,
		AUTO_FIRE_SETTING,
		TURRET_TRACKING,
		TARGET_ASTEROIDS_BASED_ON,
		BOARDING_PRIORITY,
		EXPEND_AMMO,
		FLOTSAM_SETTING,
		FIGHTER_REPAIR,
		"Fighters transfer cargo",
		"Rehire extra crew when lost",
		"",
		"Map",
		"Deadline blink by distance",
		"Hide unexplored map regions",
		"Show escort systems on map",
		"Show stored outfits on map",
		"System map sends move orders",
		"\t",
		"Other",
		"Always underline shortcuts",
		REACTIVATE_HELP,
		"Interrupt fast-forward",
		"Landing zoom",
		SCROLL_SPEED,
		DATE_FORMAT
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
		else if(page > currentSettingsPage)
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

		// Get the "on / off" text for this setting. Setting "isOn"
		// draws the setting "bright" (i.e. the setting is active).
		bool isOn = Preferences::Has(setting);
		string text;
		if(setting == ZOOM_FACTOR)
		{
			isOn = Screen::UserZoom() == Screen::Zoom();
			text = to_string(Screen::UserZoom());
		}
		else if(setting == VIEW_ZOOM_FACTOR)
		{
			isOn = true;
			text = to_string(static_cast<int>(100. * Preferences::ViewZoom()));
		}
		else if(setting == SCREEN_MODE_SETTING)
		{
			isOn = true;
			text = Preferences::ScreenModeSetting();
		}
		else if(setting == VSYNC_SETTING)
		{
			text = Preferences::VSyncSetting();
			isOn = text != "off";
		}
		else if(setting == STATUS_OVERLAYS_ALL)
		{
			text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ALL);
			isOn = text != "off";
		}
		else if(setting == CAMERA_ACCELERATION)
		{
			text = Preferences::CameraAccelerationSetting();
			isOn = text != "off";
		}
		else if(setting == STATUS_OVERLAYS_FLAGSHIP)
		{
			text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::FLAGSHIP);
			isOn = text != "off" && text != "--";
		}
		else if(setting == STATUS_OVERLAYS_ESCORT)
		{
			text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ESCORT);
			isOn = text != "off" && text != "--";
		}
		else if(setting == STATUS_OVERLAYS_ENEMY)
		{
			text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ENEMY);
			isOn = text != "off" && text != "--";
		}
		else if(setting == STATUS_OVERLAYS_NEUTRAL)
		{
			text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::NEUTRAL);
			isOn = text != "off" && text != "--";
		}
		else if(setting == CLOAK_OUTLINE)
		{
			text = Preferences::Has(CLOAK_OUTLINE) ? "fancy" : "fast";
			isOn = true;
		}
		else if(setting == AUTO_AIM_SETTING)
		{
			text = Preferences::AutoAimSetting();
			isOn = text != "off";
		}
		else if(setting == AUTO_FIRE_SETTING)
		{
			text = Preferences::AutoFireSetting();
			isOn = text != "off";
		}
		else if(setting == EXPEND_AMMO)
			text = Preferences::AmmoUsage();
		else if(setting == DATE_FORMAT)
		{
			text = Preferences::DateFormatSetting();
			isOn = true;
		}
		else if(setting == FLOTSAM_SETTING)
		{
			text = Preferences::FlotsamSetting();
			isOn = text != "off";
		}
		else if(setting == TURRET_TRACKING)
		{
			isOn = true;
			text = Preferences::Has(FOCUS_PREFERENCE) ? "focused" : "opportunistic";
		}
		else if(setting == FIGHTER_REPAIR)
		{
			isOn = true;
			text = Preferences::Has(FIGHTER_REPAIR) ? "parallel" : "series";
		}
		else if(setting == SHIP_OUTLINES)
		{
			isOn = true;
			text = Preferences::Has(SHIP_OUTLINES) ? "fancy" : "fast";
		}
		else if(setting == HUD_SHIP_OUTLINES)
		{
			isOn = true;
			text = Preferences::Has(HUD_SHIP_OUTLINES) ? "fancy" : "fast";
		}
		else if(setting == BOARDING_PRIORITY)
		{
			isOn = true;
			text = Preferences::BoardingSetting();
		}
		else if(setting == TARGET_ASTEROIDS_BASED_ON)
		{
			isOn = true;
			text = Preferences::Has(TARGET_ASTEROIDS_BASED_ON) ? "proximity" : "value";
		}
		else if(setting == BACKGROUND_PARALLAX)
		{
			text = Preferences::ParallaxSetting();
			isOn = text != "off";
		}
		else if(setting == EXTENDED_JUMP_EFFECTS)
		{
			text = Preferences::ExtendedJumpEffectsSetting();
			isOn = text != "off";
		}
		else if(setting == REACTIVATE_HELP)
		{
			// Check how many help messages have been displayed.
			const map<string, string> &help = GameData::HelpTemplates();
			int shown = 0;
			int total = 0;
			for(const auto &it : help)
			{
				// Don't count certain special help messages that are always
				// active for new players.
				bool special = false;
				const string SPECIAL_HELP[] = {"basics", "lost"};
				for(const string &str : SPECIAL_HELP)
					if(it.first.find(str) == 0)
						special = true;

				if(!special)
				{
					++total;
					shown += Preferences::Has("help: " + it.first);
				}
			}

			if(shown)
				text = to_string(shown) + " / " + to_string(total);
			else
			{
				isOn = true;
				text = "done";
			}
		}
		else if(setting == SCROLL_SPEED)
		{
			isOn = true;
			text = to_string(Preferences::ScrollSpeed());
		}
		else if(setting == ALERT_INDICATOR)
		{
			isOn = Preferences::GetAlertIndicator() != Preferences::AlertIndicator::NONE;
			text = Preferences::AlertSetting();
		}
		else
			text = isOn ? "on" : "off";

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

		table.Draw(setting, isOn ? medium : dim);
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

	for(const auto &it : Plugins::Get())
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
			AddZone(zoneBounds, [&]() { Plugins::TogglePlugin(plugin.name); });
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
void PreferencesPanel::RenderPluginDescription(const std::string &pluginName)
{
	const Plugin *plugin = Plugins::Get().Find(pluginName);
	if(plugin)
		RenderPluginDescription(*plugin);
	else
		pluginDescriptionBuffer.reset();
}



// Render the plugin description into the pluginDescriptionBuffer.
void PreferencesPanel::RenderPluginDescription(const Plugin &plugin)
{
	const Color &medium = *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);
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
	pluginDescriptionBuffer = std::make_unique<RenderBuffer>(Point(box.Width(), descriptionHeight));
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
		// Step the tooltip timer back.
		hoverCount -= hoverCount ? 1 : 0;
		return;
	}

	// Step the tooltip timer forward [0-60].
	hoverCount += hoverCount < HOVER_TIME;

	if(hoverCount < HOVER_TIME)
		return;

	// Create the tooltip text.
	if(tooltip.empty())
	{
		tooltip = GameData::Tooltip(hoverItem);
		// No tooltip for this item.
		if(tooltip.empty())
			return;
		hoverText.Wrap(tooltip);
	}

	Point size(hoverText.WrapWidth(), hoverText.Height() - hoverText.ParagraphBreak());
	size += Point(20., 20.);
	Point topLeft = hoverPoint;
	// Do not overflow the screen dimensions.
	if(topLeft.X() + size.X() > Screen::Right())
		topLeft.X() -= size.X();
	if(topLeft.Y() + size.Y() > Screen::Bottom())
		topLeft.Y() -= size.Y();
	// Draw the background fill and the tooltip text.
	FillShader::Fill(topLeft + .5 * size, size, *GameData::Colors().Get("tooltip background"));
	hoverText.Draw(topLeft + Point(10., 10.), *GameData::Colors().Get("medium"));
}



void PreferencesPanel::Exit()
{
	Command::SaveSettings(Files::Config() + "keys.txt");

	GetUI()->Pop(this);
}



void PreferencesPanel::HandleSettingsString(const string &str, Point cursorPosition)
{
	// For some settings, clicking the option does more than just toggle a
	// boolean state keyed by the option's name.
	if(str == ZOOM_FACTOR)
	{
		int newZoom = Screen::UserZoom() + ZOOM_FACTOR_INCREMENT;
		Screen::SetZoom(newZoom);
		if(newZoom > ZOOM_FACTOR_MAX || Screen::Zoom() != newZoom)
		{
			// Notify the user why setting the zoom any higher isn't permitted.
			// Only show this if it's not possible to zoom the view at all, as
			// otherwise the dialog will show every time, which is annoying.
			if(newZoom == ZOOM_FACTOR_MIN + ZOOM_FACTOR_INCREMENT)
				GetUI()->Push(new Dialog(
					"Your screen resolution is too low to support a zoom level above 100%."));
			Screen::SetZoom(ZOOM_FACTOR_MIN);
		}
		// Convert to raw window coordinates, at the new zoom level.
		cursorPosition *= Screen::Zoom() / 100.;
		cursorPosition += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
		SDL_WarpMouseInWindow(nullptr, cursorPosition.X(), cursorPosition.Y());
	}
	else if(str == BOARDING_PRIORITY)
		Preferences::ToggleBoarding();
	else if(str == BACKGROUND_PARALLAX)
		Preferences::ToggleParallax();
	else if(str == EXTENDED_JUMP_EFFECTS)
		Preferences::ToggleExtendedJumpEffects();
	else if(str == VIEW_ZOOM_FACTOR)
	{
		// Increase the zoom factor unless it is at the maximum. In that
		// case, cycle around to the lowest zoom factor.
		if(!Preferences::ZoomViewIn())
			while(Preferences::ZoomViewOut()) {}
	}
	else if(str == SCREEN_MODE_SETTING)
		Preferences::ToggleScreenMode();
	else if(str == VSYNC_SETTING)
	{
		if(!Preferences::ToggleVSync())
			GetUI()->Push(new Dialog(
				"Unable to change VSync state. (Your system's graphics settings may be controlling it instead.)"));
	}
	else if(str == CAMERA_ACCELERATION)
		Preferences::ToggleCameraAcceleration();
	else if(str == STATUS_OVERLAYS_ALL)
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ALL);
	else if(str == STATUS_OVERLAYS_FLAGSHIP)
		Preferences::CycleStatusOverlays(Preferences::OverlayType::FLAGSHIP);
	else if(str == STATUS_OVERLAYS_ESCORT)
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ESCORT);
	else if(str == STATUS_OVERLAYS_ENEMY)
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ENEMY);
	else if(str == STATUS_OVERLAYS_NEUTRAL)
		Preferences::CycleStatusOverlays(Preferences::OverlayType::NEUTRAL);
	else if(str == AUTO_AIM_SETTING)
		Preferences::ToggleAutoAim();
	else if(str == AUTO_FIRE_SETTING)
		Preferences::ToggleAutoFire();
	else if(str == EXPEND_AMMO)
		Preferences::ToggleAmmoUsage();
	else if(str == FLOTSAM_SETTING)
		Preferences::ToggleFlotsam();
	else if(str == TURRET_TRACKING)
		Preferences::Set(FOCUS_PREFERENCE, !Preferences::Has(FOCUS_PREFERENCE));
	else if(str == REACTIVATE_HELP)
	{
		for(const auto &it : GameData::HelpTemplates())
			Preferences::Set("help: " + it.first, false);
	}
	else if(str == SCROLL_SPEED)
	{
		// Toggle between six different speeds.
		int speed = Preferences::ScrollSpeed() + 10;
		if(speed > 60)
			speed = 10;
		Preferences::SetScrollSpeed(speed);
	}
	else if(str == DATE_FORMAT)
		Preferences::ToggleDateFormat();
	else if(str == ALERT_INDICATOR)
		Preferences::ToggleAlert();
	// All other options are handled by just toggling the boolean state.
	else
		Preferences::Set(str, !Preferences::Has(str));
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
		Plugins::TogglePlugin(selectedPlugin);
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
