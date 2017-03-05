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
#include <tchar.h>

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
		
		// Get a console handle
		const HANDLE conoutHandle = CreateFile(
										"CONOUT$", 
										GENERIC_READ | 
										GENERIC_WRITE, 
										FILE_SHARE_READ | 
										FILE_SHARE_WRITE, 
										0, 
										OPEN_EXISTING, 
										0, 
										0);
		if(conoutHandle == INVALID_HANDLE_VALUE) 
			throw GetLastError();
		
		// Set console's max lines for large output (--ships, --weapons)
		CONSOLE_SCREEN_BUFFER_INFO conBufferInfo;
		if(!GetConsoleScreenBufferInfo(conoutHandle, &conBufferInfo)) 
			throw GetLastError();
			
		// Make sure the user doesn't already have a larger screenbuffer size
		const short defaultSize = conBufferInfo.dwSize.Y;
		if(defaultSize < 750)
		{
			conBufferInfo.dwSize.Y = 750;
			if(!SetConsoleScreenBufferSize(conoutHandle, conBufferInfo.dwSize)) 
				throw GetLastError();
		}
		
		CloseHandle(conoutHandle);
		
		// Redirect
		if(redirStdout)
		{
			freopen("CONOUT$", "w", stdout);
			setvbuf(stdout, nullptr, _IOFBF, 4096);
		}
		
		if(redirStderr)
		{
			freopen("CONOUT$", "w", stderr);
			setvbuf(stderr, nullptr, _IOLBF, 1024);
		}
	}
	catch(const DWORD errorCode)
	{
		LPTSTR msgBuffer = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS, 
			nullptr, 
			errorCode, 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			reinterpret_cast<LPTSTR>(&msgBuffer), 
			0, 
			nullptr);
		
		WriteConsoleLog(msgBuffer);
		LocalFree(msgBuffer);
		msgBuffer = nullptr;
	}
}



void WinConsole::WriteConsoleLog(const LPTSTR message)
{
	lock_guard<mutex> lock(consoleLogMutex);
	
	if(!consoleLog)
		consoleLog = fopen("consoleLog.txt", "w");
	
	fwrite("Failed to initialize console.\n", 1, 30, consoleLog);
	fwrite(message, sizeof(message[0]), _tcslen(message), consoleLog);
	fflush(consoleLog);
}
