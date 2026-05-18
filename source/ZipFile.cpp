/* ZipFile.cpp
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

#include "ZipFile.h"

#include "Files.h"

#include <functional>
#include <numeric>

using namespace std;



ZipFile::ZipFile(const filesystem::path &zipPath)
	: basePath(zipPath)
{
	zipFile = unzOpen(basePath.string().c_str());
	if(!zipFile)
		throw runtime_error("Failed to open ZIP file" + zipPath.generic_string());

	// Check whether this zip has a single top-level directory (such as high-dpi.zip/high-dpi)
	filesystem::path topLevel;
	for(const filesystem::path &path : ListFiles("", true, false))
	{
		filesystem::path zipPath = path.lexically_relative(basePath);
		if(topLevel.empty())
			topLevel = *zipPath.begin();
		else if(*zipPath.begin() != topLevel)
			return;
	}
	topLevelDirectory = topLevel;
}



ZipFile::~ZipFile()
{
	if(zipFile)
	{
		unzClose(zipFile);
	}
}



vector<filesystem::path> ZipFile::ListFiles(const filesystem::path &directory, bool recursive, bool directories) const
{
	filesystem::path relative = GetPathInZip(directory);
	vector<filesystem::path> fileList;

	if(unzGoToFirstFile(zipFile) != UNZ_OK)
		throw runtime_error("Failed to go to first file in ZIP");
	do {
		char filename[256];
		unz_file_info fileInfo;
		unzGetCurrentFileInfo(zipFile, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0);

		filesystem::path zipEntry = filename;
		bool isDirectory = string(filename).back() == '/';
		bool isValidSubtree = Files::IsParent(relative, zipEntry);
		bool isRecursive = distance(zipEntry.begin(), zipEntry.end()) == distance(relative.begin(), relative.end()) + 1;

		if(isValidSubtree && isDirectory == directories && (!isRecursive || recursive))
			fileList.push_back(GetGlobalPath(zipEntry));
	} while(unzGoToNextFile(zipFile) == UNZ_OK);

	return fileList;
}



bool ZipFile::Exists(const filesystem::path &filePath) const
{
	filesystem::path relative = GetPathInZip(filePath);
	string name = relative.generic_string();

	return unzLocateFile(zipFile, name.c_str(), 0) == UNZ_OK ||
			unzLocateFile(zipFile, (name + "/").c_str(), 0) == UNZ_OK;
}



string ZipFile::ReadFile(const filesystem::path &filePath) const
{
	filesystem::path relative = GetPathInZip(filePath);

	if(unzLocateFile(zipFile, relative.generic_string().c_str(), 0) != UNZ_OK)
		return {};

	if(unzOpenCurrentFile(zipFile) != UNZ_OK)
		return {};

	unz_file_info64 info;

	info.uncompressed_size = 0;
	unzGetCurrentFileInfo64(zipFile, &info, nullptr, 0, nullptr, 0, nullptr, 0);

	char buffer[8192];
	string contents;
	int bytesRead = 0;
	while((bytesRead = unzReadCurrentFile(zipFile, buffer, sizeof(buffer))) > 0)
		contents.append(buffer, bytesRead);

	unzCloseCurrentFile(zipFile);

	if(bytesRead < 0)
		return {};

	return contents;
}



filesystem::path ZipFile::GetPathInZip(const filesystem::path &path) const
{
	filesystem::path relative = path.lexically_relative(basePath);
	if(!topLevelDirectory.empty())
		relative = topLevelDirectory / relative;
	return relative;
}



filesystem::path ZipFile::GetGlobalPath(const filesystem::path &path) const
{
	if(path.empty())
		return path;

	// If this zip has a top-level directory, remove it from the path.
	if(!topLevelDirectory.empty())
		return basePath / accumulate(next(path.begin()), path.end(), filesystem::path{}, std::divides{});
	return basePath / path;
}
