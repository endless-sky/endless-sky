/* Help.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Help.h"

#include "Preferences.h"
#include "GameData.h"

using namespace std;

namespace {
	static const char *TopicNames[Help::MAX] =
	{
		"bank",
		"basics 1",
		"basics 2",
		"dead",
		"disabled",
		"hiring",
		"jobs",
		"lost 1",
		"lost 2",
		"lost 3",
		"lost 4",
		"lost 5",
		"lost 6",
		"lost 7",
		"map",
		"multiple ships",
		"navigation",
		"outfitter",
		"stranded",
		"trading",
	};
}

bool Help::ShouldShow(Topic topic)
{
	bool& seen = preferences.Get(string("help: ") + TopicNames[topic]);
	if(seen)
		return false;

	seen = true;
	return true;
}

const char *Help::TopicName(Topic topic)
{
	return TopicNames[topic];
}

Help::Topic Help::FindTopic(string name)
{
	for(int t=0; t<MAX; t++)
		if(name == TopicNames[t])
			return (Topic)t;
	return (Topic)-1;
}

std::string Help::HelpMessage(Topic topic)
{
	return GameData::HelpMessage(TopicName(topic));
}
