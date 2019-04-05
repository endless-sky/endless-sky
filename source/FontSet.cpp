/* FontSet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FontSet.h"

#include "Files.h"
#include "Font.h"

#include <cstdlib>
#include <map>
#include <fontconfig/fontconfig.h>

using namespace std;

namespace {
	map<int, Font> fonts;
	
	const string defaultFontDescription("Ubuntu");
	string fontDescription(defaultFontDescription);
	string fontDescriptionForLayout;
	string fontLanguage("en");
	char envBackend[] = "PANGOCAIRO_BACKEND=fc";
}



void FontSet::Add(const std::string &fontsDir)
{
	const FcChar8* fontsDirFc8 = reinterpret_cast<const FcChar8*>(fontsDir.c_str());
	const string cfgFile = fontsDir + "fonts.conf";
	const FcChar8* cfgFileFc8 = reinterpret_cast<const FcChar8*>(cfgFile.c_str());
	FcConfig *fcConfig = FcConfigGetCurrent();
	if(!FcConfigAppFontAddDir(fcConfig, fontsDirFc8))
		Files::LogError("Warning: Fail to load fonts in \"" + fontsDir + "\".");
	if(!FcConfigParseAndLoad(fcConfig, cfgFileFc8, FcFalse))
		Files::LogError("Warning: Parse error in \"" + cfgFile + "\".");
}



const Font &FontSet::Get(int size)
{
	if(!fonts.count(size))
	{
		putenv(envBackend);
		Font &font = fonts[size];
		font.SetFontDescription(fontDescription);
		font.SetLayoutReference(fontDescriptionForLayout);
		font.SetPixelSize(size);
		font.SetLanguage(fontLanguage);
	}
	return fonts[size];
}



void FontSet::SetFontDescription(const std::string &desc)
{
	fontDescription = desc.empty() ? defaultFontDescription : desc;
	for(auto &it : fonts)
		it.second.SetFontDescription(fontDescription);
}



void FontSet::SetLayoutReference(const std::string &desc)
{
	fontDescriptionForLayout = desc;
	for(auto &it : fonts)
		it.second.SetLayoutReference(fontDescriptionForLayout);
}



void FontSet::SetLanguage(const std::string &lang)
{
	fontLanguage = lang;
	for(auto &it : fonts)
		it.second.SetLanguage(lang);
}
