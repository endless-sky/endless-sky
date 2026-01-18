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

#include "audio/Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameData.h"
#include "GameWindow.h"
#include "Interface.h"
#include "Logger.h"
#include "Screen.h"

#ifdef _WIN32
#include "windows/WinVersion.h"
#endif

#include <algorithm>
#include <cstddef>
#include <map>

using namespace std;

namespace {
	map<string, bool> settings;
	int scrollSpeed = 60;
	int tooltipActivation = 60;

	// Strings for ammo expenditure:
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";

	const vector<string> DATEFMT_OPTIONS = {"dd/mm/yyyy", "mm/dd/yyyy", "yyyy-mm-dd"};
	int dateFormatIndex = 0;

	const vector<string> NOTIF_OPTIONS = {"off", "message", "both"};
	int notifOptionsIndex = 1;

	size_t zoomIndex = 4;
	constexpr double VOLUME_SCALE = .25;

	// Default to fullscreen.
	int screenModeIndex = 1;
	const vector<string> SCREEN_MODE_SETTINGS = {"windowed", "fullscreen"};

	// Enable standard VSync by default.
	const vector<string> VSYNC_SETTINGS = {"off", "on", "adaptive"};
	int vsyncIndex = 1;

	const vector<string> CAMERA_ACCELERATION_SETTINGS = {"off", "on", "reversed"};
	int cameraAccelerationIndex = 1;

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

		const bool IsActive() const { return state != Preferences::OverlayState::DISABLED; }

		const string &ToString() const
		{
			return OVERLAY_SETTINGS[max<int>(0, min<int>(OVERLAY_SETTINGS.size() - 1, static_cast<int>(state)))];
		}

		const int ToInt() const { return static_cast<int>(state); }

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

	const vector<string> TURRET_OVERLAYS_SETTINGS = {"off", "always on", "blindspots only"};
	int turretOverlaysIndex = 2;

	const vector<string> AUTO_AIM_SETTINGS = {"off", "always on", "when firing"};
	int autoAimIndex = 2;

	const vector<string> AUTO_FIRE_SETTINGS = {"off", "on", "guns only", "turrets only"};
	int autoFireIndex = 0;

	const vector<string> BOARDING_SETTINGS = {"proximity", "value", "mixed"};
	int boardingIndex = 0;

	const vector<string> FLOTSAM_SETTINGS = {"off", "on", "flagship only", "escorts only"};
	int flotsamIndex = 1;

	// Enable "fast" parallax by default. "fancy" is too GPU heavy, especially for low-end hardware.
	const vector<string> PARALLAX_SETTINGS = {"off", "fancy", "fast"};
	int parallaxIndex = 2;

	const vector<string> EXTENDED_JUMP_EFFECT_SETTINGS = {"off", "medium", "heavy"};
	int extendedJumpEffectIndex = 0;

	const vector<string> ALERT_INDICATOR_SETTING = {"off", "audio", "visual", "both"};
	int alertIndicatorIndex = 3;

	const vector<string> MINIMAP_DISPLAY_SETTING = {"off", "when jumping", "always on"};
	int minimapDisplayIndex = 1;

	const vector<string> FLAGSHIP_SPACE_PRIORITY_SETTINGS = {"none", "passengers", "cargo", "both"};
	int flagshipSpacePriorityIndex = 1;

	const vector<string> LARGE_GRAPHICS_REDUCTION_SETTINGS = {"off", "largest only", "all"};
	int largeGraphicsReductionIndex = 0;

	const string BLOCK_SCREEN_SAVER = "Block screen saver";

	int previousSaveCount = 3;

#ifdef _WIN32
	const vector<string> TITLE_BAR_THEME_SETTINGS = {"system default", "light", "dark"};
	int titleBarThemeIndex = 0;

	const vector<string> WINDOW_ROUNDING_SETTINGS = {"system default", "off", "large", "small"};
	int windowRoundingIndex = 0;
#endif
}



