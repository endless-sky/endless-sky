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

#include "image/ImageSet.h"

#include <archive.h>
#include <archive_entry.h>

using namespace std;

namespace {
	bool InitArchive(const string &path, archive *&reading, archive_entry *&entry, string &firstEntry)
	{
		reading = archive_read_new();
		archive_read_support_filter_all(reading);
		archive_read_support_format_all(reading);
		int ret = archive_read_open_filename(reading, path.c_str(), 10240);
		if(ret != ARCHIVE_OK)
			return false;
		
		size_t start = path.rfind("/");
		string cleanedArchiveName = path.substr(start, path.size() - start - 4) + "/";

		// Find out if the archive contains a head folder.
		archive_read_next_header(reading, &entry);
		firstEntry = archive_entry_pathname(entry);
		bool hasHeadFolder = firstEntry == cleanedArchiveName;
		if(hasHeadFolder)
			firstEntry = firstEntry.substr(0, firstEntry.find("/") + 1);
		else
			firstEntry = "";
		archive_read_data_skip(reading);

		// If the archive has no head folder rewind to the start.
		if(!hasHeadFolder)
		{
			archive_read_free(reading);

			reading = archive_read_new();
			archive_read_support_filter_all(reading);
			archive_read_support_format_all(reading);
			int ret = archive_read_open_filename(reading, path.c_str(), 10240);
			if(ret != ARCHIVE_OK)
				return false;
		}

		return true;
	}
};



Archive::ArchiveResourceHandle::~ArchiveResourceHandle()
{
	if(data)
		free(data);
}



void Archive::ArchiveResourceHandle::Allocate(size_t newSize)
{
	data = reinterpret_cast<unsigned char *>(malloc(newSize));

	size = newSize;
}



void Archive::ArchiveResourceHandle::CreateFileFromData()
{
	file = File(data, size, "rb");
}



void Archive::ArchiveResourceHandle::Clear()
{
	if(data)
		free(data);
	size = 0;
}



FILE *Archive::ArchiveResourceHandle::GetFile()
{
	return file;
}



File &Archive::ArchiveResourceHandle::GetFileRAI()
{
	return file;
}



unsigned char *Archive::ArchiveResourceHandle::GetData()
{
	return data;
}



Archive::ArchiveResourceHandle::operator bool() const
{
	return size;
}



pair<string, vector<string>> Archive::GetRecursiveFileList(const string &archivePath, const string &subFolder)
{
	pair<string, vector<string>> result;

	struct archive *reading = nullptr;
	archive_entry *entry = nullptr;
	string firstEntry;

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return result;

	const string directoryPath = firstEntry + subFolder;

	result.first = archivePath + "/" + directoryPath;

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		const string name = archive_entry_pathname(entry);
		if(!name.starts_with(directoryPath))
			continue;
		result.second.emplace_back(archivePath + "/" + name);
	}
	archive_read_free(reading);

	return result;
}



string Archive::GetRootPath(const string &archivePath)
{
	struct archive *reading = nullptr;
	archive_entry *entry = nullptr;
	string firstEntry = "";

	InitArchive(archivePath, reading, entry, firstEntry);

	return firstEntry;
}



bool Archive::FileExists(const string &archiveFilePath)
{
	// First split path up into archive path and inside archive path.
	size_t start = archiveFilePath.find(".zip/");
	if(start == string::npos)
		return false;

	string archivePath = archiveFilePath.substr(0, start + 4);
	string filePath = archiveFilePath.substr(start + 5, archiveFilePath.size());

	struct archive *reading = nullptr;
	archive_entry *entry = nullptr;
	string firstEntry = "";

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return false;

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry);
		if(name == filePath)
			return true;
	}

	return false;
}



void Archive::GetArchiveFile(const std::string &archiveFilePath, Archive::ArchiveResourceHandle &handle)
{
	// First split path up into archive path and inside archive path.
	size_t start = archiveFilePath.find(".zip/");
	if(start == string::npos)
		return;

	string archivePath = archiveFilePath.substr(0, start + 4);
	string filePath = archiveFilePath.substr(start + 5, archiveFilePath.size());

	struct archive *reading = nullptr;
	archive_entry *entry = nullptr;
	string firstEntry;

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return;

	while(archive_read_next_header(reading, &entry) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry);
		if(name == filePath)
		{
			size_t size = archive_entry_size(entry);

			handle.Allocate(size);
			archive_read_data(reading, handle.GetData(), size);
			handle.CreateFileFromData();

			return;
		}
	}
	archive_read_free(reading);

	return;
}
