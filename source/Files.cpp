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
#include "text/Utf8.h"
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <memory>

#ifdef __ANDROID__
#include "AndroidAsset.h"
#endif

using namespace std;

namespace {
	string resources;
	string config;

	string dataPath;
	string imagePath;
	string soundPath;
	string savePath;
	string testPath;

	File errorLog;

	// Convert windows-style directory separators ('\\') to standard '/'.
#if defined _WIN32
	void FixWindowsSlashes(string &path)
	{
		for(char &c : path)
			if(c == '\\')
				c = '/';
	}
#endif

	// Open the given folder in a separate window.
	void OpenFolder(const string &path)
	{
#if SDL_VERSION_ATLEAST(2, 0, 14)
		if(SDL_OpenURL(("file://" + path).c_str()))
			Logger::LogError("Warning: SDL_OpenURL failed with \"" + string(SDL_GetError()) + "\"");
#elif defined(__linux__)
		// Some supported distributions do not have an up-to-date SDL.
		cout.flush();
		if(int result = WEXITSTATUS(system(("xdg-open file://" + path).c_str())))
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
#endif
	// If the resources are not here, search in the directories containing this
	// one. This allows, for example, a Mac app that does not actually have the
	// resources embedded within it.
	while(!Exists(resources + "credits.txt"))
	{
		size_t pos = resources.rfind('/', resources.length() - 2);
		if(pos == string::npos || pos == 0)
			throw runtime_error("Unable to find the resource directories!");
		resources.erase(pos + 1);
	}
	dataPath = resources + "data/";
	imagePath = resources + "images/";
	soundPath = resources + "sounds/";
	if(config.empty())
	{
		// Find the path to the directory for saved games (and create it if it does
		// not already exist). This can also be overridden in the command line.
      char *str = SDL_GetPrefPath("endless-sky", "saves");
		if(!str)
			throw runtime_error("Unable to get path to saves directory!");
		savePath = str;
		SDL_free(str);
#if defined _WIN32
		FixWindowsSlashes(savePath);
#endif
		if(savePath.back() != '/')
			savePath += '/';
		config = savePath.substr(0, savePath.rfind('/', savePath.length() - 2) + 1);
	}
	else
	{
#if defined _WIN32
		FixWindowsSlashes(config);
#endif
		if(config.back() != '/')
			config += '/';
		savePath = config + "saves/";
	}

	// Create the "plugins" directory if it does not yet exist, so that it is
	// clear to the user where plugins should go.
	{
		char *str = SDL_GetPrefPath("endless-sky", "plugins");
		if(str != nullptr)
			SDL_free(str);
	}

	// Check that all the directories exist. Allow the sound path to be missing
	if(!Exists(dataPath) || !Exists(imagePath))
		throw runtime_error("Unable to find the resource directories!");
	if(!Exists(savePath))
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
	return dataPath;
}



const string &Files::Images()
{
	return imagePath;
}



const string &Files::Sounds()
{
	return soundPath;
}



const string &Files::Saves()
{
	return savePath;
}



const string &Files::Tests()
{
	return testPath;
}



vector<string> Files::List(string directory)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';

	vector<string> list;

#if defined _WIN32
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(Utf8::ToUTF16(directory + '*').c_str(), &ffd);
	if(!hFind)
		return list;

	do {
		if(ffd.cFileName[0] == '.')
			continue;

		if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			list.push_back(directory + Utf8::ToUTF8(ffd.cFileName));
	} while(FindNextFileW(hFind, &ffd));

	FindClose(hFind);
#else
	DIR *dir = opendir(directory.c_str());
	if(!dir)
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
         if (File(directory + entry))
         {
            list.push_back(directory + entry);
         }
      }
#endif
		return list;
   }

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
#endif

	sort(list.begin(), list.end());
	return list;
}



// Get a list of any directories in the given directory.
vector<string> Files::ListDirectories(string directory)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';

	vector<string> list;

