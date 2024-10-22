/* Archive.h
Copyright (c) 2024 by RisingLeaf

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

#include "File.h"

#include <string>
#include <vector>


// Wrapper class for everything related to libarchive, allows easy access to
// any archive and is able to store files in memory.
class Archive {
public:
	// Safety class for storing files in memory, makes sure everything gets cleaned
	// up neatly and hopefully safely.
	class ArchiveResourceHandle {
	public:
		ArchiveResourceHandle() = default;
		~ArchiveResourceHandle();

		ArchiveResourceHandle(const ArchiveResourceHandle &) = delete;
		ArchiveResourceHandle(const ArchiveResourceHandle &&) = delete;
		ArchiveResourceHandle &operator=(const ArchiveResourceHandle &) = delete;
		ArchiveResourceHandle &operator=(const ArchiveResourceHandle &&) = delete;

		void Allocate(size_t newSize);
		void CreateFileFromData();
		void Clear() noexcept;

		File &GetFile();
		unsigned char * const GetData();

		explicit operator bool() const;


	private:
		File file;
		unsigned char *data = nullptr;
		size_t size = 0;
	};


public:
	static std::vector<std::string> GetRecursiveFileList(const std::string &archiveFolderPath);
	static bool FileExists(const std::string &archivePath);
	static void GetArchiveFile(const std::string &archivePath, ArchiveResourceHandle &handle);
};

