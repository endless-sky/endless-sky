/* Localization.cpp
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

#include "Localization.h"

#include "DataFile.h"
#include "DataNode.h"
#include "Files.h"

#include <map>

using namespace std;

namespace {
	map<string, map<string, string>> catalogs;
	string currentLanguage = "en";



	string BaseLanguage(const string &language)
	{
		size_t separator = language.find_first_of("-_");
		return separator == string::npos ? language : language.substr(0, separator);
	}



	const map<string, string> *FindCatalog(const string &language)
	{
		auto it = catalogs.find(language);
		if(it != catalogs.end())
			return &it->second;

		const string base = BaseLanguage(language);
		it = catalogs.find(base);
		return it == catalogs.end() ? nullptr : &it->second;
	}
}



void Localization::Reset()
{
	catalogs.clear();
	currentLanguage = "en";
}



void Localization::SetLanguage(const string &language)
{
	currentLanguage = language.empty() ? "en" : language;
}



const string &Localization::Language()
{
	return currentLanguage;
}



void Localization::Load(const DataFile &file)
{
	for(const DataNode &node : file)
	{
		if(node.Size() < 2 || node.Token(0) != "language")
		{
			node.PrintTrace("Skipping localization node without a language:");
			continue;
		}

		const string &language = node.Token(1);
		map<string, string> &catalog = catalogs[language];
		for(const DataNode &translation : node)
		{
			if(translation.Size() < 3 || translation.Token(0) != "translation")
			{
				translation.PrintTrace("Skipping malformed translation:");
				continue;
			}
			catalog[translation.Token(1)] = translation.Token(2);
		}
	}
}



void Localization::LoadSources(const vector<filesystem::path> &sources, const string &language)
{
	Reset();
	SetLanguage(language);

	for(const filesystem::path &source : sources)
	{
		const filesystem::path path = source / "data" / "_ui" / "localization.txt";
		if(Files::Exists(path))
		{
			DataFile file(path);
			Load(file);
		}
	}
}



string Localization::Translate(const string &key, const string &fallback)
{
	if(const map<string, string> *catalog = FindCatalog(currentLanguage))
	{
		auto it = catalog->find(key);
		if(it != catalog->end())
			return it->second;
	}

	if(const map<string, string> *catalog = FindCatalog("en"))
	{
		auto it = catalog->find(key);
		if(it != catalog->end())
			return it->second;
	}

	return fallback.empty() ? key : fallback;
}



bool Localization::HasTranslation(const string &key)
{
	if(const map<string, string> *catalog = FindCatalog(currentLanguage))
		return catalog->contains(key);
	return false;
}
