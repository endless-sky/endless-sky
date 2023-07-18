/* Test.cpp
Copyright (c) 2019-2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Test.h"

#include "DataNode.h"
#include "text/Format.h"
#include "GameData.h"
#include "Logger.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"
#include "TestContext.h"
#include "TestData.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>

using namespace std;

namespace {
	const auto STATUS_TO_TEXT = map<Test::Status, const string> {
		{Test::Status::ACTIVE, "active"},
		{Test::Status::BROKEN, "broken"},
		{Test::Status::KNOWN_FAILURE, "known failure"},
		{Test::Status::MISSING_FEATURE, "missing feature"},
		{Test::Status::PARTIAL, "partial"},
	};

	const auto STEPTYPE_TO_TEXT = map<Test::TestStep::Type, const string> {
		{Test::TestStep::Type::APPLY, "apply"},
		{Test::TestStep::Type::ASSERT, "assert"},
		{Test::TestStep::Type::BRANCH, "branch"},
		{Test::TestStep::Type::CALL, "call"},
		{Test::TestStep::Type::INJECT, "inject"},
		{Test::TestStep::Type::INPUT, "input"},
		{Test::TestStep::Type::LABEL, "label"},
		{Test::TestStep::Type::NAVIGATE, "navigate"},
		{Test::TestStep::Type::WATCHDOG, "watchdog"},
	};

	template<class K, class... Args>
	string ExpectedOptions(const map<K, const string, Args...> &m)
	{
		if(m.empty())
			return "no options supported";

		string beginning = "expected \"" + m.begin()->second;
		auto lastValidIt = prev(m.end());
		// Handle maps with just 1 element.
		if(lastValidIt == m.begin())
			return beginning + "\"";

		return accumulate(next(m.begin()), lastValidIt, beginning,
			[](string a, const pair<K, const string> &b) -> string
			{
				return std::move(a) + "\", \"" + b.second;
			})
			+ "\", or \"" + lastValidIt->second + '"';
	}

	// Prepare an keyboard input to one of the UIs.
	bool KeyInputToEvent(const char *keyName, Uint16 modKeys)
	{
		// Construct the event to send (from keyboard code and modifiers)
		SDL_Event event;
		event.type = SDL_KEYDOWN;
		event.key.state = SDL_PRESSED;
		event.key.repeat = 0;
		event.key.keysym.sym = SDL_GetKeyFromName(keyName);
		if(event.key.keysym.sym == SDLK_UNKNOWN)
			return false;
		event.key.keysym.mod = modKeys;
		return SDL_PushEvent(&event);
	}

	bool SendQuitEvent()
	{
		SDL_Event event;
		event.type = SDL_QUIT;
		return SDL_PushEvent(&event);
	}

	string ShipToString(const Ship &ship)
	{
		string description = "name: " + ship.Name();
		const System *system = ship.GetSystem();
		const Planet *planet = ship.GetPlanet();
		description += ", system: " + (system ? system->Name() : "<not set>");
		description += ", planet: " + (planet ? planet->TrueName() : "<not set>");
		description += ", hull: " + Format::Number(ship.Hull());
		description += ", shields: " + Format::Number(ship.Shields());
		description += ", energy: " + Format::Number(ship.Energy());
		description += ", fuel: " + Format::Number(ship.Fuel());
		description += ", heat: " + Format::Number(ship.Heat());
		return description;
	}
}



Test::TestStep::TestStep(Type stepType)
	: stepType(stepType)
{
}



void Test::TestStep::LoadInput(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "key")
		{
			for(int i = 1; i < child.Size(); ++i)
				inputKeys.insert(child.Token(i));

			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "shift")
					modKeys |= KMOD_SHIFT;
				else if(grand.Token(0) == "alt")
					modKeys |= KMOD_ALT;
				else if(grand.Token(0) == "control")
					modKeys |= KMOD_CTRL;
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(child.Token(0) == "pointer")
		{
			for(const DataNode &grand : child)
			{
				static const string BAD_AXIS_INPUT = "Error: Pointer axis input without coordinate:";
				if(grand.Token(0) == "X")
				{
					if(grand.Size() < 2)
						grand.PrintTrace(BAD_AXIS_INPUT);
					else
						XValue = grand.Value(1);
				}
				else if(grand.Token(0) == "Y")
				{
					if(grand.Size() < 2)
						grand.PrintTrace(BAD_AXIS_INPUT);
					else
						YValue = grand.Value(1);
				}
				else if(grand.Token(0) == "click")
					for(int i = 1; i < grand.Size(); ++i)
					{
						if(grand.Token(i) == "left")
							clickLeft = true;
						else if(grand.Token(i) == "right")
							clickRight = true;
						else if(grand.Token(i) == "middle")
							clickMiddle = true;
						else
							grand.PrintTrace("Warning: Unknown click/button \"" + grand.Token(i) + "\":");
					}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(child.Token(0) == "command")
			command.Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Test::LoadSequence(const DataNode &node)
{
	if(!steps.empty())
	{
		status = Status::BROKEN;
		node.PrintTrace("Error: duplicate sequence keyword");
		return;
	}

	for(const DataNode &child : node)
	{
		const string &typeName = child.Token(0);
		auto it = find_if(STEPTYPE_TO_TEXT.begin(), STEPTYPE_TO_TEXT.end(),
			[&typeName](const std::pair<TestStep::Type, const string> &e) {
				return e.second == typeName;
			});
		if(it == STEPTYPE_TO_TEXT.end())
		{
			status = Status::BROKEN;
			child.PrintTrace("Error: Unsupported step type (" + ExpectedOptions(STEPTYPE_TO_TEXT) + "):");
			// Don't bother loading more steps once broken.
			return;
		}

		steps.emplace_back(it->first);
		TestStep &step = steps.back();
		switch(step.stepType)
		{
			case TestStep::Type::APPLY:
			case TestStep::Type::ASSERT:
				step.conditions.Load(child);
				break;
			case TestStep::Type::BRANCH:
				if(child.Size() < 2)
				{
					status = Status::BROKEN;
					child.PrintTrace("Error: Invalid use of \"branch\" without target label:");
					return;
				}
				step.jumpOnTrueTarget = child.Token(1);
				if(child.Size() > 2)
					step.jumpOnFalseTarget = child.Token(2);
				step.conditions.Load(child);
				break;
			case TestStep::Type::CALL:
				if(child.Size() < 2)
				{
					status = Status::BROKEN;
					child.PrintTrace("Error: Invalid use of \"call\" without name of called (sub)test:");
					return;
				}
				else
					step.nameOrLabel = child.Token(1);
				break;
			case TestStep::Type::INJECT:
				if(child.Size() < 2)
				{
					status = Status::BROKEN;
					child.PrintTrace("Error: Invalid use of \"inject\" without data identifier:");
					return;
				}
				else
					step.nameOrLabel = child.Token(1);
				break;
			case TestStep::Type::INPUT:
				step.LoadInput(child);
				break;
			case TestStep::Type::LABEL:
				if(child.Size() < 2)
					child.PrintTrace("Ignoring empty label");
				else
				{
					step.nameOrLabel = child.Token(1);
					if(jumpTable.find(step.nameOrLabel) != jumpTable.end())
					{
						child.PrintTrace("Error: duplicate label");
						status = Status::BROKEN;
						return;
					}
					else
						jumpTable[step.nameOrLabel] = steps.size() - 1;
				}
				break;
			case TestStep::Type::NAVIGATE:
				for(const DataNode &grand : child)
				{
					if(grand.Token(0) == "travel" && grand.Size() >= 2)
						step.travelPlan.push_back(GameData::Systems().Get(grand.Token(1)));
					else if(grand.Token(0) == "travel destination" && grand.Size() >= 2)
						step.travelDestination = GameData::Planets().Get(grand.Token(1));
					else
					{
						grand.PrintTrace("Error: Invalid or incomplete keywords for navigation");
						status = Status::BROKEN;
					}
				}
				break;
			case TestStep::Type::WATCHDOG:
				step.watchdog = child.Size() >= 2 ? child.Value(1) : 0;
				break;
			default:
				child.PrintTrace("Error: unknown step type in test");
				status = Status::BROKEN;
				return;
		}
	}

	// Check if all jump-labels are present after loading the sequence.
	for(const TestStep &step : steps)
	{
		if(!step.jumpOnTrueTarget.empty() && jumpTable.find(step.jumpOnTrueTarget) == jumpTable.end())
		{
			node.PrintTrace("Error: missing label " + step.jumpOnTrueTarget);
			status = Status::BROKEN;
			return;
		}
		if(!step.jumpOnFalseTarget.empty() && jumpTable.find(step.jumpOnFalseTarget) == jumpTable.end())
		{
			node.PrintTrace("Error: missing label " + step.jumpOnFalseTarget);
			status = Status::BROKEN;
			return;
		}
	}
}



void Test::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("Error: Unnamed test:");
		return;
	}
	// If a test object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a test with the same name as the base game
	// or another plugin). Tests should be globally unique.
	if(!name.empty())
	{
		node.PrintTrace("Error: Duplicate test definition:");
		return;
	}
	// Validate if the testname contains valid characters.
	if(node.Token(1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-")
		!= std::string::npos)
	{
		node.PrintTrace("Error: Unsupported character(s) in test name:");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			const string &statusText = child.Token(1);
			auto it = find_if(STATUS_TO_TEXT.begin(), STATUS_TO_TEXT.end(),
				[&statusText](const std::pair<Status, const string> &e) {
					return e.second == statusText;
				});
			if(it != STATUS_TO_TEXT.end())
			{
				// If the test already has a broken status (due to anything
				// else in loading having failed badly), then don't update
				// the status from broken.
				if(status != Status::BROKEN)
					status = it->first;
			}
			else
			{
				status = Status::BROKEN;
				child.PrintTrace("Error: Unsupported status (" + ExpectedOptions(STATUS_TO_TEXT) + "):");
			}
		}
		else if(child.Token(0) == "sequence")
			LoadSequence(child);
		else if(child.Token(0) == "description")
		{
			// Provides a human friendly description of the test, but it is not used internally.
		}
		else
			child.PrintTrace("Error: Skipping unrecognized attribute:");
	}
}



const string &Test::Name() const
{
	return name;
}



Test::Status Test::GetStatus() const
{
	return status;
}



// Check the game status and perform the next test action.
void Test::Step(TestContext &context, PlayerInfo &player, Command &commandToGive) const
{
	// Only run tests once all data has been loaded.
	if(!GameData::IsLoaded())
		return;

	if(status == Status::BROKEN)
		Fail(context, player, "Test has a broken status.");

	// Track if we need to return to the main gameloop.
	bool continueGameLoop = false;

	// If the step to run is beyond the end of the steps, then we finished
	// the current test (and step to the step higher in the stack or we are
	// done testing if we are at toplevel).
	if(context.callstack.back().step >= steps.size())
	{
		context.callstack.pop_back();

		if(context.callstack.empty())
		{
			// If this test was supposed to fail diagnose this here.
			if(status >= Status::KNOWN_FAILURE)
				UnexpectedSuccessResult();

			// Done, no failures, exit the game.
			SendQuitEvent();
			return;
		}
		else
			// Step beyond the call statement we just finished.
			++(context.callstack.back().step);

		// We changed the active test or are quitting, so don't run the current one.
		continueGameLoop = true;
	}

	// All processing was done just before this step started.
	context.branchesSinceGameStep.clear();

	while(context.callstack.back().step < steps.size() && !continueGameLoop)
	{
		// Fail if we encounter a watchdog timeout
		if(context.watchdog == 1)
			Fail(context, player, "watchdog timeout");
		else if(context.watchdog > 1)
			--(context.watchdog);

		const TestStep &stepToRun = steps[context.callstack.back().step];
		switch(stepToRun.stepType)
		{
			case TestStep::Type::APPLY:
				stepToRun.conditions.Apply(player.Conditions());
				++(context.callstack.back().step);
				break;
			case TestStep::Type::ASSERT:
				if(!stepToRun.conditions.Test(player.Conditions()))
					Fail(context, player, "asserted false");
				++(context.callstack.back().step);
				break;
			case TestStep::Type::BRANCH:
				// If we encounter a branch entry twice, then resume the gameloop before the second encounter.
				// Encountering branch entries twice typically only happen in "wait loops" and we should give
				// the game cycles to proceed if we are in a wait loop for something that happens over time.
				if(context.branchesSinceGameStep.count(context.callstack.back()))
				{
					continueGameLoop = true;
					break;
				}
				context.branchesSinceGameStep.emplace(context.callstack.back());
				if(stepToRun.conditions.Test(player.Conditions()))
					context.callstack.back().step = jumpTable.find(stepToRun.jumpOnTrueTarget)->second;
				else if(!stepToRun.jumpOnFalseTarget.empty())
					context.callstack.back().step = jumpTable.find(stepToRun.jumpOnFalseTarget)->second;
				else
					++(context.callstack.back().step);
				break;
			case TestStep::Type::CALL:
				{
					auto calledTest = GameData::Tests().Find(stepToRun.nameOrLabel);
					if(nullptr == calledTest)
						Fail(context, player, "Calling non-existing test \"" + stepToRun.nameOrLabel + "\"");
					// Put the called test on the stack and start it from 0.
					context.callstack.push_back({calledTest, 0});
					// Break the loop to switch to the test just pushed.
				}
				continueGameLoop = true;
				break;
			case TestStep::Type::INJECT:
				{
					// Lookup the data and inject it in the game or into the environment.
					const TestData *testData = GameData::TestDataSets().Get(stepToRun.nameOrLabel);
					if(!testData->Inject())
						Fail(context, player, "injecting data failed");
				}
				++(context.callstack.back().step);
				break;
			case TestStep::Type::INPUT:
				if(stepToRun.command)
					commandToGive |= stepToRun.command;
				if(!stepToRun.inputKeys.empty())
				{
					// TODO: handle keys also in-flight (as single inputset)
					// TODO: combine keys with mouse-inputs
					for(const string &key : stepToRun.inputKeys)
						if(!KeyInputToEvent(key.c_str(), stepToRun.modKeys))
							Fail(context, player, "key \"" + key + + "\" input towards SDL eventqueue failed");
				}
				// TODO: handle mouse inputs
				// Make sure that we run a gameloop to process the input.
				continueGameLoop = true;
				++(context.callstack.back().step);
				break;
			case TestStep::Type::LABEL:
				++(context.callstack.back().step);
				break;
			case TestStep::Type::NAVIGATE:
				player.TravelPlan().clear();
				player.TravelPlan() = stepToRun.travelPlan;
				player.SetTravelDestination(stepToRun.travelDestination);
				++(context.callstack.back().step);
				break;
			case TestStep::Type::WATCHDOG:
				context.watchdog = stepToRun.watchdog;
				++(context.callstack.back().step);
				break;
			default:
				Fail(context, player, "Unknown step type");
				break;
		}
	}
}



const string &Test::StatusText() const
{
	return STATUS_TO_TEXT.at(status);
}



// Get the names of the conditions relevant for this test.
std::set<std::string> Test::RelevantConditions() const
{
	set<string> conditionNames;
	for(const auto &step : steps)
	{
		switch(step.stepType)
		{
			case TestStep::Type::APPLY:
			case TestStep::Type::ASSERT:
			case TestStep::Type::BRANCH:
				{
					for(const auto &name : step.conditions.RelevantConditions())
						conditionNames.emplace(name);
				}
				break;
			case TestStep::Type::CALL:
				{
					auto calledTest = GameData::Tests().Find(step.nameOrLabel);
					if(!calledTest)
						continue;

					for(const auto &name : calledTest->RelevantConditions())
						conditionNames.emplace(name);
				}
				break;
			default:
				continue;
		}
	}
	return conditionNames;
}



// Fail the test using the given message as reason.
void Test::Fail(const TestContext &context, const PlayerInfo &player, const string &testFailReason) const
{
	string message = "Test failed";
	if(!testFailReason.empty())
		message += ": " + testFailReason;
	message += "\n";

	Logger::LogError(message);

	// Print the callstack if we have any.
	string stackMessage = "Call-stack:\n";
	if(context.callstack.empty())
		stackMessage += "  No callstack info at moment of failure.";

	for(auto i = context.callstack.rbegin(); i != context.callstack.rend(); ++i)
	{
		stackMessage += "- \"" + i->test->Name() + "\", step: " + to_string(1 + i->step);
		if(i->step < i->test->steps.size())
			stackMessage += " (" + STEPTYPE_TO_TEXT.at(((i->test->steps)[i->step]).stepType) + ")";
		stackMessage += "\n";
	}
	Logger::LogError(stackMessage);

	// Print some debug information about the flagship and the first 5 escorts.
	const Ship *flagship = player.Flagship();
	if(!flagship)
		Logger::LogError("No flagship at the moment of failure.");
	else
	{
		string shipsOverview = "flagship " + ShipToString(*flagship) + "\n";
		int escorts = 0;
		int escortsNotPrinted = 0;
		for(auto &&ptr : flagship->GetEscorts())
		{
			auto escort = ptr.lock();
			if(!escort)
				continue;
			if(++escorts <= 5)
				shipsOverview += "escort " + ShipToString(*escort) + "\n";
			else
				++escortsNotPrinted;
		}
		if(escortsNotPrinted > 0)
			shipsOverview += "(plus " + to_string(escortsNotPrinted) + " additional escorts)\n";
		Logger::LogError(shipsOverview);
	}

	// Print all conditions that are used in the test.
	string conditions = "";
	for(const auto &it : RelevantConditions())
	{
		const auto &val = player.Conditions().HasGet(it);
		conditions += "Condition: \"" + it + "\" = " + (val.first ? to_string(val.second) : "(not set)") + "\n";
	}

	if(!conditions.empty())
		Logger::LogError(conditions);
	else
		Logger::LogError("No conditions to display at the moment of failure.");

	// If this test was expected to fail, then return a success exitcode from the program
	// because the test did what it was expected to do.
	if(status >= Status::KNOWN_FAILURE)
		throw known_failure_tag{};

	// Throwing a runtime_error is kinda rude, but works for this version of
	// the tester. Might want to add a menuPanels.QuitError() function in
	// a later version (which can set a non-zero exitcode and exit properly).
	throw runtime_error(message);
}



void Test::UnexpectedSuccessResult() const
{
	throw runtime_error("Unexpected test result: Test marked with status '" + StatusText()
		+ "' was not expected to finish succesfully.\n");
}