#if defined _WIN32
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(Utf8::ToUTF16(directory + '*').c_str(), &ffd);
	if(!hFind)
		return list;

	do {
		if(ffd.cFileName[0] == '.')
			continue;

		if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			list.push_back(directory + Utf8::ToUTF8(ffd.cFileName) + '/');
	} while(FindNextFileW(hFind, &ffd));

	FindClose(hFind);
#else
	DIR *dir = opendir(directory.c_str());
	if(!dir)
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
         if (!File(directory + entry))
         {
            list.push_back(directory + entry + "/");
         }
      }
#endif
		return list;
   }

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
#endif

	sort(list.begin(), list.end());
	return list;
}



vector<string> Files::RecursiveList(const string &directory)
{
	vector<string> list;
	RecursiveList(directory, &list);
	sort(list.begin(), list.end());
	return list;
}



void Files::RecursiveList(string directory, vector<string> *list)
{
	if(directory.empty() || directory.back() != '/')
		directory += '/';

#if defined _WIN32
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(Utf8::ToUTF16(directory + '*').c_str(), &ffd);
	if(hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		if(ffd.cFileName[0] == '.')
			continue;

		if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			list->push_back(directory + Utf8::ToUTF8(ffd.cFileName));
		else
			RecursiveList(directory + Utf8::ToUTF8(ffd.cFileName) + '/', list);
	} while(FindNextFileW(hFind, &ffd));

	FindClose(hFind);
#else
	DIR *dir = opendir(directory.c_str());
	if(!dir)
	{
#if defined __ANDROID__
	// try the asset system. We don't want to instantiate more than one
	// AndroidAsset object, so don't recurse.
	vector<string> directories;
	directories.push_back(directory);
	AndroidAsset aa;
	while (!directories.empty())
	{
		directory = directories.back();
		directories.pop_back();
		for (std::string entry: aa.DirectoryList(directory))
		{
			// Asset api doesn't have a stat or entry properties. the only
			// way to tell if its a folder is to fail to open it. Depending
			// on how slow this is, it may be worth checking for a file
			// extension instead.
			if (File(directory + entry))
			{
				list->push_back(directory + entry);
			}
			else
			{
				directories.push_back(directory + entry + "/");
			}
		}
	}
#endif
	return;
	}

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
#endif
}



bool Files::Exists(const string &filePath)
{
#if defined _WIN32
	struct _stat buf;
	return !_wstat(Utf8::ToUTF16(filePath).c_str(), &buf);
#elif defined __ANDROID__
	struct stat buf;
	bool exists = !stat(filePath.c_str(), &buf);
	if (!exists)
	{
		// stat only works for normal files, not assets, so just try to open the
		// file to see if it exists.
		SDL_RWops* f = SDL_RWFromFile(filePath.c_str(), "r");
		exists = f != nullptr;
		if (f) SDL_RWclose(f);
		if (!exists)
		{
			// check and see if it is a directory
			exists = AndroidAsset().DirectoryExists(filePath);
		}
	}
	return exists;
#else
	struct stat buf;
	return !stat(filePath.c_str(), &buf);
#endif
}



time_t Files::Timestamp(const string &filePath)
{
#if defined _WIN32
	struct _stat buf;
	_wstat(Utf8::ToUTF16(filePath).c_str(), &buf);
#else
	struct stat buf;
	stat(filePath.c_str(), &buf);
#endif
	return buf.st_mtime;
}



void Files::Copy(const string &from, const string &to)
{
#if defined _WIN32
	CopyFileW(Utf8::ToUTF16(from).c_str(), Utf8::ToUTF16(to).c_str(), false);
#else
	Write(to, Read(from));
	// Preserve the timestamps of the original file.
	struct stat buf;
	if(stat(from.c_str(), &buf))
		Logger::LogError("Error: Cannot stat \"" + from + "\".");
	else
	{
		struct utimbuf times;
		times.actime = buf.st_atime;
		times.modtime = buf.st_mtime;
		if(utime(to.c_str(), &times))
			Logger::LogError("Error: Failed to preserve the timestamps for \"" + to + "\".");
	}
#endif
}



