/* Help.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HELP_H_
#define HELP_H_

#include <string>

// Class for context help messages.
class Help {
public:
	enum Topic {
		BANK,
		BASICS_1,
		BASICS_2,
		DEAD,
		DISABLED,
		HIRING,
		JOBS,
		LOST_1,
		LOST_2,
		LOST_3,
		LOST_4,
		LOST_5,
		LOST_6,
		LOST_7,
		MAP,
		MULTIPLE_SHIPS,
		NAVIGATION,
		OUTFITTER,
		STRANDED,
		TRADING,

		MAX
	};

	// This returns true for the first time it invoked with a topic.
	// The next invocation with the same topic will return false.
	static bool ShouldShow(Topic);

	// Get the help message for this topic.
	static std::string HelpMessage(Topic);

private:
	// Get the topic label for data and save files.
	static const char* TopicName(Topic);

};



#endif
