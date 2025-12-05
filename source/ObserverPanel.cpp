/* ObserverPanel.cpp
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

#include "ObserverPanel.h"

#include "CameraController.h"
#include "Color.h"
#include "shader/FillShader.h"
#include "FollowShipCamera.h"
#include "FreeCamera.h"
#include "GameData.h"
#include "Messages.h"
#include "OrbitPlanetCamera.h"
#include "Preferences.h"
#include "Random.h"
#include "Rectangle.h"
#include "Screen.h"
#include "ShipEvent.h"
#include "System.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "UI.h"

#include "opengl.h"

#include <vector>

using namespace std;



ObserverPanel::ObserverPanel()
	: player(), engine(player)
{
	SetIsFullScreen(true);

	InitializeSystem();

	// Start with follow ship camera
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
}



void ObserverPanel::InitializeSystem()
{
	// Pick a random inhabited system with fleets
	vector<const System *> candidates;
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(system.IsValid() && !system.Fleets().empty() && system.IsInhabited(nullptr))
			candidates.push_back(&system);
	}

	const System *system = nullptr;
	if(!candidates.empty())
		system = candidates[Random::Int(candidates.size())];
	else
	{
		// Fallback: any valid system
		for(const auto &it : GameData::Systems())
			if(it.second.IsValid())
			{
				system = &it.second;
				break;
			}
	}

	if(system)
	{
		// Initialize the player as an observer in this system
		player.NewObserver(system);
		engine.EnterSystem();

		Messages::Add({"Observing the " + system->DisplayName() + " system.",
			GameData::MessageCategories().Get("info")});
	}
}



void ObserverPanel::Step()
{
	// Handle free camera movement using SDL keyboard state
	if(cameraMode == 2)  // Free camera
	{
		FreeCamera *freeCam = dynamic_cast<FreeCamera *>(cameraController.get());
		if(freeCam)
		{
			const Uint8 *keyState = SDL_GetKeyboardState(nullptr);
			double dx = 0.;
			double dy = 0.;

			if(keyState[SDL_SCANCODE_W] || keyState[SDL_SCANCODE_UP])
				dy -= 1.;
			if(keyState[SDL_SCANCODE_S] || keyState[SDL_SCANCODE_DOWN])
				dy += 1.;
			if(keyState[SDL_SCANCODE_A] || keyState[SDL_SCANCODE_LEFT])
				dx -= 1.;
			if(keyState[SDL_SCANCODE_D] || keyState[SDL_SCANCODE_RIGHT])
				dx += 1.;

			freeCam->SetMovement(dx, dy);
		}
	}

	engine.Wait();
	engine.Step(true);  // Always active

	// Track activity from ship events (combat, destruction, etc.)
	bool hadActivity = false;
	for(const ShipEvent &event : engine.Events())
	{
		int type = event.Type();
		// Combat-related events: destroy, disable, provoke, board, capture
		if(type & (ShipEvent::DESTROY | ShipEvent::DISABLE | ShipEvent::PROVOKE |
		           ShipEvent::BOARD | ShipEvent::CAPTURE))
		{
			hadActivity = true;
			// Weight different events differently
			if(type & ShipEvent::DESTROY)
			{
				recentActivity += 10;  // Big event
				++totalDestroys;
			}
			if(type & ShipEvent::DISABLE)
			{
				recentActivity += 5;
				++totalDisables;
			}
			if(type & ShipEvent::PROVOKE)
				recentActivity += 2;  // Combat starting
			if(type & (ShipEvent::BOARD | ShipEvent::CAPTURE))
				recentActivity += 3;
		}
	}
	engine.Events().clear();

	// Update session timer
	++sessionTimer;

	// Update timers (account for speed multiplier - timers run at real-time, not game-time)
	++systemTimer;
	if(hadActivity)
		quietTimer = 0;
	else
		++quietTimer;

	// Decay activity counter periodically
	if(systemTimer % ACTIVITY_DECAY_RATE == 0 && recentActivity > 0)
		--recentActivity;

	// Check if we should switch systems (only if auto-switch is enabled)
	// Scale thresholds by speed multiplier so switching happens at consistent real-time intervals
	if(autoSwitchEnabled)
	{
		int speedMult = SPEED_LEVELS[speedLevel];
		int maxTime = BASE_MAX_SYSTEM_TIME * speedMult;
		int quietThreshold = BASE_QUIET_THRESHOLD * speedMult;

		bool shouldSwitch = false;
		if(systemTimer >= maxTime)
		{
			// Time limit reached
			shouldSwitch = true;
		}
		else if(quietTimer >= quietThreshold && recentActivity == 0)
		{
			// Too quiet for too long
			shouldSwitch = true;
		}

		if(shouldSwitch)
			SwitchToNewSystem();
	}

	engine.Go();

	// Auto-save periodically
	++saveTimer;
	if(saveTimer >= SAVE_INTERVAL)
	{
		saveTimer = 0;
		player.Save();
	}
}



void ObserverPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	engine.Draw();

	// Get standard game colors for consistency
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color panelBg(0.08f, 0.08f, 0.08f, 0.85f);
	const Color combatColor(0.9f, 0.3f, 0.2f, 1.f);
	const Color activeColor(0.3f, 0.8f, 0.4f, 1.f);
	const Color accentColor(0.7f, 0.55f, 0.2f, 1.f);

	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);

	// ========== TOP-RIGHT: Status Panel ==========
	// Panel dimensions
	const double panelWidth = 200.;
	const double panelPadding = 12.;
	const double lineHeight = 18.;

	// Calculate panel height based on content
	double panelHeight = panelPadding * 2 + lineHeight * 7;  // Title + 6 lines

	// Draw semi-transparent panel background
	Point panelCenter = Screen::TopRight() + Point(-panelWidth / 2 - 10., panelHeight / 2 + 10.);
	FillShader::Fill(panelCenter, Point(panelWidth, panelHeight), panelBg);

	// Panel content position
	Point pos = Screen::TopRight() + Point(-panelWidth - 10. + panelPadding, 10. + panelPadding);

	// Title: OBSERVER MODE
	bigFont.Draw("OBSERVER", pos, medium);
	pos.Y() += lineHeight + 4.;

	// System name
	if(player.GetSystem())
		font.Draw(player.GetSystem()->DisplayName(), pos, bright);
	pos.Y() += lineHeight + 8.;

	// Activity indicator (prominent)
	// recentActivity is weighted: destroy=10, disable=5, provoke=2, board/capture=3
	if(recentActivity >= 5)
	{
		font.Draw("COMBAT", pos, combatColor);
	}
	else if(recentActivity > 0 || quietTimer < 60 * 10)
	{
		font.Draw("Active", pos, activeColor);
	}
	else
	{
		font.Draw("Quiet", pos, dim);
	}
	pos.Y() += lineHeight + 4.;

	// Stats: Ships | Destroyed | Disabled
	font.Draw("Ships: " + to_string(engine.ShipCount()), pos, medium);
	pos.Y() += lineHeight;

	font.Draw("Destroyed: " + to_string(totalDestroys), pos, medium);
	pos.Y() += lineHeight;

	font.Draw("Disabled: " + to_string(totalDisables), pos, medium);
	pos.Y() += lineHeight;

	// Session time
	int totalSeconds = sessionTimer / 60;
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;
	string timeStr;
	if(hours > 0)
		timeStr = to_string(hours) + ":" + (minutes < 10 ? "0" : "") + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds);
	else
		timeStr = to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds);
	font.Draw("Time: " + timeStr, pos, dim);

	// ========== TOP-LEFT: Camera Info (below radar) ==========
	// Position below the radar (radar is ~256px wide/tall centered at 128,128)
	Point camPos = Screen::TopLeft() + Point(20., 270.);

	// Camera mode
	font.Draw(cameraController->ModeName(), camPos, medium);
	camPos.Y() += lineHeight;

	// Target name
	string targetName = cameraController->TargetName();
	if(!targetName.empty())
	{
		font.Draw(targetName, camPos, bright);
		camPos.Y() += lineHeight;
	}

	// Speed indicator (show prominently if not 1x)
	if(speedLevel > 0)
	{
		font.Draw(GetSpeedText(), camPos, accentColor);
	}

	// ========== BOTTOM-RIGHT: Controls Hint ==========
	string hintText = "Tab: camera  |  Space: target  |  F: speed  |  N: system  |  A: auto";
	if(!autoSwitchEnabled)
		hintText += " (off)";
	hintText += "  |  Esc: exit";
	double hintWidth = font.Width(hintText);
	Point hintPos = Screen::BottomRight() + Point(-hintWidth - 20., -25.);
	font.Draw(hintText, hintPos, dim);
}



bool ObserverPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_ESCAPE || command.Has(Command::MENU))
	{
		GetUI()->Pop(this);
		return true;
	}

	if(key == SDLK_TAB)
	{
		CycleCamera();
		return true;
	}

	if(key == ' ')
	{
		// Select new target in current mode
		if(cameraMode == 0)
		{
			FollowShipCamera *follow = dynamic_cast<FollowShipCamera *>(cameraController.get());
			if(follow)
				follow->CycleTarget();
		}
		else if(cameraMode == 1)
		{
			OrbitPlanetCamera *orbit = dynamic_cast<OrbitPlanetCamera *>(cameraController.get());
			if(orbit)
				orbit->CycleTarget();
		}
		return true;
	}

	// Zoom controls (same as main game)
	if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
	{
		Preferences::ZoomViewOut();
		return true;
	}
	if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
	{
		Preferences::ZoomViewIn();
		return true;
	}

	// Speed controls: F to cycle through speeds
	if(key == 'f' || key == 'F')
	{
		CycleSpeed();
		return true;
	}

	// Number keys 1-5 for direct speed selection
	if(key >= '1' && key <= '5')
	{
		speedLevel = key - '1';
		return true;
	}

	// N for new system (manual switch)
	if(key == 'n' || key == 'N')
	{
		SwitchToNewSystem();
		return true;
	}

	// A to toggle auto-switching
	if(key == 'a' || key == 'A')
	{
		autoSwitchEnabled = !autoSwitchEnabled;
		Messages::Add({autoSwitchEnabled ? "Auto-switching enabled." : "Auto-switching disabled.",
			GameData::MessageCategories().Get("info")});
		return true;
	}

	return false;
}



bool ObserverPanel::Scroll(double dx, double dy)
{
	if(dy < 0)
		Preferences::ZoomViewOut();
	else if(dy > 0)
		Preferences::ZoomViewIn();

	return true;
}



void ObserverPanel::SwitchToNewSystem()
{
	// Reset timers
	systemTimer = 0;
	quietTimer = 0;
	recentActivity = 0;

	// Find a new random system (different from current if possible)
	const System *currentSystem = player.GetSystem();
	vector<const System *> candidates;
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(system.IsValid() && !system.Fleets().empty() && system.IsInhabited(nullptr))
		{
			// Prefer different systems, but if only one exists, allow staying
			if(&system != currentSystem || candidates.empty())
				candidates.push_back(&system);
		}
	}

	if(candidates.empty())
		return;

	// Pick a random candidate
	const System *newSystem = candidates[Random::Int(candidates.size())];

	// Move to the new system
	player.SetSystem(*newSystem);
	engine.EnterSystem();

	// Reset camera to follow mode for new system
	cameraMode = 0;
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
	if(newSystem)
		cameraController->SetStellarObjects(newSystem->Objects());

	Messages::Add({"Now observing the " + newSystem->DisplayName() + " system.",
		GameData::MessageCategories().Get("info")});
}



void ObserverPanel::CycleCamera()
{
	cameraMode = (cameraMode + 1) % 3;

	// Get current position to pass to new camera
	Point currentPos = cameraController->GetTarget();

	switch(cameraMode)
	{
	case 0:
		cameraController = make_unique<FollowShipCamera>();
		break;
	case 1:
		cameraController = make_unique<OrbitPlanetCamera>();
		break;
	case 2:
		{
			auto freeCam = make_unique<FreeCamera>();
			freeCam->SetPosition(currentPos);
			cameraController = std::move(freeCam);
		}
		break;
	}

	engine.SetCameraController(cameraController.get());

	// Provide current data to new controller
	if(player.GetSystem())
		cameraController->SetStellarObjects(player.GetSystem()->Objects());
}



void ObserverPanel::CycleSpeed()
{
	speedLevel = (speedLevel + 1) % NUM_SPEED_LEVELS;
}



string ObserverPanel::GetSpeedText() const
{
	return to_string(SPEED_LEVELS[speedLevel]) + "x";
}



int ObserverPanel::GetSpeedMultiplier() const noexcept
{
	return SPEED_LEVELS[speedLevel];
}


// Static member definition
constexpr int ObserverPanel::SPEED_LEVELS[];
