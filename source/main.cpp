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
#include "Command.h"
#include "Conversation.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataNode.h"
#include "Dialog.h"
#include "Files.h"
#include "text/Font.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "GameWindow.h"
#include "GameLoadingPanel.h"
#include "Hardpoint.h"
#include "MenuPanel.h"
#include "Outfit.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Sale.h"
#include "Screen.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "Test.h"
#include "TestContext.h"
#include "UI.h"

#include <chrono>
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
void PrintShipTable(bool printDeterrence);
void PrintTestsTable();
void PrintWeaponTable(bool printDeterrence);
void PrintEngineTable();
void PrintPowerTable();
void PrintOutfitList(bool printOutfitters);
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
	bool printEngines = false;
	bool printPower = false;
	bool printOutfits = false;
	string testToRunName = "";
	bool printDeterrence = false;
	bool printOutfitters = false;

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
		else if(arg == "-e" || arg == "--engines")
			printEngines = true;
		else if(arg == "--power")
			printPower = true;
		else if(arg == "-o" || arg == "--outfits")
			printOutfits = true;
		printDeterrence = (arg == "--deterrence");
		printOutfitters = (arg == "--outfitters");
	}
	Files::Init(argv);

	try {
		// Begin loading the game data.
		bool isConsoleOnly = loadOnly || printShips || printTests || printWeapons || printEngines || printPower || printOutfits;
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

		if(printShips || printTests || printWeapons || printEngines || printPower || printOutfits)
		{
			if(printShips)
				PrintShipTable(printDeterrence);
			if(printTests)
				PrintTestsTable();
			if(printWeapons)
				PrintWeaponTable(printDeterrence);
			if(printEngines)
				PrintEngineTable();
			if(printPower)
				PrintPowerTable();
			if(printOutfits)
				PrintOutfitList(printOutfitters);
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
	int cursorTime = 0;
	int frameRate = 60;
	FrameTimer timer(frameRate);
	bool isPaused = false;
	bool isFastForward = false;
	int testDebugUIDelay = 3 * 60;

	// If fast forwarding, keep track of whether the current frame should be drawn.
	int skipFrame = 0;

	// Limit how quickly full-screen mode can be toggled.
	int toggleTimeout = 0;

	// Data to track progress of testing if/when a test is running.
	TestContext testContext;
	if(!testToRunName.empty())
		testContext = TestContext(GameData::Tests().Get(testToRunName));

	// IsDone becomes true when the game is quit.
	while(!menuPanels.IsDone())
	{
		if(toggleTimeout)
			--toggleTimeout;
		chrono::steady_clock::time_point start = chrono::steady_clock::now();

		// Handle any events that occurred in this frame.
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);

			// If the mouse moves, reset the cursor movement timeout.
			if(event.type == SDL_MOUSEMOTION)
				cursorTime = 0;

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
				isFastForward = !isFastForward;
			}
		}
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
		((!isPaused && menuPanels.IsEmpty()) ? gamePanels : menuPanels).StepAll();

		// All manual events and processing done. Handle any test inputs and events if we have any.
		const Test *runningTest = testContext.CurrentTest();
		if(runningTest && dataFinishedLoading)
		{
			// When flying around, all test processing must be handled in the
			// thread-safe section of Engine. When not flying around (and when no
			// Engine exists), then it is safe to execute the tests from here.
			auto mainPanel = gamePanels.Root();
			if(!isPaused && inFlight && menuPanels.IsEmpty() && mainPanel)
				mainPanel->SetTestContext(testContext);
			else if(debugMode && testDebugUIDelay > 0)
				--testDebugUIDelay;
			else
			{
				// The command will be ignored, since we only support commands
				// from within the engine at the moment.
				Command ignored;
				runningTest->Step(testContext, player, ignored);
				// Reset the visual delay.
				testDebugUIDelay = 3 * 60;
			}
			// Skip drawing 29 out of every 30 in-flight frames during testing to speedup testing (unless debug mode is set).
			// We don't skip UI-frames to ensure we test the UI code more.
			if(inFlight && !debugMode)
			{
				skipFrame = (skipFrame + 1) % 30;
				if(skipFrame)
					continue;
			}
			else
				skipFrame = 0;
		}
		// Caps lock slows the frame rate in debug mode.
		// Slowing eases in and out over a couple of frames.
		else if((mod & KMOD_CAPS) && inFlight && debugMode)
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

			if(isFastForward && inFlight)
			{
				skipFrame = (skipFrame + 1) % 3;
				if(skipFrame)
					continue;
			}
		}

		Audio::Step();

		// Events in this frame may have cleared out the menu, in which case
		// we should draw the game panels instead:
		(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();
		if(isFastForward)
			SpriteShader::Draw(SpriteSet::Get("ui/fast forward"), Screen::TopLeft() + Point(10., 10.));

		GameWindow::Step();

		// When we perform automated testing, then we run the game by default as quickly as possible.
		// Except when debug-mode is set.
		if(!testContext.CurrentTest() || debugMode)
			timer.Wait();

		// If the player ended this frame in-game, count the elapsed time as played time.
		if(menuPanels.IsEmpty())
			player.AddPlayTime(chrono::steady_clock::now() - start);
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
	cerr << "    -s --deterrence: print a table of ships and corresponding armament deterrence values." << endl;
	cerr << "    -w, --weapons: print table of weapon statistics, then exit." << endl;
	cerr << "    -w --deterrance: prints a table of weapons and their deterrence values." << endl;
	cerr << "    -e, --engines: print table of engines statistics, then exit." << endl;
	cerr << "    -o, --outfits: print list of outfit names, then exit." << endl;
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



void PrintShipTable(bool printDeterrence)
{
	if(printDeterrence)
	{
		cout << "model" << ',' << "deterrence" << '\n';
		for(auto &it : GameData::Ships())
		{
			//if(it.second.ModelName() != it.first)
				//continue;

			const Ship &ship = it.second;
			double deterrence = 0;
			for(const Hardpoint &hardpoint : ship.Weapons())
				if(hardpoint.GetOutfit())
				{
					const Outfit *weapon = hardpoint.GetOutfit();
					if(weapon->Ammo() && !ship.OutfitCount(weapon->Ammo()))
						continue;
					double damage = weapon->ShieldDamage() + weapon->HullDamage()
						+ (weapon->RelativeShieldDamage() * ship.Attributes().Get("shields"))
						+ (weapon->RelativeHullDamage() * ship.Attributes().Get("hull"));
					deterrence += .12 * damage / weapon->Reload();
				}
			cout << it.first << ',' << deterrence << '\n';
		}
		return;
	}
	cout << "model" << ',' << "category" << ',' << "cost" << ',' << "shields" << ','
		<< "hull" << ',' << "mass" << ',' << "drag" << ',' << "heat dis" << ','
		<< "crew" << ',' << "bunks" << ',' << "cargo" << ',' << "fuel" << ','
		<< "outfit" << ',' << "weapon" << ',' << "engine" << ','
		/*<< "accel" << ',' << "turn" << ','
		<< "energy generation" << ',' << "max energy usage" << ',' << "energy capacity" << ','
		<< "idle/max heat" << ',' << "max heat generation" << ',' << "max heat dissipation" << ','*/
		<< "gun mounts" << ',' << "turret mounts" << '\n';
	for(auto &it : GameData::Ships())
	{
		// Skip variants and unnamed / partially-defined ships.
		if(it.second.ModelName() != it.first)
			continue;

		const Ship &ship = it.second;
		cout << it.first << ',';

		const Outfit &attributes = ship.BaseAttributes();
		cout << attributes.Category() << ',';
		//cout << ship.Cost() << ',';
		cout << ship.ChassisCost() << ',';

		auto mass = attributes.Mass() ? attributes.Mass() : 1.;
		cout << attributes.Get("shields") << ',';
		cout << attributes.Get("hull") << ',';
		cout << mass << ',';
		cout << attributes.Get("drag") << ',';
		cout << ship.HeatDissipation() * 1000.<< ',';
		cout << attributes.Get("required crew") << ',';
		cout << attributes.Get("bunks") << ',';
		cout << attributes.Get("cargo space") << ',';
		cout << attributes.Get("fuel capacity") << ',';

		cout << ship.BaseAttributes().Get("outfit space") << ',';
		cout << ship.BaseAttributes().Get("weapon capacity") << ',';
		cout << ship.BaseAttributes().Get("engine capacity") << ',';
		//cout << (attributes.Get("drag") ? (60. * attributes.Get("thrust") / attributes.Get("drag")) : 0) << '\t';
		//cout << 3600. * attributes.Get("thrust") / mass << '\t';
		//cout << 60. * attributes.Get("turn") / mass << '\t';

		/*double energyConsumed = attributes.Get("energy consumption")
			+ max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
			+ attributes.Get("turning energy")
			+ attributes.Get("afterburner energy")
			+ attributes.Get("fuel energy")
			+ (attributes.Get("hull energy") * (1 + attributes.Get("hull energy multiplier")))
			+ (attributes.Get("shield energy") * (1 + attributes.Get("shield energy multiplier")))
			+ attributes.Get("cooling energy")
			+ attributes.Get("cloaking energy");*/

		/*double heatProduced = attributes.Get("heat generation") - attributes.Get("cooling")
			+ max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
			+ attributes.Get("turning heat")
			+ attributes.Get("afterburner heat")
			+ attributes.Get("fuel heat")
			+ (attributes.Get("hull heat") * (1 + attributes.Get("hull heat multiplier")))
			+ (attributes.Get("shield heat") * (1 + attributes.Get("shield heat multiplier")))
			+ attributes.Get("solar heat")
			+ attributes.Get("cloaking heat");*/

		/*for(const auto &oit : ship.Outfits())
			if(oit.first->IsWeapon() && oit.first->Reload())
			{
				double reload = oit.first->Reload();
				energyConsumed += oit.second * oit.first->FiringEnergy() / reload;
				heatProduced += oit.second * oit.first->FiringHeat() / reload;
			}*/

		/*cout << 60. * (attributes.Get("energy generation") + attributes.Get("solar collection")) << '\t';
		cout << 60. * energyConsumed << '\t';
		cout << attributes.Get("energy capacity") << '\t';
		cout << ship.IdleHeat() / max(1., ship.MaximumHeat()) << '\t';
		cout << 60. * heatProduced << '\t';
		// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000 per 60th of a second, so:
		cout << 60. * ship.HeatDissipation() * ship.MaximumHeat() << '\t';*/

		int numTurrets = 0;
		int numGuns = 0;
		for(auto &hardpoint : ship.Weapons())
		{
			if(hardpoint.IsTurret())
				++numTurrets;
			else
				++numGuns;
		}
		cout << numGuns << ',' << numTurrets << '\n';
	}
	cout.flush();
}



void PrintWeaponTable(bool printDeterrence)
{
	if(printDeterrence)
	{
		cout << "name" << ',' << "deterrence" << ',' << "outfit" << ',' << "weapon" << '\n';
		for(auto &it : GameData::Outfits())
		{
			// Skip non-weapons and submunitions.
			if(!it.second.IsWeapon() || it.second.Category().empty())
				continue;
			const Outfit &weapon = it.second;
			double damage = weapon.ShieldDamage() + weapon.HullDamage();
			double deterrence = .12 * damage / weapon.Reload();
			cout << it.first << ',' << deterrence << ',' << -weapon.Get("outfit space") << ',' << -weapon.Get("weapon capacity") << '\n';
		}
		return;
	}

	cout << "name" << ',' << "cost" << ',' << "space" << ',' << "range" << ','
		<< "energy/s" << ',' << "heat/s" << ',' << "recoil/s" << ','
		<< "shield/s" << ',' << "hull/s" << ',' << "heatdmg/s" << ',' << "push/s" << ','
		<< "homing" << ',' << "strength" << ',';
	for(auto &it : GameData::Outfits())
	{
		// Skip non-weapons and submunitions.
		if(!it.second.IsWeapon() || it.second.Category().empty())
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
		cout << outfit.Cost() << ',';
		cout << -outfit.Get("weapon capacity") << ',';

		cout << outfit.Range() << ',';

		double energy = outfit.FiringEnergy() * 60. / outfit.Reload();
		cout << energy << ',';
		double heat = outfit.FiringHeat() * 60. / outfit.Reload();
		cout << heat << ',';
		double firingforce = outfit.FiringForce() * 60. / outfit.Reload();
		cout << firingforce << ',';

		double shield = outfit.ShieldDamage() * 60. / outfit.Reload();
		cout << shield << ',';
		double hull = outfit.HullDamage() * 60. / outfit.Reload();
		cout << hull << ',';
		double heatDmg = outfit.HeatDamage() * 60. / outfit.Reload();
		cout << heatDmg << ',';
		double hitforce = outfit.HitForce() * 60. / outfit.Reload();
		cout << hitforce << ',';

		cout << outfit.Homing() << ',';
		double strength = outfit.MissileStrength() + outfit.AntiMissile();
		cout << strength << '\n';
	}
	cout.flush();
}



void PrintEngineTable()
{
	cout << "name" << '\t' << "cost" << '\t' << "mass" << '\t' << "outfit space" << '\t'
		<< "engine capacity" << '\t' << "thrust/s" << '\t' << "thrust energy/s" << '\t'
		<< "thrust heat/s" << '\t' << "turn/s" << '\t' << "turn energy/s" << '\t'
		<< "turn heat/s" << '\t' << "reverse thrust/s" << '\t' << "reverse energy/s" << '\t'
		<< "reverse heat/s" << '\n';
	for(auto &it : GameData::Outfits())
	{
		// Ship non-engines.
		if(it.second.Category() != "Engines")
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
		cout << outfit.Cost() << ',';
		cout << outfit.Mass() << ',';
		cout << outfit.Get("outfit space") << ',';
		cout << outfit.Get("engine capacity") << ',';
		cout << outfit.Get("thrust") * 3600. << ',';
		cout << outfit.Get("thrusting energy") * 60. << ',';
		cout << outfit.Get("thrusting heat") * 60. << ',';
		cout << outfit.Get("turn") * 60. << ',';
		cout << outfit.Get("turning energy") * 60. << ',';
		cout << outfit.Get("turning heat") * 60. << ',';
		cout << outfit.Get("reverse thrust") * 3600. << ',';
		cout << outfit.Get("reverse thrusting energy") * 60. << ',';
		cout << outfit.Get("reverse thrusting heat") * 60. << '\n';
	}
	cout.flush();
}



void PrintPowerTable()
{
	cout << "name" << ',' << "cost" << ',' << "mass" << ',' << "outfit space" << ','
		<< "energy gen" << ',' << "heat gen" << ',' << "energy cap" << '\n';
	for(auto &it : GameData::Outfits())
	{
		// Ship non-power.
		if(it.second.Category() != "Power")
			continue;

		const Outfit &outfit = it.second;
		cout << it.first << ',';
		cout << outfit.Cost() << ',';
		cout << outfit.Mass() << ',';
		cout << outfit.Get("outfit space") << ',';
		cout << outfit.Get("energy generation") << ',';
		cout << outfit.Get("heat generation") << ',';
		cout << outfit.Get("energy capacity") << '\n';
	}
	cout.flush();
}



void PrintOutfitList(bool printOutfitters)
{
	if(!printOutfitters)
	{
		cout << "outfit name" << '\n';
		for(auto &it : GameData::Outfits())
			cout << it.first << '\n';
		return;
	}
	map<string, set<string>> outfits;
	for(auto &it : GameData::Outfitters())
	{
		for(auto &it2 : it.second)
		{
			outfits[it2->Name()].insert(it.first);
		}
	}
	for(auto &it : outfits)
	{
		cout << it.first;
		for(auto &it2 : it.second)
		{
			cout << ',' << it2;
		}
		cout << '\n';
	}
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
	if(redirectStdout)
	{
		FILE *fstdout = nullptr;
		freopen_s(&fstdout, "CONOUT$", "w", stdout);
		if(fstdout)
			setvbuf(stdout, nullptr, _IOFBF, 4096);
	}
	if(redirectStderr)
	{
		FILE *fstderr = nullptr;
		freopen_s(&fstderr, "CONOUT$", "w", stderr);
		if(fstderr)
			setvbuf(stderr, nullptr, _IOLBF, 1024);
	}
	if(redirectStdin)
	{
		FILE *fstdin = nullptr;
		freopen_s(&fstdin, "CONIN$", "r", stdin);
		if(fstdin)
			setvbuf(stdin, nullptr, _IONBF, 0);
	}
}
#endif
