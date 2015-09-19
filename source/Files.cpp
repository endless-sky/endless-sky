/* Files.cpp
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

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <stdexcept>

using namespace std;

namespace {
	string resources;
	string config;
	
	string data;
	string images;
	string sounds;
	string saves;
	
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



void Files::Init(const char * const *argv)
{
	// Parse the command line arguments to see if the user has specified
	// different directories to use.
	for(const char * const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if((arg == "-r" || arg == "--resources") && *++it)
			resources = *it;
		else if((arg == "-c" || arg == "--config") && *++it)
			config = *it;
			
	}
	
	if(resources.empty())
	{
		// Find the path to the resource directory. This will depend on the
		// operating system, and can be overridden by a command line argument.
		char *str = SDL_GetBasePath();
		resources = str;
		SDL_free(str);
	}
#if defined _WIN32
	FixWindowsSlashes(resources);
#endif
	if(resources.back() != '/')
		resources += '/';
#if defined __linux__ || defined __FreeBSD__ || defined __DragonFly__
	// Special case, for Linux: the resource files are not in the same place as
	// the executable, but are under the same prefix (/usr or /usr/local).
	static const string LOCAL_PATH = "/usr/local/";
	static const string STANDARD_PATH = "/usr/";
	static const string RESOURCE_PATH = "share/games/endless-sky/";
	if(!resources.compare(0, LOCAL_PATH.length(), LOCAL_PATH))
		resources = LOCAL_PATH + RESOURCE_PATH;
	else if(!resources.compare(0, STANDARD_PATH.length(), STANDARD_PATH))
		resources = STANDARD_PATH + RESOURCE_PATH;
#elif defined __APPLE__
	// Special case for Mac OS X: the resources are in ../Resources relative to
	// the folder the binary is in.
	size_t pos = resources.rfind('/', resources.length() - 2) + 1;
	resources = resources.substr(0, pos) + "Resources/";
#endif
	// If the resources are not here, search in the directories containing this
	// one. This allows, for example, a Mac app that does not actually have the
	// resources embedded within it.
	while(!Exists(resources + "credits.txt"))
	{
		size_t pos = resources.rfind('/', resources.length() - 2);
		if(pos == string::npos)
			throw runtime_error("Unable to find the resource directories!");
		resources.erase(pos + 1);
	}
	data = resources + "data/";
	images = resources + "images/";
	sounds = resources + "sounds/";
	
	if(config.empty())
	{
		// Find the path to the directory for saved games (and create it if it does
		// not already exist). This can also be overridden in the command line.
		char *str = SDL_GetPrefPath("endless-sky", "saves");
		saves = str;
#if defined _WIN32
		FixWindowsSlashes(saves);
#endif
		SDL_free(str);
		if(saves.back() != '/')
			saves += '/';
		config = saves.substr(0, saves.rfind('/', saves.length() - 2) + 1);
	}
	else
	{
#if defined _WIN32
		FixWindowsSlashes(config);
#endif
		if(config.back() != '/')
			config += '/';
		saves = config + "saves/";
	}
	
	// Create the "plugins" directory if it does not yet exist, so that it is
	// clear to the user where plugins should go.
	SDL_GetPrefPath("endless-sky", "plugins");
	
	// Check that all the directories exist.
	if(!Exists(data) || !Exists(images) || !Exists(sounds))
		throw runtime_error("Unable to find the resource directories!");
	if(!Exists(saves))
		throw runtime_error("Unable to create config directory!");
}



const string &Files::Resources()
{
	return resources;
}



const string &Files::Config()
{
	return config;
}



const string &Files::Data()
{
	return data;
}



const string &Files::Images()
{
	return images;
}



const string &Files::Sounds()
{
	return sounds;
}



const string &Files::Saves()
{
	return saves;
}



vector<string> Files::List(string directory)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';
	
	vector<string> list;
	
	DIR *dir = opendir(directory.c_str());
	if(!dir)
		return list;
	
	while(true)
	{
		dirent *ent = readdir(dir);
		if(!ent)
			break;
		// Skip dotfiles (including "." and "..").
		if(ent->d_name[0] == '.')
			continue;
		
		string name = directory + ent->d_name;
		// Don't assume that this operating system's implementation of dirent
		// includes the t_type field; in particular, on Windows it will not.
		struct stat buf;
		stat(name.c_str(), &buf);
		bool isRegularFile = S_ISREG(buf.st_mode);
		
		if(isRegularFile)
			list.push_back(name);
	}
	
	closedir(dir);
	
	return list;
}



// Get a list of any directories in the given directory.
vector<string> Files::ListDirectories(string directory)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';
	
	vector<string> list;
	
	DIR *dir = opendir(directory.c_str());
	if(!dir)
		return list;
	
	while(true)
	{
		dirent *ent = readdir(dir);
		if(!ent)
			break;
		// Skip dotfiles (including "." and "..").
		if(ent->d_name[0] == '.')
			continue;
		
		string name = directory + ent->d_name;
		// Don't assume that this operating system's implementation of dirent
		// includes the t_type field; in particular, on Windows it will not.
		struct stat buf;
		stat(name.c_str(), &buf);
		bool isDirectory = S_ISDIR(buf.st_mode);
		
		if(isDirectory)
		{
			if(name.back() != '/')
				name += '/';
			list.push_back(name);
		}
	}
	
	closedir(dir);
	
	return list;
}



vector<string> Files::RecursiveList(const string &directory)
{
	vector<string> list;
	RecursiveList(directory, &list);
	return list;
}



void Files::RecursiveList(string directory, vector<string> *list)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';
	
	DIR *dir = opendir(directory.c_str());
	if(!dir)
		return;
	
	while(true)
	{
		dirent *ent = readdir(dir);
		if(!ent)
			break;
		// Skip dotfiles (including "." and "..").
		if(ent->d_name[0] == '.')
			continue;
		
		string name = directory + ent->d_name;
		// Don't assume that this operating system's implementation of dirent
		// includes the t_type field; in particular, on Windows it will not.
		struct stat buf;
		stat(name.c_str(), &buf);
		bool isRegularFile = S_ISREG(buf.st_mode);
		bool isDirectory = S_ISDIR(buf.st_mode);
		
		if(isRegularFile)
			list->push_back(name);
		else if(isDirectory)
			RecursiveList(name + '/', list);
	}
	
	closedir(dir);
}



bool Files::Exists(const string &filePath)
{
	struct stat buf;
	return !stat(filePath.c_str(), &buf);
}



void Files::Copy(const string &from, const string &to)
{
	ifstream in(from, ios::binary);
	ofstream out(to, ios::binary);
	
	out << in.rdbuf();
}



void Files::Move(const string &from, const string &to)
{
	rename(from.c_str(), to.c_str());
}



void Files::Delete(const string &filePath)
{
	unlink(filePath.c_str());
}



// Get the filename from a path.
string Files::Name(const string &path)
{
	// string::npos = -1, so if there is no '/' in the path this will
	// return the entire string, i.e. path.substr(0).
	return path.substr(path.rfind('/') + 1);
}
