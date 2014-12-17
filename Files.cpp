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
	const string appName = "endless-sky";
	string resources;
	string config;
	
	string data;
	string images;
	string sounds;
	string saves;
}



void Files::Init(const char * const *argv)
{
	// Find the path to the resource directory. This will depend on the
	// operating system, and can be overridden by a command line argument.
	char *str = SDL_GetBasePath();
	resources = str;
	SDL_free(str);
	if(resources.back() != '/')
		resources += '/';
#ifdef __linux__
	// Special case, for Linux: the resource files are not in the same place as
	// the executable, but are under the same prefix (/usr or /usr/local).
	static const string LOCAL_PATH = "/usr/local/";
	static const string STANDARD_PATH = "/usr/";
	static const string RESOURCE_PATH = "share/games/endless-sky/";
	if(!resources.compare(0, LOCAL_PATH.length(), LOCAL_PATH))
		resources = LOCAL_PATH + RESOURCE_PATH;
	else if(!resources.compare(0, STANDARD_PATH.length(), STANDARD_PATH))
		resources = STANDARD_PATH + RESOURCE_PATH;
#elif __APPLE__
	// Special case for Mac OS X: the resources are in ../Resources relative to
	// the folder the binary is in.
	size_t pos = resources.rfind('/', resources.length() - 2) + 1;
	resources = resources.substr(0, pos) + "Resources/";
#endif
	data = resources + "data/";
	images = resources + "images/";
	sounds = resources + "sounds/";
	
	// Find the path to the directory for saved games (and create it if it does
	// not already exist). This can also be overridden in the command line.
	str = SDL_GetPrefPath("endless-sky", "saves");
	saves = str;
	SDL_free(str);
	if(saves.back() != '/')
		saves += '/';
	config = saves.substr(0, saves.rfind('/', saves.length() - 2) + 1);
	
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



vector<string> Files::List(const string &directory)
{
	vector<string> list;
	List(directory, &list);
	return list;
}



void Files::List(const string &directory, vector<string> *list)
{
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
		
		if(isRegularFile)
			list->push_back(name);
	}
	
	closedir(dir);
}



vector<string> Files::RecursiveList(const string &directory)
{
	vector<string> list;
	RecursiveList(directory, &list);
	return list;
}



void Files::RecursiveList(const string &directory, vector<string> *list)
{
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



bool Files::Exists(const std::string &filePath)
{
	struct stat buf;
	return !stat(filePath.c_str(), &buf);
}



void Files::Copy(const std::string &from, const std::string &to)
{
	ifstream in(from, ios::binary);
	ofstream out(to, ios::binary);
	
	out << in.rdbuf();
}



void Files::Move(const std::string &from, const std::string &to)
{
	rename(from.c_str(), to.c_str());
}



void Files::Delete(const std::string &filePath)
{
	unlink(filePath.c_str());
}
