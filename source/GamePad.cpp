/* GamePad.cpp
Copyright (c) 2022 by Kari Pahula

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GamePad.h"
#include "Files.h"

//#include "pi.h"
//#include "Preferences.h"

#include <SDL_gamecontroller.h>
#include <SDL_guid.h>
#include <SDL_rwops.h>
#include <SDL_stdinc.h>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>

#include <SDL2/SDL.h>
#include <string>

namespace {

	GamePad::Axes g_axes = {};
	GamePad::Buttons g_held = {};

	// int g_jsDeviceId = -1;
	// SDL_Joystick* g_js = nullptr;
	SDL_GameController* g_gc = nullptr;
	// this is guaranteed to be unique per controller per connection. It changes
	// if the controller disconnects and reconnects. This is different from the
	// index value, which can change or get reused as controllers connect and
	// disconnect from the system.
	SDL_JoystickID g_joystickId = -1; // Id != Idx
	SDL_JoystickGUID g_guid{};
	std::vector<std::pair<std::string, std::string>> g_mapping;
	std::string g_joystick_last_input;
	bool g_capture_next_button = false;

	// I keep hitting a bug in the linux nintendo joystick controller that
	// rapidly triggers input at the polling rate.
	uint32_t g_last_input_ticks = 0;
	const uint32_t RAPID_INPUT_COOLDOWN = 100; // ms
	
	// Store extra mappings here.
	const char EXTRA_MAPPINGS_FILE[] = "gamepad_mappings.txt";


	// If we have marked a joystick axis, we have to wait for it to dip back
	// below the deadzone before we accept more input (otherwise, it will
	// repeatedly flag the same joystick events as new inputs)
	int g_last_axis = -1;
	int g_DeadZone = 5000; // ~ 20% of range. TODO: This should be configurable
	// threshold before we count an axis as a binary input
	int g_AxisIsButtonThreshold = 25000;

	// We don't support every button/axis. just mark the ones we do care about
	const std::set<SDL_GameControllerButton> g_UsedButtons = {
		SDL_CONTROLLER_BUTTON_A,
		SDL_CONTROLLER_BUTTON_B,
		SDL_CONTROLLER_BUTTON_X,
		SDL_CONTROLLER_BUTTON_Y,
		SDL_CONTROLLER_BUTTON_BACK,
		SDL_CONTROLLER_BUTTON_GUIDE,
		SDL_CONTROLLER_BUTTON_START,
		// SDL_CONTROLLER_BUTTON_LEFTSTICK,
		// SDL_CONTROLLER_BUTTON_RIGHTSTICK,
		SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
		SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
		SDL_CONTROLLER_BUTTON_DPAD_UP,
		SDL_CONTROLLER_BUTTON_DPAD_DOWN,
		SDL_CONTROLLER_BUTTON_DPAD_LEFT,
		SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	};
	const std::set<SDL_GameControllerAxis> g_UsedAxes = {
		SDL_CONTROLLER_AXIS_LEFTX,
		SDL_CONTROLLER_AXIS_LEFTY,
		SDL_CONTROLLER_AXIS_RIGHTX,
		SDL_CONTROLLER_AXIS_RIGHTY,
		SDL_CONTROLLER_AXIS_TRIGGERLEFT,
		SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
	};

	// TODO: this needs to be in a header somewhere
	std::vector<std::string> StringSplit(const char* s, char d)
	{
		std::vector<std::string> ret;
		const char* p = s;
		while(p && *p)
		{
			while (*p && *p != d) ++p;
			ret.emplace_back(s, p);
			if(*p) ++p;
			s = p;
		}
		return ret;
	}

	// Add a controller. Index should already have been validated
	void AddController(int idx)
	{
		g_gc = SDL_GameControllerOpen(idx);
		g_joystickId = SDL_JoystickGetDeviceInstanceID(idx);
		g_guid = SDL_JoystickGetDeviceGUID(idx);
		std::shared_ptr<char> mappingStr(SDL_GameControllerMapping(g_gc), SDL_free);
		const std::vector<std::string> mapList = StringSplit(mappingStr.get(), ',');
		if(mapList.size() > 2)
		{
			auto it = mapList.begin();
			++it; ++it; // skip past guid/description
			g_mapping.clear();
			for(; it != mapList.end(); ++it)
			{
				auto keyValue = StringSplit(it->c_str(), ':');
				if(keyValue.size() == 2)
				{
					g_mapping.emplace_back(keyValue.front(), keyValue.back());
				}
			}
		}

		SDL_Log("Added controller %s", SDL_GameControllerName(g_gc));
	}

	// Remove a controller
	void RemoveController()
	{
		SDL_Log("Removing controller %s", SDL_GameControllerName(g_gc));

		SDL_GameControllerClose(g_gc);
		g_joystickId = -1;
		g_gc = nullptr;
		g_guid = SDL_JoystickGUID{};
		g_mapping.clear();
	}

	inline std::string NullCheck(const char* s)
	{
		return s ? s : "";
	}
}



// Init function. Loads any cached gamepad mappings the user has configured
void GamePad::Init()
{
	// TODO: include a game controller database in the assets like this one:
	// https://github.com/gabomdq/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt
	// For now, relying on SDL to auto-configure the game controller based on
	// the identifier, and allowing for user configuration as a fallback.
	// Note that SDL will automatically load a file called
	// 	SDL_AndroidGetInternalStoragePath() + "/controller_map.txt"
	// However, we write our save files to SDL_AndroidGetInternalStoragePath(),
	// and all of the other configs to the parent directory of that folder.

	// Read any mappings the user has created.
	SDL_RWops* in = SDL_RWFromFile((Files::Config() + EXTRA_MAPPINGS_FILE).c_str(), "rb");

	if (in)
	{
		SDL_GameControllerAddMappingsFromRW(in, 1);
	}
}



// Save the current mapping to the mapping file
void GamePad::SaveMapping()
{
	if(g_gc)
	{
		const std::string MAPPING_FILE_PATH = Files::Config() + EXTRA_MAPPINGS_FILE;
		char guidstr[64];
		SDL_GUIDToString(g_guid, guidstr, sizeof(guidstr));

		// Load the existing entries into memory
		std::vector<char> existing_entries;
		std::shared_ptr<SDL_RWops> in(SDL_RWFromFile(MAPPING_FILE_PATH.c_str(), "rb"), [](SDL_RWops* p) { if(p) SDL_RWclose(p); });
		if(in)
		{
			existing_entries.resize(SDL_RWsize(in.get()) + 1);
			size_t bytes_read = SDL_RWread(in.get(), existing_entries.data(), 1, existing_entries.size());
			existing_entries.resize(bytes_read + 1);
			existing_entries[bytes_read] = 0;
		}
		in.reset();

		std::shared_ptr<SDL_RWops> out(SDL_RWFromFile(MAPPING_FILE_PATH.c_str(), "wb"), [](SDL_RWops* p) { if(p) SDL_RWclose(p); });
		// write any existing entries back to the file
		if(!existing_entries.empty())
		{
			size_t guidlen = strlen(guidstr);
			for(auto& s: StringSplit(existing_entries.data(), '\n'))
			{
				if(s.empty())
					continue;
				// if this has the same guid we are updating, then drop this line
				if(s.compare(0, guidlen, guidstr) == 0)
					continue;
				SDL_RWwrite(out.get(), s.data(), 1, s.size());
				SDL_RWwrite(out.get(), "\n", 1, 1);
			}
		}
		std::shared_ptr<char> current_mapping(SDL_GameControllerMapping(g_gc), SDL_free);
		SDL_RWwrite(out.get(), current_mapping.get(), 1, strlen(current_mapping.get()));
		SDL_RWwrite(out.get(), ",", 1, 1);
	}
}



// Handle an event. Will update GamePad state for further queries.
void GamePad::Handle(const SDL_Event &event)
{
	switch(event.type)
	{
	case SDL_CONTROLLERDEVICEADDED:
		// For this event, event.cdevice.which is the joystick *index* (not instance id)
		if(!g_gc)
		{
			AddController(event.cdevice.which);
		}
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		// For this event, event.cdevice.which is the joystick *instance id* (not index)
		if(g_gc && event.cdevice.which == g_joystickId)
		{
			// Our controller has disconnected
			RemoveController();

			// TODO: log warning to screen?
			int num_joysticks = SDL_NumJoysticks();
			if(num_joysticks > 0)
			{
				// Select the most recently added controller instead
				AddController(num_joysticks - 1);
			}
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		//SDL_Log("Axis %d: %d", static_cast<int>(event.caxis.axis), static_cast<int>(event.caxis.value));
		g_axes[event.caxis.axis] = event.caxis.value;
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		//SDL_Log("Button down: %d", static_cast<int>(event.cbutton.button));
		g_held[event.cbutton.button] = true;
		break;
	case SDL_CONTROLLERBUTTONUP:
		//SDL_Log("Button up: %d", static_cast<int>(event.cbutton.button));
		g_held[event.cbutton.button] = false;
		break;
	
	// cache joypad events if we are doing remapping
	case SDL_JOYAXISMOTION:
		if(g_last_axis != -1)
		{
			if(event.jaxis.axis == g_last_axis && event.jaxis.value < g_DeadZone && event.jaxis.value > -g_DeadZone)
			{
				// axis has returned to center. allow new axis inputs
				g_last_axis = -1;
			}
		}
		else if(g_capture_next_button && g_last_axis == -1)
		{
			if(event.jaxis.value > g_AxisIsButtonThreshold)
			{
				g_joystick_last_input = "+a" + std::to_string(event.jaxis.axis);
				g_capture_next_button = false;
				g_last_axis = event.jaxis.axis;
			}
			else if(event.jaxis.value < -g_AxisIsButtonThreshold)
			{
				g_joystick_last_input = "-a" + std::to_string(event.jaxis.axis);
				g_capture_next_button = false;
				g_last_axis = event.jaxis.axis;
			}
		}

		break;
	case SDL_JOYBUTTONDOWN:
		// SDL_Log("Joystick button %d down", event.jbutton.button);
		break;
	case SDL_JOYBUTTONUP:
		// SDL_Log("Joystick button %d up", event.jbutton.button);
		if(g_capture_next_button && SDL_GetTicks() - g_last_input_ticks > RAPID_INPUT_COOLDOWN)
		{
			g_joystick_last_input = "b" + std::to_string(event.jbutton.button);
			g_capture_next_button = false;
		}
		g_last_input_ticks = SDL_GetTicks();
		break;
	case SDL_JOYHATMOTION:
		// Hats are weird. They are a mask indicating which bits are held, so
		// we need to know what was *previously* set to know what changed.
		if(g_capture_next_button)
		{
			// for buttons, we capture button-up events, but hats, in theory,
			// can be switches, so allow the user to toggle it and leave it.
			if(event.jhat.value)
			{
				// A hat has been turned on. Save off the value as the hat mask
				// TODO: event.jhat.value is a mask indicating which hat switches
				//       are turned on... should we restrict them to just one bit?
				//       It probably doesn't matter, since most controller hats are
				//       actually dpad directional buttons, and the user will only
				//       press one at a time during mapping operations.
				g_joystick_last_input = "h" + std::to_string(event.jhat.hat)
				                      + "." + std::to_string(event.jhat.value);
				g_capture_next_button = false;
			}
		}
		break;
	}
}



int GamePad::DeadZone()
{
	return g_DeadZone;
}



void GamePad::SetDeadZone(int dz)
{
	g_DeadZone = dz;
}



int GamePad::AxisIsButtonPressThreshold()
{
	return g_AxisIsButtonThreshold;
}



void GamePad::SetAxisIsButtonPressThreshold(int t)
{
	g_AxisIsButtonThreshold = t;
}




// Button counts as held if there's been a down event but no corresponding up event yet.
//map<Uint8, chrono::milliseconds> GamePad::HeldButtons() const
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
//	std::map<Uint8, chrono::milliseconds> result;
//	for(auto it = held.cbegin(); it != held.cend(); ++it)
//	{
//		if(released.count(it->first) == 0)
//			result[it->first] = chrono::duration_cast<chrono::milliseconds>(now - it->second);
//	}
//	return result;
//}



//map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> GamePad::HeldButtonsSince() const
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> result(held);
//	for(auto it = result.begin(); it != result.cend();)
//	{
//		if(released.find(it->first) != released.cend())
//			it = result.erase(it);
//		else
//			++it;
//	}
//	return result;
//}



//std::map<Uint8, chrono::milliseconds> GamePad::ReleasedButtons() const
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	std::map<Uint8, chrono::milliseconds> result;
//	for(auto it = released.cbegin(); it != released.cend(); ++it)
//	{
//		if(unreportedReleases.find(it->first) != unreportedReleases.cend())
//		{
//			auto start = held.find(it->first);
//			result[it->first] = start != held.cend() ?
//				chrono::duration_cast<chrono::milliseconds>(it->second - start->second) :
//					chrono::milliseconds::zero();
//		}
//	}
//	return result;
//}



//bool GamePad::Held(Uint8 button) const
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	return held.find(button) != held.cend() && released.find(button) == released.cend();
//}



//void GamePad::Clear()
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	held.clear();
//	released.clear();
//	unreportedReleases.clear();
//}



//void GamePad::Clear(const set<Uint8> &toClear)
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	for(auto it = toClear.cbegin(); it != toClear.cend(); ++it)
//	{
//		held.erase(*it);
//		released.erase(*it);
//		unreportedReleases.erase(*it);
//	}
//}



//void GamePad::Clear(Uint8 button)
//{
//	lock_guard<mutex> lock(gamePadMutex);
//	held.erase(button);
//	released.erase(button);
//	unreportedReleases.erase(button);
//}



//bool GamePad::HavePads() const
//{
//	return activePads > 0;
//}



//std::set<Uint8> GamePad::ReadHeld(const std::set<Uint8> &interest)
//{
//	std::set<Uint8> result;
//	lock_guard<mutex> lock(gamePadMutex);
//	for(auto it = held.cbegin(); it != held.cend(); ++it)
//		if(released.find(it->first) == released.cend() && interest.find(it->first) != interest.cend())
//			result.insert(it->first);
//	return result;
//}
const GamePad::Buttons& GamePad::Held()
{
	return g_held;
}



const GamePad::Axes& GamePad::Positions()
{
	return g_axes;
}



Point GamePad::LeftStick()
{
	Point p(g_axes[SDL_CONTROLLER_AXIS_LEFTX], g_axes[SDL_CONTROLLER_AXIS_LEFTY]);
	return p.LengthSquared() < g_DeadZone * g_DeadZone ? Point() : p;
}



Point GamePad::RightStick()
{
	Point p(g_axes[SDL_CONTROLLER_AXIS_RIGHTX], g_axes[SDL_CONTROLLER_AXIS_RIGHTY]);
	return p.LengthSquared() < g_DeadZone * g_DeadZone ? Point() : p;
}



//double GamePad::LeftStickX() const
//{
//	return axis[SDL_CONTROLLER_AXIS_LEFTX];
//}



//double GamePad::LeftStickY() const
//{
//	return axis[SDL_CONTROLLER_AXIS_LEFTY];
//}



//double GamePad::RightStickX() const
//{
//	return axis[SDL_CONTROLLER_AXIS_RIGHTX];
//}



//double GamePad::RightStickY() const
//{
//	return axis[SDL_CONTROLLER_AXIS_RIGHTY];
//}



bool GamePad::LeftTrigger()
{
	return g_axes[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > g_AxisIsButtonThreshold;
}



bool GamePad::RightTrigger()
{
	return g_axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > g_AxisIsButtonThreshold;
}



//bool GamePad::RepeatAxis(Uint8 which)
//{
//	double threshold;
//	double gradient = axis[which];
//	if(gradient > 0.5)
//		threshold = 1500;
//	else
//		return false;
//	return Repeat(which, threshold, which + SDL_CONTROLLER_AXIS_MAX);
//}



//bool GamePad::RepeatAxisNeg(Uint8 which)
//{
//	double threshold;
//	double gradient = axis[which];
//	if(gradient < -0.5)
//		threshold = 1500;
//	else
//		return false;
//	return Repeat(which + SDL_CONTROLLER_AXIS_MAX, threshold);
//}



//bool GamePad::RepeatButton(Uint8 button)
//{
//	if(held.find(button) == held.cend())
//		return false;
//	else if(released.find(button) != released.cend())
//		return false;
//	else
//		return Repeat(button + SDL_CONTROLLER_AXIS_MAX * 2, 200.);
//}



//Command GamePad::ToCommand()
//{
//	Command command;
//	std::map<Uint8, std::chrono::milliseconds> heldButtons = HeldButtons();
//	std::map<Uint8, std::chrono::milliseconds> releasedButtons = ReleasedButtons();
//	lock_guard<mutex> lock(gamePadMutex);
//	// Overloaded buttons with short press commands
//	for(auto it = releasedButtons.cbegin(); it != releasedButtons.cend(); ++it)
//	{
//		if(it->first == SDL_CONTROLLER_BUTTON_B
//				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
//			command.Set(Command::LAND);
//		else if(it->first == SDL_CONTROLLER_BUTTON_X
//				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
//			command.Set(Command::CLOAK);
//		else if(it->first == SDL_CONTROLLER_BUTTON_Y
//				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
//		{
//			command.Set(Command::BOARD);
//		}
//		else
//			continue;
//		held.erase(it->first);
//		released.erase(it->first);
//	}
//
//	// Single click commands and held or long press triggered alternative commands
//	for(auto it = heldButtons.cbegin(); it != heldButtons.cend(); ++it)
//	{
//		if(it->first == SDL_CONTROLLER_BUTTON_A)
//			command.Set(Command::JUMP);
//		else if(it->first == SDL_CONTROLLER_BUTTON_X
//				&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
//			command.Set(Command::SCAN);
//		else if(it->first == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
//			command.Set(Command::SECONDARY);
//		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
//			command.Set(Command::PRIMARY);
//		else if(it->first >= SDL_CONTROLLER_BUTTON_DPAD_UP
//				&& it->first <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
//		{
//			if(it->first == SDL_CONTROLLER_BUTTON_DPAD_UP)
//				command.Set(Command::SELECT);
//			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
//				command.Set(Command::NEAREST);
//			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
//				command.Set(Command::TARGET);
//			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
//				command.Set(Command::TARGET | Command::SHIFT);
//			held.erase(it->first);
//		}
//		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSTICK)
//		{
//			Command rightStickCommand = RightStickCommand();
//			if(rightStickCommand && !rightStickCommand.Has(Command::AMMO))
//			{
//				command |= rightStickCommand;
//				held.erase(it->first);
//			}
//		}
//	}
//	//Point leftStick = LeftStick();
//	//if(leftStick.LengthSquared() > GamePad::VECTOR_TURN_THRESHOLD)
//	//	command.SetTurnPoint(leftStick);
//	//if(axis[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > 0.1)
//	//{
//	//	double thrust = axis[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] * THRUST_MULTI;
//	//	command.SetThrustGradient(thrust);
//	//	if(thrust >= 1. && axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 0.5)
//	//		command.Set(Command::AFTERBURNER);
//	//}
//	//else if(axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 0.1)
//	//{
//	//	double thrust = -axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] * THRUST_MULTI;
//	//	command.SetThrustGradient(thrust);
//	//}
//	return command;
//}



//Command GamePad::ToPanelCommand()
//{
//	Command command;
//	std::map<Uint8, std::chrono::milliseconds> heldButtons = HeldButtons();
//	lock_guard<mutex> lock(gamePadMutex);
//	for(std::map<Uint8, std::chrono::milliseconds>::const_iterator it = heldButtons.cbegin(); it != heldButtons.cend(); ++it)
//	{
//		if(it->first == SDL_CONTROLLER_BUTTON_B
//			&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
//			command.Set(Command::SHIFT | Command::HAIL);
//		else if(it->first == SDL_CONTROLLER_BUTTON_Y
//				&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
//			command.Set(Command::HAIL);
//		else if(it->first == SDL_CONTROLLER_BUTTON_BACK)
//			command.Set(Command::INFO);
//		else if(it->first == SDL_CONTROLLER_BUTTON_LEFTSTICK)
//			command.Set(Command::MAP);
//		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSTICK && RightStickCommand().Has(Command::AMMO))
//			command.Set(Command::AMMO);
//		else
//			continue;
//		held.erase(it->first);
//	}
//
//	return command;
//}



//Command GamePad::RightStickCommand() const
//{
//	Command command;
//	Point stick = RightStick();
//	if(stick.LengthSquared() > GamePad::VECTOR_TURN_THRESHOLD)
//	{
//		double deg = std::atan2(stick.Y(), stick.X()) * TO_DEG;
//		double degAbs = fabs(deg);
//		if(degAbs >= 150.)
//			command.Set(Command::DEPLOY);
//		else if(degAbs <= 30.)
//			command.Set(Command::BOARD | Command::SHIFT);
//		else if(deg < 150. && deg >= 90.)
//			command.Set(Command::HOLD);
//		else if(deg < 90. && deg > 30.)
//			command.Set(Command::GATHER);
//		else if(deg < -30. && deg >= -90.)
//			command.Set(Command::FIGHT);
//		else
//			command.Set(Command::AMMO);
//	}
//	return command;
//}



//bool GamePad::Repeat(Uint8 which, double threshold, Uint8 opposite)
//{
//	auto it = repeatTimer.find(which);
//	chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
//	if(it != repeatTimer.cend())
//	{
//		threshold *= Preferences::ScrollSpeed() / 60.;
//		int diff = chrono::duration_cast<chrono::milliseconds>(now - it->second).count();
//		if(diff > threshold)
//			repeatTimer[which] = now;
//		else
//			return false;
//	}
//	else
//		repeatTimer[which] = chrono::steady_clock::now();
//	// Releasing a stick quickly may cause opposite spikes.
//	if(opposite != 255)
//	{
//		chrono::time_point<std::chrono::steady_clock> adjust = chrono::steady_clock::now();
//		now -= chrono::milliseconds(int(threshold) - 100);
//		repeatTimer[opposite] = adjust;
//	}
//	return true;
//}



std::vector<std::pair<std::string, std::string>> GamePad::GetCurrentSdlMappings()
{
	std::vector<std::pair<std::string, std::string>> ret;
	if(g_gc)
	{
		// Return g_mapping, but restrict this to buttons we actually use in-game
		for(auto& kv: g_mapping)
		{
			auto axis = SDL_GameControllerGetAxisFromString(kv.first.c_str());
			auto button = SDL_GameControllerGetButtonFromString(kv.first.c_str());
			if(g_UsedButtons.count(button) || g_UsedAxes.count(axis))
				ret.emplace_back(kv);
		}
	}
	return ret;
}



std::vector<std::string> GamePad::GetControllerList()
{
	std::vector<std::string> ret;
	int num_joysticks = SDL_NumJoysticks();
	if(num_joysticks > 0)
	{
		for(int i = 0; i < num_joysticks; ++i)
		{
			const char* controllerName = SDL_GameControllerNameForIndex(i);
			if(controllerName)
			{
				ret.emplace_back(controllerName);
			}
			else
			{
				// there is no controller name. Use the guid instead.
				// TODO: this is a stupid choice. maybe "Controller %d" instead?
				SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
				char buffer[64];
				SDL_JoystickGetGUIDString(guid, buffer, sizeof(buffer));
				ret.emplace_back(buffer);
			}
		}
	}
	return ret;
}



int GamePad::CurrentControllerIdx()
{
	int ret = -1;
	// indexes change. id's persist for the connected lifetime of the device.
	int numJoysticks = SDL_NumJoysticks();
	if(numJoysticks > 0)
	{
		for(int i = 0; i < numJoysticks; ++i)
		{
			if(SDL_JoystickGetDeviceInstanceID(i) == g_joystickId)
			{
				ret = i;
				break;
			}
		}
	}
	return ret;
}



void GamePad::SetControllerIdx(int idx)
{
	if(idx == CurrentControllerIdx())
	{
		// attempting to reselect our already selected gamepad. Do nothing.
		return;
	}
	else if (idx >= 0 && idx < SDL_NumJoysticks())
	{
		RemoveController();
		AddController(idx);
	}
	else
	{
		SDL_Log("Attempted to select an invalid controller %d", static_cast<int>(idx));
	}
}



void GamePad::ClearMappings()
{
	if (g_joystickId != -1)
	{
		char guid[64];
		SDL_JoystickGetGUIDString(g_guid, guid, sizeof(guid));
		std::string blankMapping = std::string(guid) + "," + NullCheck(SDL_GameControllerName(g_gc)) + ",";
		SDL_GameControllerAddMapping(blankMapping.c_str());
		g_mapping.clear();
	}
}



void GamePad::CaptureNextJoystickInput()
{
	g_capture_next_button = true;
	g_joystick_last_input.clear();
}



const std::string& GamePad::GetNextJoystickInput()
{
	return g_joystick_last_input;
}



void GamePad::SetControllerButtonMapping(const std::string& controllerButton, const std::string& joystickButton)
{
	if(controllerButton.empty() || joystickButton.empty())
		return;
	
	// TODO: should we be merging/splitting axes?
	// 1. If a +joyaxis and -joyaxis are assigned to the +controlleraxis and -controlleraxis, then merge them
	// 2. if a -joyaxis and +joyaxis are assigend to +controlleraxis and -controlleraxis, then merge them, and mark them as backwards
	// 3. If a +joyaxis is added and joyaxis already exists, then we need to split them.
	// auto FindButton = [](const std::string& s)
	// {
	// 	auto it = g_mapping.begin();
	// 	for(; it != g_mapping.end(); ++it)
	// 	{
	// 		if(it->first == s)
	// 			break;
	// 	}
	// 	return it;
	// };
	
	g_mapping.emplace_back(controllerButton, joystickButton);

	// Reconstruct the mapping string
	char guidStr[64];
	SDL_JoystickGetGUIDString(g_guid, guidStr, sizeof(guidStr));

	std::string newMapping = guidStr;
	newMapping += ",";
	newMapping += NullCheck(SDL_GameControllerName(g_gc));
	newMapping += ",";

	for(auto& kv: g_mapping)
	{
		newMapping += kv.first + ":" + kv.second + ",";
	}
	// note that the terminating comma is intentional
	SDL_GameControllerAddMapping(newMapping.c_str());
	
	std::shared_ptr<char> mappingStr(SDL_GameControllerMapping(g_gc), SDL_free);
	//SDL_Log("Updating controller mapping: %s", mappingStr.get());
}


