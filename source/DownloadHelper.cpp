/* DownloadHelper.cpp
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

#include "DownloadHelper.h"

#include <archive.h>
#include <cstring>
#include <curl/curl.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

using namespace std;



namespace DownloadHelper {
	size_t WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
		size_t written = fwrite(ptr, size, nmemb, stream);
		return written;
	}



	bool Download(const char *url, const char *location)
	{
		CURL *curl;
		CURLcode res = CURLE_OK;

		curl_global_init(CURL_GLOBAL_DEFAULT);

		curl = curl_easy_init();
		if(curl)
		{
			FILE *out = fopen(location, "wb");
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);
			curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);

			fclose(out);
		}

		curl_global_cleanup();

		return res == CURLE_OK;
	}



	int CopyData(struct archive *ar, struct archive *aw)
	{
		int retVal;
		const void *buff;
		size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
		int64_t offset;
#else
		off_t offset;
#endif

		for(;;)
		{
			retVal = archive_read_data_block(ar, &buff, &size, &offset);
			if(retVal == ARCHIVE_EOF)
				return (ARCHIVE_OK);
			if(retVal != ARCHIVE_OK)
				return (retVal);
			retVal = archive_write_data_block(aw, buff, size, offset);
			if(retVal != ARCHIVE_OK)
			{
				printf("archive_write_data_block(), %s", archive_error_string(aw));
				return (retVal);
			}
		}
	}



	bool ExtractZIP(const char *filename, const char *destination)
	{
		char originalDir[1000];
		getcwd(originalDir, 1000);
		chdir(destination);
		struct archive *a;
		struct archive *ext;
		struct archive_entry *entry;
		int retVal;

		int flags;
		flags = ARCHIVE_EXTRACT_TIME;
		flags |= ARCHIVE_EXTRACT_PERM;
		flags |= ARCHIVE_EXTRACT_ACL;
		flags |= ARCHIVE_EXTRACT_FFLAGS;

		a = archive_read_new();
		ext = archive_write_disk_new();
		archive_write_disk_set_options(ext, flags);
		archive_read_support_format_all(a);
		if(filename != NULL && strcmp(filename, "-") == 0)
			filename = NULL;
		if((retVal = archive_read_open_filename(a, filename, 10240)))
		{
			printf("archive_read_open_filename(), %s, %i", archive_error_string(a), retVal);
			chdir(originalDir);
			return false;
		}
		for(;;)
		{
			retVal = archive_read_next_header(a, &entry);
			if(retVal == ARCHIVE_EOF)
				break;
			if(retVal != ARCHIVE_OK)
			{
				printf("archive_read_next_header(), %s, %i", archive_error_string(a), retVal);
				chdir(originalDir);
				return false;
			}
			retVal = archive_write_header(ext, entry);
			if(retVal != ARCHIVE_OK)
				printf("archive_write_header(), %s", archive_error_string(ext));
			else
			{
				CopyData(a, ext);
				retVal = archive_write_finish_entry(ext);
				if(retVal != ARCHIVE_OK)
				{
					printf("archive_write_finish_entry(), %s, %i", archive_error_string(ext), 1);
					chdir(originalDir);
					return false;
				}
			}
		}
		archive_read_close(a);
		archive_read_free(a);

		archive_write_close(ext);
		archive_write_free(ext);
		chdir(originalDir);
		return true;
	}
}
