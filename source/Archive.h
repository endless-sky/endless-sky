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

#include <string>
#include <vector>

class File;



class Archive {
public:
	static std::vector<std::pair<std::string, std::string>> GetImagePaths(std::string archivePath);
	static std::vector<std::string> GetDataPaths(std::string archivePath);
	static std::pair<File, size_t> GetArchiveFile(std::string archivePath);
	static void FreeResource(size_t index);
};

