/* Files.h
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

#pragma once

#include <filesystem>
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

	// The game's installation directory, or whichever directory was passed on the command line via `--resources`
	static const std::filesystem::path &Resources();
	// The user-specific configuration directory, or whichever directory was passed on the command line via `--config`
	static const std::filesystem::path &Config();

	static const std::filesystem::path &Data();
	static const std::filesystem::path &Images();
	static const std::filesystem::path &Sounds();
	static const std::filesystem::path &Saves();
	static const std::filesystem::path &UserPlugins();
	static const std::filesystem::path &GlobalPlugins();
	static const std::filesystem::path &Tests();

	// Get a list of all regular files in the given directory.
	static std::vector<std::filesystem::path> List(const std::filesystem::path &directory);
	// Get a list of any directories in the given directory.
	static std::vector<std::filesystem::path> ListDirectories(const std::filesystem::path &directory);
	// Get a list of all regular files in the given directory or any directory
	// that it contains, recursively.
	static std::vector<std::filesystem::path> RecursiveList(const std::filesystem::path &directory);

	static bool Exists(const std::filesystem::path &filePath);
	static std::filesystem::file_time_type Timestamp(const std::filesystem::path &filePath);
	static bool Copy(const std::filesystem::path &from, const std::filesystem::path &to);
	static void Move(const std::filesystem::path &from, const std::filesystem::path &to);
	static void Delete(const std::filesystem::path &filePath);

	// Get the filename from a path.
	static std::string Name(const std::filesystem::path &path);

	/// Check whether one path is a parent of another.
	static bool IsParent(const std::filesystem::path &parent, const std::filesystem::path &child);

	// File IO.
	static std::shared_ptr<std::iostream> Open(const std::filesystem::path &path, bool write = false);
	static std::string Read(const std::filesystem::path &path);
	static std::string Read(std::shared_ptr<std::iostream> file);
	static void Write(const std::filesystem::path &path, const std::string &data);
	static void Write(std::shared_ptr<std::iostream> file, const std::string &data);
	static void CreateFolder(const std::filesystem::path &path);

	// Open this user's plugins directory in their native file explorer.
	static void OpenUserPluginFolder();
	// Open this user's save file directory in their native file explorer.
	static void OpenUserSavesFolder();

	// Logging to the error-log. Actual calls should be done through Logger
	// and not directly here to ensure that other logging actions also
	// happen (and to ensure thread safety on the logging).
	static void LogErrorToFile(const std::string &message);
};