void Preferences::Load()
{
	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	settings["Landing zoom"] = true;
	settings["Render motion blur"] = true;
	settings["Cloaked ship outlines"] = true;
	settings[FRUGAL_ESCORTS] = true;
	settings[EXPEND_AMMO] = true;
	settings["Damaged fighters retreat"] = true;
	settings["Show escort systems on map"] = true;
	settings["Show stored outfits on map"] = true;
	settings["Show planet labels"] = true;
	settings["Show asteroid scanner overlay"] = true;
	settings["Show hyperspace flash"] = true;
	settings["Draw background haze"] = true;
	settings["Draw starfield"] = true;
	settings["Animate main menu background"] = true;
	settings["Hide unexplored map regions"] = true;
	settings["Turrets focus fire"] = true;
	settings["Ship outlines in shops"] = true;
	settings["Ship outlines in HUD"] = true;
	settings["Extra fleet status messages"] = true;
	settings["Target asteroid based on"] = true;
	settings["Deadline blink by distance"] = true;

	DataFile prefs(Files::Config() / "preferences.txt");
	for(const DataNode &node : prefs)
	{
		const string &key = node.Token(0);
		bool hasValue = node.Size() >= 2;
		if(key == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2), true);
		else if(key == "zoom" && hasValue)
			Screen::SetZoom(node.Value(1), true);
		else if(VOLUME_SETTINGS.contains(key) && hasValue)
			Audio::SetVolume(node.Value(1) * VOLUME_SCALE, VOLUME_SETTINGS.at(key));
		else if(key == "scroll speed" && hasValue)
			scrollSpeed = node.Value(1);
		else if(key == "Tooltip activation time" && hasValue)
			tooltipActivation = node.Value(1);
		else if(key == "boarding target")
			boardingIndex = max<int>(0, min<int>(node.Value(1), BOARDING_SETTINGS.size() - 1));
		else if(key == "Flotsam collection")
			flotsamIndex = max<int>(0, min<int>(node.Value(1), FLOTSAM_SETTINGS.size() - 1));
		else if(key == "view zoom")
			zoomIndex = max(0., node.Value(1));
		else if(key == "vsync")
			vsyncIndex = max<int>(0, min<int>(node.Value(1), VSYNC_SETTINGS.size() - 1));
		else if(key == "camera acceleration")
			cameraAccelerationIndex = max<int>(0, min<int>(node.Value(1), CAMERA_ACCELERATION_SETTINGS.size() - 1));
		else if(key == "Show all status overlays")
			statusOverlaySettings[OverlayType::ALL].SetState(node.Value(1));
		else if(key == "Show flagship overlay")
			statusOverlaySettings[OverlayType::FLAGSHIP].SetState(node.Value(1));
		else if(key == "Show escort overlays")
			statusOverlaySettings[OverlayType::ESCORT].SetState(node.Value(1));
		else if(key == "Show enemy overlays")
			statusOverlaySettings[OverlayType::ENEMY].SetState(node.Value(1));
		else if(key == "Show neutral overlays")
			statusOverlaySettings[OverlayType::NEUTRAL].SetState(node.Value(1));
		else if(key == "Turret overlays")
			turretOverlaysIndex = clamp<int>(node.Value(1), 0, TURRET_OVERLAYS_SETTINGS.size() - 1);
		else if(key == "Automatic aiming")
			autoAimIndex = max<int>(0, min<int>(node.Value(1), AUTO_AIM_SETTINGS.size() - 1));
		else if(key == "Automatic firing")
			autoFireIndex = max<int>(0, min<int>(node.Value(1), AUTO_FIRE_SETTINGS.size() - 1));
		else if(key == "Parallax background")
			parallaxIndex = max<int>(0, min<int>(node.Value(1), PARALLAX_SETTINGS.size() - 1));
		else if(key == "Extended jump effects")
			extendedJumpEffectIndex = max<int>(0, min<int>(node.Value(1), EXTENDED_JUMP_EFFECT_SETTINGS.size() - 1));
		else if(key == "fullscreen")
			screenModeIndex = max<int>(0, min<int>(node.Value(1), SCREEN_MODE_SETTINGS.size() - 1));
		else if(key == "date format")
			dateFormatIndex = max<int>(0, min<int>(node.Value(1), DATEFMT_OPTIONS.size() - 1));
		else if(key == "alert indicator")
			alertIndicatorIndex = max<int>(0, min<int>(node.Value(1), ALERT_INDICATOR_SETTING.size() - 1));
		else if(key == "Show mini-map")
			minimapDisplayIndex = max<int>(0, min<int>(node.Value(1), MINIMAP_DISPLAY_SETTING.size() - 1));
		else if(key == "Prioritize flagship use")
			flagshipSpacePriorityIndex = clamp<int>(node.Value(1), 0, FLAGSHIP_SPACE_PRIORITY_SETTINGS.size() - 1);
		else if(key == "Reduce large graphics")
			largeGraphicsReductionIndex = clamp<int>(node.Value(1), 0, LARGE_GRAPHICS_REDUCTION_SETTINGS.size() - 1);
		else if(key == "previous saves" && hasValue)
			previousSaveCount = max<int>(3, node.Value(1));
		else if(key == "alt-mouse turning")
			settings["Control ship with mouse"] = (!hasValue || node.Value(1));
		else if(key == "notification settings")
			notifOptionsIndex = max<int>(0, min<int>(node.Value(1), NOTIF_OPTIONS.size() - 1));
#ifdef _WIN32
		else if(key == "Title bar theme")
			titleBarThemeIndex = clamp<int>(node.Value(1), 0, TITLE_BAR_THEME_SETTINGS.size() - 1);
		else if(key == "Window rounding")
			windowRoundingIndex = clamp<int>(node.Value(1), 0, WINDOW_ROUNDING_SETTINGS.size() - 1);
#endif
		else
			settings[key] = (node.Size() == 1 || node.Value(1));
	}

	// For people updating from a version before the visual red alert indicator,
	// if they have already disabled the warning siren, don't turn the audible alert back on.
	auto it = settings.find("Warning siren");
	if(it != settings.end())
	{
		if(!it->second)
			alertIndicatorIndex = 2;
		settings.erase(it);
	}

	// For people updating from a version before the status overlay customization
	// changes, don't turn all the overlays on if they were off before.
	it = settings.find("Show status overlays");
	if(it != settings.end())
	{
		if(it->second)
			statusOverlaySettings[OverlayType::ALL] = OverlayState::DISABLED;
		settings.erase(it);
	}

	// For people updating from a version after 0.10.1 (where "Flagship flotsam collection" was added),
	// but before 0.10.3 (when it was replaced with "Flotsam Collection").
	it = settings.find("Flagship flotsam collection");
	if(it != settings.end())
	{
		if(!it->second)
			flotsamIndex = static_cast<int>(FlotsamCollection::ESCORT);
		settings.erase(it);
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() / "preferences.txt");

	for(const auto &[name, category] : VOLUME_SETTINGS)
		out.Write(name, Audio::Volume(category) / VOLUME_SCALE);
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::UserZoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("Tooltip activation time", tooltipActivation);
	out.Write("boarding target", boardingIndex);
	out.Write("Flotsam collection", flotsamIndex);
	out.Write("view zoom", zoomIndex);
	out.Write("vsync", vsyncIndex);
	out.Write("camera acceleration", cameraAccelerationIndex);
	out.Write("date format", dateFormatIndex);
	out.Write("notification settings", notifOptionsIndex);
	out.Write("Show all status overlays", statusOverlaySettings[OverlayType::ALL].ToInt());
	out.Write("Show flagship overlay", statusOverlaySettings[OverlayType::FLAGSHIP].ToInt());
	out.Write("Show escort overlays", statusOverlaySettings[OverlayType::ESCORT].ToInt());
	out.Write("Show enemy overlays", statusOverlaySettings[OverlayType::ENEMY].ToInt());
	out.Write("Show neutral overlays", statusOverlaySettings[OverlayType::NEUTRAL].ToInt());
	out.Write("Turret overlays", turretOverlaysIndex);
	out.Write("Automatic aiming", autoAimIndex);
	out.Write("Automatic firing", autoFireIndex);
	out.Write("Parallax background", parallaxIndex);
	out.Write("Extended jump effects", extendedJumpEffectIndex);
	out.Write("alert indicator", alertIndicatorIndex);
	out.Write("Show mini-map", minimapDisplayIndex);
	out.Write("Prioritize flagship use", flagshipSpacePriorityIndex);
	out.Write("Reduce large graphics", largeGraphicsReductionIndex);
	out.Write("previous saves", previousSaveCount);
