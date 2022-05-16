/* main.cpp
Copyright (c) 2014 by Michael Zahniser

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Audio.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataNode.h"
#include "Dialog.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "GameWindow.h"
#include "GameLoadingPanel.h"
#include "Hardpoint.h"
#include "MenuPanel.h"
#include "Outfit.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "RenderState.h"
#include "Screen.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "Test.h"
#include "TestContext.h"
#include "UI.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <thread>

#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

using namespace std;

void PrintHelp();
void PrintVersion();
void GameLoop(PlayerInfo &player, const Conversation &conversation, const string &testToRun, bool debugMode);
Conversation LoadConversation();
void PrintShipTable();
void PrintTestsTable();
void PrintWeaponTable();
#ifdef _WIN32
void InitConsole();
#endif



// Entry point for the EndlessSky executable
int main(int argc, char *argv[])
{
	// Handle command-line arguments
#ifdef _WIN32
	if(argc > 1)
		InitConsole();
#endif
	Conversation conversation;
	bool debugMode = false;
	bool loadOnly = false;
	bool printShips = false;
	bool printTests = false;
	bool printWeapons = false;
	string testToRunName = "";

	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-h" || arg == "--help")
		{
			PrintHelp();
			return 0;
		}
		else if(arg == "-v" || arg == "--version")
		{
			PrintVersion();
			return 0;
		}
		else if(arg == "-t" || arg == "--talk")
			conversation = LoadConversation();
		else if(arg == "-d" || arg == "--debug")
			debugMode = true;
		else if(arg == "-p" || arg == "--parse-save")
			loadOnly = true;
		else if(arg == "--test" && *++it)
			testToRunName = *it;
		else if(arg == "--tests")
			printTests = true;
		else if(arg == "-s" || arg == "--ships")
			printShips = true;
		else if(arg == "-w" || arg == "--weapons")
			printWeapons = true;
	}
	Files::Init(argv);

	try {
		// Begin loading the game data.
		bool isConsoleOnly = loadOnly || printShips || printTests || printWeapons;
		future<void> dataLoading = GameData::BeginLoad(isConsoleOnly, debugMode);

		// If we are not using the UI, or performing some automated task, we should load
		// all data now. (Sprites and sounds can safely be deferred.)
		if(isConsoleOnly || !testToRunName.empty())
			dataLoading.wait();

		if(!testToRunName.empty() && !GameData::Tests().Has(testToRunName))
		{
			Files::LogError("Test \"" + testToRunName + "\" not found.");
			return 1;
		}

		if(printShips || printTests || printWeapons)
		{
			if(printShips)
				PrintShipTable();
			if(printTests)
				PrintTestsTable();
			if(printWeapons)
				PrintWeaponTable();
			return 0;
		}

		PlayerInfo player;
		if(loadOnly)
		{
			// Set the game's initial internal state.
			GameData::FinishLoading();

			// Reference check the universe, as known to the player. If no player found,
			// then check the default state of the universe.
			if(!player.LoadRecent())
				GameData::CheckReferences();
			cout << "Parse completed." << endl;
			return 0;
		}
		assert(!isConsoleOnly && "Attempting to use UI when only data was loaded!");

		// On Windows, make sure that the sleep timer has at least 1 ms resolution
		// to avoid irregular frame rates.
#ifdef _WIN32
		timeBeginPeriod(1);
#endif

		Preferences::Load();

		if(!GameWindow::Init())
			return 1;

		GameData::LoadShaders(!GameWindow::HasSwizzle());

		// Show something other than a blank window.
		GameWindow::Step();

		Audio::Init(GameData::Sources());

		// This is the main loop where all the action begins.
		GameLoop(player, conversation, testToRunName, debugMode);
	}
	catch(const runtime_error &error)
	{
		Audio::Quit();
		bool doPopUp = testToRunName.empty();
		GameWindow::ExitWithError(error.what(), doPopUp);
		return 1;
	}

	// Remember the window state and preferences if quitting normally.
	Preferences::Set("maximized", GameWindow::IsMaximized());
	Preferences::Set("fullscreen", GameWindow::IsFullscreen());
	Screen::SetRaw(GameWindow::Width(), GameWindow::Height());
	Preferences::Save();

	Audio::Quit();
	GameWindow::Quit();

	return 0;
}



void GameLoop(PlayerInfo &player, const Conversation &conversation, const string &testToRunName, bool debugMode)
{
	// gamePanels is used for the main panel where you fly your spaceship.
	// All other game content related dialogs are placed on top of the gamePanels.
	// If there are both menuPanels and gamePanels, then the menuPanels take
	// priority over the gamePanels. The gamePanels will not be shown until
	// the stack of menuPanels is empty.
	UI gamePanels;

	// menuPanels is used for the panels related to pilot creation, preferences,
	// game loading and game saving.
	UI menuPanels;

	// Whether the game data is done loading. This is used to trigger any
	// tests to run.
	bool dataFinishedLoading = false;
	menuPanels.Push(new GameLoadingPanel(player, conversation, gamePanels, dataFinishedLoading));

	bool showCursor = true;
	double cursorTime = 0.;
	bool isPaused = false;

	enum MotionState
	{
		SlowMo,
		Normal,
		FastForward,
	};
	MotionState lastMotion = Normal;
	MotionState motion = Normal;

	// Limit how quickly full-screen mode can be toggled.
	int toggleTimeout = 0;

	// Data to track progress of testing if/when a test is running.
	TestContext testContext;
	if(!testToRunName.empty())
		testContext = TestContext(GameData::Tests().Get(testToRunName));

	// The speed of the physics update loop in ms.
	constexpr auto defaultFps = 1000. / 60.;
	constexpr auto fastForwardFps = 1000. / 180.;
	constexpr auto slowMotionFps = 1000. / 20.;

	// The max amount of frame time between frames in ms.
	constexpr auto frameTimeLimit = 250.;

	// A helper function to get the current time in ms.
	auto CurrentTime = []
	{
		return chrono::duration<double, milli>(chrono::high_resolution_clock::now().time_since_epoch()).count();
	};

	// The current frame rate of the physics loop.
	double updateFps = defaultFps;
	// The current time in ms since epoch. Used to calculate the frame delta.
	auto currentTime = CurrentTime();
	// Used to store any "extra" time after a frame.
	double accumulator = 0.;
	// The previous frame's information for interpolation.
	RenderState previous;

	const auto &font = FontSet::Get(14);
	const int maxFramerate = Preferences::Maxframerate();

	// Used to smooth fps display.
	int cpuLoadCount = 0;
	double cpuLoadSum = 0.;
	string cpuLoad;
	int totalFramesGpu = 0;
	double totalElapsedTimeGpu = 0.;
	string fpsStringGpu;

	// When running tests, fast forward is on by default unless starting the game in debug mode.
	if(!testToRunName.empty() && !debugMode)
		motion = FastForward;
	// IsDone becomes true when the game is quit.
	while(!menuPanels.IsDone())
	{
		auto newTime = CurrentTime();
		double frameTime = newTime - currentTime;
		currentTime = newTime;

		if(frameTime > frameTimeLimit)
			frameTime = frameTimeLimit;
		accumulator += frameTime;

		if(toggleTimeout)
			--toggleTimeout;

		// Handle any events that occurred in this frame.
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);

			// If the mouse moves, reset the cursor movement timeout.
			if(event.type == SDL_MOUSEMOTION)
				cursorTime = 0.;

			if(debugMode && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKQUOTE)
			{
				isPaused = !isPaused;
			}
			else if(event.type == SDL_KEYDOWN && menuPanels.IsEmpty()
					&& Command(event.key.keysym.sym).Has(Command::MENU)
					&& !gamePanels.IsEmpty() && gamePanels.Top()->IsInterruptible())
			{
				// User pressed the Menu key.
				menuPanels.Push(shared_ptr<Panel>(
					new MenuPanel(player, gamePanels)));
			}
			else if(event.type == SDL_QUIT)
			{
				menuPanels.Quit();
			}
			else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				// The window has been resized. Adjust the raw screen size
				// and the OpenGL viewport to match.
				GameWindow::AdjustViewport();
			}
			else if(activeUI.Handle(event))
			{
				// The UI handled the event.
			}
			else if(event.type == SDL_KEYDOWN && !toggleTimeout
					&& (Command(event.key.keysym.sym).Has(Command::FULLSCREEN)
					|| (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))))
			{
				toggleTimeout = 30;
				GameWindow::ToggleFullscreen();
			}
			else if(event.type == SDL_KEYDOWN && !event.key.repeat
					&& (Command(event.key.keysym.sym).Has(Command::FASTFORWARD)))
			{
				switch(motion)
				{
				case SlowMo:
					motion = Normal;
					break;
				case Normal:
					motion = FastForward;
					break;
				case FastForward:
					motion = SlowMo;
					break;
				}
			}
		}
		SDL_Keymod mod = SDL_GetModState();
		Font::ShowUnderlines(mod & KMOD_ALT);

		const bool inFlight = (menuPanels.IsEmpty() && gamePanels.Root() == gamePanels.Top());

		// In full-screen mode, hide the cursor if inactive for ten seconds,
		// but only if the player is flying around in the main view.
		cursorTime += frameTime;
		bool shouldShowCursor = (!GameWindow::IsFullscreen() || cursorTime < 10000. || !inFlight);
		if(shouldShowCursor != showCursor)
		{
			showCursor = shouldShowCursor;
			SDL_ShowCursor(showCursor);
		}

		bool didUpdate = false;
		while(accumulator >= updateFps)
		{
			didUpdate = true;
			auto cpuStart = CurrentTime();

			// We are starting a new physics frame. Cache the last state for interpolation.
			previous = std::move(RenderState::current);

			// Tell all the panels to step forward, then draw them.
			((!isPaused && menuPanels.IsEmpty()) ? gamePanels : menuPanels).StepAll();

			Audio::Step();

			accumulator -= updateFps;

			auto updateTime = CurrentTime() - cpuStart;
			cpuLoadSum += updateTime;
			if(++cpuLoadCount >= lround(1000. / updateFps))
			{
				cpuLoad = to_string(lround(cpuLoadSum / 10.)) + "% CPU";
				cpuLoadCount = 0;
				cpuLoadSum = 0.;
			}

			// Aggresively drop frames if the CPU cannot keep up.
			// This will slow down the game for the player.
			if(updateTime >= updateFps)
				while(accumulator >= updateFps)
					accumulator -= updateFps;
		}

		if(lastMotion != motion)
		{
			switch(motion)
			{
			case SlowMo:
				updateFps = slowMotionFps;
				break;
			case Normal:
				updateFps = defaultFps;
				break;
			case FastForward:
				updateFps = fastForwardFps;
				break;
			}
			lastMotion = motion;
		}

		// Run any tests if the game updated.
		if(didUpdate)
		{
			// All manual events and processing done. Handle any test inputs and events if we have any.
			const Test *runningTest = testContext.CurrentTest();
			if(runningTest && dataFinishedLoading)
			{
				// When flying around, all test processing must be handled in the
				// thread-safe section of Engine. When not flying around (and when no
				// Engine exists), then it is safe to execute the tests from here.
				auto mainPanel = gamePanels.Root().get();
				if(!isPaused && inFlight && menuPanels.IsEmpty() && mainPanel)
					mainPanel->SetTestContext(testContext);
				else
				{
					// The command will be ignored, since we only support commands
					// from within the engine at the moment.
					Command ignored;
					runningTest->Step(testContext, player, ignored);
				}
			}
		}

		const double alpha = accumulator / updateFps;
		// Interpolate the last two physics states. The interpolated state will
		// be used by the drawing code.
		RenderState::Interpolate(previous, alpha);

		// Events in this frame may have cleared out the menu, in which case
		// we should draw the game panels instead:
		(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll(frameTime);
		if(motion == FastForward)
			SpriteShader::Draw(SpriteSet::Get("ui/fast forward"), Screen::TopLeft() + Point(10., 10.));
		else if(motion == SlowMo)
			SpriteShader::Draw(SpriteSet::Get("ui/slow motion"), Screen::TopLeft() + Point(10., 10.));

		if(Preferences::Has("Show CPU / GPU load"))
		{
			if(inFlight)
				font.Draw(cpuLoad,
					Point(-10 - font.Width(cpuLoad), Screen::Height() * -.5 + 5.),
						*GameData::Colors().Get("medium"));

			totalElapsedTimeGpu += frameTime;
			++totalFramesGpu;
			if(totalElapsedTimeGpu >= 250.)
			{
				fpsStringGpu = "GPU: " + to_string(lround(1000. * totalFramesGpu / totalElapsedTimeGpu)) + " FPS";
				totalElapsedTimeGpu = 0.;
				totalFramesGpu = 0;
			}

			if(inFlight)
				font.Draw(fpsStringGpu,
					Point(10, Screen::Height() * -.5 + 5.),
						*GameData::Colors().Get("medium"));
		}

		GameWindow::Step();

		// If the player ended this frame in-game, count the elapsed time as played time.
		if(menuPanels.IsEmpty())
			player.AddPlayTime(frameTime);

		// Limit the frame rate only if vsync is disabled.
		if(maxFramerate > 0 && Preferences::VSyncState() == Preferences::VSync::off)
		{
			auto now = CurrentTime();
			double ms = 1000. / maxFramerate - (now - newTime);
			double targetTime = now + ms;

			if(ms > 1.)
				this_thread::sleep_for(chrono::duration<double, milli>(ms - 1));
			// To limit the frame rate accurately we busy wait for the last millisecond.
			while(CurrentTime() < targetTime)
				; // Do nothing.
		}
	}

	// If player quit while landed on a planet, save the game if there are changes.
	if(player.GetPlanet() && gamePanels.CanSave())
		player.Save();
}



void PrintHelp()
{
	cerr << endl;
	cerr << "Command line options:" << endl;
	cerr << "    -h, --help: print this help message." << endl;
	cerr << "    -v, --version: print version information." << endl;
	cerr << "    -s, --ships: print table of ship statistics, then exit." << endl;
	cerr << "    -w, --weapons: print table of weapon statistics, then exit." << endl;
	cerr << "    -t, --talk: read and display a conversation from STDIN." << endl;
	cerr << "    -r, --resources <path>: load resources from given directory." << endl;
	cerr << "    -c, --config <path>: save user's files to given directory." << endl;
	cerr << "    -d, --debug: turn on debugging features (e.g. Caps Lock slows down instead of speeds up)." << endl;
	cerr << "    -p, --parse-save: load the most recent saved game and inspect it for content errors" << endl;
	cerr << "    --tests: print table of available tests, then exit." << endl;
	cerr << "    --test <name>: run given test from resources directory" << endl;
	cerr << endl;
	cerr << "Report bugs to: <https://github.com/endless-sky/endless-sky/issues>" << endl;
	cerr << "Home page: <https://endless-sky.github.io>" << endl;
	cerr << endl;
}



void PrintVersion()
{
	cerr << endl;
	cerr << "Endless Sky ver. 0.9.15-alpha" << endl;
	cerr << "License GPLv3+: GNU GPL version 3 or later: <https://gnu.org/licenses/gpl.html>" << endl;
	cerr << "This is free software: you are free to change and redistribute it." << endl;
	cerr << "There is NO WARRANTY, to the extent permitted by law." << endl;
	cerr << endl;
	cerr << GameWindow::SDLVersions() << endl;
	cerr << endl;
}



Conversation LoadConversation()
{
	Conversation conversation;
	DataFile file(cin);
	for(const DataNode &node : file)
		if(node.Token(0) == "conversation")
		{
			conversation.Load(node);
			break;
		}

	map<string, string> subs = {
		{"<bunks>", "[N]"},
		{"<cargo>", "[N tons of Commodity]"},
		{"<commodity>", "[Commodity]"},
		{"<date>", "[Day Mon Year]"},
		{"<day>", "[The Nth of Month]"},
		{"<destination>", "[Planet in the Star system]"},
		{"<fare>", "[N passengers]"},
		{"<first>", "[First]"},
		{"<last>", "[Last]"},
		{"<origin>", "[Origin Planet]"},
		{"<passengers>", "[your passengers]"},
		{"<planet>", "[Planet]"},
		{"<ship>", "[Ship]"},
		{"<system>", "[Star]"},
		{"<tons>", "[N tons]"}
	};
	return conversation.Instantiate(subs);
}



// This prints out the list of tests that are available and their status
// (active/missing feature/known failure)..
void PrintTestsTable()
{
	cout << "status" << '\t' << "name" << '\n';
	for(auto &it : GameData::Tests())
	{
		const Test &test = it.second;
		cout << test.StatusText() << '\t';
		cout << "\"" << test.Name() << "\"" << '\n';
	}
	cout.flush();
}



void PrintShipTable()
{
	cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull" << '\t'
		<< "mass" << '\t' << "crew" << '\t' << "cargo" << '\t' << "bunks" << '\t'
		<< "fuel" << '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
		<< "speed" << '\t' << "accel" << '\t' << "turn" << '\t'
		<< "energy generation" << '\t' << "max energy usage" << '\t' << "energy capacity" << '\t'
		<< "idle/max heat" << '\t' << "max heat generation" << '\t' << "max heat dissipation" << '\t'
		<< "gun mounts" << '\t' << "turret mounts" << '\n';
	for(auto &it : GameData::Ships())
	{
		// Skip variants and unnamed / partially-defined ships.
		if(it.second.ModelName() != it.first)
			continue;

		const Ship &ship = it.second;
		cout << it.first << '\t';
		cout << ship.Cost() << '\t';

		const Outfit &attributes = ship.Attributes();
		auto mass = attributes.Mass() ? attributes.Mass() : 1.;
		cout << attributes.Get("shields") << '\t';
		cout << attributes.Get("hull") << '\t';
		cout << mass << '\t';
		cout << attributes.Get("required crew") << '\t';
		cout << attributes.Get("cargo space") << '\t';
		cout << attributes.Get("bunks") << '\t';
		cout << attributes.Get("fuel capacity") << '\t';

		cout << ship.BaseAttributes().Get("outfit space") << '\t';
		cout << ship.BaseAttributes().Get("weapon capacity") << '\t';
		cout << ship.BaseAttributes().Get("engine capacity") << '\t';
		cout << (attributes.Get("drag") ? (60. * attributes.Get("thrust") / attributes.Get("drag")) : 0) << '\t';
		cout << 3600. * attributes.Get("thrust") / mass << '\t';
		cout << 60. * attributes.Get("turn") / mass << '\t';

		double energyConsumed = attributes.Get("energy consumption")
			+ max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
			+ attributes.Get("turning energy")
			+ attributes.Get("afterburner energy")
			+ attributes.Get("fuel energy")
			+ (attributes.Get("hull energy") * (1 + attributes.Get("hull energy multiplier")))
			+ (attributes.Get("shield energy") * (1 + attributes.Get("shield energy multiplier")))
			+ attributes.Get("cooling energy")
			+ attributes.Get("cloaking energy");

		double heatProduced = attributes.Get("heat generation") - attributes.Get("cooling")
			+ max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
			+ attributes.Get("turning heat")
			+ attributes.Get("afterburner heat")
			+ attributes.Get("fuel heat")
			+ (attributes.Get("hull heat") * (1 + attributes.Get("hull heat multiplier")))
			+ (attributes.Get("shield heat") * (1 + attributes.Get("shield heat multiplier")))
			+ attributes.Get("solar heat")
			+ attributes.Get("cloaking heat");

		for(const auto &oit : ship.Outfits())
			if(oit.first->IsWeapon() && oit.first->Reload())
			{
				double reload = oit.first->Reload();
				energyConsumed += oit.second * oit.first->FiringEnergy() / reload;
				heatProduced += oit.second * oit.first->FiringHeat() / reload;
			}
		cout << 60. * (attributes.Get("energy generation") + attributes.Get("solar collection")) << '\t';
		cout << 60. * energyConsumed << '\t';
		cout << attributes.Get("energy capacity") << '\t';
		cout << ship.IdleHeat() / max(1., ship.MaximumHeat()) << '\t';
		cout << 60. * heatProduced << '\t';
		// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000 per 60th of a second, so:
		cout << 60. * ship.HeatDissipation() * ship.MaximumHeat() << '\t';

		int numTurrets = 0;
		int numGuns = 0;
		for(auto &hardpoint : ship.Weapons())
		{
			if(hardpoint.IsTurret())
				++numTurrets;
			else
				++numGuns;
		}
		cout << numGuns << '\t' << numTurrets << '\n';
	}
	cout.flush();
}



void PrintWeaponTable()
{
	cout << "name" << '\t' << "cost" << '\t' << "space" << '\t' << "range" << '\t'
		<< "energy/s" << '\t' << "heat/s" << '\t' << "recoil/s" << '\t'
		<< "shield/s" << '\t' << "hull/s" << '\t' << "push/s" << '\t'
		<< "homing" << '\t' << "strength" <<'\n';
	for(auto &it : GameData::Outfits())
	{
		// Skip non-weapons and submunitions.
		if(!it.second.IsWeapon() || it.second.Category().empty())
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << '\t';
		cout << outfit.Cost() << '\t';
		cout << -outfit.Get("weapon capacity") << '\t';

		cout << outfit.Range() << '\t';

		double energy = outfit.FiringEnergy() * 60. / outfit.Reload();
		cout << energy << '\t';
		double heat = outfit.FiringHeat() * 60. / outfit.Reload();
		cout << heat << '\t';
		double firingforce = outfit.FiringForce() * 60. / outfit.Reload();
		cout << firingforce << '\t';

		double shield = outfit.ShieldDamage() * 60. / outfit.Reload();
		cout << shield << '\t';
		double hull = outfit.HullDamage() * 60. / outfit.Reload();
		cout << hull << '\t';
		double hitforce = outfit.HitForce() * 60. / outfit.Reload();
		cout << hitforce << '\t';

		cout << outfit.Homing() << '\t';
		double strength = outfit.MissileStrength() + outfit.AntiMissile();
		cout << strength << '\n';
	}
	cout.flush();
}



#ifdef _WIN32
void InitConsole()
{
	const int UNINITIALIZED = -2;
	bool redirectStdout = _fileno(stdout) == UNINITIALIZED;
	bool redirectStderr = _fileno(stderr) == UNINITIALIZED;
	bool redirectStdin = _fileno(stdin) == UNINITIALIZED;

	// Bail if stdin, stdout, and stderr are already initialized (e.g. writing to a file)
	if(!redirectStdout && !redirectStderr && !redirectStdin)
		return;

	// Bail if we fail to attach to the console
	if(!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole())
		return;

	// Perform console redirection.
	if(redirectStdout && freopen("CONOUT$", "w", stdout))
		setvbuf(stdout, nullptr, _IOFBF, 4096);
	if(redirectStderr && freopen("CONOUT$", "w", stderr))
		setvbuf(stderr, nullptr, _IOLBF, 1024);
	if(redirectStdin && freopen("CONIN$", "r", stdin))
		setvbuf(stdin, nullptr, _IONBF, 0);
}
#endif
