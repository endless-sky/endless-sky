/* main.cpp
Copyright (c) 2014 by Michael Zahniser

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "audio/Audio.h"
#include "Command.h"
#include "Conversation.h"
#include "CustomEvents.h"
#include "DataFile.h"
#include "DataNode.h"
#include "Engine.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "GameLoadingPanel.h"
#include "GameVersion.h"
#include "GameWindow.h"
#include "Interface.h"
#include "Logger.h"
#include "MainPanel.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Plugins.h"
#include "Preferences.h"
#include "PrintData.h"
#include "Screen.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "TaskQueue.h"
#include "test/Test.h"
#include "test/TestContext.h"
#include "UI.h"

#ifdef _WIN32
#include "windows/TimerResolutionGuard.h"
#include "windows/WinConsole.h"
#include "windows/WinVersion.h"
#endif

#include <chrono>
#include <iostream>
#include <map>

#include <cassert>
#include <future>
#include <exception>
#include <string>

#ifdef _WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>

#define PSAPI_VERSION 1
#include <processthreadsapi.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#else
#include <fstream>
#include <unistd.h>
#endif

using namespace std;

void PrintHelp();
void PrintVersion();
void GameLoop(PlayerInfo &player, TaskQueue &queue, const Conversation &conversation,
	const string &testToRun, bool debugMode);
Conversation LoadConversation(const PlayerInfo &player);
void PrintTestsTable();



// Entry point for the EndlessSky executable
int main(int argc, char *argv[])
{
#ifdef _WIN32
	WinVersion::Init();
	// Handle command-line arguments
	if(argc > 1)
		WinConsole::Init();
#endif
	PlayerInfo player;
	Conversation conversation;
	bool debugMode = false;
	bool loadOnly = false;
	bool checkAssets = false;
	bool printTests = false;
	bool printData = false;
	bool noTestMute = false;
	string testToRunName;

	// Whether the game has encountered errors while loading.
	bool hasErrors = false;
	// Ensure that we log errors to the errors.txt file.
	Logger::SetLogCallback([&hasErrors](const string &errorMessage, Logger::Level level)
	{
		hasErrors |= level != Logger::Level::INFO;
		Files::LogErrorToFile(errorMessage);
	});

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
			conversation = LoadConversation(player);
		else if(arg == "-d" || arg == "--debug")
			debugMode = true;
		else if(arg == "-p" || arg == "--parse-save")
			loadOnly = true;
		else if(arg == "--parse-assets")
			checkAssets = true;
		else if(arg == "--test" && *++it)
			testToRunName = *it;
		else if(arg == "--tests")
			printTests = true;
		else if(arg == "--nomute")
			noTestMute = true;
	}
	printData = PrintData::IsPrintDataArgument(argv);
	Files::Init(argv);

	// Whether we are running an integration test.
	const bool isTesting = !testToRunName.empty();
	bool isConsoleOnly = loadOnly || printTests || printData;

	Logger::Session logSession{isConsoleOnly || isTesting};

	try {

		// Load plugin preferences before game data if any.
		Plugins::LoadSettings();

		TaskQueue queue;

		// Begin loading the game data.
		auto dataFuture = GameData::BeginLoad(queue, player, isConsoleOnly, debugMode,
			isConsoleOnly || checkAssets || (isTesting && !debugMode));

		// If we are not using the UI, or performing some automated task, we should load
		// all data now.
		if(isConsoleOnly || checkAssets || isTesting)
			dataFuture.wait();

		if(isTesting && !GameData::Tests().Has(testToRunName))
		{
			Logger::Log("Test \"" + testToRunName + "\" not found.", Logger::Level::ERROR);
			return 1;
		}

		if(printData)
		{
			PrintData::Print(argv, player);
			return 0;
		}
		if(printTests)
		{
			PrintTestsTable();
			return 0;
		}

		if(loadOnly || checkAssets)
		{
			if(checkAssets)
			{
				Audio::LoadSounds(GameData::Sources());
				while(GameData::GetProgress() < 1.)
				{
					queue.ProcessSyncTasks();
					this_thread::yield();
				}
				if(GameData::IsLoaded())
				{
					// Now that we have finished loading all the basic sprites and sounds, we can look for invalid file paths,
					// e.g. due to capitalization errors or other typos.
					SpriteSet::CheckReferences();
					Audio::CheckReferences(true);
				}
			}

			// Set the game's initial internal state.
			GameData::FinishLoading();

			// Reference check the universe, as known to the player. If no player found,
			// then check the default state of the universe.
			if(!player.LoadRecent())
				GameData::CheckReferences();
			cout << "Parse completed with " << (hasErrors ? "at least one" : "no") << " error(s)." << endl;
			if(checkAssets)
				Audio::Quit();
			return hasErrors;
		}
		assert(!isConsoleOnly && "Attempting to use UI when only data was loaded!");

		Preferences::Load();

		// Load global conditions:
		DataFile globalConditions(Files::Config() / "global conditions.txt");
		for(const DataNode &node : globalConditions)
			if(node.Token(0) == "conditions")
				GameData::GlobalConditions().Load(node);

		if(!GameWindow::Init(isTesting && !debugMode))
			return 1;

		GameData::LoadSettings();

#ifdef _WIN32
		TimerResolutionGuard windowsTimerGuard;
#endif

		if(!isTesting || debugMode)
		{
			GameData::LoadShaders();

			// Show something other than a blank window.
			GameWindow::Step();
		}

		Audio::Init(GameData::Sources());

		if(isTesting && !noTestMute)
			Audio::SetVolume(0, SoundCategory::MASTER);

		CustomEvents::Init();
		// This is the main loop where all the action begins.
		GameLoop(player, queue, conversation, testToRunName, debugMode);
	}
	catch(Test::known_failure_tag)
	{
		// This is not an error. Simply exit successfully.
	}
	catch(const exception &error)
	{
		Audio::Quit();
		GameWindow::ExitWithError(error.what(), !isTesting);
		return 1;
	}

	// Remember the window state and preferences if quitting normally.
	Preferences::Set("maximized", GameWindow::IsMaximized());
	Preferences::Set("fullscreen", GameWindow::IsFullscreen());
	Screen::SetRaw(GameWindow::Width(), GameWindow::Height(), true);
	Preferences::Save();
	Plugins::Save();

	Audio::Quit();
	GameWindow::Quit();

	return 0;
}



void GameLoop(PlayerInfo &player, TaskQueue &queue, const Conversation &conversation,
		const string &testToRunName, bool debugMode)
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
	menuPanels.Push(new GameLoadingPanel(player, queue, conversation, gamePanels, dataFinishedLoading));

	bool showCursor = true;
	int cursorTime = 0;
	int frameRate = 60;
	FrameTimer timer(frameRate);
	bool isDebugPaused = false;
	bool isFastForward = false;

	// Limit how quickly full-screen mode can be toggled.
	int toggleTimeout = 0;

	// Data to track progress of testing if/when a test is running.
	TestContext testContext;
	if(!testToRunName.empty())
		testContext = TestContext(GameData::Tests().Get(testToRunName));

	const bool isHeadless = (testContext.CurrentTest() && !debugMode);

	auto ProcessEvents = [&menuPanels, &gamePanels, &player, &cursorTime, &toggleTimeout, &debugMode, &isDebugPaused,
			&isFastForward]
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);

			// If the mouse moves, reset the cursor movement timeout.
			if(event.type == SDL_MOUSEMOTION)
				cursorTime = 0;

			if(debugMode && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKQUOTE)
			{
				isDebugPaused = !isDebugPaused;
				if(isDebugPaused)
					Audio::Pause();
				else
					Audio::Resume();
			}
			else if(event.type == SDL_KEYDOWN && menuPanels.IsEmpty()
					&& Command(event.key.keysym.sym).Has(Command::MENU)
					&& !gamePanels.IsEmpty() && gamePanels.Top()->IsInterruptible())
			{
				// User pressed the Menu key.
				menuPanels.Push(shared_ptr<Panel>(
					new MenuPanel(player, gamePanels)));
				UI::PlaySound(UI::UISound::NORMAL);
			}
			else if(event.type == SDL_QUIT)
				menuPanels.Quit();
			else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				// The window has been resized. Adjust the raw screen size and the OpenGL viewport to match.
				GameWindow::AdjustViewport();
			else if(event.type == CustomEvents::GetResize())
			{
				menuPanels.AdjustViewport();
				gamePanels.AdjustViewport();
			}
			else if(event.type == SDL_KEYDOWN && !toggleTimeout
					&& (Command(event.key.keysym.sym).Has(Command::FULLSCREEN)
					|| (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))))
			{
				toggleTimeout = 30;
				Preferences::ToggleScreenMode();
			}
			else if(event.type == SDL_KEYDOWN && Command(event.key.keysym.sym).Has(Command::PERFORMANCE_DISPLAY))
				Preferences::Set("Show CPU / GPU load", !Preferences::Has("Show CPU / GPU load"));
			else if(activeUI.Handle(event))
			{
				// The UI handled the event.
			}
			else if(event.type == SDL_KEYDOWN && !event.key.repeat
					&& (Command(event.key.keysym.sym).Has(Command::FASTFORWARD))
					&& !Command(SDLK_CAPSLOCK).Has(Command::FASTFORWARD))
			{
				isFastForward = !isFastForward;
			}
		}

		// Special case: If fastforward is on capslock, update on mod state and not
		// on keypress.
		if(Command(SDLK_CAPSLOCK).Has(Command::FASTFORWARD))
			isFastForward = SDL_GetModState() & KMOD_CAPS;
	};

	// Game loop when running the game normally.
	if(!testContext.CurrentTest())
	{
		// How much time in total was spent on calculations and drawing since the last
		// load cache update (every 60 drawing frames; usually 1 second).
		chrono::steady_clock::duration cpuLoadSum{};
		string cpuLoadString;
		chrono::steady_clock::duration gpuLoadSum{};
		string gpuLoadString;
		string memoryString;
		bool isPerformanceDisplayReady = false;
		int step = 0;
		int drawStep = 0;

		while(!menuPanels.IsDone())
		{
			if(++step == 60)
				step = 0;
			if(toggleTimeout)
				--toggleTimeout;
			chrono::steady_clock::time_point start = chrono::steady_clock::now();

			ProcessEvents();

			SDL_Keymod mod = SDL_GetModState();
			Font::ShowUnderlines(mod & KMOD_ALT);

			// In full-screen mode, hide the cursor if inactive for ten seconds,
			// but only if the player is flying around in the main view.
			bool inFlight = (menuPanels.IsEmpty() && gamePanels.Root() == gamePanels.Top());
			++cursorTime;
			bool shouldShowCursor = (!GameWindow::IsFullscreen() || cursorTime < 600 || !inFlight);
			if(shouldShowCursor != showCursor)
			{
				showCursor = shouldShowCursor;
				SDL_ShowCursor(showCursor);
			}

			// Switch off fast-forward if the player is not in flight or flight-related screen
			// (for example when the boarding dialog shows up or when the player lands). The player
			// can switch fast-forward on again when flight is resumed.
			bool allowFastForward = !gamePanels.IsEmpty() && gamePanels.Top()->AllowsFastForward();
			if(Preferences::Has("Interrupt fast-forward") && !inFlight && isFastForward && !allowFastForward)
				isFastForward = false;

			// Tell all the panels to step forward, then draw them.
			((!isDebugPaused && menuPanels.IsEmpty()) ? gamePanels : menuPanels).StepAll();

			// Caps lock slows the frame rate in debug mode.
			// Slowing eases in and out over a couple of frames.
			if((mod & KMOD_CAPS) && inFlight && debugMode)
			{
				if(frameRate > 10)
				{
					frameRate = max(frameRate - 5, 10);
					timer.SetFrameRate(frameRate);
				}
			}
			else
			{
				if(frameRate < 60)
				{
					frameRate = min(frameRate + 5, 60);
					timer.SetFrameRate(frameRate);
				}

				if(isFastForward && inFlight && step % 3)
				{
					cpuLoadSum += chrono::steady_clock::now() - start;
					continue;
				}
			}

			Audio::Step(isFastForward);

			cpuLoadSum += chrono::steady_clock::now() - start;
			++drawStep;
			chrono::steady_clock::time_point drawStart = chrono::steady_clock::now();

			// Events in this frame may have cleared out the menu, in which case
			// we should draw the game panels instead:
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();

			MainPanel *mainPanel = static_cast<MainPanel *>(gamePanels.Root().get());
			if(mainPanel && mainPanel->GetEngine().IsPaused())
				SpriteShader::Draw(SpriteSet::Get("ui/paused"), Screen::TopLeft() + Point(10., 10.));
			else if(isFastForward)
				SpriteShader::Draw(SpriteSet::Get("ui/fast forward"), Screen::TopLeft() + Point(10., 10.));

			gpuLoadSum += chrono::steady_clock::now() - drawStart;

			if(Preferences::Has("Show CPU / GPU load"))
			{
				Information performanceInfo;
				performanceInfo.SetString("cpu", cpuLoadString);
				performanceInfo.SetString("gpu", gpuLoadString);
				performanceInfo.SetString("mem", memoryString);
				if(isPerformanceDisplayReady)
					performanceInfo.SetCondition("ready");
				static const Interface &performanceDisplay = *GameData::Interfaces().Get("performance info");
				performanceDisplay.Draw(performanceInfo);
				if(drawStep == 60)
				{
					drawStep = 0;
					// The load sums are in nanoseconds, accumulated throughout the last second, so divide
					// by 10^6 to get milliseconds, then by 60 (or 180 with fast-forward for the CPU load)
					// to get the average milliseconds per step.
					// In case of percentage, 100% is exactly one second, so divide by 10^7.
					auto cpuNano = chrono::duration_cast<chrono::nanoseconds>(cpuLoadSum).count();
					cpuLoadString = "CPU: " + Format::Decimal(cpuNano / (isFastForward && inFlight ? 1.8e8 : 6e7), 2)
						+ " ms (" + to_string(static_cast<int>(round(cpuNano / 1e7))) + "%)";
					cpuLoadSum = {};
					auto gpuNano = chrono::duration_cast<chrono::nanoseconds>(gpuLoadSum).count();
					gpuLoadString = "GPU: " + Format::Decimal(gpuNano / 6e7, 2)
						+ " ms (" + to_string(static_cast<int>(round(gpuNano / 1e7))) + "%)";
					gpuLoadSum = {};
					// Get how much memory we have (in bytes).
					static size_t virtualMemoryUse;
#ifdef _WIN32
					static PROCESS_MEMORY_COUNTERS info;
					GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
					virtualMemoryUse = info.PagefileUsage;
#elif defined(__APPLE__)
					static mach_task_basic_info_data_t info;
					static mach_msg_type_number_t infoSize = MACH_TASK_BASIC_INFO_COUNT;
					task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &infoSize);
					virtualMemoryUse = info.virtual_size;
#else
					static string statmStr;
					ifstream statm{"/proc/self/statm"};
					getline(statm, statmStr, ' ');
					virtualMemoryUse = stoul(statmStr) * getpagesize();
#endif
					// bytes / (1024 * 1024) = megabytes
					memoryString = "MEM: " + Format::Decimal(virtualMemoryUse / 1048576., 2) + " MB";
					isPerformanceDisplayReady = true;
				}
			}
			else
			{
				// Clear the values to avoid reading old data when the player re-enables the display.
				drawStep = 0;
				cpuLoadSum = {};
				gpuLoadSum = {};
				isPerformanceDisplayReady = false;
			}

			GameWindow::Step();

			// Lock the game loop to 60 FPS.
			timer.Wait();

			// If the player ended this frame in-game, count the elapsed time as played time.
			if(menuPanels.IsEmpty())
				player.AddPlayTime(chrono::steady_clock::now() - start);
		}
	}
	// Game loop when running the game as part of an integration test.
	else
	{
		int integrationStepCounter = 0;
		while(!menuPanels.IsDone())
		{
			ProcessEvents();

			// Handle any integration test steps.
			if(dataFinishedLoading)
			{
				// Run a single integration step every 30 frames.
				integrationStepCounter = (integrationStepCounter + 1) % 30;
				if(!integrationStepCounter)
				{
					assert(!gamePanels.IsEmpty() && "main panel missing?");

					// The main panel is always at the root of the game panels.
					MainPanel *mainPanel = static_cast<MainPanel *>(gamePanels.Root().get());

					// The engine needs to have finished calculating the current frame so
					// that it is safe to run any additional processing here.
					if(menuPanels.IsEmpty())
						mainPanel->GetEngine().Wait();

					// The current test that is running, if any.
					const Test *runningTest = testContext.CurrentTest();
					assert(runningTest && "no running test while running an integration test?");
					Command command;
					runningTest->Step(testContext, player, command);

					// Send any commands to the engine, if it is active.
					if(menuPanels.IsEmpty())
						mainPanel->GetEngine().GiveCommand(command);
				}
			}

			// Tell all the panels to step forward, then draw them.
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).StepAll();

			if(!isHeadless)
			{
				Audio::Step(isFastForward);

				// Events in this frame may have cleared out the menu, in which case
				// we should draw the game panels instead:
				(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();

				GameWindow::Step();

				// When we perform automated testing, then we run the game by default as quickly as possible.
				// Except when not in headless mode so that the user can follow along.
				timer.Wait();
			}
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
	cerr << "    -t, --talk: read and display a conversation from STDIN." << endl;
	cerr << "    -r, --resources <path>: load resources from given directory." << endl;
	cerr << "    -c, --config <path>: save user's files to given directory." << endl;
	cerr << "    -d, --debug: turn on debugging features (e.g. Caps Lock slows down instead of speeds up)." << endl;
	cerr << "    -p, --parse-save: load the most recent saved game and inspect it for content errors." << endl;
	cerr << "    --parse-assets: load all game data, images, and sounds,"
		" and the latest save game, and inspect data for errors." << endl;
	cerr << "    --tests: print table of available tests, then exit." << endl;
	cerr << "    --test <name>: run given test from resources directory." << endl;
	cerr << "    --nomute: don't mute the game while running tests." << endl;
	PrintData::Help();
	cerr << endl;
	cerr << "Report bugs to: <https://github.com/endless-sky/endless-sky/issues>" << endl;
	cerr << "Home page: <https://endless-sky.github.io>" << endl;
	cerr << endl;
}



void PrintVersion()
{
	cerr << endl;
	cerr << "Endless Sky ver. " << GameVersion::Running().ToString() << endl;
	cerr << "License GPLv3+: GNU GPL version 3 or later: <https://gnu.org/licenses/gpl.html>" << endl;
	cerr << "This is free software: you are free to change and redistribute it." << endl;
	cerr << "There is NO WARRANTY, to the extent permitted by law." << endl;
	cerr << endl;
	cerr << GameWindow::SDLVersions() << endl;
	cerr << endl;
}



Conversation LoadConversation(const PlayerInfo &player)
{
	const ConditionsStore *conditions = &player.Conditions();
	Conversation conversation;
	DataFile file(cin);
	for(const DataNode &node : file)
		if(node.Token(0) == "conversation")
		{
			conversation.Load(node, conditions);
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
		{"<model>", "[Ship Model]"},
		{"<flagship>", "[Flagship]"},
		{"<flagship model>", "[Flagship Model]"},
		{"<system>", "[Star]"},
		{"<tons>", "[N tons]"}
	};
	return conversation.Instantiate(subs);
}



// This prints out the list of tests that are available and their status
// (active/missing feature/known failure)..
void PrintTestsTable()
{
	for(auto &it : GameData::Tests())
		if(it.second.GetStatus() != Test::Status::PARTIAL
				&& it.second.GetStatus() != Test::Status::BROKEN)
			cout << it.second.Name() << '\n';
	cout.flush();
}
