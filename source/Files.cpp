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
#include <SDL2/SDL_rwops.h>

#if defined _WIN32
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
#include <memory>

#ifdef __ANDROID__
#include "AndroidAsset.h"
#include <sys/stat.h>
#include <unistd.h>
#endif

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



	void CheckBug_96()
	{
		// Old code used `SDL_GetPrefPath("endless-sky", "saves")`, which on
		// linux and other operating systems would return "path/endless-sky/saves".
		// Then other code would go up a directory to create the plugin folder.
		// This does not produce the expected result on android, and android
		// always returns "path/files", and expects you to create your own
		// subdirectories there.
		//
		// This means that for android, we need to check for the "path/files/../*.txt"
		// files, and they exist, they all need moved into the files directory.
		// See https://github.com/thewierdnut/endless-mobile/issues/96

#ifndef __ANDROID__
		return;
#endif

		// Does this bug need fixed?
		char* pref_path = SDL_GetPrefPath(nullptr, "endless-sky");
		std::filesystem::path path = pref_path;
		SDL_free(pref_path);
		if(path.empty())
			return;

		if(Files::Exists(path.parent_path() / "preferences.txt"))
		{
			SDL_Log("Fixing bug 96 by moving config files from %s to %s", path.parent_path().c_str(), path.c_str());

			// Yes, this needs fixed.
			// files/*.txt needs moved to files/saves
			Files::CreateFolder(path.parent_path() / "saves");
			for(const auto& f: Files::List(path))
			{
				if(f.extension() == ".txt")
				{
					Files::Move(f, f.parent_path() / "/saves/" / f.filename());
				}
			}

			// *.txt  needs moved into files/
			for(const auto& f: Files::List(path.parent_path()))
			{
				if(f.extension() == ".txt")
				{
					Files::Move(f, path / f.filename());
				}
			}

			// the plugin directory needs moved into files/
			// Don't care if this fails
			Files::Move(path.parent_path() / "plugins", path / "plugins");
		}
	}

#ifdef __ANDROID__
	void CrossFileSystemMove(const std::filesystem::path &old_path, const std::filesystem::path &new_path)
	{
		// used in situations where rename would fail, due to crossing a
		// filesystem boundary
		// TODO: add error checking?

		DIR *dir = opendir(old_path.c_str());
		while(true)
		{
			dirent *ent = readdir(dir);
			if(!ent)
				break;
			// Skip dotfiles (including "." and "..").
			if(ent->d_name[0] == '.')
				continue;

			auto name = old_path / ent->d_name;
			// Don't assume that this operating system's implementation of dirent
			// includes the t_type field; in particular, on Windows it will not.
			struct stat buf;
			stat(name.c_str(), &buf);

			if(S_ISREG(buf.st_mode))
			{
				Files::Copy(name, new_path / ent->d_name);
				SDL_Log("Moving %s to %s", name.c_str(), (new_path / ent->d_name).c_str());
				unlink(name.c_str());
			}
			else if(S_ISDIR(buf.st_mode))
			{
				const auto recursive_path = new_path / ent->d_name;
				std::filesystem::create_directories(recursive_path);
				CrossFileSystemMove(name, recursive_path);
				rmdir(name.c_str());
			}
		}
		closedir(dir);
	}

	void CheckBug_104(const std::filesystem::path &old_path, const std::filesystem::path &new_path)
	{
		// On android, we changed the default config path from internal private
		// to external public storage. Copy any configs from the old to the new
		// location.

		if(Files::Exists(old_path / "preferences.txt"))
		{
			// Yes, this needs fixed.
			SDL_Log("Fixing bug 104 by moving config files from %s to %s", old_path.c_str(), new_path.c_str());

			CrossFileSystemMove(old_path, new_path);
		}
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
		if(str)
		{
			resources = str;
			SDL_free(str);
		}
		// This is ok on android. we will read the resources from the assets
#if not defined __ANDROID__
		else throw runtime_error("Unable to get path to resource directory!");
#else
		else resources = "endless-sky-data"; // within assets directory
#endif

#ifndef __ANDROID__
		if(Exists(resources))
			resources = filesystem::canonical(resources);
#endif

		SDL_Log("canonical path = %s", resources.c_str());
#if defined __linux__ || defined __FreeBSD__ || defined __DragonFly__
		// Special case, for Linux: the resource files are not in the same place as
		// the executable, but are under the same prefix (/usr or /usr/local).
		static const filesystem::path LOCAL_PATH = "/usr/local/";
		static const filesystem::path STANDARD_PATH = "/usr/";
		static const filesystem::path RESOURCE_PATH = "share/games/endless-sky/";

		const auto IsParent = [](const auto parent, const auto child) -> bool {
			if(distance(child.begin(), child.end()) < distance(parent.begin(), parent.end()))
				return false;
			return equal(parent.begin(), parent.end(), child.begin());
		};

		if(IsParent(LOCAL_PATH, resources))
			resources = LOCAL_PATH / RESOURCE_PATH;
		else if(IsParent(STANDARD_PATH, resources))
			resources = STANDARD_PATH / RESOURCE_PATH;
#endif
	}
	// If the resources are not here, search in the directories containing this
	// one. This allows, for example, a Mac app that does not actually have the
	// resources embedded within it.
	while(!Exists(resources / "credits.txt"))
	{
		if(!resources.has_parent_path() || resources.parent_path() == resources)
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

		CheckBug_96();

#ifdef __ANDROID__
		SDL_Log("SDL_AndroidGetExternalStorageState() == %d", SDL_AndroidGetExternalStorageState());
		SDL_Log("SDL_AndroidGetExternalStoragePath() == %s", SDL_AndroidGetExternalStoragePath());

		// Use the external path if its available
		if ((SDL_AndroidGetExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE) != 0)
		{
			const char* path = SDL_AndroidGetExternalStoragePath();
			std::string old_path = config;
			config = path;

			CheckBug_104(old_path, config);
		}
#endif
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
	{
#if defined __ANDROID__
		// try the asset system instead
		AndroidAsset aa;
		for (std::string entry: aa.DirectoryList(directory))
		{
			// Asset api doesn't have a stat or entry properties. the only
			// way to tell if its a folder is to fail to open it. Depending
			// on how slow this is, it may be worth checking for a file
			// extension instead.
			if (File(directory / entry))
			{
				list.push_back(directory / entry);
			}
		}
#endif
		return list;
	}

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
	{
#if defined __ANDROID__
		// try the asset system
		AndroidAsset aa;
		for (std::string entry: aa.DirectoryList(directory))
		{
			// Asset api doesn't have a stat or entry properties. the only
			// way to tell if its a folder is to fail to open it. Depending
			// on how slow this is, it may be worth checking for a file
			// extension instead.
			if (!File(directory / entry))
			{
				list.push_back(directory / entry);
			}
		}
#endif
		return list;
	}

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
	{
#if defined __ANDROID__
		// try the asset system. We don't want to instantiate more than one
		// AndroidAsset object, so don't recurse.
		vector<std::filesystem::path> directories;
		directories.push_back(directory);
		AndroidAsset aa;
		while (!directories.empty())
		{
			auto path = directories.back();
			directories.pop_back();
			for (std::string entry: aa.DirectoryList(path))
			{
				// Asset api doesn't have a stat or entry properties. the only
				// way to tell if its a folder is to fail to open it. Depending
				// on how slow this is, it may be worth checking for a file
				// extension instead.
				if (File(path / entry))
				{
					list.push_back(path / entry);
				}
				else
				{
					directories.push_back(path / entry);
				}
			}
		}
#endif
		return list;
	}
	for(const auto &entry : filesystem::recursive_directory_iterator(directory))
		if(entry.is_regular_file())
			list.emplace_back(entry);

	sort(list.begin(), list.end());
	return list;
}



