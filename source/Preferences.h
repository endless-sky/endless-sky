/* Preferences.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include "Help.h"

#include <string>


class Preferences {
public:
	void Load();
	void Save();

	bool& Get(std::string);

	// Toogle the ammo usage preferences, cycling between "never," "frugally,"
	// and "always."
	void ToggleAmmoUsage();
	std::string AmmoUsage();
	
	// View zoom.
	double ViewZoom();
	bool ZoomViewIn();
	bool ZoomViewOut();

	// Preference values

	// Display
	int zoomIndex;
	bool showStatusOverlays;
	bool highlightPlayersFlagship;
	bool rotateFlagshipInHud;
	bool showPlanetLabels;
	bool showMinimap;
	bool fullscreen; // Implicit
	bool maximized; // Implicit

	// AI
	bool automaticAiming;
	bool automaticFiring;
	bool escortsExpendAmmo;
	bool frugalEscorts;
	bool turretsFocusFire;
	bool damagedFightersRetreat; // Hidden

	// Performance
	bool showCpuGpuLoad;
	bool renderMotionBlur;
	bool reduceLargeGraphics;
	bool drawBackgroundHaze;
	bool showHyperspaceFlash;

	// Other
	bool clickableRadarDisplay;
	bool hideUnexploredMapRegions;
	bool seenHelp[Help::MAX];
	bool rehireExtraCrewWhenLost;
	int scrollSpeed;
	bool showEscortSystemsOnMap;
	bool warningSiren;

private:
	bool* Find(std::string);
};


extern Preferences preferences;

#endif
