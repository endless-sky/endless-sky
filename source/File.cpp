/* File.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "File.h"

#include "Files.h"

#if defined _WIN32
#include <io.h>
#include <share.h>
#include <windows.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

namespace {
#if defined _WIN32
	// Adapted from https://github.com/Arryboom/fmemopen_windows
	FILE *fmemopen(unsigned char *data, size_t size, const char *type)
	{
		int fd;
		FILE *fp;
		char tp[MAX_PATH - 13];
		char fn[MAX_PATH + 1];
		int *pfd = &fd;
		char tfname[] = "MemTF_";
		if(!GetTempPathA(sizeof(tp), tp))
			return nullptr;
		if(!GetTempFileNameA(tp, tfname, 0, fn))
			return nullptr;
		int retner = _sopen_s(pfd, fn, _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY
			| _O_RDWR | _O_BINARY | _O_NOINHERIT, _SH_DENYRW, _S_IREAD | _S_IWRITE);

		if(retner != 0 || fd == -1)
			return nullptr;
		fp = _fdopen(fd, "wb+");
		if(!fp)
		{
			_close(fd);
			return nullptr;
		}
		fwrite(data, size, 1, fp);
		rewind(fp);
		return fp;
	}
#endif
};



File::File(const string &path, bool write)
{
	file = Files::Open(path, write);
}



File::File(File &&other) noexcept
{
	swap(file, other.file);
}



File::File(unsigned char *data, size_t size, const char *type)
{
	file = fmemopen(data, size, type);
}



File::~File() noexcept
{
	if(file)
		fclose(file);
}



File &File::operator=(File &&other) noexcept
{
	if(this != &other)
		swap(file, other.file);
	return *this;
}



File::operator bool() const
{
	return file;
}



File::operator FILE*() const
{
	return file;
}
