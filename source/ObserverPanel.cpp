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
#include "Command.h"
#include "PanelUtils.h"
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
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "UI.h"

#include "opengl.h"

#include <algorithm>
#include <vector>

using namespace std;

// Static member definitions for persistent state
const System *ObserverPanel::lastSystem = nullptr;
int ObserverPanel::persistentDestroys = 0;
int ObserverPanel::persistentDisables = 0;
int ObserverPanel::persistentSessionTime = 0;



ObserverPanel::ObserverPanel(const System *startSystem)
	: player(), engine(player)
{
	SetIsFullScreen(true);

	// Restore persistent state from previous session
	totalDestroys = persistentDestroys;
	totalDisables = persistentDisables;
	sessionTimer = persistentSessionTime;

	InitializeSystem(startSystem);

	// Start with follow ship camera
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
}



ObserverPanel::~ObserverPanel()
{
	// Wait for the engine's background thread to finish before destroying
	// the camera controller, to avoid use-after-free.
	engine.Wait();
}



void ObserverPanel::InitializeSystem(const System *startSystem)
{
	const System *system = nullptr;

	// Priority: 1) explicit startSystem, 2) persistent lastSystem, 3) random
	if(startSystem)
		system = startSystem;
	else if(lastSystem)
		system = lastSystem;
	else
	{
		// Pick a random inhabited system with fleets
		vector<const System *> candidates;
		for(const auto &it : GameData::Systems())
		{
			const System &sys = it.second;
			if(sys.IsValid() && !sys.Fleets().empty() && sys.IsInhabited(nullptr))
				candidates.push_back(&sys);
		}

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
	}

	if(system)
	{
		// Initialize the player as an observer in this system
		player.NewObserver(system);
		engine.EnterSystem();

		// Save as last system for persistence
		lastSystem = system;

		Messages::Add({"Observing the " + system->DisplayName() + " system.",
			GameData::MessageCategories().Get("info")});
	}
}



