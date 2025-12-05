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
#include "FollowShipCamera.h"
#include "FreeCamera.h"
#include "GameData.h"
#include "Messages.h"
#include "OrbitPlanetCamera.h"
#include "Random.h"
#include "Screen.h"
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

	// Draw minimal observer HUD
	const Font &font = FontSet::Get(14);
	Color dimColor(0.5f, 0.5f, 0.5f, 1.f);
	Color brightColor(0.8f, 0.8f, 0.8f, 1.f);

	// Camera mode in top-left
	Point hudPos = Screen::TopLeft() + Point(20., 20.);
	string modeText = "Mode: " + cameraController->ModeName();
	font.Draw(modeText, hudPos, dimColor);

	// Target name below mode
	string targetName = cameraController->TargetName();
	if(!targetName.empty())
	{
		hudPos.Y() += 20.;
		font.Draw("Target: " + targetName, hudPos, brightColor);
	}

	// System name
	if(player.GetSystem())
	{
		hudPos.Y() += 20.;
		font.Draw("System: " + player.GetSystem()->DisplayName(), hudPos, dimColor);
	}

	// Controls hint at bottom
	Point hintPos = Screen::BottomLeft() + Point(20., -30.);
	font.Draw("Tab: cycle camera | Space: new target | ESC: exit", hintPos, dimColor);
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

	return false;
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
