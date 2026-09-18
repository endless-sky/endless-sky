/* Preferences.h
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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class Alignment;



class Preferences {
public:
	enum class VSync : int_fast8_t {
		off = 0,
		on,
		adaptive,
	};

	enum class CameraAccel : int_fast8_t {
		OFF = 0,
		ON,
		REVERSED,
	};

	enum class DateFormat : int_fast8_t {
		DMY = 0, ///< Day-first format. (Sat, 4 Oct 1941)
		MDY,     ///< Month-first format. (Sat, Oct 4, 1941)
		YMD      ///< All-numeric ISO 8601. (1941-10-04)
	};

	enum class NotificationSetting : int_fast8_t {
		OFF = 0,
		MESSAGE,
		BOTH
	};

	enum class OverlayState : int_fast8_t {
		OFF = 0,
		ON,
		DAMAGED,
		DISABLED,
		ON_HIT,
	};

	enum class OverlayType : int_fast8_t {
		ALL = 0,
		FLAGSHIP,
		ESCORT,
		ENEMY,
		NEUTRAL
	};

	enum class TurretOverlays : int_fast8_t {
		OFF = 0,
		ALWAYS_ON,
		BLINDSPOTS_ONLY
	};

	enum class AutoAim : int_fast8_t {
		OFF = 0,
		ALWAYS_ON,
		WHEN_FIRING
	};

	enum class AutoFire : int_fast8_t {
		OFF = 0,
		ON,
		GUNS_ONLY,
		TURRETS_ONLY
	};

	enum class BoardingPriority : int_fast8_t {
		PROXIMITY = 0,
		VALUE,
		MIXED
	};

	enum class FlotsamCollection : int_fast8_t {
		OFF = 0,
		ON,
		FLAGSHIP,
		ESCORT
	};

	enum class BackgroundParallax : int {
		OFF = 0,
		FANCY,
		FAST
	};

	enum class ExtendedJumpEffects : int {
		OFF = 0,
		MEDIUM,
		HEAVY
	};

	enum class AlertIndicator : int_fast8_t {
		NONE = 0,
		AUDIO,
		VISUAL,
		BOTH
	};

	enum class MinimapDisplay : int_fast8_t {
		OFF = 0,
		WHEN_JUMPING,
		ALWAYS_ON
	};

	enum class FlagshipSpacePriority : int_fast8_t {
		NONE = 0,
		PASSENGERS,
		CARGO,
		BOTH
	};

	enum class LargeGraphicsReduction : int_fast8_t {
		OFF,
		LARGEST_ONLY,
		ALL
	};

	enum class HighlightShips : int_fast8_t {
		OFF,
		FLAGSHIP,
		OWNED_SHIPS,
		ALL
	};

	enum class TributeConfirmation : int_fast8_t {
		OFF = 0,
		FRIENDLY_ONLY,
		ALWAYS
	};

	enum class AmmoRefill : int_fast8_t {
		NEVER = 0,
		ASK,
		WHEN_FREE,
		ALWAYS
	};

	enum class FastForwardCapsLockSync : int_fast8_t {
		DEFAULT = 0,
		NEVER,
		ALWAYS
	};

	enum class TargetAsteroidStrategy : int_fast8_t {
		PROXIMITY = 0,
		EXPECTED_VALUE,
		QUALITY
	};

	enum class EscortAmmoUsage : int_fast8_t {
		NEVER = 0,
		FRUGALLY,
		ALWAYS
	};

#ifdef _WIN32
	enum class TitleBarTheme : int_fast8_t {
		DEFAULT,
		LIGHT,
		DARK
	};

	enum class WindowRounding : int_fast8_t {
		DEFAULT,
		OFF,
		LARGE,
		SMALL
	};
#endif


public:
	static void Init();
	static void Load();
	static void Save();

	static bool Has(const std::string &name);
	static void Set(const std::string &name, bool on = true);
	static int Toggle(const std::string &name);

	static const std::string &DisplayName(const std::string &name);
	static std::string DisplayValue(const std::string &name);
	static bool IsOn(const std::string &name);

	static bool HelpShown(const std::string &name);
	static void SetHelp(const std::string &name, bool shown = true);

	static EscortAmmoUsage AmmoUsage();

	/// Date format preferences.
	static DateFormat GetDateFormat();

	// Notification preferences.
	static NotificationSetting GetNotificationSetting();

	// Scroll speed preference.
	static int ScrollSpeed();
	static void SetScrollSpeed(int speed);

	static int TooltipActivation();
	static void SetTooltipActivation(int steps);

	static double ViewZoom();
	static bool ZoomViewIn();
	static bool ZoomViewOut();
	static double MinViewZoom();
	static double MaxViewZoom();
	static const std::vector<double> &Zooms();

	static void ToggleScreenMode();

	/// VSync setting, either "on", "off", or "adaptive".
	static bool ToggleVSync();
	static VSync VSyncState();

	static CameraAccel CameraAcceleration();

	static void CycleStatusOverlays(OverlayType type);
	static OverlayState StatusOverlaysState(OverlayType type);
	static const std::string &StatusOverlaysSetting(OverlayType type);

	/// Turret overlays setting, either "off", "always on", or "blindspots only".
	static TurretOverlays GetTurretOverlays();

	/// Highlight ships setting, either "off", "flagship", "owned ships", or "all".
	static HighlightShips GetHighlightShips();

	/// Auto aim setting, either "off", "always on", or "when firing".
	static AutoAim GetAutoAim();

	/// Auto fire setting, either "off", "on", "guns only", or "turrets only".
	static AutoFire GetAutoFire();

	/// Background parallax setting, either "fast", "fancy", or "off".
	static BackgroundParallax GetBackgroundParallax();

	/// Extended jump effects setting, either "off", "medium", or "heavy".
	static ExtendedJumpEffects GetExtendedJumpEffects();

	/// Boarding target setting, either "proximity", "value" or "mixed".
	static BoardingPriority GetBoardingPriority();

	/// Flotsam setting, either "off", "on", "flagship only", or "escorts only".
	static FlotsamCollection GetFlotsamCollection();

	/// Red alert siren and symbol.
	static AlertIndicator GetAlertIndicator();
	static bool PlayAudioAlert();
	static bool DisplayVisualAlert();
	static bool DoAlertHelper(AlertIndicator toDo);

	/// Minimap display settings.
	static MinimapDisplay GetMinimapDisplay();

	/// Flagship space priority setting.
	static FlagshipSpacePriority GetFlagshipSpacePriority();

	/// Large graphics reduction setting.
	static LargeGraphicsReduction GetLargeGraphicsReduction();

	/// Tribute confirmation dialog setting.
	static TributeConfirmation GetTributeConfirmation();

	/// Outfitter ammo refill confirmation setting.
	static AmmoRefill GetAmmoRefill();

	/// Fast-forward CapsLock sync setting
	static FastForwardCapsLockSync GetFastForwardCapsLockSync();

	/// Text alignment setting.
	static Alignment GetTextAlignment();

	/// Target asteroid strategy setting.
	static TargetAsteroidStrategy GetTargetAsteroidStrategy();

	/// Font size setting.
	static int GetFontSize();

	static void ToggleBlockScreenSaver();

	static int GetPreviousSaveCount();

#ifdef _WIN32
	static void ToggleTitleBarTheme();
	static TitleBarTheme GetTitleBarTheme();

	static void ToggleWindowRounding();
	static WindowRounding GetWindowRounding();
#endif


public:
	// Identifiers for specific preferences.
	static const std::string ALERT_INDICATOR;
	static const std::string AMMO_REFILL;
	static const std::string ANIMATE_MENU_BACKGROUND;
	static const std::string ASTEROID_TARGETING;
	static const std::string AUTO_AIM;
	static const std::string AUTO_FIRE;
	static const std::string AUTO_UNPARK_FLAGSHIP;
	static const std::string BLOCK_SCREEN_SAVER;
	static const std::string BOARDING_TARGET_PRIORITY;
	static const std::string CAMERA_ACCELERATION;
	static const std::string DAMAGED_FIGHTERS_RETREAT;
	static const std::string DATE_FORMAT;
	static const std::string DEFER_LOADING_IMAGES;
	static const std::string DESTINATION_NOTIFICATION;
	static const std::string DRAW_BACKGROUND_HAZE;
	static const std::string DRAW_STARFIELD;
	static const std::string ESCORT_AMMO_USAGE;
	static const std::string EXTENDED_JUMP_EFFECTS;
	static const std::string FF_CAPSLOCK_SYNC;
	static const std::string FIGHTERS_REPAIR_IN;
	static const std::string FIGHTERS_TRANSFER_CARGO;
	static const std::string FIXED_STARFIELD_ZOOM;
	static const std::string FLAGSHIP_SPACE_PRIORITY;
	static const std::string FLOTSAM_COLLECTION;
	static const std::string FONT_SIZE;
	static const std::string HUD_ASTEROID_OVERLAY;
	static const std::string HUD_CLICKABLE_RADAR;
	static const std::string HUD_DISABLE_RADAR_VIEWPORT;
	static const std::string HUD_EXTRA_STATUS_MSGS;
	static const std::string HUD_MISSILE_OVERLAY;
	static const std::string HUD_ROTATE_FLAGSHIP;
	static const std::string HUD_TURRET_OVERLAY;
	static const std::string HUD_PLANET_LABELS;
	static const std::string INTERRUPT_FAST_FORWARD;
	static const std::string LANDING_ZOOM;
	static const std::string MAP_DEADLINE_BLINK_BY_DISTANCE;
	static const std::string MAP_HIDE_UNEXPLORED;
	static const std::string MAP_SHOW_ESCORTS;
	static const std::string MAP_SHOW_OUTFITS;
	static const std::string MAP_PARENTHESIZE_PROFIT;
	static const std::string MINIMAP_DISPLAY;
	static const std::string MOTION_BLUR;
	static const std::string MOUSE_CONTROL_FLAGSHIP;
	static const std::string MOUSE_CONTROL_TURRETS;
	static const std::string PARALLAX;
	static const std::string PREVIOUS_SAVES;
	static const std::string REACTIVATE_HELP;
	static const std::string REDUCE_LARGE_GRAPHICS;
	static const std::string REHIRE_LOST_CREW;
	static const std::string SAVE_MSG_LOGS;
	static const std::string SCREEN_MODE;
	static const std::string SCREEN_MAXIMIZED;
	static const std::string SCROLL_SPEED;
	static const std::string SHIP_HIGHLIGHTS;
	static const std::string SHIP_OUTLINES_CLOAKED;
	static const std::string SHIP_OUTLINES_HUD;
	static const std::string SHIP_OUTLINES_SHOP;
	static const std::string SHOW_HYPERSPACE_FLASH;
	static const std::string SHOW_PERFORMANCE_METRICS;
	static const std::string STATUS_OVERLAYS_ALL;
	static const std::string STATUS_OVERLAYS_FLAGSHIP;
	static const std::string STATUS_OVERLAYS_ESCORT;
	static const std::string STATUS_OVERLAYS_ENEMY;
	static const std::string STATUS_OVERLAYS_NEUTRAL;
	static const std::string TEXT_ALIGNMENT;
	static const std::string TEXTURE_FILTERING;
	static const std::string TOOLTIP_ACTIVATION_TIME;
	static const std::string TRADE_SELL_OUTFITS_WITHOUT_SHOP;
	static const std::string TRADE_CONFIRM_MINABLES;
	static const std::string TRADE_CONFIRM_OUTFITS;
	static const std::string TRIBUTE_CONFIRMATION;
	static const std::string TURRETS_FOCUS_FIRE;
	static const std::string UNDERLINE_SHORTCUTS;
	static const std::string VSYNC;
	static const std::string WINDOW_SIZE;
	static const std::string ZOOM_FACTOR_MAIN;
	static const std::string ZOOM_FACTOR_VIEW;
#ifdef _WIN32
	static const std::string TITLE_BAR_THEME;
	static const std::string WINDOW_ROUNDING;
#endif
};
