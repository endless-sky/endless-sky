/* test_localization.cpp
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

#include "es-test.hpp"

#include "../../../source/DataFile.h"
#include "../../../source/Localization.h"

#include <sstream>

namespace {

TEST_CASE("Localization catalogs", "[localization]")
{
	Localization::Reset();

	std::istringstream input(
		"language \"en\"\n"
		"\ttranslation \"ui.quit\" \"_Quit\"\n"
		"language \"fr\"\n"
		"\ttranslation \"ui.quit\" \"_Quitter\"\n"
		"\ttranslation \"ui.only_french\" \"Seulement francais\"\n");
	DataFile file(input);
	Localization::Load(file);

	Localization::SetLanguage("fr-CA");
	CHECK(Localization::Language() == "fr-CA");
	CHECK(Localization::Translate("ui.quit") == "_Quitter");
	CHECK(Localization::HasTranslation("ui.only_french"));
	CHECK(Localization::Translate("ui.missing", "Fallback") == "Fallback");

	Localization::SetLanguage("de");
	CHECK(Localization::Translate("ui.quit") == "_Quit");
	CHECK_FALSE(Localization::HasTranslation("ui.quit"));

	Localization::Reset();
	CHECK(Localization::Language() == "en");
}

} // namespace
