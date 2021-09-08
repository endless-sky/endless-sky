/* FilesPath.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Files.h"

#if defined _WIN32
#include "text/Utf8.h"
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <string>

using namespace std;

namespace {
	// Convert windows-style directory separators ('\\') to standard '/'.
#if defined _WIN32
	void FixWindowsSlashes(string &path)
	{
		for(char &c : path)
			if(c == '\\')
				c = '/';
	}
#endif
}



string Files::GetBasePath()
{
	return {};
}



string Files::GetSavePath()
{
	return {};
}



void Files::CreatePluginDirectory()
{
}
