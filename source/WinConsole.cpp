/* WinConsole.cpp
Copyright (c) 2017 by Frederick Goy IV

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WinConsole.h"

#include <fcntl.h>
#include <io.h>

#include <mutex>

using namespace std;

namespace {
	mutex consoleLogMutex;
	FILE *consoleLog = nullptr;
}

void WinConsole::Init()
{
	try {
		// Find out what to redirect
		bool redirStdout = (_fileno(stdout) == -2 ? true : false);
		bool redirStderr = (_fileno(stderr) == -2 ? true : false);
		
		// Everything is being redirected at the command line
		if(!redirStdout && !redirStderr) 
			return;
			
		// Make sure we can get the std handles
		const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if(stdoutHandle == INVALID_HANDLE_VALUE) 
			throw GetLastError();
		
		const HANDLE stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
		if(stderrHandle == INVALID_HANDLE_VALUE) 
			throw GetLastError();
		
		// Attach the parent console, and handle the case where we launched with
		// arguments, but without a console (like a shortcut with target args)
		if(!AttachConsole(ATTACH_PARENT_PROCESS))
		{
			if(AllocConsole())
			{
				redirStdout = false;
				redirStderr = false;
			}
			// We shouldn't get here
			else
				throw GetLastError();
		}
		
		// Set console's max lines for large output (--ships, --weapons)
		CONSOLE_SCREEN_BUFFER_INFO conBufferInfo;
		if(!GetConsoleScreenBufferInfo(stdoutHandle, &conBufferInfo)) 
			throw GetLastError();
			
		// Make sure the user doesn't already have a larger screenbuffer size
		const short defaultSize = conBufferInfo.dwSize.Y;
		if(defaultSize < 750)
		{
			conBufferInfo.dwSize.Y = 750;
			if(!SetConsoleScreenBufferSize(stdoutHandle, conBufferInfo.dwSize)) 
				throw GetLastError();
		}
		
		// Redirect
		if(redirStdout) 
			Redirect(stdoutHandle, stdout);
		if(redirStderr) 
			Redirect(stderrHandle, stderr);
	}
	catch(DWORD errorCode)
	{
		LPTSTR msgBuffer = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&msgBuffer),
			0,
			NULL);
		
		WriteConsoleLog(msgBuffer);
		LocalFree(msgBuffer);
		msgBuffer = nullptr;
	}
}



void WinConsole::Redirect(const HANDLE stdHandle, FILE *stdStream)
{
	const int conHandle = _open_osfhandle(reinterpret_cast<intptr_t>(stdHandle), _O_TEXT);
	
	FILE *conFd = _fdopen(conHandle, "w");
	
	// Set Buffering
	if(stdStream == stdout) 
		setvbuf(conFd, nullptr, _IOFBF, 4096);
	else
		setvbuf(conFd, nullptr, _IONBF, 0);
	
	*stdStream = *conFd;
}



void WinConsole::WriteConsoleLog(const LPTSTR message)
{
	lock_guard<mutex> lock(consoleLogMutex);
	
	if(!consoleLog)
		consoleLog = fopen("consoleLog.txt", "w");
	
	fwrite("Failed to initialize console.\n", 1, 30, consoleLog);
	fwrite(message, 1, strlen(message), consoleLog);
	fflush(consoleLog);
}
