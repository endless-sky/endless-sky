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

#include <memory>

class CameraController;

// Panel for observer/screensaver mode - watches the universe simulate itself.
class ObserverPanel : public Panel {
public:
	// Create an observer panel for a random system.
	ObserverPanel();

	void Step() override;
	void Draw() override;

	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	bool Scroll(double dx, double dy) override;

	// Allow fast-forward in observer mode.
	bool AllowsFastForward() const noexcept override { return true; }


private:
	void CycleCamera();
	void InitializeSystem();


private:
	PlayerInfo player;
	Engine engine;

	std::unique_ptr<CameraController> cameraController;
	int cameraMode = 0;  // 0=follow, 1=orbit, 2=free

	// Auto-save timer
	int saveTimer = 0;
	static const int SAVE_INTERVAL = 60 * 60 * 5;  // 5 minutes at 60 FPS
};
