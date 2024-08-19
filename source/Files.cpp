/* Files.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Files.h"

#include "File.h"
#include "Logger.h"

#include <SDL2/SDL.h>

#if defined _WIN32
#include "text/Utf8.h"
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif

#include <algorithm>
#include <iostream>
#include <mutex>
#include <stdexcept>

using namespace std;

namespace {
	filesystem::path resources;
	filesystem::path config;

	filesystem::path dataPath;
	filesystem::path imagePath;
	filesystem::path soundPath;
	filesystem::path savePath;
	filesystem::path userPluginPath;
	filesystem::path globalPluginPath;
	filesystem::path testPath;

	File errorLog;

	// Open the given folder in a separate window.
	void OpenFolder(const filesystem::path &path)
	{
		// TODO: Remove SDL version check after Ubuntu 20.04 reaches end of life
#if SDL_VERSION_ATLEAST(2, 0, 14)
		if(SDL_OpenURL(("file://" + path.string()).c_str()))
			Logger::LogError("Warning: SDL_OpenURL failed with \"" + string(SDL_GetError()) + "\"");
#elif defined(__linux__)
		// Some supported distributions do not have an up-to-date SDL.
		cout.flush();
		if(int result = WEXITSTATUS(system(("xdg-open file://" + path.string()).c_str())))
			Logger::LogError("Warning: xdg-open failed with error code " + to_string(result) + ".");
#else
#warning SDL 2.0.14 or higher is needed for opening folders!
		Logger::LogError("Warning: No handler found to open \"" + path + "\" in a new window.");
#endif
	}
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
		resources = filesystem::current_path();
		if(resources.empty())
			throw runtime_error("Unable to get path to resource directory!");

		if(exists(resources))
			resources = filesystem::canonical(resources);

		#if defined __linux__ || defined __FreeBSD__ || defined __DragonFly__
			// Special case, for Linux: the resource files are not in the same place as
			// the executable, but are under the same prefix (/usr or /usr/local).
			static const filesystem::path LOCAL_PATH = "/usr/local/";
			static const filesystem::path STANDARD_PATH = "/usr/";
			static const filesystem::path RESOURCE_PATH = "share/games/endless-sky/";
			if(resources.string().starts_with(LOCAL_PATH.string()))
				resources = LOCAL_PATH / RESOURCE_PATH;
			else if(resources.string().starts_with(STANDARD_PATH.string()))
				resources = STANDARD_PATH / RESOURCE_PATH;
		#endif
	}
	// If the resources are not here, search in the directories containing this
	// one. This allows, for example, a Mac app that does not actually have the
	// resources embedded within it.
	while(!Exists(resources / "credits.txt"))
	{
		if(!resources.has_parent_path() || resources == "/")
			throw runtime_error("Unable to find the resource directories!");
		resources = resources.parent_path();
	}
	dataPath = resources / "data/";
	imagePath = resources / "images/";
	soundPath = resources / "sounds/";
	globalPluginPath = resources / "plugins/";

	if(config.empty())
	{
		// Create the directory for the saved games, preferences, etc., if necessary.
		char *str = SDL_GetPrefPath(nullptr, "endless-sky");
		if(!str)
			throw runtime_error("Unable to get path to config directory!");
		config = str;
		SDL_free(str);
	}

	if(!Exists(config))
		throw runtime_error("Unable to create config directory!");

	config = filesystem::canonical(config);

	savePath = config / "saves/";
	CreateFolder(savePath);

	// Create the "plugins" directory if it does not yet exist, so that it is
	// clear to the user where plugins should go.
	userPluginPath = config / "plugins/";
	CreateFolder(userPluginPath);

	// Check that all the directories exist.
	if(!Exists(dataPath) || !Exists(imagePath) || !Exists(soundPath))
		throw runtime_error("Unable to find the resource directories!");
	if(!Exists(savePath))
		throw runtime_error("Unable to create save directory!");
	if(!Exists(userPluginPath))
		throw runtime_error("Unable to create plugins directory!");
}



const filesystem::path &Files::Resources()
{
	return resources;
}



const filesystem::path &Files::Config()
{
	return config;
}



const filesystem::path &Files::Data()
{
	return dataPath;
}



const filesystem::path &Files::Images()
{
	return imagePath;
}



const filesystem::path &Files::Sounds()
{
	return soundPath;
}



const filesystem::path &Files::Saves()
{
	return savePath;
}



const filesystem::path &Files::UserPlugins()
{
	return userPluginPath;
}



const filesystem::path &Files::GlobalPlugins()
{
	return globalPluginPath;
}



const filesystem::path &Files::Tests()
{
	return testPath;
}



vector<filesystem::path> Files::List(const filesystem::path &directory)
{
	vector<filesystem::path> list;

	if(!Exists(directory) || !is_directory(directory))
		return list;

	for(const auto &entry : filesystem::directory_iterator(directory))
		if(entry.is_regular_file())
			list.emplace_back(entry);

	sort(list.begin(), list.end());

	return list;
}



// Get a list of any directories in the given directory.
vector<filesystem::path> Files::ListDirectories(const filesystem::path &directory)
{
	vector<filesystem::path> list;

	if(!Exists(directory) || !is_directory(directory))
		return list;

	for(const auto &entry : filesystem::directory_iterator(directory))
		if(entry.is_directory())
			list.emplace_back(entry);

	sort(list.begin(), list.end());
	return list;
}



vector<filesystem::path> Files::RecursiveList(const filesystem::path &directory)
{
	vector<filesystem::path> list;
	if(!Exists(directory) || !is_directory(directory))
		return list;

	for(const auto &entry : filesystem::recursive_directory_iterator(directory))
		if(entry.is_regular_file())
			list.emplace_back(entry);

	sort(list.begin(), list.end());
	return list;
}



bool Files::Exists(const filesystem::path &filePath)
{
	return exists(filePath);
}



time_t Files::Timestamp(const filesystem::path &filePath)
{
	auto time = last_write_time(filePath);
	auto systemTime = time_point_cast<chrono::system_clock::duration>(time - chrono::file_clock::now()
			+ chrono::system_clock::now());
	return chrono::system_clock::to_time_t(systemTime);
}



void Files::Copy(const filesystem::path &from, const filesystem::path &to)
{
	copy(from, to, filesystem::copy_options::overwrite_existing);
}



void Files::Move(const filesystem::path &from, const filesystem::path &to)
{
	rename(from, to);
}



void Files::Delete(const filesystem::path &filePath)
{
	remove_all(filePath);
}



// Get the filename from a path.
string Files::Name(const filesystem::path &path)
{
	return path.filename().string();
}



FILE *Files::Open(const filesystem::path &path, bool write)
{
#if defined _WIN32
	FILE *file = nullptr;
	_wfopen_s(&file, path.u16string().c_str(), write ? L"w" : L"rb");
	return file;
#else
	return fopen(path.c_str(), write ? "wb" : "rb");
#endif
}



string Files::Read(const filesystem::path &path)
{
	File file(path);
	return Read(file);
}



string Files::Read(FILE *file)
{
	string result;
	if(!file)
		return result;

	// Find the remaining number of bytes in the file.
	size_t start = ftell(file);
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file) - start;
	// Reserve one extra byte because DataFile appends a '\n' to the end of each
	// file it reads, and that's the most common use of this function.
	result.reserve(size + 1);
	result.resize(size);
	fseek(file, start, SEEK_SET);

	// Read the file data.
	size_t bytes = fread(&result[0], 1, result.size(), file);
	if(bytes != result.size())
		throw runtime_error("Error reading file!");

	return result;
}



void Files::Write(const filesystem::path &path, const string &data)
{
	File file(path, true);
	Write(file, data);
}



void Files::Write(FILE *file, const string &data)
{
	if(!file)
		return;

	fwrite(&data[0], 1, data.size(), file);
}



void Files::CreateFolder(const filesystem::path &path)
{
	if(Exists(path))
		return;

	if(filesystem::create_directory(path))
		filesystem::permissions(path, filesystem::perms(std::filesystem::perms::owner_all));
	else
		throw runtime_error("Error creating directory!");
}



// Open this user's plugins directory in their native file explorer.
void Files::OpenUserPluginFolder()
{
	OpenFolder(userPluginPath);
}



// Open this user's save file directory in their native file explorer.
void Files::OpenUserSavesFolder()
{
	OpenFolder(savePath);
}



void Files::LogErrorToFile(const string &message)
{
	if(!errorLog)
	{
		errorLog = File(config / "errors.txt", true);
		if(!errorLog)
		{
			cerr << "Unable to create \"errors.txt\" " << (config.empty()
				? "in current directory" : "in \"" + config.string() + "\"") << endl;
			return;
		}
	}

	Write(errorLog, message);
	fwrite("\n", 1, 1, errorLog);
	fflush(errorLog);
}
