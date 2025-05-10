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
	AddLog(message);
	const Message::Category *category = message.GetCategory();
	if(!category)
		return;
	lock_guard<mutex> lock(incomingMutex);
	incoming.emplace_back(message.Text(), category);
}



// Add a message to the log. For messages meant to be shown
// also on the main panel, use Add instead.
void Messages::AddLog(const Message &message)
{
	const Message::Category *category = message.GetCategory();
	if(!category)
		return;
	if(!category->LogDeduplication() || logged.empty() || message.Text() != logged.front().first)
	{
		logged.emplace_front(message.Text(), category);
		if(logged.size() > MAX_LOG)
			logged.pop_back();
	}
}



// Get the messages for the given game step. Any messages that are too old
// will be culled out, and new ones that have just been added will have
// their "step" set to the given value.
const vector<Messages::Entry> &Messages::Get(int step)
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

		// For each incoming message, if it exactly matches an existing message,
		// replace that one with this new one.
		auto it = recent.begin();
		while(it != recent.end())
		{
			// Each time a new message comes in, "age" all the existing ones to
			// limit how many of them appear at once.
			it->step -= 60;
			// Also erase messages that have reached the end of their lifetime.
			if((!category->AggressiveDeduplication() && it->message == message) || it->step < step - 1000)
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
