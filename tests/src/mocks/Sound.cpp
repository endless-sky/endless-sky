/* Sound.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Sound.h"

#include <cstdio>
#include <vector>

using namespace std;



bool Sound::Load(const string &path, const string &name)
{
	return true;
}



const string &Sound::Name() const
{
	return name;
}



unsigned Sound::Buffer() const
{
	return buffer;
}



bool Sound::IsLooping() const
{
	return isLooped;
}