#ifdef _WIN32
	if(WinVersion::SupportsDarkTheme())
		out.Write("Title bar theme", titleBarThemeIndex);
	if(WinVersion::SupportsWindowRounding())
		out.Write("Window rounding", windowRoundingIndex);
#endif

	for(const auto &it : settings)
		out.Write(it.first, it.second);
}



bool Preferences::Has(const string &name)
{
	auto it = settings.find(name);
	return (it != settings.end() && it->second);
}



void Preferences::Set(const string &name, bool on)
{
	settings[name] = on;
}



void Preferences::ToggleAmmoUsage()
{
	bool expend = Has(EXPEND_AMMO);
	bool frugal = Has(FRUGAL_ESCORTS);
	Preferences::Set(EXPEND_AMMO, !(expend && !frugal));
	Preferences::Set(FRUGAL_ESCORTS, !expend);
}



string Preferences::AmmoUsage()
{
	return Has(EXPEND_AMMO) ? Has(FRUGAL_ESCORTS) ? "frugally" : "always" : "never";
}



void Preferences::ToggleDateFormat()
{
	if(dateFormatIndex == static_cast<int>(DATEFMT_OPTIONS.size() - 1))
		dateFormatIndex = 0;
	else
		++dateFormatIndex;
}



Preferences::DateFormat Preferences::GetDateFormat()
{
	return static_cast<DateFormat>(dateFormatIndex);
}



