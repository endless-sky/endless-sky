/* FilesPath.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Files.h"

#include <SDL2/SDL.h>

#include <stdexcept>

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
	// Find the path to the resource directory. This will depend on the
	// operating system, and can be overridden by a command line argument.
	char *str = SDL_GetBasePath();
	if(!str)
		throw runtime_error("Unable to get path to resource directory!");

	string value = str;
	SDL_free(str);
	return value;
}



string Files::GetSavePath()
{
	// Find the path to the directory for saved games (and create it if it does
	// not already exist). This can also be overridden in the command line.
	char *str = SDL_GetPrefPath("endless-sky", "saves");
	if(!str)
		throw runtime_error("Unable to get path to saves directory!");

	string value = str;
#if defined _WIN32
	FixWindowsSlashes(savePath);
#endif
	SDL_free(str);
	return value;
}



void Files::CreatePluginDirectory()
{
	// Create the "plugins" directory if it does not yet exist, so that it is
	// clear to the user where plugins should go.
	char *str = SDL_GetPrefPath("endless-sky", "plugins");
	if(str != nullptr)
		SDL_free(str);
}
