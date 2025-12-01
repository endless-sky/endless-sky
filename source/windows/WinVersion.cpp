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

// Declare RtlGetVersion here so that we don't need DDK headers.
extern "C" NTSTATUS RtlGetVersion(PRTL_OSVERSIONINFOW);

namespace {
	bool supportsDarkTheme = false;
	bool supportsWindowRounding = false;
}



void WinVersion::Init()
{
	RTL_OSVERSIONINFOW versionInfo = {};
	RtlGetVersion(&versionInfo);

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
