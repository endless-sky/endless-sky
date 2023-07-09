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
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "Plugins.h"
#include "Preferences.h"
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
#include <SDL2/SDL.h>

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
	const string STATUS_OVERLAYS_ALL = "Show status overlays";
	const string STATUS_OVERLAYS_FLAGSHIP = "   Show flagship overlay";
	const string STATUS_OVERLAYS_ESCORT = "   Show escort overlays";
	const string STATUS_OVERLAYS_ENEMY = "   Show enemy overlays";
	const string STATUS_OVERLAYS_NEUTRAL = "   Show neutral overlays";
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string TURRET_TRACKING = "Turret tracking";
	const string FOCUS_PREFERENCE = "Turrets focus fire";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";
	const string REACTIVATE_HELP = "Reactivate first-time help";
	const string SCROLL_SPEED = "Scroll speed";
	const string FIGHTER_REPAIR = "Repair fighters in";
	const string SHIP_OUTLINES = "Ship outlines in shops";
	const string BOARDING_PRIORITY = "Boarding target priority";
	const string TARGET_ASTEROIDS_BASED_ON = "Target asteroid based on";
	const string BACKGROUND_PARALLAX = "Parallax background";
	const string ALERT_INDICATOR = "Alert indicator";

	// How many pages of settings there are.
	const int SETTINGS_PAGE_COUNT = 2;
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
	if(SETTINGS_PAGE_COUNT > 1)
		info.SetCondition("multiple pages");
	if(currentSettingsPage > 0)
		info.SetCondition("show previous");
	if(currentSettingsPage + 1 < SETTINGS_PAGE_COUNT)
		info.SetCondition("show next");
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : "plugins");
	GameData::Interfaces().Get(pageName)->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);

	zones.clear();
	prefZones.clear();
	pluginZones.clear();
	if(page == 'c')
		DrawControls();
	else if(page == 's')
		DrawSettings();
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

	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		Exit();
	else if(key == 'c' || key == 's' || key == 'p')
		page = key;
	else if(key == 'o' && page == 'p')
		Files::OpenUserPluginFolder();
	else if((key == 'n' || key == SDLK_PAGEUP) && currentSettingsPage < SETTINGS_PAGE_COUNT - 1)
		++currentSettingsPage;
	else if((key == 'r' || key == SDLK_PAGEDOWN) && currentSettingsPage > 0)
		--currentSettingsPage;
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
			// For some settings, clicking the option does more than just toggle a
			// boolean state keyed by the option's name.
			if(zone.Value() == ZOOM_FACTOR)
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
				point *= Screen::Zoom() / 100.;
				point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
				SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
			}
			else if(zone.Value() == BOARDING_PRIORITY)
				Preferences::ToggleBoarding();
			else if(zone.Value() == BACKGROUND_PARALLAX)
				Preferences::ToggleParallax();
			else if(zone.Value() == VIEW_ZOOM_FACTOR)
			{
				// Increase the zoom factor unless it is at the maximum. In that
				// case, cycle around to the lowest zoom factor.
				if(!Preferences::ZoomViewIn())
					while(Preferences::ZoomViewOut()) {}
			}
			else if(zone.Value() == SCREEN_MODE_SETTING)
				Preferences::ToggleScreenMode();
			else if(zone.Value() == VSYNC_SETTING)
			{
				if(!Preferences::ToggleVSync())
					GetUI()->Push(new Dialog(
						"Unable to change VSync state. (Your system's graphics settings may be controlling it instead.)"));
			}
			else if(zone.Value() == STATUS_OVERLAYS_ALL)
				Preferences::CycleStatusOverlays(Preferences::OverlayType::ALL);
			else if(zone.Value() == STATUS_OVERLAYS_FLAGSHIP)
				Preferences::CycleStatusOverlays(Preferences::OverlayType::FLAGSHIP);
			else if(zone.Value() == STATUS_OVERLAYS_ESCORT)
				Preferences::CycleStatusOverlays(Preferences::OverlayType::ESCORT);
			else if(zone.Value() == STATUS_OVERLAYS_ENEMY)
				Preferences::CycleStatusOverlays(Preferences::OverlayType::ENEMY);
			else if(zone.Value() == STATUS_OVERLAYS_NEUTRAL)
				Preferences::CycleStatusOverlays(Preferences::OverlayType::NEUTRAL);
			else if(zone.Value() == AUTO_AIM_SETTING)
				Preferences::ToggleAutoAim();
			else if(zone.Value() == AUTO_FIRE_SETTING)
				Preferences::ToggleAutoFire();
			else if(zone.Value() == EXPEND_AMMO)
				Preferences::ToggleAmmoUsage();
			else if(zone.Value() == TURRET_TRACKING)
				Preferences::Set(FOCUS_PREFERENCE, !Preferences::Has(FOCUS_PREFERENCE));
			else if(zone.Value() == REACTIVATE_HELP)
			{
				for(const auto &it : GameData::HelpTemplates())
					Preferences::Set("help: " + it.first, false);
			}
			else if(zone.Value() == SCROLL_SPEED)
			{
				// Toggle between three different speeds.
				int speed = Preferences::ScrollSpeed() + 20;
				if(speed > 60)
					speed = 20;
				Preferences::SetScrollSpeed(speed);
			}
			else if(zone.Value() == ALERT_INDICATOR)
				Preferences::ToggleAlert();
			// All other options are handled by just toggling the boolean state.
			else
				Preferences::Set(zone.Value(), !Preferences::Has(zone.Value()));
			break;
		}

	for(const auto &zone : pluginZones)
		if(zone.Contains(point))
		{
			selectedPlugin = zone.Value();
			break;
		}

	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);

	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(hoverPoint))
			hover = index;

	hoverPreference.clear();
	for(const auto &zone : prefZones)
		if(zone.Contains(hoverPoint))
			hoverPreference = zone.Value();

	hoverPlugin.clear();
	for(const auto &zone : pluginZones)
		if(zone.Contains(hoverPoint))
			hoverPlugin = zone.Value();

	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool PreferencesPanel::Scroll(double dx, double dy)
{
	if(!dy || hoverPreference.empty())
		return false;

	if(hoverPreference == ZOOM_FACTOR)
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
	else if(hoverPreference == VIEW_ZOOM_FACTOR)
	{
		if(dy < 0.)
			Preferences::ZoomViewOut();
		else
			Preferences::ZoomViewIn();
	}
	else if(hoverPreference == SCROLL_SPEED)
	{
		int speed = Preferences::ScrollSpeed();
		if(dy < 0.)
			speed = max(20, speed - 20);
		else
			speed = min(60, speed + 20);
		Preferences::SetScrollSpeed(speed);
	}
	return true;
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

	static const string CATEGORIES[] = {
		"Keyboard Navigation",
		"Interface",
		"Targeting",
		"Weapons",
		"Interface",
		"Fleet"
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
		Command::MAP,
		Command::INFO,
		Command::NONE,
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::NEAREST_ASTEROID,
		Command::SCAN,
		Command::NONE,
		Command::PRIMARY,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::MOUSE_TURNING_HOLD,
		Command::NONE,
		Command::MENU,
		Command::FULLSCREEN,
		Command::FASTFORWARD,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD,
		Command::AMMO,
		Command::HARVEST
	};
	static const Command *BREAK = &COMMANDS[19];
	for(const Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
			table.DrawAt(Point(130, firstY));

		if(!command)
		{
			table.DrawGap(10);
			table.DrawUnderline(medium);
			if(category != end(CATEGORIES))
				table.Draw(*category++, bright);
			else
				table.Advance();
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
				table.SetHighlight(-120, 54);
				table.DrawHighlight(back);
			}

			// Highlight whichever row the mouse hovers over.
			table.SetHighlight(-120, 120);
			if(isHovering)
				table.DrawHighlight(back);

			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);

			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}

	Table shiftTable;
	shiftTable.AddColumn(125, {150, Alignment::RIGHT});
	shiftTable.SetUnderline(0, 130);
	shiftTable.DrawAt(Point(-400, 32));

	shiftTable.DrawUnderline(medium);
	shiftTable.Draw("With <shift> key", bright);
	shiftTable.DrawGap(5);
	shiftTable.Draw("Select nearest ship", medium);
	shiftTable.Draw("Select next escort", medium);
	shiftTable.Draw("Talk to planet", medium);
	shiftTable.Draw("Board disabled escort", medium);
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
		"",
		"Performance",
		"Show CPU / GPU load",
		"Render motion blur",
		"Reduce large graphics",
		"Draw background haze",
		"Draw starfield",
		BACKGROUND_PARALLAX,
		"Show hyperspace flash",
		SHIP_OUTLINES,
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
		"Flagship flotsam collection",
		EXPEND_AMMO,
		FIGHTER_REPAIR,
		"Fighters transfer cargo",
		"Rehire extra crew when lost",
		"",
		"Map",
		"Hide unexplored map regions",
		"Show escort systems on map",
		"Show stored outfits on map",
		"System map sends move orders",
		"\t",
		"Other",
		"Always underline shortcuts",
		REACTIVATE_HELP,
		"Interrupt fast-forward",
		SCROLL_SPEED
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

		if(setting == hoverPreference)
			table.DrawHighlight(back);
		table.Draw(setting, isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}
}



