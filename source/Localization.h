/* Localization.h
Copyright (c) 2026 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

class DataFile;



// The localization catalog used for user-visible, non-content strings.
//
// Catalogs are loaded from the active resource sources in order. A later source
// may override an earlier translation, matching the existing plugin precedence
// rules. The English catalog is used as the fallback for a missing translation.
class Localization {
public:
	// Clear all loaded catalogs and restore the default language.
	static void Reset();

	// Set the language used for lookups. The value is a language tag such as
	// "en" or "ko". A regional tag falls back to its base language when needed.
	static void SetLanguage(const std::string &language);
	static const std::string &Language();

	// Load translations from one data file. The file uses the following format:
	//
	// language "en"
	// 	translation "ui.menu.quit" "Quit"
	static void Load(const DataFile &file);

	// Load localization files from all active resource sources.
	static void LoadSources(const std::vector<std::filesystem::path> &sources,
		const std::string &language);

	// Translate a key. If no translation exists, return the given fallback, or
	// the key itself when no fallback was supplied.
	static std::string Translate(const std::string &key, const std::string &fallback = {});
	static bool HasTranslation(const std::string &key);
};
