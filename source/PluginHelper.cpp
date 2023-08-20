/* PluginHelper.cpp
Copyright (c) 2023 by RisingLeaf(https://github.com/RisingLeaf)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PluginHelper.h"

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <archive.h>
#include <archive_entry.h>
#include <cstring>
#include <curl/curl.h>
#include <sys/stat.h>
#include <cstdio>

#ifdef _WIN32
#include "text/Utf8.h"
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;



namespace PluginHelper {
	// Max size for extracting an archive in bytes this will be 1GB.
	const int MAX_SIZE = 1000000000;

	size_t WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
		size_t written = fwrite(ptr, size, nmemb, stream);
		return written;
	}



	bool Download(string url, string location)
	{
		CURL *curl = curl_easy_init();
		if(!curl)
			return false;
#if defined _WIN32
		FILE *out = nullptr;
		_wfopen_s(&out, Utf8::ToUTF16(location).c_str(), L"wb");
#else
		FILE *out = fopen(location.c_str(), "wb");
#endif
		// Set the url that gets downloaded
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		// Follow redirects
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);
		// How long we will wait
		curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);
		// What is the maximum filesize in bytes.
		curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, MAX_SIZE);
		// Set the write function and the output file used in the write function
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

		CURLcode res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
		fclose(out);
		return res == CURLE_OK;
	}



	// Copy an entry from one archive to the other
	void CopyData(struct archive *ar, struct archive *aw)
	{
		int retVal;
		const void *buff;
		size_t size;
		int64_t offset;

		while(true)
		{
			retVal = archive_read_data_block(ar, &buff, &size, &offset);
			if(retVal == ARCHIVE_EOF)
				return;
			if(retVal != ARCHIVE_OK)
				return;
			if(archive_write_data_block(aw, buff, size, offset) != ARCHIVE_OK)
				return;
		}
	}



	bool ExtractZIP(string filename, string destination, string expectedName)
	{
		int size = 0;

		int flags = ARCHIVE_EXTRACT_TIME;
		flags |= ARCHIVE_EXTRACT_PERM;
		flags |= ARCHIVE_EXTRACT_ACL;
		flags |= ARCHIVE_EXTRACT_FFLAGS;

		// Create the handles for reading/writing
		archive *read = archive_read_new();
		archive *ext = archive_write_disk_new();
		archive_write_disk_set_options(ext, flags);
		archive_read_support_format_all(read);
		if(!filename.empty() && filename.front() == '-')
			filename.clear();
		if(archive_read_open_filename(read, filename.c_str(), 10240))
			return false;

		// Check if this plugin has the right head folder name
		archive_entry *entry;
		archive_read_next_header(read, &entry);
		string firstEntry = archive_entry_pathname(entry);
		bool fitsExpected = firstEntry == (expectedName);
		archive_read_data_skip(read);

		// Check if this plugin has a head folder, if not create one in the destination
		archive_read_next_header(read, &entry);
		string secondEntry = archive_entry_pathname(entry);
		bool hasHeadFolder = secondEntry.find(firstEntry) != std::string::npos;
		if(!hasHeadFolder)
#if defined(_WIN32)
			_wmkdir(Utf8::ToUTF16(destination + expectedName).c_str());
#else
			mkdir((destination + expectedName).c_str(), 0777);
#endif

		// Close the archive so we can start again from the beginning
		archive_read_close(read);
		archive_read_free(read);

		// Read another time, this time for writing.
		read = archive_read_new();
		archive_read_support_format_all(read);
		archive_read_open_filename(read, filename.c_str(), 10240);

		string dest_file;
		int retVal;
		while (true)
		{
			retVal = archive_read_next_header(read, &entry);
			if(retVal == ARCHIVE_EOF)
				break;
			if(retVal != ARCHIVE_OK)
				return false;

			size += archive_entry_size(entry);
			if(size > MAX_SIZE)
				return false;

			// Adjust root folder name if neccessary.
			if(!fitsExpected && hasHeadFolder)
			{
				string thisEntryName = archive_entry_pathname(entry);
				size_t start_pos = thisEntryName.find(firstEntry);
				if(start_pos != std::string::npos)
					thisEntryName.replace(start_pos, firstEntry.length(), expectedName);

				archive_entry_set_pathname(entry, thisEntryName.c_str());
			}

			// Add root folder to path if neccessary.
			dest_file = (destination + (hasHeadFolder ? "" : expectedName)) + archive_entry_pathname(entry);
			archive_entry_set_pathname(entry, dest_file.c_str());

			// Write files.
			if(archive_write_header(ext, entry) == ARCHIVE_OK)
			{
				CopyData(read, ext);
				if(archive_write_finish_entry(ext) != ARCHIVE_OK)
					return false;
			}
		}

		// Free all data.
		archive_read_close(read);
		archive_read_free(read);
		archive_write_close(ext);
		archive_write_free(ext);
		return true;
	}
}
