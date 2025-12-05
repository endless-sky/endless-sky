/* ObserverPanel.h
Copyright (c) 2024 by the Endless Sky developers

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

#include "Panel.h"

#include "Engine.h"
#include "PlayerInfo.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

class CameraController;
class System;

// Panel for observer/screensaver mode - watches the universe simulate itself.
class ObserverPanel : public Panel {
public:
	// Create an observer panel. If startSystem is provided, starts there;
	// otherwise uses persistent state or picks a random system.
	ObserverPanel(const System *startSystem = nullptr);
	~ObserverPanel();

	void Step() override;
	void Draw() override;

	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	bool Scroll(double dx, double dy) override;

	// Allow fast-forward in observer mode.
	bool AllowsFastForward() const noexcept override { return true; }

	// Get observer-mode specific speed multiplier.
	int GetSpeedMultiplier() const noexcept override;


private:
	void CycleCamera();
	void InitializeSystem(const System *startSystem);
	void SwitchToNewSystem();
	void SwitchToPreviousSystem();
	void DrawGraph() const;
	void UpdateGraphData();


private:
	PlayerInfo player;
	Engine engine;

	std::unique_ptr<CameraController> cameraController;
	int cameraMode = 0;  // 0=follow, 1=orbit, 2=free

	// Speed control: 1-5 keys for 1x, 2x, 3x, 5x, 10x
	int speedLevel = 0;
	static constexpr int SPEED_LEVELS[] = {1, 2, 3, 5, 10};
	static constexpr int NUM_SPEED_LEVELS = 5;

	// Auto-switch system tracking
	int systemTimer = 0;  // Frames in current system
	int quietTimer = 0;   // Frames since last interesting activity
	int recentActivity = 0;  // Recent activity counter (more responsive than just destroys)
	bool autoSwitchEnabled = true;  // Toggle for auto-switching

	// Auto-switch timing (in frames at 60 FPS, at 1x speed - scaled by speed multiplier)
	static const int BASE_MAX_SYSTEM_TIME = 60 * 60 * 5;   // 5 minutes max in one system
	static const int BASE_QUIET_THRESHOLD = 60 * 60 * 2;   // 2 minutes of quiet before switching
	static const int ACTIVITY_DECAY_RATE = 60 * 3;         // Activity decays every 3 seconds

	// Statistics for HUD
	int totalDestroys = 0;      // Total ships destroyed during session
	int totalDisables = 0;      // Total ships disabled during session
	int sessionTimer = 0;       // Total frames in observer mode

	// Auto-save timer
	int saveTimer = 0;
	static const int SAVE_INTERVAL = 60 * 60 * 5;  // 5 minutes at 60 FPS

	// System history for P key navigation (most recent at back)
	std::deque<const System *> systemHistory;
	static const int MAX_SYSTEM_HISTORY = 10;

	// Graph data for destroyed/disabled ships over time
	std::vector<int> destroyGraph;  // Destroyed ships per interval
	std::vector<int> disableGraph;  // Disabled ships per interval
	int graphTimer = 0;
	int graphDestroys = 0;  // Destroys since last graph update
	int graphDisables = 0;  // Disables since last graph update
	static const int GRAPH_UPDATE_INTERVAL = 60 * 5;  // Update every 5 seconds
	static const int GRAPH_MAX_POINTS = 60;  // Keep last 5 minutes of data

	// HUD visibility toggle (H key for clean screenshots)
	bool showHUD = true;

	// Persistent state across observer mode sessions
	static const System *lastSystem;
	static int persistentDestroys;
	static int persistentDisables;
	static int persistentSessionTime;
};
