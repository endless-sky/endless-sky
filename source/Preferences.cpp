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

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameWindow.h"
#include "Logger.h"
#include "Screen.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	map<string, bool> settings;
	int scrollSpeed = 60;

	// Strings for ammo expenditure:
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";

	const vector<double> ZOOMS = {.25, .35, .50, .70, 1.00, 1.40, 2.00};
	int zoomIndex = 4;
	constexpr double VOLUME_SCALE = .25;

	// Default to fullscreen.
	int screenModeIndex = 1;
	const vector<string> SCREEN_MODE_SETTINGS = {"windowed", "fullscreen"};

	// Enable standard VSync by default.
	const vector<string> VSYNC_SETTINGS = {"off", "on", "adaptive"};
	int vsyncIndex = 1;

	class OverlaySetting {
	public:
		OverlaySetting() = default;
		OverlaySetting(const Preferences::OverlayState &state) : state(state) {}

		operator Preferences::OverlayState() const { return state; }

		const bool IsActive() const { return state != Preferences::OverlayState::DISABLED; }

		const std::string &ToString() const
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

	const vector<string> OverlaySetting::OVERLAY_SETTINGS = {"off", "always on", "damaged", "--"};

	map<Preferences::OverlayType, OverlaySetting> statusOverlaySettings = {
		{Preferences::OverlayType::ALL, Preferences::OverlayState::DISABLED},
		{Preferences::OverlayType::FLAGSHIP, Preferences::OverlayState::ON},
		{Preferences::OverlayType::ESCORT, Preferences::OverlayState::ON},
		{Preferences::OverlayType::ENEMY, Preferences::OverlayState::ON},
		{Preferences::OverlayType::NEUTRAL, Preferences::OverlayState::OFF},
	};

	const vector<string> AUTO_AIM_SETTINGS = {"off", "always on", "when firing"};
	int autoAimIndex = 2;

	const vector<string> AUTO_FIRE_SETTINGS = {"off", "on", "guns only", "turrets only"};
	int autoFireIndex = 0;

	const vector<string> BOARDING_SETTINGS = {"proximity", "value", "mixed"};
	int boardingIndex = 0;

	// Enable "fast" parallax by default. "fancy" is too GPU heavy, especially for low-end hardware.
	const vector<string> PARALLAX_SETTINGS = {"off", "fancy", "fast"};
	int parallaxIndex = 2;

	const vector<string> ALERT_INDICATOR_SETTING = {"off", "audio", "visual", "both"};
	int alertIndicatorIndex = 3;

	int previousSaveCount = 3;
}



