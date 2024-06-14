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

#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <cstdint>
#include <string>
#include <vector>



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
		DMY = 0, // Day-first format. (Sat, 4 Oct 1941)
		MDY,     // Month-first format. (Sat, Oct 4, 1941)
		YMD      // All-numeric ISO 8601. (1941-10-04)
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


public:
	static void Load();
	static void Save();

	static bool Has(const std::string &name);
	static void Set(const std::string &name, bool on = true);

	// Toggle the ammo usage preferences, cycling between "never," "frugally,"
	// and "always."
	static void ToggleAmmoUsage();
	static std::string AmmoUsage();

	// Date format preferences.
	static void ToggleDateFormat();
	static DateFormat GetDateFormat();
	static const std::string &DateFormatSetting();

	// Scroll speed preference.
	static int ScrollSpeed();
	static void SetScrollSpeed(int speed);

	// View zoom.
	static double ViewZoom();
	static bool ZoomViewIn();
	static bool ZoomViewOut();
	static double MinViewZoom();
	static double MaxViewZoom();
	static const std::vector<double> &Zooms();

	static void ToggleScreenMode();
	static const std::string &ScreenModeSetting();

	// VSync setting, either "on", "off", or "adaptive".
	static bool ToggleVSync();
	static VSync VSyncState();
	static const std::string &VSyncSetting();

	static void ToggleCameraAcceleration();
	static CameraAccel CameraAcceleration();
	static const std::string &CameraAccelerationSetting();

	static void CycleStatusOverlays(OverlayType type);
	static OverlayState StatusOverlaysState(OverlayType type);
	static const std::string &StatusOverlaysSetting(OverlayType type);

	// Auto aim setting, either "off", "always on", or "when firing".
	static void ToggleAutoAim();
	static AutoAim GetAutoAim();
	static const std::string &AutoAimSetting();

	// Auto fire setting, either "off", "on", "guns only", or "turrets only".
	static void ToggleAutoFire();
	static AutoFire GetAutoFire();
	static const std::string &AutoFireSetting();

	// System parallax setting, either "on", or "off".
	static void ToggleSystemParallax();
	static bool GetSystemParallax();
	static const std::string &SystemParallaxSetting();

	// Background parallax setting, either "fast", "fancy", or "off".
	static void ToggleBackgroundParallax();
	static BackgroundParallax GetBackgroundParallax();
	static const std::string &BackgroundParallaxSetting();

	// Extended jump effects setting, either "off", "medium", or "heavy".
	static void ToggleExtendedJumpEffects();
	static ExtendedJumpEffects GetExtendedJumpEffects();
	static const std::string &ExtendedJumpEffectsSetting();

	// Boarding target setting, either "proximity", "value" or "mixed".
	static void ToggleBoarding();
	static BoardingPriority GetBoardingPriority();
	static const std::string &BoardingSetting();

	// Flotsam setting, either "off", "on", "flagship only", or "escorts only".
	static void ToggleFlotsam();
	static FlotsamCollection GetFlotsamCollection();
	static const std::string &FlotsamSetting();

	// Red alert siren and symbol
	static void ToggleAlert();
	static AlertIndicator GetAlertIndicator();
	static const std::string &AlertSetting();
	static bool PlayAudioAlert();
	static bool DisplayVisualAlert();
	static bool DoAlertHelper(AlertIndicator toDo);

	static int GetPreviousSaveCount();
};



#endif
