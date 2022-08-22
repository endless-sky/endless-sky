/* SoundSet.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SoundSet.h"

#include "Logger.h"
#include "Sound.h"

using namespace std;



const Sound *SoundSet::Get(const string &name) const
{
	lock_guard<mutex> guard(modifyMutex);

	auto it = sounds.find(name);
	if(it == sounds.end())
		it = sounds.emplace(name, Sound()).first;
	return &it->second;
}



Sound *SoundSet::Modify(const string &name)
{
	return const_cast<Sound *>(Get(name));
}



void SoundSet::CheckReferences() const
{
	for(auto &&it : sounds)
		if(it.second.Name().empty())
			Logger::LogError("Warning: sound \"" + it.first + "\" is referred to, but does not exist.");
}
