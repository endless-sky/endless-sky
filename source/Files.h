/* Files.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FILES_H_
#define FILES_H_

#include <cstdio>
#include <ctime>
#include <string>
#include <vector>



// File paths and file handling are different on each operating system. This
// class stores the path, on each operating system, to the game's resources -
// images, data files, etc. - and also to the "configuration" directory where
// saved games and other user-specific information can be stored. It also
// provides an interface for file operations so that the rest of the code can
// be completely platform-agnostic.
class Files {
public:
	static void Init(const char * const *argv);
	
	static const std::string &Resources();
	static const std::string &Config();
	
	static const std::string &Data();
	static const std::string &Images();
	static const std::string &Sounds();
	static const std::string &Saves();
	
	// Get a list of all regular files in the given directory.
	static std::vector<std::string> List(std::string directory);
	// Get a list of any directories in the given directory.
	static std::vector<std::string> ListDirectories(std::string directory);
	// Get a list of all regular files in the given directory or any directory
	// that it contains, recursively.
	static std::vector<std::string> RecursiveList(const std::string &directory);
	static void RecursiveList(std::string directory, std::vector<std::string> *list);
	
	static bool Exists(const std::string &filePath);
	static std::time_t Timestamp(const std::string &filePath);
	static void Copy(const std::string &from, const std::string &to);
	static void Move(const std::string &from, const std::string &to);
	static void Delete(const std::string &filePath);
	
	// Get the filename from a path.
	static std::string Name(const std::string &path);
	
	// File IO.
	static FILE *Open(const std::string &path, bool write = false);
	static std::string Read(const std::string &path);
	static std::string Read(FILE *file);
	static void Write(const std::string &path, const std::string &data);
	static void Write(FILE *file, const std::string &data);
	
	static void LogError(const std::string &message);
};



#endif
