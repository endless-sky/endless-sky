/* GamePad.cpp
Copyright (c) 2022 by Kari Pahula
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GamePad.h"

#include "Files.h"

#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <SDL2/SDL.h>

// Make sure these match. Note that GamePad.h can't include SDL_gamecontroller.h
static_assert(GamePad::AXIS_MAX == SDL_CONTROLLER_AXIS_MAX, "AXIS_MAX needs to match SDL_CONTROLLER_AXIS_MAX");
static_assert(GamePad::BUTTON_MAX == SDL_CONTROLLER_BUTTON_MAX, "AXIS_MAX needs to match SDL_CONTROLLER_BUTTON_MAX");

namespace {

	GamePad::Axes g_axes = {};
	enum {TRIGGER_NONE, TRIGGER_POSITIVE, TRIGGER_NEGATIVE} g_triggers[SDL_CONTROLLER_AXIS_MAX] = {};
	struct AxisInfo
	{
		int16_t low;
		int16_t zero;
		int16_t high;
	};
	std::vector<AxisInfo> g_joyAxisInfo;
	GamePad::Buttons g_held = {};

	SDL_GameController* g_gc = nullptr;
	// this is guaranteed to be unique per controller per connection. It changes
	// if the controller disconnects and reconnects. This is different from the
	// index value, which can change or get reused as controllers connect and
	// disconnect from the system.
	SDL_JoystickID g_joystickId = -1; // Id != Idx
	SDL_JoystickGUID g_guid{};
	std::vector<std::pair<std::string, std::string>> g_mapping;
	std::string g_joystickLastInput;
	bool g_captureNextButton = false;
	bool g_captureAxisRange = false;
	
	// Store extra mappings here.
	const char EXTRA_MAPPINGS_FILE[] = "gamepad_mappings.txt";
	const char CONFIG_FILE[] = "gamepad_config.txt";

	// If we have marked a joystick axis, we have to wait for it to dip back
	// below the deadzone before we accept more input (otherwise, it will
	// repeatedly flag the same joystick events as new inputs)
	int g_lastAxis = -1;
	int g_DeadZone = 5000;
	// threshold before we count an axis as a binary input
	int g_AxisIsButtonThreshold = 24576;

	std::string g_event_debug[10];
	size_t g_event_debug_idx = 0;

	// We don't support every button/axis. just mark the ones we do care about
	const std::set<SDL_GameControllerButton> g_UsedButtons = {
		SDL_CONTROLLER_BUTTON_A,
		SDL_CONTROLLER_BUTTON_B,
		SDL_CONTROLLER_BUTTON_X,
		SDL_CONTROLLER_BUTTON_Y,
		SDL_CONTROLLER_BUTTON_BACK,
		SDL_CONTROLLER_BUTTON_GUIDE,
		SDL_CONTROLLER_BUTTON_START,
		SDL_CONTROLLER_BUTTON_LEFTSTICK,
		SDL_CONTROLLER_BUTTON_RIGHTSTICK,
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

	void AddEventDebugString(const std::string& s)
	{
		// SDL_Log("%s", s.c_str());
		g_event_debug[g_event_debug_idx] = s;

		++g_event_debug_idx;
		if(g_event_debug_idx == sizeof(g_event_debug) / sizeof(*g_event_debug))
			g_event_debug_idx = 0;
	}

	void ConsolidateMappingAxes()
	{
		// We will have created mapping axes in pairs, but we can combine them
		// if the controller and joystick axes match up.
		std::map<std::string, std::string> axisMap;
		std::vector<std::string> to_remove;
		std::vector<std::pair<std::string, std::string>> to_add;
		auto opposite = [](const std::string& s) {
			// if we get -rightx, we want +rightx here
			if(!s.empty())
			{
				if(s.front() == '-') return '+' + s.substr(1);
				if(s.front() == '+') return '-' + s.substr(1);
			}
			return std::string();
		};
		for(const auto& kv: g_mapping)
		{
			if(kv.first == "+rightx" || kv.first == "-rightx" ||
			   kv.first == "+righty" || kv.first == "-righty" ||
			   kv.first == "+leftx" || kv.first == "-leftx" ||
			   kv.first == "+lefty" || kv.first == "-lefty") // don't worry about triggers
			{
				std::string o = opposite(kv.first);
				auto it = axisMap.find(o);
				if(it != axisMap.end() && it->second == opposite(kv.second))
				{
					// we found both halves of the same axis. Combine them.
					to_remove.push_back(kv.first);
					to_remove.push_back(o);
					// some devices have their axes backwards.
					const std::string flipped = kv.first.front() == kv.second.front() ? "" : "~";
					to_add.push_back(std::make_pair(kv.first.substr(1), kv.second.substr(1) + flipped));
				}
				else
					axisMap.insert(kv);
			}
		}
		for(const std::string& r: to_remove)
		{
			for(auto it = g_mapping.begin(); it != g_mapping.end();)
			{
				if(it->first == r)
					it = g_mapping.erase(it);
				else
					++it;
			}
		}
		for(const auto& kv: to_add)
		{
			g_mapping.push_back(kv);
		}
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
	SDL_RWops* in = SDL_RWFromFile((Files::Config() / EXTRA_MAPPINGS_FILE).c_str(), "rb");

	if(in)
	{
		// This function call closes the RWops when it is done
		SDL_GameControllerAddMappingsFromRW(in, 1);
	}

	// Read any additional config options.
	in = SDL_RWFromFile((Files::Config() / CONFIG_FILE).c_str(), "rb");

	if(in)
	{
		ssize_t pos = SDL_RWseek(in, 0, RW_SEEK_END);
		if(pos > 0)
		{
			std::vector<char> buffer(pos);
			SDL_RWseek(in, 0, RW_SEEK_SET);
			if(SDL_RWread(in, buffer.data(), 1, buffer.size()) == buffer.size())
			{
				buffer.push_back(0);
				for(const std::string& line: StringSplit(buffer.data(), '\n'))
				{
					auto kv = StringSplit(line.c_str(), ' ');
					if(kv.size() == 2)
					{
						try
						{
							if(kv.front() == "dead_zone")
								g_DeadZone = std::stoul(kv.back());
							else if(kv.front() == "trigger_threshold")
								g_AxisIsButtonThreshold = std::stoul(kv.back());
						}
						catch(...)
						{}
					}
				}
			}
		}
		SDL_RWclose(in);
	}
}



// Save the current mapping to the mapping file
void GamePad::SaveMapping()
{
	if(g_gc)
	{
		const std::string MAPPING_FILE_PATH = Files::Config() / EXTRA_MAPPINGS_FILE;
		char guidstr[64];
		SDL_JoystickGetGUIDString(g_guid, guidstr, sizeof(guidstr));

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
		if(!g_mapping.empty())
		{
			std::shared_ptr<char> current_mapping(SDL_GameControllerMapping(g_gc), SDL_free);
			SDL_RWwrite(out.get(), current_mapping.get(), 1, strlen(current_mapping.get()));
			SDL_RWwrite(out.get(), ",", 1, 1);
		}
	}
}



void GamePad::SaveConfig()
{
	const std::string CONFIG_FILE_PATH = Files::Config() / CONFIG_FILE;
	std::shared_ptr<SDL_RWops> out(SDL_RWFromFile(CONFIG_FILE_PATH.c_str(), "wb"), [](SDL_RWops* p) { if(p) SDL_RWclose(p); });

	if(out)
	{
		std::string config =
			"dead_zone " + std::to_string(g_DeadZone) + "\n" +
			"trigger_threshold " + std::to_string(g_AxisIsButtonThreshold) + "\n"
		;
		SDL_RWwrite(out.get(), config.data(), 1, config.size());
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
		if(g_triggers[event.caxis.axis] != TRIGGER_NONE)
		{
			if(event.caxis.value < SDL_abs(g_DeadZone))
				g_triggers[event.caxis.axis] = TRIGGER_NONE;
		}
		else if(g_triggers[event.caxis.axis] == TRIGGER_NONE)
		{
			if(event.caxis.value > g_AxisIsButtonThreshold)
				g_triggers[event.caxis.axis] = TRIGGER_POSITIVE;
			else if(event.caxis.value < -g_AxisIsButtonThreshold)
				g_triggers[event.caxis.axis] = TRIGGER_NEGATIVE;
		}
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
		AddEventDebugString("Axis " + std::to_string(static_cast<int>(event.jaxis.axis)) + " " + std::to_string(event.jaxis.value));
		if(g_captureAxisRange)
		{
			if(event.jaxis.axis < g_joyAxisInfo.size())
			{
				// I'm not sure this information is useful, but cache it anyways,
				// just in case. the important info is the zero value, which we
				// collect when calibration stops.
				if(event.jaxis.value < g_joyAxisInfo[event.jaxis.axis].low)
					g_joyAxisInfo[event.jaxis.axis].low = event.jaxis.value;
				if(event.jaxis.value > g_joyAxisInfo[event.jaxis.axis].high)
					g_joyAxisInfo[event.jaxis.axis].high = event.jaxis.value;
			}
		}
		else if(g_lastAxis != -1)
		{
			if(event.jaxis.axis == g_lastAxis)
			{
				if(event.jaxis.axis < g_joyAxisInfo.size() && g_joyAxisInfo[event.jaxis.axis].zero < -g_DeadZone)
				{
					// This is a trigger, not a joystick. its resting zero-value is -32767
					if(event.jaxis.value < g_joyAxisInfo[event.jaxis.axis].zero + g_DeadZone)
					{
						// trigger has returned to center. allow new axis inputs
						AddEventDebugString("...Released");
						g_lastAxis = -1;
					}
				}
				else if(event.jaxis.value < g_DeadZone && event.jaxis.value > -g_DeadZone)
				{
					// axis has returned to center. allow new axis inputs
					AddEventDebugString("...Released");
					g_lastAxis = -1;
				}
			}
		}
		else if(g_captureNextButton && g_lastAxis == -1)
		{
			if(event.jaxis.value > g_AxisIsButtonThreshold)
			{
				AddEventDebugString("...Triggered");
				g_joystickLastInput = "+a" + std::to_string(event.jaxis.axis);
				g_captureNextButton = false;
				g_lastAxis = event.jaxis.axis;
			}
			else if(event.jaxis.value < -g_AxisIsButtonThreshold &&
			        event.jaxis.axis < g_joyAxisInfo.size() &&    // only look at negative joysticks
			        g_joyAxisInfo[event.jaxis.axis].zero == 0)    // not negative trigger values
			{
				AddEventDebugString("...Triggered");
				g_joystickLastInput = "-a" + std::to_string(event.jaxis.axis);
				g_captureNextButton = false;
				g_lastAxis = event.jaxis.axis;
			}
		}

		break;
	case SDL_JOYBUTTONDOWN:
		AddEventDebugString("Button " + std::to_string(static_cast<int>(event.jbutton.button)) + " Down");
		// SDL_Log("Joystick button %d down", event.jbutton.button);
		if(g_captureNextButton)
		{
			g_joystickLastInput = "b" + std::to_string(event.jbutton.button);
			g_captureNextButton = false;
		}
		break;
	case SDL_JOYBUTTONUP:
		AddEventDebugString("Button " + std::to_string(static_cast<int>(event.jbutton.button)) + " Up");
		// SDL_Log("Joystick button %d up", event.jbutton.button);
		break;
	case SDL_JOYHATMOTION:
		AddEventDebugString("Hat " + std::to_string(static_cast<int>(event.jhat.hat)) + " mask " + std::to_string(static_cast<int>(event.jhat.value)));
		// Hats are weird. They are a mask indicating which bits are held, so
		// we need to know what was *previously* set to know what changed.
		if(g_captureNextButton)
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
				g_joystickLastInput = "h" + std::to_string(event.jhat.hat)
				                      + "." + std::to_string(event.jhat.value);
				g_captureNextButton = false;
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
	SaveConfig();
}



int GamePad::AxisIsButtonPressThreshold()
{
	return g_AxisIsButtonThreshold;
}



void GamePad::SetAxisIsButtonPressThreshold(int t)
{
	g_AxisIsButtonThreshold = t;
	SaveConfig();
}



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



bool GamePad::Trigger(uint8_t axis, bool positive)
{
	return g_triggers[axis] == (positive ? TRIGGER_POSITIVE : TRIGGER_NEGATIVE);
}



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



void GamePad::ResetMappings()
{
	// SDL doesn't really provide a way to remove a mapping. The only way I know
	// of to clear it is to remove it from the config file, then restart the
	// game controller subsystem.
	if (g_joystickId != -1)
	{
		ClearMappings();
		SaveMapping(); // removes this device from the config file

		// Need to force the game controller subsystem to shutdown and restart
		RemoveController();
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
		SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
		Init();
	}
}



void GamePad::CaptureNextJoystickInput()
{
	g_captureNextButton = true;
	g_joystickLastInput.clear();
}



const std::string& GamePad::GetNextJoystickInput()
{
	return g_joystickLastInput;
}



void GamePad::SetControllerButtonMapping(const std::string& controllerButton, const std::string& joystickButton)
{
	if(controllerButton.empty() || joystickButton.empty())
		return;
	
	// TODO: should we be merging/splitting axes?
	// 1. If a +joyaxis and -joyaxis are assigned to the +controlleraxis and -controlleraxis, then merge them
	// 2. if a -joyaxis and +joyaxis are assigend to +controlleraxis and -controlleraxis, then merge them, and mark them as backwards
	// 3. If a +joyaxis is added and joyaxis already exists, then we need to split them.

	
	g_mapping.emplace_back(controllerButton, joystickButton);
	ConsolidateMappingAxes();

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

	//std::shared_ptr<char> mappingStr(SDL_GameControllerMapping(g_gc), SDL_free);
	//SDL_Log("Updating controller mapping: %s", mappingStr.get());
}



// Calibrate the joystick axes
void GamePad::BeginAxisCalibration()
{
	// We need to know the range and zero setting of each axis to determine if
	// it is a joystick or an analog trigger. Let the user wiggle the axes
	// around and capture the high and low setting for each one.
	SDL_Joystick* js = SDL_JoystickFromInstanceID(g_joystickId);
	g_joyAxisInfo.resize(SDL_JoystickNumAxes(js));
	for(size_t i = 0; i < g_joyAxisInfo.size(); ++i)
		g_joyAxisInfo[i] = AxisInfo{};

	g_captureAxisRange = true;
}



// Done calibrating joystick axes
void GamePad::EndAxisCalibration()
{
	// Assume that the user followed instructions, and wiggled all of the axes.
	// SDL should now know what the current value of each axis is (it initially
	// doesn't, which is why we can't do this automatically).
	SDL_Joystick* js = SDL_JoystickFromInstanceID(g_joystickId);
	g_joyAxisInfo.resize(SDL_JoystickNumAxes(js)); // just in case
	for(size_t i = 0; i < g_joyAxisInfo.size(); ++i)
	{
		int16_t value = SDL_JoystickGetAxis(js, i);
		if(-g_DeadZone < value && value < g_DeadZone)
		{
			g_joyAxisInfo[i].zero = 0;
			AddEventDebugString("Axis " + std::to_string(i) + " is a joystick\n");
		}
		else
		{
			g_joyAxisInfo[i].zero = value;
			AddEventDebugString("Axis " + std::to_string(i) + " is a trigger\n");
		}
	}
	g_captureAxisRange = false;
}



const char* GamePad::AxisDescription(uint8_t axis)
{
	return SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(axis));
}



const char* GamePad::ButtonDescription(uint8_t button)
{
	return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
}



void GamePad::DebugEvents(DebugStrings v)
{
	size_t idx = g_event_debug_idx;
	for(int i = 0; i < 10; ++i)
	{
		v[i] = g_event_debug[idx].c_str();
		++idx;
		if(idx == sizeof(g_event_debug) / sizeof(*g_event_debug))
			idx = 0;
	}
}
