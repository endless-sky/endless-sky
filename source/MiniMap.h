/* MiniMap.h
Copyright (c) 2025 by Amazinite

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

#include "Point.h"

#include <memory>

class PlayerInfo;
class Ship;
class System;



// A miniature map of the nearby systems that displays while on the main panel.
class MiniMap {
public:
	MiniMap(const PlayerInfo &player);

	void Step(const std::shared_ptr<Ship> &flagship);
	void Draw(int step) const;


private:
	const PlayerInfo &player;
	// The system that the player is currently in.
	const System *current = nullptr;
	// The system that is next in the player's travel plan.
	const System *target = nullptr;
	// Where the minimap is focused.
	Point center;
	// Tracks the old target, the next target, and the number of frames that the center
	// has been interpolating between the two.
	Point oldCenter;
	Point targetCenter;
	int lerpCount = 0;

	// How many frames the mini-map should be displayed for when it is set to only appear
	// when jumping.
	int displayMinimap = 0;
	// Controls the fading in and out of the minimap. The minimap should fade in and out over
	// the course of 30 frames (0.5 seconds).
	int fadeMinimap = 0;
};