const string &Preferences::DateFormatSetting()
{
	return DATEFMT_OPTIONS[dateFormatIndex];
}



void Preferences::ToggleNotificationSetting()
{
	if(notifOptionsIndex == static_cast<int>(NOTIF_OPTIONS.size() - 1))
		notifOptionsIndex = 0;
	else
		++notifOptionsIndex;
}



Preferences::NotificationSetting Preferences::GetNotificationSetting()
{
	return static_cast<NotificationSetting>(notifOptionsIndex);
}



const string &Preferences::NotificationSettingString()
{
	return NOTIF_OPTIONS[notifOptionsIndex];
}



// Scroll speed preference.
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



// View zoom.
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



// Starfield parallax.
void Preferences::ToggleParallax()
{
	int targetIndex = parallaxIndex + 1;
	if(targetIndex == static_cast<int>(PARALLAX_SETTINGS.size()))
		targetIndex = 0;
	parallaxIndex = targetIndex;
}



Preferences::BackgroundParallax Preferences::GetBackgroundParallax()
{
	return static_cast<BackgroundParallax>(parallaxIndex);
}



const string &Preferences::ParallaxSetting()
{
	return PARALLAX_SETTINGS[parallaxIndex];
}



void Preferences::ToggleExtendedJumpEffects()
{
	int targetIndex = extendedJumpEffectIndex + 1;
	if(targetIndex == static_cast<int>(EXTENDED_JUMP_EFFECT_SETTINGS.size()))
		targetIndex = 0;
	extendedJumpEffectIndex = targetIndex;
}



Preferences::ExtendedJumpEffects Preferences::GetExtendedJumpEffects()
{
	return static_cast<ExtendedJumpEffects>(extendedJumpEffectIndex);
}



