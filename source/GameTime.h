// GameTime.h

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <tlhelp32.h>

// This function test if the game is running
/* Souce page: https://stackoverflow.com/questions/1591342/c-how-to-determine-if-a-windows-process-is-running */
bool IsProcessRunning(const wchar_t *processName)
{
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
            if (!wcsicmp(entry.szExeFile, processName))
                exists = true;

    CloseHandle(snapshot);
    return exists;
}