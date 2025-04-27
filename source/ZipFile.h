/* ZipFile.h
Copyright (c) 2025 by tibetiroka

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

#include <minizip/unzip.h>

#include <filesystem>
#include <mutex>
#include <vector>



/// Wrapper around a zip file that provides basic file listing and reading functions.
/// This class supports zips both with and without a top-level directory, as long as
/// the directory's name matches the zip's name. The necessary path translations are
/// performed within this class, and aren't visible to the user.
/// ZipFiles are not thread safe. A zip file may only be used on one thread at a time.
class ZipFile {
public:
	explicit ZipFile(const std::filesystem::path &zipPath);
	ZipFile(const ZipFile &other) = delete;
	~ZipFile();

	/// Lists files in a directory inside of a zip file.
	/// @param directory The complete file path, including the zip's path.
	/// @param recursive List all files in the subtree, rather than just the top level.
	/// @param directories Whether to list only directories instead of only regular files.
	std::vector<std::filesystem::path> ListFiles(const std::filesystem::path &directory, bool recursive,
		bool directories) const;

	/// Checks whether the given file or directory exists in the zip.
	/// @param filePath The complete file path, including the zip's path.
	bool Exists(const std::filesystem::path &filePath) const;

	/// Reads a frp, from the zip.
	/// @param filePath The complete file path, including the zip's path.
	std::string ReadFile(const std::filesystem::path &filePath) const;


private:
	/// Translates a global filesystem path to a relative path within the zip file.
	/// @param path The complete file path, including the zip's path.
	std::filesystem::path GetPathInZip(const std::filesystem::path &path) const;
	/// Translates an in-zip relative path to a global filesystem path.
	/// @param path The in-zip path, without the zip's own path.
	std::filesystem::path GetGlobalPath(const std::filesystem::path &path) const;


private:
	/// The zip handle
	unzFile zipFile = nullptr;
	/// The path of the zip file in the filesystem
	std::filesystem::path basePath;
	/// The name of the top-level directory inside the zip, or an empty string if it doesn't have such a directory
	std::filesystem::path topLevelDirectory;
};
