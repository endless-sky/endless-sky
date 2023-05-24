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
#include <functional>
#include <string>
#include <vector>



class Preferences {
public:
	enum class ScreenMode : int_fast8_t {
		WINDOWED = 0,
		FULLSCREEN
	};

	enum class VSync : int_fast8_t {
		off = 0,
		on,
		adaptive,
	};

	enum class OverlayState : int_fast8_t {
		OFF = 0,
		ON,
		DAMAGED,
		DISABLED
	};

	enum class OverlayType : int_fast8_t {
		ALL = 0,
		FLAGSHIP,
		ESCORT,
		ENEMY,
		NEUTRAL
	};

	enum class AmmoUsage : int_fast8_t {
		NEVER = 0,
		FRUGAL,
		ALWAYS
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

	enum class BackgroundParallax : int {
		OFF = 0,
		FANCY,
		FAST
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

	// Scroll speed preference.
	static int ScrollSpeed();
	static void SetScrollSpeed(int speed);

	// View zoom.
	static double ViewZoom();
	static bool ZoomViewIn();
	static bool ZoomViewOut();
	static double MinViewZoom();
	static double MaxViewZoom();

	static void SetScreenMode(ScreenMode setting);
	static void ToggleScreenMode();
	static ScreenMode GetScreenMode();
	static const std::string &ScreenModeSetting();

	// VSync setting, either "on", "off", or "adaptive".
	static bool SetVSync(VSync setting);
	static bool ToggleVSync();
	static VSync VSyncState();
	static const std::string &VSyncSetting();

	// Red alert siren and symbol
	static void SetAlert(AlertIndicator setting);
	static AlertIndicator GetAlertIndicator();
	static const std::string &AlertSetting();
	static bool PlayAudioAlert();
	static bool DisplayVisualAlert();
	static bool DoAlertHelper(AlertIndicator toDo);

	static void SetAmmoUsage(AmmoUsage setting);
	static void ToggleAmmoUsage();
	static AmmoUsage GetAmmoUsage();
	static std::string AmmoUsageSetting();

	// Auto aim setting, either "off", "always on", or "when firing".
	static void SetAutoAim(AutoAim setting);
	static AutoAim GetAutoAim();
	static const std::string &AutoAimSetting();

	// Auto fire setting, either "off", "on", "guns only", or "turrets only".
	static void SetAutoFire(AutoFire setting);
	static AutoFire GetAutoFire();
	static const std::string &AutoFireSetting();

	// Boarding target setting, either "proximity", "value" or "mixed".
	static void SetBoarding(BoardingPriority setting);
	static BoardingPriority GetBoardingPriority();
	static const std::string &BoardingSetting();

	static OverlayState StatusOverlaysState(OverlayType type);
	static void SetOverlaysState(OverlayType type, OverlayState state);
	static const std::string &StatusOverlaysSetting(OverlayType type);

	// Background parallax setting, either "fast", "fancy", or "off".
	static void SetParallax(BackgroundParallax setting);
	static BackgroundParallax GetBackgroundParallax();
	static const std::string &ParallaxSetting();

	static int GetPreviousSaveCount();
};



namespace PreferenceSettings {
#define ___v(str, func) std::make_pair(std::string(str), [](){func;})
	typedef std::vector<std::pair<std::string, std::function<void()>>> StringCallbackVec;

	const StringCallbackVec SCREEN_MODE_SETTINGS = {
		___v("windowed", Preferences::SetScreenMode(Preferences::ScreenMode::WINDOWED)),
		___v("fullscreen", Preferences::SetScreenMode(Preferences::ScreenMode::FULLSCREEN))
	};

	const StringCallbackVec VSYNC_SETTINGS = {
		___v("off", Preferences::SetVSync(Preferences::VSync::off)),
		___v("on", Preferences::SetVSync(Preferences::VSync::on)),
		___v("adaptive", Preferences::SetVSync(Preferences::VSync::adaptive)),
	};

	const StringCallbackVec PARALLAX_SETTINGS = {
		___v("off", Preferences::SetParallax(Preferences::BackgroundParallax::OFF)),
		___v("fancy", Preferences::SetParallax(Preferences::BackgroundParallax::FANCY)),
		___v("fast", Preferences::SetParallax(Preferences::BackgroundParallax::FAST))
	};

	const StringCallbackVec AUTO_AIM_SETTINGS = {
		___v("off", Preferences::SetAutoAim(Preferences::AutoAim::OFF)),
		___v("always on", Preferences::SetAutoAim(Preferences::AutoAim::ALWAYS_ON)),
		___v("when firing", Preferences::SetAutoAim(Preferences::AutoAim::WHEN_FIRING))
	};

	const StringCallbackVec ALERT_INDICATOR_SETTING = {
		___v("off", Preferences::SetAlert(Preferences::AlertIndicator::NONE)),
		___v("audio", Preferences::SetAlert(Preferences::AlertIndicator::AUDIO)),
		___v("visual", Preferences::SetAlert(Preferences::AlertIndicator::VISUAL)),
		___v("both", Preferences::SetAlert(Preferences::AlertIndicator::BOTH))
	};

	const StringCallbackVec BOARDING_SETTINGS = {
		___v("proximity", Preferences::SetBoarding(Preferences::BoardingPriority::PROXIMITY)),
		___v("value", Preferences::SetBoarding(Preferences::BoardingPriority::VALUE)),
		___v("mixed", Preferences::SetBoarding(Preferences::BoardingPriority::MIXED)),
	};

	const StringCallbackVec AUTO_FIRE_SETTINGS = {
		___v("off", Preferences::SetAutoFire(Preferences::AutoFire::OFF)),
		___v("on", Preferences::SetAutoFire(Preferences::AutoFire::ON)),
		___v("guns only", Preferences::SetAutoFire(Preferences::AutoFire::GUNS_ONLY)),
		___v("turrets only", Preferences::SetAutoFire(Preferences::AutoFire::TURRETS_ONLY))
	};

	const StringCallbackVec AMMO_USAGE_SETTINGS = {
		___v("never", Preferences::SetAmmoUsage(Preferences::AmmoUsage::NEVER)),
		___v("frugally", Preferences::SetAmmoUsage(Preferences::AmmoUsage::FRUGAL)),
		___v("always", Preferences::SetAmmoUsage(Preferences::AmmoUsage::ALWAYS))
	};

}



#endif