const string &Preferences::ExtendedJumpEffectsSetting()
{
	return EXTENDED_JUMP_EFFECT_SETTINGS[extendedJumpEffectIndex];
}



void Preferences::ToggleScreenMode()
{
	GameWindow::ToggleFullscreen();
	screenModeIndex = GameWindow::IsFullscreen();
}



const string &Preferences::ScreenModeSetting()
{
	return SCREEN_MODE_SETTINGS[screenModeIndex];
}



bool Preferences::ToggleVSync()
{
	int targetIndex = vsyncIndex + 1;
	if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
		targetIndex = 0;
	if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
	{
		// Not all drivers support adaptive VSync. Increment desired VSync again.
		++targetIndex;
		if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
			targetIndex = 0;
		if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
		{
			// Restore original saved setting.
			Logger::Log("Unable to change VSync state.", Logger::Level::WARNING);
			GameWindow::SetVSync(static_cast<VSync>(vsyncIndex));
			return false;
		}
	}
	vsyncIndex = targetIndex;
	return true;
}



// Return the current VSync setting
Preferences::VSync Preferences::VSyncState()
{
	return static_cast<VSync>(vsyncIndex);
}



const string &Preferences::VSyncSetting()
{
	return VSYNC_SETTINGS[vsyncIndex];
}



void Preferences::ToggleCameraAcceleration()
{
	cameraAccelerationIndex = (cameraAccelerationIndex + 1) % CAMERA_ACCELERATION_SETTINGS.size();
}



Preferences::CameraAccel Preferences::CameraAcceleration()
{
	return static_cast<CameraAccel>(cameraAccelerationIndex);
}



