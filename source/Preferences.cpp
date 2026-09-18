/* Preferences.cpp
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

#include "Preferences.h"

#include "Command.h"
#include "CustomEvents.h"
#include "text/Alignment.h"
#include "audio/Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "DialogPanel.h"
#include "Files.h"
#include "text/Format.h"
#include "GameData.h"
#include "GameWindow.h"
#include "Interface.h"
#include "Logger.h"
#include "Screen.h"
#include "Setting.h"
#include "UI.h"

#ifdef _WIN32
#include "windows/WinVersion.h"
#endif

#include <algorithm>
#include <cstddef>
#include <map>

using namespace std;

namespace {
	map<string, Setting> settings;
	map<string, bool> help;
	map<string, int> unrecognized;

	int scrollSpeed = 60;
	int tooltipActivation = 60;

	const int ZOOM_FACTOR_MIN = 100;
	const int ZOOM_FACTOR_INCREMENT = 10;

	size_t zoomIndex = 4;
	constexpr double VOLUME_SCALE = .25;

	const map<string, SoundCategory> VOLUME_SETTINGS = {
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

	class OverlaySetting {
	public:
		OverlaySetting() = default;
		OverlaySetting(const Preferences::OverlayState &state) : state(state) {}

		operator Preferences::OverlayState() const { return state; }

		bool IsActive() const { return state != Preferences::OverlayState::DISABLED; }

		const string &ToString() const
		{
			return OVERLAY_SETTINGS[max<int>(0, min<int>(OVERLAY_SETTINGS.size() - 1, static_cast<int>(state)))];
		}

		int ToInt() const { return static_cast<int>(state); }

		void SetState(int value)
		{
			value = max<int>(value, 0);
			value = min<int>(value, OVERLAY_SETTINGS.size() - 1);
			state = static_cast<Preferences::OverlayState>(value);
		}

		void Increment()
		{
			switch(state)
			{
				case Preferences::OverlayState::OFF:
					state = Preferences::OverlayState::ON;
					break;
				case Preferences::OverlayState::ON:
					state = Preferences::OverlayState::DAMAGED;
					break;
				case Preferences::OverlayState::DAMAGED:
					state = Preferences::OverlayState::ON_HIT;
					break;
				case Preferences::OverlayState::ON_HIT:
					state = Preferences::OverlayState::OFF;
					break;
				case Preferences::OverlayState::DISABLED:
					state = Preferences::OverlayState::OFF;
					break;
			}
		}

	private:
		static const vector<string> OVERLAY_SETTINGS;

	private:
		Preferences::OverlayState state = Preferences::OverlayState::OFF;
	};

	const vector<string> OverlaySetting::OVERLAY_SETTINGS = {"off", "always on", "damaged", "--", "on hit"};

	map<Preferences::OverlayType, OverlaySetting> statusOverlaySettings = {
		{Preferences::OverlayType::ALL, Preferences::OverlayState::OFF},
		{Preferences::OverlayType::FLAGSHIP, Preferences::OverlayState::ON},
		{Preferences::OverlayType::ESCORT, Preferences::OverlayState::ON},
		{Preferences::OverlayType::ENEMY, Preferences::OverlayState::ON},
		{Preferences::OverlayType::NEUTRAL, Preferences::OverlayState::OFF},
	};

	int previousSaveCount = 3;

	// The font size to be used for various UI panels that can support displaying larger text than the default.
	const vector<int> FONT_SIZES = {14, 18};
}



const string Preferences::ALERT_INDICATOR = "alert indicator";
const string Preferences::AMMO_REFILL = "Ammo refill";
const string Preferences::ANIMATE_MENU_BACKGROUND = "Animate main menu background";
const string Preferences::ASTEROID_TARGETING = "Target asteroid based on";
const string Preferences::AUTO_AIM = "Automatic aiming";
const string Preferences::AUTO_FIRE = "Automatic firing";
const string Preferences::AUTO_UNPARK_FLAGSHIP = "Automatically unpark flagship";
const string Preferences::BLOCK_SCREEN_SAVER = "Block screen saver";
const string Preferences::BOARDING_TARGET_PRIORITY = "boarding target";
const string Preferences::CAMERA_ACCELERATION = "camera acceleration";
const string Preferences::DAMAGED_FIGHTERS_RETREAT = "Damaged fighters retreat";
const string Preferences::DATE_FORMAT = "date format";
const string Preferences::DEFER_LOADING_IMAGES = "Defer loading images";
const string Preferences::DESTINATION_NOTIFICATION = "notification settings";
const string Preferences::DRAW_BACKGROUND_HAZE = "Draw background haze";
const string Preferences::DRAW_STARFIELD = "Draw starfield";
const string Preferences::ESCORT_AMMO_USAGE = "Escort ammo usage";
const string Preferences::EXTENDED_JUMP_EFFECTS = "Extended jump effects";
const string Preferences::FF_CAPSLOCK_SYNC = "Sync FF to CapsLock";
const string Preferences::FIGHTERS_REPAIR_IN = "Repair fighters in";
const string Preferences::FIGHTERS_TRANSFER_CARGO = "Fighters transfer cargo";
const string Preferences::FIXED_STARFIELD_ZOOM = "Fixed starfield zoom";
const string Preferences::FLAGSHIP_SPACE_PRIORITY = "Prioritize flagship use";
const string Preferences::FLOTSAM_COLLECTION = "Flotsam collection";
const string Preferences::FONT_SIZE = "font size";
const string Preferences::HUD_ASTEROID_OVERLAY = "Show asteroid scanner overlay";
const string Preferences::HUD_CLICKABLE_RADAR = "Clickable radar display";
const string Preferences::HUD_DISABLE_RADAR_VIEWPORT = "Disable viewport on radar";
const string Preferences::HUD_EXTRA_STATUS_MSGS = "Extra fleet status messages";
const string Preferences::HUD_MISSILE_OVERLAY = "Show missile overlays";
const string Preferences::HUD_ROTATE_FLAGSHIP = "Rotate flagship in HUD";
const string Preferences::HUD_TURRET_OVERLAY = "Turret overlays";
const string Preferences::HUD_PLANET_LABELS = "Show planet labels";
const string Preferences::INTERRUPT_FAST_FORWARD = "Interrupt fast-forward";
const string Preferences::LANDING_ZOOM = "Landing zoom";
const string Preferences::MAP_DEADLINE_BLINK_BY_DISTANCE = "Deadline blink by distance";
const string Preferences::MAP_HIDE_UNEXPLORED = "Hide unexplored map regions";
const string Preferences::MAP_SHOW_ESCORTS = "Show escort systems on map";
const string Preferences::MAP_SHOW_OUTFITS = "Show stored outfits on map";
const string Preferences::MAP_PARENTHESIZE_PROFIT = "Parenthesize trade profits";
const string Preferences::MINIMAP_DISPLAY = "Show mini-map";
const string Preferences::MOTION_BLUR = "Render motion blur";
const string Preferences::MOUSE_CONTROL_FLAGSHIP = "Control ship with mouse";
const string Preferences::MOUSE_CONTROL_TURRETS = "Aim turrets with mouse";
const string Preferences::PARALLAX = "Parallax background";
const string Preferences::PREVIOUS_SAVES = "previous saves";
const string Preferences::REACTIVATE_HELP = "reactivate help";
const string Preferences::REDUCE_LARGE_GRAPHICS = "Reduce large graphics";
const string Preferences::REHIRE_LOST_CREW = "Rehire extra crew when lost";
const string Preferences::SAVE_MSG_LOGS = "Save message log";
const string Preferences::SCREEN_MODE = "fullscreen";
const string Preferences::SCREEN_MAXIMIZED = "maximized";
const string Preferences::SCROLL_SPEED = "scroll speed";
const string Preferences::SHIP_HIGHLIGHTS = "Highlight ships";
const string Preferences::SHIP_OUTLINES_CLOAKED = "Cloaked ship outlines";
const string Preferences::SHIP_OUTLINES_HUD = "Ship outlines in HUD";
const string Preferences::SHIP_OUTLINES_SHOP = "Ship outlines in shops";
const string Preferences::SHOW_HYPERSPACE_FLASH = "Show hyperspace flash";
const string Preferences::SHOW_PERFORMANCE_METRICS = "Show CPU / GPU load";
const string Preferences::STATUS_OVERLAYS_ALL = "Show all status overlays";
const string Preferences::STATUS_OVERLAYS_FLAGSHIP = "Show flagship overlay";
const string Preferences::STATUS_OVERLAYS_ESCORT = "Show escort overlays";
const string Preferences::STATUS_OVERLAYS_ENEMY = "Show enemy overlays";
const string Preferences::STATUS_OVERLAYS_NEUTRAL = "Show neutral overlays";
const string Preferences::TEXT_ALIGNMENT = "Text alignment";
const string Preferences::TEXTURE_FILTERING = "Texture filtering";
const string Preferences::TOOLTIP_ACTIVATION_TIME = "Tooltip activation time";
const string Preferences::TRIBUTE_CONFIRMATION = "Tribute confirmation";
const string Preferences::TRADE_SELL_OUTFITS_WITHOUT_SHOP = "Sell outfits without outfitter";
const string Preferences::TRADE_CONFIRM_MINABLES = "Confirm selling minables";
const string Preferences::TRADE_CONFIRM_OUTFITS = "Confirm selling outfits";
const string Preferences::TURRETS_FOCUS_FIRE = "Turrets focus fire";
const string Preferences::UNDERLINE_SHORTCUTS = "Always underline shortcuts";
const string Preferences::VSYNC = "vsync";
const string Preferences::WINDOW_SIZE = "window size";
const string Preferences::ZOOM_FACTOR_MAIN = "zoom";
const string Preferences::ZOOM_FACTOR_VIEW = "view zoom";
#ifdef _WIN32
const string Preferences::TITLE_BAR_THEME = "Title bar theme";
const string Preferences::WINDOW_ROUNDING = "Window rounding";
#endif



void Preferences::Init()
{
	settings.clear();
	help.clear();
	unrecognized.clear();

	// Define allowable and default values for all settings.
	settings[ALERT_INDICATOR] = Setting::List("Alert indicator", 3, {"off", "audio", "visual", "both"});
	settings[AMMO_REFILL] = Setting::List("Auto refill ammo", 1, {"never", "ask", "when free", "always"});
	settings[ANIMATE_MENU_BACKGROUND] = Setting::Boolean("Animate main menu background", true);
	settings[ASTEROID_TARGETING] = Setting::List("Asteroid targeting", 0,
		{"proximity", "expected value", "quality"}, true);
	settings[AUTO_AIM] = Setting::List("Automatic aiming", 2, {"off", "always on", "when firing"});
	settings[AUTO_FIRE] = Setting::List("Automatic firing", 0, {"off", "on", "guns only", "turrets only"});
	settings[AUTO_UNPARK_FLAGSHIP] = Setting::Boolean("Automatically unpark flagship", false);

	settings[BLOCK_SCREEN_SAVER] = Setting::Boolean("Block screen saver", false);
	settings[BLOCK_SCREEN_SAVER].SetOnToggleFunc([](UI *ui) -> void { GameWindow::ToggleBlockScreenSaver(); });

	settings[BOARDING_TARGET_PRIORITY] = Setting::List("Boarding target priority", 0,
		{"proximity", "value", "mixed"}, true);
	settings[CAMERA_ACCELERATION] = Setting::List("Camera acceleration", 0, {"off", "on", "reversed"});
	settings[DAMAGED_FIGHTERS_RETREAT] = Setting::Boolean("Damaged fighters retreat", true);
	settings[DATE_FORMAT] = Setting::List("Date format", 0, {"dd/mm/yyyy", "mm/dd/yyyy", "yyyy-mm-dd"}, true);
	settings[DEFER_LOADING_IMAGES] = Setting::Boolean("Defer loading images", false);
	settings[DESTINATION_NOTIFICATION] = Setting::List("Notify on destination", 1, {"off", "message", "both"});
	settings[DRAW_BACKGROUND_HAZE] = Setting::Boolean("Draw background haze", true);
	settings[DRAW_STARFIELD] = Setting::Boolean("Draw starfield", true);
	settings[ESCORT_AMMO_USAGE] = Setting::List("Escorts expend ammo", 1, {"never", "frugally", "always"});

	settings[EXTENDED_JUMP_EFFECTS] = Setting::List("Extended jump effects", 0, {"off", "medium", "heavy"});
	settings[FF_CAPSLOCK_SYNC] = Setting::List("Sync FF to CapsLock", 0, {"default", "never", "always"}, true);
	settings[FF_CAPSLOCK_SYNC].SetDisplayFunc([](int index) -> pair<string, bool> {
		const FastForwardCapsLockSync sync = GetFastForwardCapsLockSync();
		bool isOn = sync == FastForwardCapsLockSync::ALWAYS
			|| (sync == FastForwardCapsLockSync::DEFAULT && Command(SDLK_CAPSLOCK).Has(Command::FASTFORWARD));
		return make_pair("TODO", isOn);
	});

	settings[FIGHTERS_REPAIR_IN] = Setting::List("Repair fighters in", 1, {"series", "parallel"}, true);
	settings[FIGHTERS_TRANSFER_CARGO] = Setting::Boolean("Fighters transfer cargo", false);
	settings[FIXED_STARFIELD_ZOOM] = Setting::Boolean("Fixed starfield zoom", false);
	settings[FLAGSHIP_SPACE_PRIORITY] = Setting::List("Prioritize flagship use", 1,
		{"none", "passengers", "cargo", "both"});
	settings[FLOTSAM_COLLECTION] = Setting::List("Flotsam collection", 1,
		{"off", "on", "flagship only", "escorts only"});

	settings[FONT_SIZE] = Setting::List("UI font size", 0, {"14", "18"}, true);
	settings[FONT_SIZE].SetOnToggleFunc([](UI *ui) -> void { CustomEvents::SendAdjustText(); });

	settings[HUD_ASTEROID_OVERLAY] = Setting::Boolean("Show asteroid scanner overlay", true);
	settings[HUD_CLICKABLE_RADAR] = Setting::Boolean("Clickable radar display", false);
	// TODO: This has been a preferences since 2018, but it isn't in PreferencesPanel.
	settings[HUD_DISABLE_RADAR_VIEWPORT] = Setting::Boolean("Disable viewport on radar", false);
	settings[HUD_EXTRA_STATUS_MSGS] = Setting::Boolean("Extra fleet status messages", true);
	settings[HUD_MISSILE_OVERLAY] = Setting::Boolean("Show missile overlays", false);
	settings[HUD_ROTATE_FLAGSHIP] = Setting::Boolean("Rotate flagship in HUD", false);
	settings[HUD_TURRET_OVERLAY] = Setting::List("Turret overlays", 2, {"off", "always on", "blindspots only"});
	settings[HUD_PLANET_LABELS] = Setting::Boolean("Show planet labels", true);
	settings[INTERRUPT_FAST_FORWARD] = Setting::Boolean("Interrupt fast-forward", false);
	settings[LANDING_ZOOM] = Setting::Boolean("Landing zoom", false);
	settings[MAP_DEADLINE_BLINK_BY_DISTANCE] = Setting::Boolean("Deadline blink by distance", true);
	settings[MAP_HIDE_UNEXPLORED] = Setting::Boolean("Hide unexplored map regions", true);
	settings[MAP_SHOW_ESCORTS] = Setting::Boolean("Show escort systems on map", true);
	settings[MAP_SHOW_OUTFITS] = Setting::Boolean("Show stored outfits on map", true);
	settings[MAP_PARENTHESIZE_PROFIT] = Setting::Boolean("Parenthesize trade profits", false);
	settings[MINIMAP_DISPLAY] = Setting::List("Show mini-map", 1, {"off", "when jumping", "always on"});
	settings[MOTION_BLUR] = Setting::Boolean("Render motion blur", true);
	settings[MOUSE_CONTROL_FLAGSHIP] = Setting::Boolean("Control ship with mouse", false);
	settings[MOUSE_CONTROL_TURRETS] = Setting::Boolean("Aim turrets with mouse", false);
	settings[PARALLAX] = Setting::List("Parallax background", 2, {"off", "fancy", "fast"});
	// "Previous saves" is unique and not rendered in PreferencesPanel, and so it doesn't need a Setting.

	settings[REACTIVATE_HELP] = Setting::Unique("Reactivate first-time help");
	settings[REACTIVATE_HELP].SetDisplayFunc([](int index) -> pair<string, bool> {
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
				shown += Preferences::HelpShown("help: " + it.first);
			}
		}

		if(shown)
			return make_pair(to_string(shown) + " / " + to_string(total), false);
		return make_pair("done", true);
	});
	settings[REACTIVATE_HELP].SetOnToggleFunc([](UI *ui) -> void {
		for(const auto &it : GameData::HelpTemplates())
			SetHelp("help: " + it.first, false);
	});

	settings[REDUCE_LARGE_GRAPHICS] = Setting::List("Reduce large graphics", 0, {"off", "largest only", "all"});
	settings[REHIRE_LOST_CREW] = Setting::Boolean("Rehire extra crew when lost", false);
	settings[SAVE_MSG_LOGS] = Setting::Boolean("Save message log", false);

	settings[SCREEN_MODE] = Setting::List("Screen mode", 1, {"windowed", "fullscreen"}, true);
	settings[SCREEN_MODE].SetToggleFunc([](int index) -> int {
		GameWindow::ToggleFullscreen();
		return GameWindow::IsFullscreen();
	});

	settings[SCROLL_SPEED] = Setting::Unique("Scroll speed");
	settings[SCROLL_SPEED].SetDisplayFunc([](int index) -> pair<string, bool> {
		return make_pair(to_string(ScrollSpeed()), true);
	});
	settings[SCROLL_SPEED].SetOnToggleFunc([](UI *ui) -> void {
		// Toggle between six different speeds.
		int speed = ScrollSpeed() + 10;
		if(speed > 60)
			speed = 10;
		SetScrollSpeed(speed);
	});
	settings[SCROLL_SPEED].SetScrollFunc([](double dy) -> void {
		int speed = ScrollSpeed();
		if(dy < 0.)
			speed = max(10, speed - 10);
		else
			speed = min(60, speed + 10);
		SetScrollSpeed(speed);
	});

	settings[SCREEN_MAXIMIZED] = Setting::Boolean("Screen maximized", false);
	settings[SHIP_HIGHLIGHTS] = Setting::List("Highlight ships", 0, {"off", "flagship", "owned ships", "all"});
	settings[SHIP_OUTLINES_CLOAKED] = Setting::List("Cloaked ship outlines", 1, {"fast", "fancy"}, true);
	settings[SHIP_OUTLINES_HUD] = Setting::List("Ship outlines in HUD", 1, {"fast", "fancy"}, true);
	settings[SHIP_OUTLINES_SHOP] = Setting::List("Ship outlines in shops", 1, {"fast", "fancy"}, true);
	settings[SHOW_HYPERSPACE_FLASH] = Setting::Boolean("Show hyperspace flash", false);
	settings[SHOW_PERFORMANCE_METRICS] = Setting::Boolean("Show CPU / GPU load", false);

	settings[STATUS_OVERLAYS_ALL] = Setting::Unique("Show status overlays");
	settings[STATUS_OVERLAYS_ALL].SetDisplayFunc([](int index) -> pair<string, bool> {
		string text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ALL);
		bool isOn = text != "off";
		return make_pair(text, isOn);
	});
	settings[STATUS_OVERLAYS_ALL].SetOnToggleFunc([](UI *ui) -> void {
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ALL);
	});

	settings[STATUS_OVERLAYS_FLAGSHIP] = Setting::Unique("   Show flagship overlay");
	settings[STATUS_OVERLAYS_FLAGSHIP].SetDisplayFunc([](int index) -> pair<string, bool> {
		string text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::FLAGSHIP);
		bool isOn = text != "off" && text != "--";
		return make_pair(text, isOn);
	});
	settings[STATUS_OVERLAYS_FLAGSHIP].SetOnToggleFunc([](UI *ui) -> void {
		Preferences::CycleStatusOverlays(Preferences::OverlayType::FLAGSHIP);
	});

	settings[STATUS_OVERLAYS_ESCORT] = Setting::Unique("   Show escort overlays");
	settings[STATUS_OVERLAYS_ESCORT].SetDisplayFunc([](int index) -> pair<string, bool> {
		string text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ESCORT);
		bool isOn = text != "off" && text != "--";
		return make_pair(text, isOn);
	});
	settings[STATUS_OVERLAYS_ESCORT].SetOnToggleFunc([](UI *ui) -> void {
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ESCORT);
	});

	settings[STATUS_OVERLAYS_ENEMY] = Setting::Unique("   Show enemy overlays");
	settings[STATUS_OVERLAYS_ENEMY].SetDisplayFunc([](int index) -> pair<string, bool> {
		string text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::ENEMY);
		bool isOn = text != "off" && text != "--";
		return make_pair(text, isOn);
	});
	settings[STATUS_OVERLAYS_ENEMY].SetOnToggleFunc([](UI *ui) -> void {
		Preferences::CycleStatusOverlays(Preferences::OverlayType::ENEMY);
	});

	settings[STATUS_OVERLAYS_NEUTRAL] = Setting::Unique("   Show neutral overlays");
	settings[STATUS_OVERLAYS_NEUTRAL].SetDisplayFunc([](int index) -> pair<string, bool> {
		string text = Preferences::StatusOverlaysSetting(Preferences::OverlayType::NEUTRAL);
		bool isOn = text != "off" && text != "--";
		return make_pair(text, isOn);
	});
	settings[STATUS_OVERLAYS_NEUTRAL].SetOnToggleFunc([](UI *ui) -> void {
		Preferences::CycleStatusOverlays(Preferences::OverlayType::NEUTRAL);
	});

	settings[TEXT_ALIGNMENT] = Setting::List("Text alignment", 3, {"left", "center", "right", "justified"}, true);
	settings[TEXT_ALIGNMENT].SetOnToggleFunc([](UI *ui) -> void { CustomEvents::SendAdjustText(); });

	settings[TEXTURE_FILTERING] = Setting::List("Texture filtering", 1, {"nearest", "linear"}, true);

	settings[TOOLTIP_ACTIVATION_TIME] = Setting::Unique("Tooltip activation time");
	settings[TOOLTIP_ACTIVATION_TIME].SetDisplayFunc([](int index) -> pair<string, bool> {
		return make_pair(Format::StepsToSeconds(TooltipActivation()), true);
	});
	settings[TOOLTIP_ACTIVATION_TIME].SetOnToggleFunc([](UI *ui) -> void {
		int steps = TooltipActivation() + 20;
		if(steps > 120)
			steps = 0;
		SetTooltipActivation(steps);
		CustomEvents::SendTooltipUpdate();
	});
	settings[TOOLTIP_ACTIVATION_TIME].SetScrollFunc([](double dy) -> void {
		int steps = TooltipActivation();
		if(dy < 0.)
			steps = max(0, steps - 20);
		else
			steps = min(120, steps + 20);
		SetTooltipActivation(steps);
		CustomEvents::SendTooltipUpdate();
	});

	settings[TRIBUTE_CONFIRMATION] = Setting::List("Tribute confirmation", 1, {"off", "friendly only", "always"});
	settings[TRADE_SELL_OUTFITS_WITHOUT_SHOP] = Setting::Boolean("Sell outfits without outfitter", true);
	settings[TRADE_CONFIRM_MINABLES] = Setting::Boolean("Confirm selling minables", true);
	settings[TRADE_CONFIRM_OUTFITS] = Setting::Boolean("Confirm selling outfits", true);
	settings[TURRETS_FOCUS_FIRE] = Setting::List("Turret tracking", 1, {"opportunistic", "focused"}, true);
	settings[UNDERLINE_SHORTCUTS] = Setting::Boolean("Always underline shortcuts", false);

	settings[VSYNC] = Setting::List("VSync", 1, {"off", "on", "adaptive"});
	settings[VSYNC].SetOnToggleFunc([](UI *ui) -> void {
		if(!Preferences::ToggleVSync() && ui)
			ui->Push(DialogPanel::Info("Unable to change VSync state. (Your system's graphics settings may "
				"be controlling it instead.)"));
	});

	// "Window size" is unique and not rendered in PreferencesPanel, and so it doesn't need a Setting.

	settings[ZOOM_FACTOR_MAIN] = Setting::Unique("Main zoom factor");
	settings[ZOOM_FACTOR_MAIN].SetDisplayFunc([](int index) -> pair<string, bool> {
		return make_pair(to_string(Screen::UserZoom()), Screen::UserZoom() == Screen::Zoom());
	});
	settings[ZOOM_FACTOR_MAIN].SetOnToggleFunc([](UI *ui) -> void {
		int newZoom = Screen::UserZoom() + ZOOM_FACTOR_INCREMENT;
		Screen::SetZoom(newZoom);
		if(Screen::Zoom() != newZoom)
		{
			// Notify the user why setting the zoom any higher isn't permitted.
			// Only show this if it's not possible to zoom the view at all, as
			// otherwise the dialog will show every time, which is annoying.
			if(newZoom == ZOOM_FACTOR_MIN + ZOOM_FACTOR_INCREMENT && ui)
				ui->Push(DialogPanel::Info("Your screen resolution is too low to support a zoom level above 100%."));
			Screen::SetZoom(ZOOM_FACTOR_MIN);
		}

		int x = 0;
		int y = 0;
		SDL_GetMouseState(&x, &y);
		Point hoverPoint = Point(x, y);
		// Convert to raw window coordinates, at the new zoom level.
		hoverPoint *= Screen::Zoom() / 100.;
		hoverPoint += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
		SDL_WarpMouseInWindow(nullptr, hoverPoint.X(), hoverPoint.Y());
	});
	settings[ZOOM_FACTOR_MAIN].SetScrollFunc([](double dy) -> void {
		int zoom = Screen::UserZoom();
		if(dy < 0. && zoom > ZOOM_FACTOR_MIN)
			zoom -= ZOOM_FACTOR_INCREMENT;
		if(dy > 0.)
			zoom += ZOOM_FACTOR_INCREMENT;

		Screen::SetZoom(zoom);
		if(Screen::Zoom() != zoom)
			Screen::SetZoom(Screen::Zoom());

		int x = 0;
		int y = 0;
		SDL_GetMouseState(&x, &y);
		Point hoverPoint = Point(x, y);
		// Convert to raw window coordinates, at the new zoom level.
		hoverPoint *= (Screen::Zoom() / 100.);
		hoverPoint += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
		SDL_WarpMouseInWindow(nullptr, hoverPoint.X(), hoverPoint.Y());
	});

	settings[ZOOM_FACTOR_VIEW] = Setting::Unique("View zoom factor");
	settings[ZOOM_FACTOR_VIEW].SetDisplayFunc([](int index) -> pair<string, bool> {
		return make_pair(to_string(static_cast<int>(100. * ViewZoom())), true);
	});
	settings[ZOOM_FACTOR_VIEW].SetOnToggleFunc([](UI *ui) -> void {
		// Increase the zoom factor unless it is at the maximum. In that
		// case, cycle around to the lowest zoom factor.
		if(!ZoomViewIn())
			while(ZoomViewOut()) {}
	});
	settings[ZOOM_FACTOR_VIEW].SetScrollFunc([](double dy) -> void {
		if(dy < 0.)
			ZoomViewOut();
		else
			ZoomViewIn();
	});

#ifdef _WIN32
	settings[TITLE_BAR_THEME] = Setting::List("Title bar theme", 0, {"system default", "light", "dark"});
	settings[TITLE_BAR_THEME].SetDisplayFunc([](int index) -> pair<string, bool> {
		bool isOn = WinVersion::SupportsDarkTheme();
		string text = isOn ? Preferences::DisplayValue(TITLE_BAR_THEME) : "N/A";
		return make_pair(text, isOn);
	});
	settings[TITLE_BAR_THEME].SetOnToggleFunc([](UI *ui) -> void { GameWindow::UpdateTitleBarTheme(); });

	settings[WINDOW_ROUNDING] = Setting::List("Window rounding", 0, {"system default", "off", "large", "small"});
	settings[WINDOW_ROUNDING].SetDisplayFunc([](int index) -> pair<string, bool> {
		bool isOn = WinVersion::SupportsWindowRounding();
		string text = isOn ? Preferences::DisplayValue(WINDOW_ROUNDING) : "N/A";
		return make_pair(text, isOn);
	});
	settings[WINDOW_ROUNDING].SetOnToggleFunc([](UI *ui) -> void { GameWindow::UpdateWindowRounding(); });
#endif
}



void Preferences::Load()
{
	DataFile prefs(Files::Config() / "preferences.txt");
	for(const DataNode &node : prefs)
	{
		const string &key = node.Token(0);
		bool hasValue = node.Size() >= 2;
		if(key == WINDOW_SIZE && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2), true);
		else if(key == ZOOM_FACTOR_MAIN && hasValue)
			Screen::SetZoom(node.Value(1), true);
		else if(VOLUME_SETTINGS.contains(key) && hasValue)
			Audio::SetVolume(node.Value(1) * VOLUME_SCALE, VOLUME_SETTINGS.at(key));
		else if(key == SCROLL_SPEED && hasValue)
			scrollSpeed = node.Value(1);
		else if(key == TOOLTIP_ACTIVATION_TIME && hasValue)
			tooltipActivation = node.Value(1);
		else if(key == ZOOM_FACTOR_VIEW)
			zoomIndex = max(0., node.Value(1));
		else if(key == STATUS_OVERLAYS_ALL)
			statusOverlaySettings[OverlayType::ALL].SetState(node.Value(1));
		else if(key == STATUS_OVERLAYS_FLAGSHIP)
			statusOverlaySettings[OverlayType::FLAGSHIP].SetState(node.Value(1));
		else if(key == STATUS_OVERLAYS_ESCORT)
			statusOverlaySettings[OverlayType::ESCORT].SetState(node.Value(1));
		else if(key == STATUS_OVERLAYS_ENEMY)
			statusOverlaySettings[OverlayType::ENEMY].SetState(node.Value(1));
		else if(key == STATUS_OVERLAYS_NEUTRAL)
			statusOverlaySettings[OverlayType::NEUTRAL].SetState(node.Value(1));
		else if(key == PREVIOUS_SAVES && hasValue)
			previousSaveCount = max<int>(3, node.Value(1));
		else if(settings.contains(key))
			settings[key].SetIndex(!hasValue || node.Value(1));
		else if(key.starts_with("help: "))
			help[key] = (!hasValue || node.Value(1));
		else
			unrecognized[key] = (!hasValue || node.Value(1));
	}

	// For people updating from a version before the visual red alert indicator,
	// if they have already disabled the warning siren, don't turn the audible alert back on.
	auto it = unrecognized.find("Warning siren");
	if(it != unrecognized.end())
	{
		if(!it->second)
			settings[ALERT_INDICATOR].SetIndex(2);
		unrecognized.erase(it);
	}

	it = unrecognized.find("alt-mouse turning");
	if(it != unrecognized.end())
	{
		if(it->second)
			settings[MOUSE_CONTROL_FLAGSHIP].SetIndex(1);
		unrecognized.erase(it);
	}

	// For people updating from a version before the status overlay customization
	// changes, don't turn all the overlays on if they were off before.
	it = unrecognized.find("Show status overlays");
	if(it != unrecognized.end())
	{
		if(it->second)
			statusOverlaySettings[OverlayType::ALL] = OverlayState::DISABLED;
		unrecognized.erase(it);
	}

	// For people updating from a version after 0.10.1 (where "Flagship flotsam collection" was added),
	// but before 0.10.3 (when it was replaced with "Flotsam Collection").
	it = unrecognized.find("Flagship flotsam collection");
	if(it != unrecognized.end())
	{
		if(!it->second)
			settings[FLOTSAM_COLLECTION].SetIndex(static_cast<int>(FlotsamCollection::ESCORT));
		unrecognized.erase(it);
	}

	// For people updating from a version before 0.11.1,
	// where "Highlight player's ship" was replaced with "Highlight ships".
	it = unrecognized.find("Highlight player's flagship");
	if(it != unrecognized.end())
	{
		if(it->second)
			settings[SHIP_HIGHLIGHTS].SetIndex(static_cast<int>(HighlightShips::FLAGSHIP));
		unrecognized.erase(it);
	}

	// Some settings have been renamed. If the preferences file contains the old names,
	// load the state from them, then erase them so only the new names are written back when saving settings.
	const array<pair<string, string>, 4> RENAMED_BOOLEAN_SETTINGS = {{
		{"'Sell Outfits' without outfitter", TRADE_SELL_OUTFITS_WITHOUT_SHOP},
		{"Confirm 'Sell Outfits' button", TRADE_CONFIRM_OUTFITS},
		{"Confirm 'Sell Minables' button", TRADE_CONFIRM_MINABLES},
		{"Show parenthesis", MAP_PARENTHESIZE_PROFIT}
	}};
	for(auto const &[oldName, newName] : RENAMED_BOOLEAN_SETTINGS)
	{
		it = unrecognized.find(oldName);
		if(it != unrecognized.end())
		{
			unrecognized[newName] = it->second;
			unrecognized.erase(it);
		}
	}

	// For people updating from a version before 0.11.3,
	// where "Escorts expend ammo" and "Escorts use ammo frugally" were replaced with "Escort ammo usage".
	it = unrecognized.find("Escorts expend ammo");
	if(it != unrecognized.end())
	{
		auto sit = unrecognized.find("Escorts use ammo frugally");
		if(sit != unrecognized.end() && sit->second)
		{
			settings[ESCORT_AMMO_USAGE].SetIndex(static_cast<int>(EscortAmmoUsage::FRUGALLY));
			unrecognized.erase(sit);
		}
		else if(it->second)
			settings[ESCORT_AMMO_USAGE].SetIndex(static_cast<int>(EscortAmmoUsage::ALWAYS));
		unrecognized.erase(it);
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() / "preferences.txt");

	for(const auto &[name, category] : VOLUME_SETTINGS)
		out.Write(name, Audio::Volume(category) / VOLUME_SCALE);
	out.Write(WINDOW_SIZE, Screen::RawWidth(), Screen::RawHeight());
	out.Write(ZOOM_FACTOR_MAIN, Screen::UserZoom());
	out.Write(SCROLL_SPEED, scrollSpeed);
	out.Write(TOOLTIP_ACTIVATION_TIME, tooltipActivation);
	out.Write(ZOOM_FACTOR_VIEW, zoomIndex);
	out.Write(STATUS_OVERLAYS_ALL, statusOverlaySettings[OverlayType::ALL].ToInt());
	out.Write(STATUS_OVERLAYS_FLAGSHIP, statusOverlaySettings[OverlayType::FLAGSHIP].ToInt());
	out.Write(STATUS_OVERLAYS_ESCORT, statusOverlaySettings[OverlayType::ESCORT].ToInt());
	out.Write(STATUS_OVERLAYS_ENEMY, statusOverlaySettings[OverlayType::ENEMY].ToInt());
	out.Write(STATUS_OVERLAYS_NEUTRAL, statusOverlaySettings[OverlayType::NEUTRAL].ToInt());
	out.Write(PREVIOUS_SAVES, previousSaveCount);

	for(const auto &[identifier, setting] : settings)
		if(setting.Save())
			out.Write(identifier, setting.Index());
	for(const auto &[identifier, value] : help)
		out.Write(identifier, value);
	// Retain unrecognized settings in case they're here because the player jumped between versions.
	for(const auto &[identifier, value] : unrecognized)
		out.Write(identifier, value);
}



bool Preferences::Has(const string &name)
{
	auto it = settings.find(name);
	return it != settings.end() ? it->second.Index() : false;
}



void Preferences::Set(const string &name, bool on)
{
	auto it = settings.find(name);
	if(it != settings.end())
		it->second.SetIndex(on);
}



int Preferences::Toggle(const string &name, UI *ui)
{
	auto it = settings.find(name);
	if(it != settings.end())
		return it->second.Toggle(ui);
	return 0;
}



void Preferences::Scroll(const string &name, double dy)
{
	auto it = settings.find(name);
	if(it != settings.end())
		it->second.Scroll(dy);
}



const string &Preferences::DisplayName(const string &name)
{
	static const string UNKNOWN = "UNKNOWN SETTING";
	auto it = settings.find(name);
	return it != settings.end() ? it->second.DisplayName() : UNKNOWN;
}



pair<string, bool> Preferences::DisplayValue(const string &name)
{
	static const string UNKNOWN = "UNKNOWN SETTING";
	auto it = settings.find(name);
	return it != settings.end() ? it->second.DisplayValue() : make_pair(UNKNOWN, false);
}



bool Preferences::HelpShown(const string &name)
{
	auto it = help.find(name);
	return it != help.end() ? it->second : false;
}



void Preferences::SetHelp(const string &name, bool shown)
{
	help[name] = shown;
}



Preferences::EscortAmmoUsage Preferences::AmmoUsage()
{
	return static_cast<EscortAmmoUsage>(settings[ESCORT_AMMO_USAGE].Index());
}



Preferences::DateFormat Preferences::GetDateFormat()
{
	return static_cast<DateFormat>(settings[DATE_FORMAT].Index());
}



Preferences::NotificationSetting Preferences::GetNotificationSetting()
{
	return static_cast<NotificationSetting>(settings[DESTINATION_NOTIFICATION].Index());
}



int Preferences::ScrollSpeed()
{
	return scrollSpeed;
}



void Preferences::SetScrollSpeed(int speed)
{
	scrollSpeed = speed;
}



int Preferences::TooltipActivation()
{
	return tooltipActivation;
}



void Preferences::SetTooltipActivation(int steps)
{
	tooltipActivation = steps;
}



double Preferences::ViewZoom()
{
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	if(zoomIndex >= zooms.size())
		return zooms.empty() ? 1. : zooms.back();
	return zooms[zoomIndex];
}



bool Preferences::ZoomViewIn()
{
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	if(zooms.empty() || zoomIndex >= zooms.size() - 1)
		return false;

	++zoomIndex;
	return true;
}



bool Preferences::ZoomViewOut()
{
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	if(!zoomIndex || zooms.size() <= 1)
		return false;

	// Make sure that we're actually zooming out. This can happen if the zoom index
	// is out of range.
	if(zoomIndex >= zooms.size())
		zoomIndex = zooms.size() - 1;

	--zoomIndex;
	return true;
}



double Preferences::MinViewZoom()
{
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	return zooms.empty() ? 1. : zooms.front();
}



double Preferences::MaxViewZoom()
{
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	return zooms.empty() ? 1. : zooms.back();
}



const vector<double> &Preferences::Zooms()
{
	static vector<double> DEFAULT_ZOOMS{1.};
	const auto &zooms = GameData::Interfaces().Get("main view")->GetList("zooms");
	return zooms.empty() ? DEFAULT_ZOOMS : zooms;
}



Preferences::BackgroundParallax Preferences::GetBackgroundParallax()
{
	return static_cast<BackgroundParallax>(settings[PARALLAX].Index());
}



Preferences::ExtendedJumpEffects Preferences::GetExtendedJumpEffects()
{
	return static_cast<ExtendedJumpEffects>(settings[EXTENDED_JUMP_EFFECTS].Index());
}



bool Preferences::ToggleVSync()
{
	int original = settings[VSYNC].Index();
	int targetIndex = settings[VSYNC].Toggle(nullptr);
	if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
	{
		// Not all drivers support adaptive VSync. Increment desired VSync again.
		targetIndex = settings[VSYNC].Toggle(nullptr);
		if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
		{
			// Restore original saved setting.
			Logger::Log("Unable to change VSync state.", Logger::Level::WARNING);
			GameWindow::SetVSync(static_cast<VSync>(original));
			settings[VSYNC].SetIndex(original);
			return false;
		}
	}
	return true;
}



Preferences::VSync Preferences::VSyncState()
{
	return static_cast<VSync>(settings[VSYNC].Index());
}



Preferences::FastForwardCapsLockSync Preferences::GetFastForwardCapsLockSync()
{
	return static_cast<FastForwardCapsLockSync>(settings[FF_CAPSLOCK_SYNC].Index());
}



Preferences::CameraAccel Preferences::CameraAcceleration()
{
	return static_cast<CameraAccel>(settings[CAMERA_ACCELERATION].Index());
}



void Preferences::CycleStatusOverlays(Preferences::OverlayType type)
{
	// Calling OverlaySetting::Increment when the state is ON_HIT will cycle to off.
	// But, for the ALL overlay type, allow it to cycle to DISABLED.
	if(type == OverlayType::ALL && statusOverlaySettings[OverlayType::ALL] == OverlayState::ON_HIT)
		statusOverlaySettings[OverlayType::ALL] = OverlayState::DISABLED;
	// If one of the child types was clicked, but the all overlay state is the one currently being used,
	// set the all overlay state to DISABLED but do not increment any of the child settings.
	else if(type != OverlayType::ALL && statusOverlaySettings[OverlayType::ALL].IsActive())
		statusOverlaySettings[OverlayType::ALL] = OverlayState::DISABLED;
	else
		statusOverlaySettings[type].Increment();
}



Preferences::OverlayState Preferences::StatusOverlaysState(Preferences::OverlayType type)
{
	if(statusOverlaySettings[OverlayType::ALL].IsActive())
		return statusOverlaySettings[OverlayType::ALL];
	return statusOverlaySettings[type];
}



const string &Preferences::StatusOverlaysSetting(Preferences::OverlayType type)
{
	const auto &allOverlaysSetting = statusOverlaySettings[OverlayType::ALL];
	if(allOverlaysSetting.IsActive())
	{
		static const OverlaySetting DISABLED = OverlayState::DISABLED;
		if(type != OverlayType::ALL)
			return DISABLED.ToString();
	}
	return statusOverlaySettings[type].ToString();
}



Preferences::TurretOverlays Preferences::GetTurretOverlays()
{
	return static_cast<TurretOverlays>(settings[HUD_TURRET_OVERLAY].Index());
}



Preferences::HighlightShips Preferences::GetHighlightShips()
{
	return static_cast<HighlightShips>(settings[SHIP_HIGHLIGHTS].Index());
}



Preferences::AutoAim Preferences::GetAutoAim()
{
	return static_cast<AutoAim>(settings[AUTO_AIM].Index());
}



Preferences::AutoFire Preferences::GetAutoFire()
{
	return static_cast<AutoFire>(settings[AUTO_FIRE].Index());
}



Preferences::BoardingPriority Preferences::GetBoardingPriority()
{
	return static_cast<BoardingPriority>(settings[BOARDING_TARGET_PRIORITY].Index());
}



Preferences::FlotsamCollection Preferences::GetFlotsamCollection()
{
	return static_cast<FlotsamCollection>(settings[FLOTSAM_COLLECTION].Index());
}



Preferences::AlertIndicator Preferences::GetAlertIndicator()
{
	return static_cast<AlertIndicator>(settings[ALERT_INDICATOR].Index());
}



bool Preferences::PlayAudioAlert()
{
	return DoAlertHelper(AlertIndicator::AUDIO);
}



bool Preferences::DisplayVisualAlert()
{
	return DoAlertHelper(AlertIndicator::VISUAL);
}



bool Preferences::DoAlertHelper(Preferences::AlertIndicator toDo)
{
	auto value = GetAlertIndicator();
	return value == AlertIndicator::BOTH || value == toDo;
}



int Preferences::GetPreviousSaveCount()
{
	return previousSaveCount;
}



Preferences::MinimapDisplay Preferences::GetMinimapDisplay()
{
	return static_cast<MinimapDisplay>(settings[MINIMAP_DISPLAY].Index());
}



Preferences::FlagshipSpacePriority Preferences::GetFlagshipSpacePriority()
{
	return static_cast<FlagshipSpacePriority>(settings[FLAGSHIP_SPACE_PRIORITY].Index());
}



Preferences::LargeGraphicsReduction Preferences::GetLargeGraphicsReduction()
{
	return static_cast<LargeGraphicsReduction>(settings[REDUCE_LARGE_GRAPHICS].Index());
}



Preferences::TributeConfirmation Preferences::GetTributeConfirmation()
{
	return static_cast<TributeConfirmation>(settings[TRIBUTE_CONFIRMATION].Index());
}



Preferences::AmmoRefill Preferences::GetAmmoRefill()
{
	return static_cast<AmmoRefill>(settings[AMMO_REFILL].Index());
}



Alignment Preferences::GetTextAlignment()
{
	return static_cast<Alignment>(settings[TEXT_ALIGNMENT].Index());
}



Preferences::TargetAsteroidStrategy Preferences::GetTargetAsteroidStrategy()
{
	return static_cast<TargetAsteroidStrategy>(settings[ASTEROID_TARGETING].Index());
}



int Preferences::GetFontSize()
{
	return FONT_SIZES[settings[FONT_SIZE].Index()];
}



#ifdef _WIN32
Preferences::TitleBarTheme Preferences::GetTitleBarTheme()
{
	return static_cast<TitleBarTheme>(settings[TITLE_BAR_THEME].Index());
}



Preferences::WindowRounding Preferences::GetWindowRounding()
{
	return static_cast<WindowRounding>(settings[WINDOW_ROUNDING].Index());
}
#endif