bool Files::Exists(const filesystem::path &filePath)
{
	if (exists(filePath))
	{
		return true;
	}
#ifdef __ANDROID__
	else
	{
		// only works for normal files, not assets, so just try to open the
		// file to see if it exists.
		SDL_RWops* f = SDL_RWFromFile(filePath.c_str(), "r");
		bool exists = f != nullptr;
		if (f) SDL_RWclose(f);
		if (!exists)
		{
			// check and see if it is a directory
			exists = AndroidAsset().DirectoryExists(filePath);
		}
		return exists;
	}
#endif
	return false;
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



struct SDL_RWops *Files::Open(const filesystem::path &path, bool write)
{
	return SDL_RWFromFile(path.c_str(), write ? "wb" : "rb");
}



void Files::Close(struct SDL_RWops * ops)
{
	SDL_RWclose(ops);
}



string Files::Read(const filesystem::path &path)
{
	File file(path);
	return Read(file);
}



string Files::Read(struct SDL_RWops *file)
{
	string result;
	if(!file)
		return result;

	// Find the remaining number of bytes in the file.
	size_t start = SDL_RWtell(file);
	size_t size = SDL_RWseek(file, 0, RW_SEEK_END)  - start;
	// Reserve one extra byte because DataFile appends a '\n' to the end of each
	// file it reads, and that's the most common use of this function.
	result.reserve(size + 1);
	result.resize(size);
	SDL_RWseek(file, start, SEEK_SET);

	// Read the file data.
	size_t bytes = SDL_RWread(file, &result[0], 1, result.size());
	if(bytes != result.size())
		throw runtime_error("Error reading file!");

	return result;
}



void Files::Write(const filesystem::path &path, const string &data)
{
	File file(path, true);
	Write(file, data);
}



void Files::Write(struct SDL_RWops *file, const string &data)
{
	if(!file)
		return;

	SDL_RWwrite(file, data.data(), 1, data.size());
}



void Files::CreateFolder(const filesystem::path &path)
{
	if(Exists(path))
		return;

	if(filesystem::create_directory(path))
		filesystem::permissions(path, filesystem::perms(filesystem::perms::owner_all));
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

#if defined __ANDROID__
	// On android, this can be read with logcat | grep SDL. We don't want to do
	// it for other operating systems though, cause we can read errors.txt
	// directly
	SDL_Log("%s", message.c_str());
#endif

	SDL_RWwrite(errorLog, (message + '\n').c_str(), 1, message.size() + 1);
}



bool Files::RmDir(const std::filesystem::path &path)
{
	return std::filesystem::remove_all(path);
}
