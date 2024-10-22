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
using ReadingHandle = unique_ptr<archive, int(*)(archive *)>;
using EntryHandle = unique_ptr<archive_entry, void(*)(archive_entry *)>;

namespace {
	bool InitArchive(const string &path, ReadingHandle &reading, EntryHandle &entry, string &firstEntry)
	{
		archive_read_support_filter_all(reading.get());
		archive_read_support_format_all(reading.get());
		int status = archive_read_open_filename(reading.get(), path.c_str(), 10240);
		if(status != ARCHIVE_OK)
			return false;
		size_t start = path.rfind("/");
		string cleanedArchiveName = path.substr(start, path.size() - start - 4) + "/";

		// Find out if the archive contains a head folder.
		if(archive_read_next_header2(reading.get(), entry.get()) != ARCHIVE_OK)
			return false;

		firstEntry = archive_entry_pathname(entry.get());
		bool hasHeadFolder = firstEntry == cleanedArchiveName;
		if(hasHeadFolder)
			firstEntry = firstEntry.substr(0, firstEntry.find("/") + 1);
		else
			firstEntry = "";
		archive_read_data_skip(reading.get());

		// If the archive has no head folder rewind to the start.
		if(!hasHeadFolder)
		{
			reading.reset();
			reading = ReadingHandle(archive_read_new(), archive_read_free);
			archive_read_support_filter_all(reading.get());
			archive_read_support_format_all(reading.get());
			int ret = archive_read_open_filename(reading.get(), path.c_str(), 10240);
			if(ret != ARCHIVE_OK)
				return false;
		}

		return true;
	}
};



Archive::ArchiveResourceHandle::~ArchiveResourceHandle()
{
	Clear();
}



void Archive::ArchiveResourceHandle::Allocate(size_t newSize)
{
	Clear();
	size = newSize;
	data = new unsigned char[size];
}



void Archive::ArchiveResourceHandle::CreateFileFromData()
{
	file = File(data, size, "rb");
}



void Archive::ArchiveResourceHandle::Clear() noexcept
{
	delete[] data;
	size = 0;
}


File &Archive::ArchiveResourceHandle::GetFile()
{
	return file;
}



unsigned char * const Archive::ArchiveResourceHandle::GetData()
{
	return data;
}



Archive::ArchiveResourceHandle::operator bool() const
{
	return size;
}



vector<string> Archive::GetRecursiveFileList(const string &archiveFolderPath)
{
	vector<string> result;

	size_t start = archiveFolderPath.find(".zip/");
	if(start == string::npos)
		return result;

	string archivePath = archiveFolderPath.substr(0, start + 4);
	string filePath = archiveFolderPath.substr(start + 5, archiveFolderPath.size());

	ReadingHandle reading(archive_read_new(), archive_read_free);
	EntryHandle entry(archive_entry_new(), archive_entry_free);
	string firstEntry;

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return result;

	const string directoryPath = firstEntry + filePath;


	while(archive_read_next_header2(reading.get(), entry.get()) == ARCHIVE_OK)
	{
		const string name = archive_entry_pathname(entry.get());
		if(!name.starts_with(directoryPath))
			continue;

		result.emplace_back(archivePath + "/" + name.substr(firstEntry.size(), name.size() - firstEntry.size()));
	}

	return result;
}



bool Archive::FileExists(const string &archiveFilePath)
{
	// First split path up into archive path and inside archive path.
	size_t start = archiveFilePath.find(".zip/");
	if(start == string::npos)
		return false;

	string archivePath = archiveFilePath.substr(0, start + 4);
	string filePath = archiveFilePath.substr(start + 5, archiveFilePath.size());


	ReadingHandle reading(archive_read_new(), archive_read_free);
	EntryHandle entry(archive_entry_new(), archive_entry_free);
	string firstEntry = "";

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return false;

	filePath = firstEntry + filePath;

	while(archive_read_next_header2(reading.get(), entry.get()) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry.get());
		if(name.ends_with("/"))
			name = name.substr(0, name.size() - 1);
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

	ReadingHandle reading(archive_read_new(), archive_read_free);
	EntryHandle entry(archive_entry_new(), archive_entry_free);
	string firstEntry;

	if(!InitArchive(archivePath, reading, entry, firstEntry))
		return;

	filePath = firstEntry + filePath;

	while(archive_read_next_header2(reading.get(), entry.get()) == ARCHIVE_OK)
	{
		string name = archive_entry_pathname(entry.get());
		if(name == filePath)
		{
			size_t size = archive_entry_size(entry.get());

			handle.Allocate(size);
			archive_read_data(reading.get(), handle.GetData(), size);
			handle.CreateFileFromData();

			return;
		}
	}
}