void PreferencesPanel::DrawPlugins()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const Sprite *box[2] = { SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked") };

	const int MAX_TEXT_WIDTH = 230;
	Table table;
	table.AddColumn(-115, {MAX_TEXT_WIDTH, Truncate::MIDDLE});
	table.SetUnderline(-120, 100);

	int firstY = -238;
	// Table is at -110 while checkbox is at -130
	table.DrawAt(Point(-110, firstY));
	table.DrawUnderline(medium);
	table.DrawGap(25);

	const Font &font = FontSet::Get(14);

	for(const auto &it : Plugins::Get())
	{
		const auto &plugin = it.second;
		if(!plugin.IsValid())
			continue;

		pluginZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), plugin.name);

		bool isSelected = (plugin.name == selectedPlugin);
		if(isSelected || plugin.name == hoverPlugin)
			table.DrawHighlight(back);

		const Sprite *sprite = box[plugin.currentState];
		Point topLeft = table.GetRowBounds().TopLeft() - Point(sprite->Width(), 0.);
		Rectangle spriteBounds = Rectangle::FromCorner(topLeft, Point(sprite->Width(), sprite->Height()));
		SpriteShader::Draw(sprite, spriteBounds.Center());

		topLeft.X() += 6.;
		topLeft.Y() += 7.;
		Rectangle zoneBounds = Rectangle::FromCorner(topLeft, Point(sprite->Width() - 8., sprite->Height() - 8.));

		AddZone(zoneBounds, [&]() { Plugins::TogglePlugin(plugin.name); });
		if(isSelected)
			table.Draw(plugin.name, bright);
		else
			table.Draw(plugin.name, plugin.enabled ? medium : dim);

		if(isSelected)
		{
			const Sprite *sprite = SpriteSet::Get(plugin.name);
			Point top(15., firstY);
			if(sprite)
			{
				Point center(130., top.Y() + .5 * sprite->Height());
				SpriteShader::Draw(sprite, center);
				top.Y() += sprite->Height() + 10.;
			}

			WrappedText wrap(font);
			wrap.SetWrapWidth(MAX_TEXT_WIDTH);
			static const string EMPTY = "(No description given.)";
			wrap.Wrap(plugin.aboutText.empty() ? EMPTY : plugin.aboutText);
			wrap.Draw(top, medium);
		}
	}
}



void PreferencesPanel::Exit()
{
	Command::SaveSettings(Files::Config() + "keys.txt");

	GetUI()->Pop(this);
}
