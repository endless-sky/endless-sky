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

	vector<pair<string, const Message::Category *>> incoming;
	vector<Messages::Entry> recent;
	deque<pair<string, const Message::Category *>> logged;
}



// Add a message to the list along with its level of importance.
void Messages::Add(const Message &message)
{
	const Message::Category *category = message.GetCategory();
	if(!category)
		return;
	string text = message.Text();
	if(!category->LogDeduplication() || logged.empty() || text != logged.front().first)
	{
		logged.emplace_front(text, category);
		if(logged.size() > MAX_LOG)
			logged.pop_back();
	}

	if(category->LogOnly())
		return;
	lock_guard<mutex> lock(incomingMutex);
	incoming.emplace_back(text, category);
}



// Get the messages for the given game step. Any messages that are too old
// will be culled out, and new ones that have just been added will have
// their "step" set to the given value.
const vector<Messages::Entry> &Messages::Get(int step, int animationDuration)
{
	lock_guard<mutex> lock(incomingMutex);

	// Load the incoming messages.
	for(const auto &[message, category] : incoming)
	{
		// If this message is not important and it is already being shown in the
		// list, ignore it.
		if(category->AggressiveDeduplication())
		{
			bool skip = false;
			for(const Messages::Entry &entry : recent)
				skip |= (entry.message == message);
			if(skip)
				continue;
		}

		auto it = recent.begin();
		while(it != recent.end())
		{
			int age = step - it->step;
			// Each time a new message comes in, "age" all the existing ones,
			// except for cases where it would interrupt an animation, to
			// limit how many of them appear at once.
			if(age > animationDuration)
				it->step -= 60;
			// For each incoming message, if it exactly matches an existing message,
			// replace that one with this new one by scheduling the old one for removal.
			if(!category->AggressiveDeduplication() && it->message == message && it->deathStep < 0)
				it->deathStep = step + animationDuration;
			// Erase messages that have reached the end of their lifetime.
			if(age > 1000 + animationDuration || (it->deathStep >= 0 && it->deathStep <= step))
				it = recent.erase(it);
			else
				++it;
		}
		recent.emplace_back(step, message, category);
	}
	incoming.clear();
	return recent;
}



const deque<pair<string, const Message::Category *>> &Messages::GetLog()
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
