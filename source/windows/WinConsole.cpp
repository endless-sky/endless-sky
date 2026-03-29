/* WinConsole.cpp
Copyright (c) 2019 by comnom

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WinConsole.h"

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>



void WinConsole::Init()
{
	const int UNINITIALIZED = -2;
	bool redirectStdout = _fileno(stdout) == UNINITIALIZED;
	bool redirectStderr = _fileno(stderr) == UNINITIALIZED;
	bool redirectStdin = _fileno(stdin) == UNINITIALIZED;

	// Bail if stdin, stdout, and stderr are already initialized (e.g. writing to a file)
	if(!redirectStdout && !redirectStderr && !redirectStdin)
		return;

	// Bail if we fail to attach to the console
	if(!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole())
		return;

	// Perform console redirection.
#ifdef _MSC_VER
	// Use the "safe" function with MSVC.
	if(redirectStdout)
	{
		FILE *fstdout = nullptr;
		freopen_s(&fstdout, "CONOUT$", "w", stdout);
		if(fstdout)
			setvbuf(stdout, nullptr, _IOFBF, 4096);
	}
	if(redirectStderr)
	{
		FILE *fstderr = nullptr;
		freopen_s(&fstderr, "CONOUT$", "w", stderr);
		if(fstderr)
			setvbuf(stderr, nullptr, _IOLBF, 1024);
	}
	if(redirectStdin)
	{
		FILE *fstdin = nullptr;
		freopen_s(&fstdin, "CONIN$", "r", stdin);
		if(fstdin)
			setvbuf(stdin, nullptr, _IONBF, 0);
	}
#else
	if(redirectStdout && freopen("CONOUT$", "w", stdout))
		setvbuf(stdout, nullptr, _IOFBF, 4096);
	if(redirectStderr && freopen("CONOUT$", "w", stderr))
		setvbuf(stderr, nullptr, _IOLBF, 1024);
	if(redirectStdin && freopen("CONIN$", "r", stdin))
		setvbuf(stdin, nullptr, _IONBF, 0);
#endif
}