void ObserverPanel::Step()
{
	// Check for camera movement using game's directional commands
	Command command;
	command.ReadKeyboard();

	double dx = 0.;
	double dy = 0.;

	if(command.Has(Command::FORWARD))
		dy -= 1.;
	if(command.Has(Command::BACK))
		dy += 1.;
	if(command.Has(Command::LEFT))
		dx -= 1.;
	if(command.Has(Command::RIGHT))
		dx += 1.;

	// Auto-switch to free camera if movement keys are pressed
	if((dx != 0. || dy != 0.) && cameraMode != 2)
	{
		// Switch to free camera mode
		Point currentPos = cameraController->GetTarget();
		cameraMode = 2;
		auto freeCam = make_unique<FreeCamera>();
		freeCam->SetPosition(currentPos);
		cameraController = std::move(freeCam);
		engine.SetCameraController(cameraController.get());
		if(player.GetSystem())
			cameraController->SetStellarObjects(player.GetSystem()->Objects());
	}

	// Handle free camera movement (uses virtual SetMovement on CameraController)
	if(cameraMode == 2)
	{
		// Scale movement inversely with game speed so camera feels consistent
		double speedScale = 1.0 / SPEED_LEVELS[speedLevel];
		cameraController->SetMovement(dx * speedScale, dy * speedScale);
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
				++graphDestroys;

				// Highlight capital ship destructions with a special message
				const shared_ptr<Ship> &ship = event.Target();
				if(ship)
				{
					const string &category = ship->Attributes().Category();
					// Check for capital ships (Heavy Warship, Heavy Freighter, etc.)
					if(category.find("Heavy") != string::npos)
					{
						string name = ship->GivenName().empty() ? ship->DisplayModelName() : ship->GivenName();
						string msg = "CAPITAL SHIP DESTROYED: " + name + " (" + ship->DisplayModelName() + ")";
						Messages::Add({msg, GameData::MessageCategories().Get("high")});
					}
				}
			}
			if(type & ShipEvent::DISABLE)
			{
				recentActivity += 5;
				++totalDisables;
				++graphDisables;
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

	// Update graph data periodically
	UpdateGraphData();

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

	// Save persistent state
	persistentDestroys = totalDestroys;
	persistentDisables = totalDisables;
	persistentSessionTime = sessionTimer;

	// Auto-save periodically
	++saveTimer;
	if(saveTimer >= SAVE_INTERVAL)
	{
		saveTimer = 0;
		player.Save();
	}
}



void ObserverPanel::UpdateGraphData()
{
	++graphTimer;
	if(graphTimer >= GRAPH_UPDATE_INTERVAL)
	{
		graphTimer = 0;

		// Add current interval data
		destroyGraph.push_back(graphDestroys);
		disableGraph.push_back(graphDisables);

		// Reset interval counters
		graphDestroys = 0;
		graphDisables = 0;

		// Keep only the last N points
		while(destroyGraph.size() > GRAPH_MAX_POINTS)
			destroyGraph.erase(destroyGraph.begin());
		while(disableGraph.size() > GRAPH_MAX_POINTS)
			disableGraph.erase(disableGraph.begin());
	}
}



void ObserverPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	engine.Draw();

	// Skip HUD drawing if hidden (clean screenshot mode)
	if(!showHUD)
		return;

	// Get standard game colors for consistency
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color panelBg(0.05f, 0.05f, 0.05f, 0.6f);  // More transparent
	const Color combatColor(0.9f, 0.3f, 0.2f, 1.f);
	const Color activeColor(0.3f, 0.8f, 0.4f, 1.f);
	const Color graphDestroyColor(0.9f, 0.2f, 0.2f, 0.8f);
	const Color graphDisableColor(0.9f, 0.7f, 0.2f, 0.8f);

	const Font &font = FontSet::Get(14);

	// ========== BOTTOM-RIGHT: Status Panel ==========
	const double panelWidth = 220.;
	const double panelPadding = 10.;
	const double lineHeight = 16.;
	const double graphHeight = 40.;

	// Calculate panel height based on content
	// Title + Activity + Camera + Ships + Time + Graph + Legend
	double panelHeight = panelPadding * 2 + lineHeight * 6 + graphHeight + 10.;

	// Draw semi-transparent panel background
	Point panelCenter = Screen::BottomRight() + Point(-panelWidth / 2 - 10., -panelHeight / 2 - 50.);
	FillShader::Fill(panelCenter, Point(panelWidth, panelHeight), panelBg);

	// Panel content position (from top of panel)
	Point pos = Screen::BottomRight() + Point(-panelWidth - 10. + panelPadding, -panelHeight - 50. + panelPadding);

	// Title
	font.Draw("OBSERVER", pos, bright);
	pos.Y() += lineHeight;

	// Activity indicator
	if(recentActivity >= 5)
		font.Draw("Status: COMBAT", pos, combatColor);
	else if(recentActivity > 0 || quietTimer < 60 * 10)
		font.Draw("Status: Active", pos, activeColor);
	else
		font.Draw("Status: Quiet", pos, dim);
	pos.Y() += lineHeight;

	// Camera mode
	font.Draw("Camera: " + cameraController->ModeName(), pos, medium);
	pos.Y() += lineHeight;

	// Ship count
	font.Draw("Ships: " + to_string(engine.ShipCount()), pos, medium);
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
	font.Draw("Session: " + timeStr, pos, dim);
	pos.Y() += lineHeight + 5.;

	// Draw the activity graph
	double graphWidth = panelWidth - panelPadding * 2;
	double graphX = Screen::BottomRight().X() - panelWidth - 10. + panelPadding;
	double graphY = pos.Y();

	if(!destroyGraph.empty() || !disableGraph.empty())
	{
		// Find max value for scaling
		int maxVal = 1;
		for(int v : destroyGraph)
			maxVal = max(maxVal, v);
		for(int v : disableGraph)
			maxVal = max(maxVal, v);

		// Draw graph bars
		double barWidth = graphWidth / GRAPH_MAX_POINTS;
		size_t numPoints = max(destroyGraph.size(), disableGraph.size());

		for(size_t i = 0; i < numPoints; ++i)
		{
			double x = graphX + i * barWidth + barWidth / 2;

			// Destroyed bars (red)
			if(i < destroyGraph.size() && destroyGraph[i] > 0)
			{
				double h = (graphHeight - 2) * destroyGraph[i] / maxVal;
				Point barCenter(x, graphY + graphHeight - h / 2 - 1);
				FillShader::Fill(barCenter, Point(barWidth - 1, h), graphDestroyColor);
			}

			// Disabled bars (yellow, offset slightly)
			if(i < disableGraph.size() && disableGraph[i] > 0)
			{
				double h = (graphHeight - 2) * disableGraph[i] / maxVal;
				Point barCenter(x + barWidth * 0.3, graphY + graphHeight - h / 2 - 1);
				FillShader::Fill(barCenter, Point(barWidth * 0.6 - 1, h), graphDisableColor);
			}
		}
	}

	// Graph legend below the graph (with matching colors)
	pos.Y() = graphY + graphHeight + 3.;
	font.Draw("Destroyed: " + to_string(totalDestroys), pos, graphDestroyColor);
	double destroyedWidth = font.Width("Destroyed: " + to_string(totalDestroys));
	Point disabledPos = pos + Point(destroyedWidth + 15., 0.);
	font.Draw("Disabled: " + to_string(totalDisables), disabledPos, graphDisableColor);

	// ========== BOTTOM-RIGHT: Controls Hint (below panel) ==========
	// Build hints using configurable key names
	string cameraKey = Command::OBSERVER_CYCLE_CAMERA.KeyName();
	string targetKey = Command::OBSERVER_CYCLE_TARGET.KeyName();
	string nextKey = Command::OBSERVER_NEXT_SYSTEM.KeyName();
	string prevKey = Command::OBSERVER_PREV_SYSTEM.KeyName();
	string autoKey = Command::OBSERVER_AUTO_SWITCH.KeyName();

	string pauseKey = Command::PAUSE.KeyName();

	vector<string> hints = {
		cameraKey + ": camera  |  " + targetKey + ": target  |  Arrows: free camera",
		nextKey + ": next system  |  " + prevKey + ": prev system  |  " + autoKey + ": auto" + string(autoSwitchEnabled ? "" : " (off)"),
		pauseKey + ": pause  |  H: hide HUD  |  1-5: speed  |  Esc: exit"
	};

	double hintY = Screen::BottomRight().Y() - 15.;
	for(auto it = hints.rbegin(); it != hints.rend(); ++it)
	{
		double hintWidth = font.Width(*it);
		Point hintPos(Screen::BottomRight().X() - hintWidth - 20., hintY);
		font.Draw(*it, hintPos, dim);
		hintY -= lineHeight;
	}
}



