/* Archive.h
Copyright (c) 2024 by RisingLeaf (the one and only if you notice this tell me to delete it XD)

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



class Archive {
public:
	class ArchiveResourceHandle {
	public:
		ArchiveResourceHandle() = default;
		~ArchiveResourceHandle();

		void Allocate(size_t newSize);
		void CreateFileFromData();
		void Clear();

		FILE *GetFile();
		File &GetFileRAI();
		unsigned char *GetData();

		explicit operator bool() const;


	private:
		File file;
		unsigned char *data = nullptr;
		size_t size = 0;
	};


public:
	static std::pair<std::string, std::vector<std::string>> GetRecursiveFileList(
		const std::string &archivePath, const std::string &subFolder);
	static std::string GetRootPath(const std::string &archivePath);
	static bool FileExists(const std::string &archivePath, const std::string &path);
	static void GetArchiveFile(const std::string &archivePath, ArchiveResourceHandle &handle);
};