const string &Preferences::CameraAccelerationSetting()
{
	return CAMERA_ACCELERATION_SETTINGS[cameraAccelerationIndex];
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



void Preferences::ToggleTurretOverlays()
{
	turretOverlaysIndex = (turretOverlaysIndex + 1) % TURRET_OVERLAYS_SETTINGS.size();
}



Preferences::TurretOverlays Preferences::GetTurretOverlays()
{
	return static_cast<TurretOverlays>(turretOverlaysIndex);
}



const string &Preferences::TurretOverlaysSetting()
{
	return TURRET_OVERLAYS_SETTINGS[turretOverlaysIndex];
}



void Preferences::ToggleAutoAim()
{
	autoAimIndex = (autoAimIndex + 1) % AUTO_AIM_SETTINGS.size();
}



Preferences::AutoAim Preferences::GetAutoAim()
{
	return static_cast<AutoAim>(autoAimIndex);
}



const string &Preferences::AutoAimSetting()
{
	return AUTO_AIM_SETTINGS[autoAimIndex];
}



void Preferences::ToggleAutoFire()
{
	autoFireIndex = (autoFireIndex + 1) % AUTO_FIRE_SETTINGS.size();
}



Preferences::AutoFire Preferences::GetAutoFire()
{
	return static_cast<AutoFire>(autoFireIndex);
}



const string &Preferences::AutoFireSetting()
{
	return AUTO_FIRE_SETTINGS[autoFireIndex];
}



void Preferences::ToggleBoarding()
{
	int targetIndex = boardingIndex + 1;
	if(targetIndex == static_cast<int>(BOARDING_SETTINGS.size()))
		targetIndex = 0;
	boardingIndex = targetIndex;
}



Preferences::BoardingPriority Preferences::GetBoardingPriority()
{
	return static_cast<BoardingPriority>(boardingIndex);
}



const string &Preferences::BoardingSetting()
{
	return BOARDING_SETTINGS[boardingIndex];
}



void Preferences::ToggleFlotsam()
{
	flotsamIndex = (flotsamIndex + 1) % FLOTSAM_SETTINGS.size();
}



Preferences::FlotsamCollection Preferences::GetFlotsamCollection()
{
	return static_cast<FlotsamCollection>(flotsamIndex);
}



const string &Preferences::FlotsamSetting()
{
	return FLOTSAM_SETTINGS[flotsamIndex];
}



void Preferences::ToggleAlert()
{
	if(++alertIndicatorIndex >= static_cast<int>(ALERT_INDICATOR_SETTING.size()))
		alertIndicatorIndex = 0;
}



Preferences::AlertIndicator Preferences::GetAlertIndicator()
{
	return static_cast<AlertIndicator>(alertIndicatorIndex);
}



const string &Preferences::AlertSetting()
{
	return ALERT_INDICATOR_SETTING[alertIndicatorIndex];
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
	if(value == AlertIndicator::BOTH)
		return true;
	else if(value == toDo)
		return true;
	return false;
}



int Preferences::GetPreviousSaveCount()
{
	return previousSaveCount;
}



void Preferences::ToggleMinimapDisplay()
{
	if(++minimapDisplayIndex >= static_cast<int>(MINIMAP_DISPLAY_SETTING.size()))
		minimapDisplayIndex = 0;
}



Preferences::MinimapDisplay Preferences::GetMinimapDisplay()
{
	return static_cast<MinimapDisplay>(minimapDisplayIndex);
}



const string &Preferences::MinimapSetting()
{
	return MINIMAP_DISPLAY_SETTING[minimapDisplayIndex];
}



void Preferences::ToggleFlagshipSpacePriority()
{
	if(++flagshipSpacePriorityIndex >= static_cast<int>(FLAGSHIP_SPACE_PRIORITY_SETTINGS.size()))
		flagshipSpacePriorityIndex = 0;
}



Preferences::FlagshipSpacePriority Preferences::GetFlagshipSpacePriority()
{
	return static_cast<FlagshipSpacePriority>(flagshipSpacePriorityIndex);
}



const string &Preferences::FlagshipSpacePrioritySetting()
{
	return FLAGSHIP_SPACE_PRIORITY_SETTINGS[flagshipSpacePriorityIndex];
}



void Preferences::ToggleLargeGraphicsReduction()
{
	if(++largeGraphicsReductionIndex >= static_cast<int>(LARGE_GRAPHICS_REDUCTION_SETTINGS.size()))
		largeGraphicsReductionIndex = 0;
}



Preferences::LargeGraphicsReduction Preferences::GetLargeGraphicsReduction()
{
	return static_cast<LargeGraphicsReduction>(largeGraphicsReductionIndex);
}



const string &Preferences::LargeGraphicsReductionSetting()
{
	return LARGE_GRAPHICS_REDUCTION_SETTINGS[largeGraphicsReductionIndex];
}



void Preferences::ToggleBlockScreenSaver()
{
	GameWindow::ToggleBlockScreenSaver();
	Set(BLOCK_SCREEN_SAVER, !Has(BLOCK_SCREEN_SAVER));
}



#ifdef _WIN32
void Preferences::ToggleTitleBarTheme()
{
	if(++titleBarThemeIndex >= static_cast<int>(TITLE_BAR_THEME_SETTINGS.size()))
		titleBarThemeIndex = 0;
	GameWindow::UpdateTitleBarTheme();
}



Preferences::TitleBarTheme Preferences::GetTitleBarTheme()
{
	return static_cast<TitleBarTheme>(titleBarThemeIndex);
}



const string &Preferences::TitleBarThemeSetting()
{
	return TITLE_BAR_THEME_SETTINGS[titleBarThemeIndex];
}



void Preferences::ToggleWindowRounding()
{
	if(++windowRoundingIndex >= static_cast<int>(WINDOW_ROUNDING_SETTINGS.size()))
		windowRoundingIndex = 0;
	GameWindow::UpdateWindowRounding();
}



Preferences::WindowRounding Preferences::GetWindowRounding()
{
	return static_cast<WindowRounding>(windowRoundingIndex);
}



const string &Preferences::WindowRoundingSetting()
{
	return WINDOW_ROUNDING_SETTINGS[windowRoundingIndex];
}
#endif