void Files::Move(const string &from, const string &to)
{
#if defined _WIN32
	MoveFileExW(Utf8::ToUTF16(from).c_str(), Utf8::ToUTF16(to).c_str(), MOVEFILE_REPLACE_EXISTING);
#else
	rename(from.c_str(), to.c_str());
#endif
}



void Files::Delete(const string &filePath)
{
#if defined _WIN32
	DeleteFileW(Utf8::ToUTF16(filePath).c_str());
#else
	unlink(filePath.c_str());
#endif
}



// Get the filename from a path.
string Files::Name(const string &path)
{
	// string::npos = -1, so if there is no '/' in the path this will
	// return the entire string, i.e. path.substr(0).
	return path.substr(path.rfind('/') + 1);
}



struct SDL_RWops *Files::Open(const string &path, bool write)
{
	return SDL_RWFromFile(path.c_str(), write ? "wb" : "rb");
}



void Files::Close(struct SDL_RWops * ops)
{
	SDL_RWclose(ops);
}



string Files::Read(const string &path)
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



void Files::Write(const string &path, const string &data)
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



// Open this user's plugins directory in their native file explorer.
void Files::OpenUserPluginFolder()
{
	OpenFolder(Config() + "plugins");
}



void Files::LogErrorToFile(const string &message)
{
	if(!errorLog)
	{
		std::string path = config + "errors.txt";
		errorLog = File(path, true);

		if(!errorLog)
		{
			cerr << "Unable to create \"errors.txt\" " << (config.empty()
				? "in current directory" : "in \"" + config + "\"") << endl;
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



bool Files::MakeDir(const std::string &path)
{
	// SDL doesn't expose a file system api for creating directories. we will
	// need to use O/S primitives here.
#ifndef _WIN32
	size_t next = path.find_first_of("/\\");
	size_t pos = 0;
	std::string built_path;
   if (!path.empty() && path.front() != '/')
   {
      built_path = '.';
   }
	std::vector<std::string> to_create;
	for(;;)
	{
		std::string component = (next == path.npos) ? path.substr(pos)
		                                            : path.substr(pos, next - pos);

		if (!component.empty() && component != ".")
		{
			if (component == "..")
			{
				return false; // not doing this
			}
			built_path += "/" + component;
			struct stat buf;
			if (stat(built_path.c_str(), &buf) == 0)
			{
				if (!S_ISDIR(buf.st_mode))
				{
					// it exists, and its not a directory. can't continue
					return false;
				}
			}
			else
			{
				// doesn't exist. add it
				to_create.push_back(built_path);
			}
		}
		if (next == std::string::npos)
		{
			break;
		}
		pos = next + 1; //discard slash
	   next = path.find_first_of("/\\", pos);
	}
	for (const std::string& component: to_create)
	{
		if (mkdir(component.c_str(), 0700) != 0)
		{
			return false;
		}
	}
	return true;

#else
	return false;
#endif
}



bool Files::RmDir(const std::string &path)
{
	// SDL doesn't expose a file system api for deleting directories. We need to
	// use operating system primitives here.
#ifndef _WIN32
	// TODO: some sort of safety?
	DIR *dir = opendir(path.c_str());
	if(dir)
	{
		dirent* ent = nullptr;
		errno = 0;
		while((ent = readdir(dir)))
		{
			// Skip "." and ".."
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
				continue;

			string name = path + "/" + ent->d_name;
			struct stat st;
			stat(name.c_str(), &st);
			bool isRegularFile = S_ISREG(st.st_mode);
			bool isDirectory = S_ISDIR(st.st_mode);

			if(isRegularFile)
			{
				unlink(name.c_str());
			}
			else if(isDirectory)
			{
				RmDir(name);
			}
		}
		closedir(dir);
		rmdir(path.c_str());
		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}
