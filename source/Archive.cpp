/* Archive.cpp
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

#include "Archive.h"

#include "File.h"
#include "image/ImageSet.h"

#include <archive.h>
#include <archive_entry.h>
#include "Logger.h"

using namespace std;

namespace {
	vector<void *> resources(1);
}


vector<pair<string, string>> Archive::GetImagePaths(string archivePath)
{
	vector<pair<string, string>> result;

	struct archive *reading = archive_read_new();
	archive_read_support_filter_all(reading);
	archive_read_support_format_all(reading);
	int ret = archive_read_open_filename(reading, archivePath.c_str(), 10240);
	if(ret != ARCHIVE_OK)
		return result;
	
	archive_entry *entry;
	archive_read_next_header(reading, &entry);
	string firstEntry = archive_entry_pathname(entry);
	firstEntry = firstEntry.substr(0, firstEntry.find("/")) + "/";
	archive_read_data_skip(reading);


	string directoryPath = firstEntry + "images/";
	size_t start = directoryPath.size();
	

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry);
		if(!name.starts_with(directoryPath))
			continue;
		if(!ImageSet::IsImage(name))
			continue;
		result.emplace_back(archivePath + "/" + name, ImageSet::Name(name.substr(start)));
	}
	archive_read_free(reading);

	return result;
}



vector<string> Archive::GetDataPaths(string archivePath)
{
	vector<string> result;

	struct archive *reading = archive_read_new();
	archive_read_support_filter_all(reading);
	archive_read_support_format_all(reading);
	int ret = archive_read_open_filename(reading, archivePath.c_str(), 10240);
	if(ret != ARCHIVE_OK)
		return result;
	
	archive_entry *entry;
	archive_read_next_header(reading, &entry);
	string firstEntry = archive_entry_pathname(entry);
	firstEntry = firstEntry.substr(0, firstEntry.find("/")) + "/";
	archive_read_data_skip(reading);


	string directoryPath = firstEntry + "data/";
	

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry);
		if(!name.starts_with(directoryPath))
			continue;
		result.emplace_back(archivePath + "/" + name);
	}
	archive_read_free(reading);

	return result;
}



pair<File, size_t> Archive::GetArchiveFile(std::string archiveFilePath)
{
	// First split path up into archive path and inside archive path.
	size_t start = archiveFilePath.find(".zip/");
	if(start == string::npos)
		return std::make_pair(File(), 0);

	string archivePath = archiveFilePath.substr(0, start + 4);
	string filePath = archiveFilePath.substr(start + 5, archiveFilePath.size());
	
	struct archive *reading = archive_read_new();
	archive_read_support_filter_all(reading);
	archive_read_support_format_all(reading);
	int ret = archive_read_open_filename(reading, archivePath.c_str(), 10240);
	if(ret != ARCHIVE_OK)
		return std::make_pair(File(), 0);
	
	archive_entry *entry;
	archive_read_next_header(reading, &entry);
	string firstEntry = archive_entry_pathname(entry);
	firstEntry = firstEntry.substr(0, firstEntry.find("/")) + "/";
	archive_read_data_skip(reading);
	

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry);
		if(name == filePath)
		{
			size_t size = archive_entry_size(entry);
			resources.emplace_back();
			resources.back() = malloc(size);
			archive_read_data(reading, resources.back(), size);

			File file(resources.back(), size, "rb");

			return std::make_pair(std::move(file), resources.size() - 1);
		}
	}
	archive_read_free(reading);

	return std::make_pair(File(), 0);
}



void Archive::FreeResource(size_t index)
{
	free(resources[index]);
}
