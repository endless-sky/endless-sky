/* Preferences.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Preferences.h"

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "Screen.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	const vector<double> ZOOMS = {.25, .35, .50, .70, 1.00, 1.40, 2.00};

	static map<string, bool*> settings =
	{
		// Display
		{ "Show status overlays", &preferences.showStatusOverlays },
		{ "Highlight player's flagship", &preferences.highlightPlayersFlagship },
		{ "Rotate flagship in HUD", &preferences.rotateFlagshipInHud },
		{ "Show planet labels", &preferences.showPlanetLabels },
		{ "Show mini-map", &preferences.showMinimap },
		{ "fullscreen", &preferences.fullscreen }, // Implicit
		{ "maximized", &preferences.maximized }, // Implicit

		// AI
		{ "Automatic aiming", &preferences.automaticAiming },
		{ "Automatic firing", &preferences.automaticFiring },
		{ "Escorts expend ammo", &preferences.escortsExpendAmmo },
		{ "Escorts use ammo frugally", &preferences.frugalEscorts},
		{ "Turrets focus fire", &preferences.turretsFocusFire },
		{ "Damaged fighters retreat", &preferences.damagedFightersRetreat }, // Hidden

		// Performance
		{ "Show CPU / GPU load", &preferences.showCpuGpuLoad },
		{ "Render motion blur", &preferences.renderMotionBlur },
		{ "Reduce large graphics", &preferences.reduceLargeGraphics },
		{ "Draw background haze", &preferences.drawBackgroundHaze },
		{ "Show hyperspace flash", &preferences.showHyperspaceFlash },

		// Other
		{ "Clickable radar display", &preferences.clickableRadarDisplay },
		{ "Hide unexplored map regions", &preferences.hideUnexploredMapRegions },
		{ "Rehire extra crew when lost", &preferences.rehireExtraCrewWhenLost },
		{ "Show escort systems on map", &preferences.showEscortSystemsOnMap },
		{ "Warning siren", &preferences.warningSiren },
	};
}


Preferences preferences;

void Preferences::Load()
{
	*this = {}; // Clear

	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	zoomIndex = 4;
	automaticAiming = true;
	renderMotionBlur = true;
	frugalEscorts = true;
	escortsExpendAmmo = true;
	damagedFightersRetreat = true;
	warningSiren = true;
	showEscortSystemsOnMap = true;
	showMinimap = true;
	showPlanetLabels = true;
	showHyperspaceFlash = true;
	drawBackgroundHaze = true;
	hideUnexploredMapRegions = true;
	turretsFocusFire = true;
	scrollSpeed = 60;

	DataFile prefs(Files::Config() + "preferences.txt");
	for(const DataNode &node : prefs)
	{
		if(node.Token(0) == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2));
		else if(node.Token(0) == "zoom" && node.Size() >= 2)
			Screen::SetZoom(node.Value(1));
		else if(node.Token(0) == "volume" && node.Size() >= 2)
			Audio::SetVolume(node.Value(1));
		else if(node.Token(0) == "scroll speed" && node.Size() >= 2)
			scrollSpeed = node.Value(1);
		else if(node.Token(0) == "view zoom")
			zoomIndex = node.Value(1);
		else
		{
			bool *pValue = Find(node.Token(0));
			if(pValue)
				*pValue = (node.Size() == 1 || node.Value(1));
		}
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() + "preferences.txt");
	
	out.Write("volume", Audio::Volume());
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::Zoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("view zoom", zoomIndex);
	
	for(const auto &it : settings)
		out.Write(it.first, *it.second);

	for(int t=0; t<Help::MAX; t++)
	{
		string name = string("help: ") + Help::TopicName((Help::Topic)t);
		out.Write(name, Get(name));
	}
}



bool* Preferences::Find(string name)
{
	if(name.find("help: ") == 0)
	{
		Help::Topic topic = Help::FindTopic(name.substr(6));
		if(topic < 0)
			return nullptr;
		return &seenHelp[topic];
	}

	auto it = settings.find(name);
	if(it == settings.end())
		return nullptr;
	return it->second;
}



bool& Preferences::Get(string name)
{
	bool *pValue = Find(name);
	if(!pValue)
		throw runtime_error("Unknown preference: " + name);
	return *pValue;
}



void Preferences::ToggleAmmoUsage()
{
	bool expend = escortsExpendAmmo;
	bool frugal = frugalEscorts;
	escortsExpendAmmo = !(expend && !frugal);
	frugalEscorts = !expend;
}



string Preferences::AmmoUsage()
{
	return escortsExpendAmmo ? frugalEscorts ? "frugally" : "always" : "never";
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
