/* WinVersion.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WinVersion.h"

#include "../text/Utf8.h"

#include <windows.h>

using namespace std;

namespace {
	RTL_OSVERSIONINFOW versionInfo;
}



void WinVersion::Init()
{
	HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
	auto rtlGetVersion = reinterpret_cast<NTSTATUS (*)(PRTL_OSVERSIONINFOW)>(GetProcAddress(ntdll, "RtlGetVersion"));
	rtlGetVersion(&versionInfo);
	FreeLibrary(ntdll);
}



string WinVersion::ToString()
{
	string servicePack = Utf8::ToUTF8(versionInfo.szCSDVersion);
	return "Windows NT " + to_string(versionInfo.dwMajorVersion) + '.' + to_string(versionInfo.dwMinorVersion) + '.'
		+ to_string(versionInfo.dwBuildNumber) + (servicePack.empty() ? string{} : ' ' + servicePack);
}



bool WinVersion::SupportsDarkTheme()
{
	return versionInfo.dwBuildNumber >= 19041;
}



bool WinVersion::SupportsWindowRounding()
{
	return versionInfo.dwBuildNumber >= 22000;
}
