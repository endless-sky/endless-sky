/* Sound.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Sound.h"

#include <AL/alut.h>

using namespace std;



void Sound::Load(const string &path)
{
	if(path.length() < 5 || path.compare(path.length() - 4, 4, ".wav"))
		return;
	
	isLooped = path[path.length() - 5] == '~';
	buffer = alutCreateBufferFromFile(path.c_str());
}



unsigned Sound::Buffer() const
{
	return buffer;
}



bool Sound::IsLooping() const
{
	return isLooped;
}
