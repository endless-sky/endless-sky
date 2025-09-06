/* Messages.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Messages.h"

#include "Color.h"
#include "GameData.h"

#include <mutex>

using namespace std;

namespace {
	const int MAX_LOG = 10000;

	mutex incomingMutex;

	vector<pair<string, Messages::Importance>> incoming;
	vector<Messages::Entry> recent;
	deque<pair<string, Messages::Importance>> logged;
}



// Add a message to the list along with its level of importance
// When forced, the message is forcibly added to the log, but not to the list.
void Messages::Add(const string &message, Importance importance, bool force)
{
	lock_guard<mutex> lock(incomingMutex);
	incoming.emplace_back(message, importance);
	AddLog(message, importance, force);
}



// Add a message to the log. For messages meant to be shown
// also on the main panel, use Add instead.
void Messages::AddLog(const string &message, Importance importance, bool force)
{
	if(force || logged.empty() || message != logged.front().first)
	{
		logged.emplace_front(message, importance);
		if(logged.size() > MAX_LOG)
			logged.pop_back();
	}
}



// Get the messages for the given game step. Any messages that are too old
// will be culled out, and new ones that have just been added will have
// their "step" set to the given value.
const vector<Messages::Entry> &Messages::Get(int step, int animationDuration)
{
	lock_guard<mutex> lock(incomingMutex);

	// Erase messages that have reached the end of their lifetime.
	auto it = recent.begin();
	while(it != recent.end())
	{
		if(step - it->step > 1000 + animationDuration || (it->deathStep >= 0 && it->deathStep <= step))
			it = recent.erase(it);
		else
			++it;
	}

	// Load the incoming messages.
	for(const pair<string, Importance> &item : incoming)
	{
		const string &message = item.first;
		Importance importance = item.second;

		// If this message is not important and it is already being shown in the
		// list, ignore it.
		if(importance == Importance::Low || importance == Importance::HighestNoRepeat)
		{
			bool skip = false;
			for(const Messages::Entry &entry : recent)
				skip |= (entry.message == message);
			if(skip)
				continue;
		}

		for(auto &it : recent)
		{
			// Each time a new message comes in, "age" all the existing ones,
			// except for cases where it would interrupt an animation, to
			// limit how many of them appear at once.
			if(step - it.step > animationDuration)
				it.step -= 60;
			// For each incoming message, if it exactly matches an existing message,
			// replace that one with this new one by scheduling the old one for removal.
			if(importance != Importance::Low && importance != Importance::HighestNoRepeat
					&& it.message == message && it.deathStep < 0)
				it.deathStep = step + animationDuration;
		}
		recent.emplace_back(step, message, importance);
	}
	incoming.clear();
	return recent;
}



const deque<pair<string, Messages::Importance>> &Messages::GetLog()
{
	return logged;
}



// Reset the messages (i.e. because a new game was loaded).
void Messages::Reset()
{
	lock_guard<mutex> lock(incomingMutex);
	incoming.clear();
	recent.clear();
	logged.clear();
}



// Get color that should be used for drawing messages of given importance.
const Color *Messages::GetColor(Importance importance, bool isLogPanel)
{
	string prefix = isLogPanel ? "message log importance " : "message importance ";
	switch(importance)
	{
		case Messages::Importance::Highest:
		case Messages::Importance::HighestNoRepeat:
			return GameData::Colors().Get(prefix + "highest");
		case Messages::Importance::High:
			return GameData::Colors().Get(prefix + "high");
		case Messages::Importance::Info:
			return GameData::Colors().Get(prefix + "info");
		case Messages::Importance::Daily:
			return GameData::Colors().Get(prefix + "daily");
		case Messages::Importance::Low:
		default:
			return GameData::Colors().Get(prefix + "low");
	}
}