void ObserverPanel::DrawGraph() const
{
	// Graph drawing is now integrated into Draw()
}



bool ObserverPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_ESCAPE || command.Has(Command::MENU))
	{
		GetUI()->Pop(this);
		return true;
	}

	// Observer mode specific commands (configurable in Preferences)
	if(command.Has(Command::OBSERVER_CYCLE_CAMERA))
	{
		CycleCamera();
		return true;
	}

	if(command.Has(Command::OBSERVER_CYCLE_TARGET))
	{
		// Select new target in current mode (uses virtual CycleTarget)
		cameraController->CycleTarget();
		return true;
	}

	if(command.Has(Command::OBSERVER_NEXT_SYSTEM))
	{
		SwitchToNewSystem();
		return true;
	}

	if(command.Has(Command::OBSERVER_PREV_SYSTEM))
	{
		SwitchToPreviousSystem();
		return true;
	}

	if(command.Has(Command::OBSERVER_AUTO_SWITCH))
	{
		autoSwitchEnabled = !autoSwitchEnabled;
		Messages::Add({autoSwitchEnabled ? "Auto-switching enabled." : "Auto-switching disabled.",
			GameData::MessageCategories().Get("info")});
		return true;
	}

	// H key toggles HUD visibility for clean screenshots
	if(key == 'h' || key == 'H')
	{
		showHUD = !showHUD;
		engine.SetHideInterface(!showHUD);
		return true;
	}

	// Pause key - must be handled here since Engine's HandleKeyboardInputs
	// requires a flagship which observer mode doesn't have.
	if(command.Has(Command::PAUSE))
	{
		engine.TogglePause();
		return true;
	}

	// Zoom controls (same as main game)
	if(PanelUtils::HandleZoomKey(key, command, false))
		return true;

	// Number keys 1-5 for direct speed selection
	if(key >= '1' && key <= '5')
	{
		speedLevel = key - '1';
		return true;
	}

	return false;
}



bool ObserverPanel::Scroll(double dx, double dy)
{
	return PanelUtils::HandleZoomScroll(dy);
}



void ObserverPanel::SwitchToNewSystem()
{
	// Save current system to history before switching
	const System *currentSystem = player.GetSystem();
	if(currentSystem)
	{
		// Add to history if not already the most recent
		if(systemHistory.empty() || systemHistory.back() != currentSystem)
		{
			systemHistory.push_back(currentSystem);
			// Limit history size
			while(systemHistory.size() > MAX_SYSTEM_HISTORY)
				systemHistory.pop_front();
		}
	}

	// Reset timers
	systemTimer = 0;
	quietTimer = 0;
	recentActivity = 0;

	// Find a new random system (different from current if possible)
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

	// Save as last system for persistence
	lastSystem = newSystem;

	// Reset camera to follow mode for new system
	cameraMode = 0;
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
	if(newSystem)
		cameraController->SetStellarObjects(newSystem->Objects());

	Messages::Add({"Now observing the " + newSystem->DisplayName() + " system.",
		GameData::MessageCategories().Get("info")});
}



void ObserverPanel::SwitchToPreviousSystem()
{
	if(systemHistory.empty())
	{
		Messages::Add({"No previous system in history.",
			GameData::MessageCategories().Get("info")});
		return;
	}

	// Get the most recent system from history
	const System *prevSystem = systemHistory.back();
	systemHistory.pop_back();

	// Reset timers
	systemTimer = 0;
	quietTimer = 0;
	recentActivity = 0;

	// Move to the previous system
	player.SetSystem(*prevSystem);
	engine.EnterSystem();

	// Save as last system for persistence
	lastSystem = prevSystem;

	// Reset camera to follow mode for new system
	cameraMode = 0;
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
	if(prevSystem)
		cameraController->SetStellarObjects(prevSystem->Objects());

	Messages::Add({"Returned to the " + prevSystem->DisplayName() + " system.",
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



int ObserverPanel::GetSpeedMultiplier() const noexcept
{
	if(engine.IsPaused())
		return 0;
	return SPEED_LEVELS[speedLevel];
}


// Static member definition
constexpr int ObserverPanel::SPEED_LEVELS[];
