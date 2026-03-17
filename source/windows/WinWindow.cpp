/* WinWindow.cpp
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

#include "WinWindow.h"

#include "../Preferences.h"
#include "WinVersion.h"

#include <SDL2/SDL_syswm.h>

#include <dwmapi.h>



void WinWindow::UpdateTitleBarTheme(SDL_Window *window)
{
	if(!WinVersion::SupportsDarkTheme())
		return;

	SDL_SysWMinfo windowInfo;
	SDL_VERSION(&windowInfo.version);
	SDL_GetWindowWMInfo(window, &windowInfo);

	BOOL value;
	Preferences::TitleBarTheme themePreference = Preferences::GetTitleBarTheme();
	// If the default option is selected, check the system-wide preference.
	if(themePreference == Preferences::TitleBarTheme::DEFAULT)
	{
		HKEY systemPreference;
		if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			0, KEY_READ, &systemPreference) == ERROR_SUCCESS)
		{
			DWORD size = sizeof(value);
			if(RegQueryValueExW(systemPreference, L"AppsUseLightTheme", 0, nullptr,
					reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
				// The key says about light theme, while DWM expects information about dark theme.
				value = !value;
			else
				value = 1;
			RegCloseKey(systemPreference);
		}
		else
			value = 1;
	}
	else
		value = themePreference == Preferences::TitleBarTheme::DARK;

	HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
	auto dwmSetWindowAttribute = reinterpret_cast<HRESULT (*)(HWND, DWORD, LPCVOID, DWORD)>(
		GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
	dwmSetWindowAttribute(windowInfo.info.win.window, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	FreeLibrary(dwmapi);
}



void WinWindow::UpdateWindowRounding(SDL_Window *window)
{
	if(!WinVersion::SupportsWindowRounding())
		return;

	SDL_SysWMinfo windowInfo;
	SDL_VERSION(&windowInfo.version);
	SDL_GetWindowWMInfo(window, &windowInfo);

	auto value = static_cast<DWM_WINDOW_CORNER_PREFERENCE>(Preferences::GetWindowRounding());

	HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
	auto dwmSetWindowAttribute = reinterpret_cast<HRESULT (*)(HWND, DWORD, LPCVOID, DWORD)>(
		GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
	dwmSetWindowAttribute(windowInfo.info.win.window, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
	FreeLibrary(dwmapi);
}