void Preferences::Load()
{
	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	settings["Render motion blur"] = true;
	settings["Flagship flotsam collection"] = true;
	settings[FRUGAL_ESCORTS] = true;
	settings[EXPEND_AMMO] = true;
	settings["Damaged fighters retreat"] = true;
	settings["Show escort systems on map"] = true;
	settings["Show stored outfits on map"] = true;
	settings["Show mini-map"] = true;
	settings["Show planet labels"] = true;
	settings["Show asteroid scanner overlay"] = true;
	settings["Show hyperspace flash"] = true;
	settings["Draw background haze"] = true;
	settings["Draw starfield"] = true;
	settings["Hide unexplored map regions"] = true;
	settings["Turrets focus fire"] = true;
	settings["Ship outlines in shops"] = true;
	settings["Extra fleet status messages"] = true;
	settings["Target asteroid based on"] = true;

	DataFile prefs(Files::Config() + "preferences.txt");
	for(const DataNode &node : prefs)
	{
		if(node.Token(0) == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2));
		else if(node.Token(0) == "zoom" && node.Size() >= 2)
			Screen::SetZoom(node.Value(1));
		else if(node.Token(0) == "volume" && node.Size() >= 2)
			Audio::SetVolume(node.Value(1) * VOLUME_SCALE);
		else if(node.Token(0) == "scroll speed" && node.Size() >= 2)
			scrollSpeed = node.Value(1);
		else if(node.Token(0) == "boarding target")
			boardingIndex = max<int>(0, min<int>(node.Value(1), BOARDING_SETTINGS.size() - 1));
		else if(node.Token(0) == "view zoom")
			zoomIndex = max<int>(0, min<int>(node.Value(1), ZOOMS.size() - 1));
		else if(node.Token(0) == "vsync")
			vsyncIndex = max<int>(0, min<int>(node.Value(1), VSYNC_SETTINGS.size() - 1));
		else if(node.Token(0) == "Show all status overlays")
			statusOverlaySettings[OverlayType::ALL].SetState(node.Value(1));
		else if(node.Token(0) == "Show flagship overlay")
			statusOverlaySettings[OverlayType::FLAGSHIP].SetState(node.Value(1));
		else if(node.Token(0) == "Show escort overlays")
			statusOverlaySettings[OverlayType::ESCORT].SetState(node.Value(1));
		else if(node.Token(0) == "Show enemy overlays")
			statusOverlaySettings[OverlayType::ENEMY].SetState(node.Value(1));
		else if(node.Token(0) == "Show neutral overlays")
			statusOverlaySettings[OverlayType::NEUTRAL].SetState(node.Value(1));
		else if(node.Token(0) == "Automatic aiming")
			autoAimIndex = max<int>(0, min<int>(node.Value(1), AUTO_AIM_SETTINGS.size() - 1));
		else if(node.Token(0) == "Automatic firing")
			autoFireIndex = max<int>(0, min<int>(node.Value(1), AUTO_FIRE_SETTINGS.size() - 1));
		else if(node.Token(0) == "Parallax background")
			parallaxIndex = max<int>(0, min<int>(node.Value(1), PARALLAX_SETTINGS.size() - 1));
		else if(node.Token(0) == "fullscreen")
			screenModeIndex = max<int>(0, min<int>(node.Value(1), SCREEN_MODE_SETTINGS.size() - 1));
		else if(node.Token(0) == "alert indicator")
			alertIndicatorIndex = max<int>(0, min<int>(node.Value(1), ALERT_INDICATOR_SETTING.size() - 1));
		else if(node.Token(0) == "previous saves" && node.Size() >= 2)
			previousSaveCount = max<int>(3, node.Value(1));
		else if(node.Token(0) == "alt-mouse turning")
			settings["Control ship with mouse"] = (node.Size() == 1 || node.Value(1));
		else
			settings[node.Token(0)] = (node.Size() == 1 || node.Value(1));
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
		if(!it->second)
			statusOverlaySettings[OverlayType::ALL] = OverlayState::OFF;
		settings.erase(it);
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() + "preferences.txt");

	out.Write("volume", Audio::Volume() / VOLUME_SCALE);
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::UserZoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("boarding target", boardingIndex);
	out.Write("view zoom", zoomIndex);
	out.Write("vsync", vsyncIndex);
	out.Write("Show all status overlays", statusOverlaySettings[OverlayType::ALL].ToInt());
	out.Write("Show flagship overlay", statusOverlaySettings[OverlayType::FLAGSHIP].ToInt());
	out.Write("Show escort overlays", statusOverlaySettings[OverlayType::ESCORT].ToInt());
	out.Write("Show enemy overlays", statusOverlaySettings[OverlayType::ENEMY].ToInt());
	out.Write("Show neutral overlays", statusOverlaySettings[OverlayType::NEUTRAL].ToInt());
	out.Write("Automatic aiming", autoAimIndex);
	out.Write("Automatic firing", autoFireIndex);
	out.Write("Parallax background", parallaxIndex);
	out.Write("alert indicator", alertIndicatorIndex);
	out.Write("previous saves", previousSaveCount);

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



// Scroll speed preference.
int Preferences::ScrollSpeed()
{
	return scrollSpeed;
}



void Preferences::SetScrollSpeed(int speed)
{
	scrollSpeed = speed;
}



// View zoom.
double Preferences::ViewZoom()
{
	return ZOOMS[zoomIndex];
}



bool Preferences::ZoomViewIn()
{
	if(zoomIndex == static_cast<int>(ZOOMS.size() - 1))
		return false;

	++zoomIndex;
	return true;
}



bool Preferences::ZoomViewOut()
{
	if(zoomIndex == 0)
		return false;

	--zoomIndex;
	return true;
}



double Preferences::MinViewZoom()
{
	return ZOOMS[0];
}



double Preferences::MaxViewZoom()
{
	return ZOOMS[ZOOMS.size() - 1];
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
			Logger::LogError("Unable to change VSync state");
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



void Preferences::CycleStatusOverlays(Preferences::OverlayType type)
{
	// Calling OverlaySetting::Increment when the state is DAMAGED will cycle to off.
	// But, for the ALL overlay type, allow it to cycle to DISABLED.
	if(type == OverlayType::ALL && statusOverlaySettings[OverlayType::ALL] == OverlayState::DAMAGED)
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



void Preferences::ToggleAlert()
{
	if(++alertIndicatorIndex >= static_cast<int>(ALERT_INDICATOR_SETTING.size()))
		alertIndicatorIndex = 0;
}



Preferences::AlertIndicator Preferences::GetAlertIndicator()
{
	return static_cast<AlertIndicator>(alertIndicatorIndex);
}



const std::string &Preferences::AlertSetting()
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
