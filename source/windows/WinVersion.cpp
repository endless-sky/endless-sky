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

#include <windows.h>

namespace {
	bool supportsDarkTheme = false;
	bool supportsWindowRounding = false;
}



void WinVersion::Init()
{
	HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
	auto rtlGetVersion = reinterpret_cast<NTSTATUS (*)(PRTL_OSVERSIONINFOW)>(GetProcAddress(ntdll, "RtlGetVersion"));
	RTL_OSVERSIONINFOW versionInfo = {};
	rtlGetVersion(&versionInfo);
	FreeLibrary(ntdll);

	supportsDarkTheme = versionInfo.dwBuildNumber >= 19041;
	supportsWindowRounding = versionInfo.dwBuildNumber >= 22000;
}



bool WinVersion::SupportsDarkTheme()
{
	return supportsDarkTheme;
}



bool WinVersion::SupportsWindowRounding()
{
	return supportsWindowRounding;
}
