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
#if defined(__APPLE__)
	// Get the full path to the executable file, then find ../Resources/.
	string app = argv[0];
	size_t pos = app.rfind('/');
	pos = app.rfind('/', pos - 1);
	resources = app.substr(0, pos) + "/Resources/";
	
	config = getenv("HOME") + ("/Library/Application Support/" + appName + "/");
#elif defined(__linux__)
	// By default, if there is an "installed" copy of the code, use the "local"
	// install if it exists, and otherwise the non-local one. If neither one
	// exists, this is a development machine, so use the current directory.
	if(Exists("keys.txt") && Exists("credits.txt") && Exists("data") && Exists("images"))
		resources = "./";
	else if(Exists("/usr/local/bin/" + appName))
		resources = "/usr/local/share/" + appName + "/";
	else if(Exists("/usr/bin/" + appName))
		resources = "/usr/share/" + appName + "/";
	else
		resources = "./";
	
	// Make sure the root ".config" directory exists.
	config = getenv("HOME") + string("/.config/");
	if(!Exists(config) && mkdir(config.c_str(), 0700))
		throw runtime_error("Unable to create config directory!");
	config += appName + '/';
#else
#error "Unsupported operating system. No code for Files::Init()."
#endif
	
	// Parse command line arguments to override the defaults from above.
	for(const char * const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if((arg == "-r" || arg == "--resources") && *++it)
			resources = *it;
		else if((arg == "-c" || arg == "--config") && *++it)
			config = *it;
	}
	
	// Create the config directory if it does not exist.
	if(!Exists(config) && mkdir(config.c_str(), 0700))
		throw runtime_error("Unable to create config directory!");
	
	data = resources + "data/";
	images = resources + "images/";
	sounds = resources + "sounds/";
	if(!Exists(data) || !Exists(images) || !Exists(sounds))
		throw runtime_error("Unable to find the resource directories!");
	
	saves = config + "saves/";
	if(!Exists(saves) && mkdir(saves.c_str(), 0700))
		throw runtime_error("Unable to create saves directory!");
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
