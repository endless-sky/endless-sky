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

#include <map>
#ifdef CAIRO_HAS_WIN32_FONT
#include <windows.h>
#else
#include <fontconfig/fontconfig.h>
#endif

using namespace std;

namespace {
	map<int, Font> fonts;
	
	string fontDescription("Ubuntu");
	string fontDescriptionForLayout("Ubuntu");
	string fontLanguage("en");
}



void FontSet::Add(const std::string &fontsDir)
{
#ifdef CAIRO_HAS_WIN32_FONT
    for(const auto &file : Files::List(fontsDir))
        AddFontResourceExW(Files::NativeFilePath(file).c_str(), FR_PRIVATE, nullptr);
#else
	const FcChar8* fontsDirFc8 = reinterpret_cast<const FcChar8*>(fontsDir.c_str());
	const string cfgFile = fontsDir + "fonts.conf";
	const FcChar8* cfgFileFc8 = reinterpret_cast<const FcChar8*>(cfgFile.c_str());
	FcConfig *fcConfig = FcConfigGetCurrent();
	if(!FcConfigAppFontAddDir(fcConfig, fontsDirFc8))
		Files::LogError("Warning: Fail to load fonts in \"" + fontsDir + "\".");
	if(!FcConfigParseAndLoad(fcConfig, cfgFileFc8, FcFalse))
		Files::LogError("Warning: Parse error in \"" + cfgFile + "\".");
#endif
}



const Font &FontSet::Get(int size)
{
	if(!fonts.count(size))
	{
		Font &font = fonts[size];
		font.SetFontDescription(fontDescription);
		font.SetLayoutReference(fontDescriptionForLayout);
		font.SetPixelSize(size);
		font.SetLanguage(fontLanguage);
	}
	return fonts[size];
}



void SetFontDescription(const std::string &desc)
{
	fontDescription = desc;
	for(auto &it : fonts)
		it.second.SetFontDescription(desc);
}



void SetReferenceForLayout(const std::string &desc)
{
	fontDescriptionForLayout = desc;
	for(auto &it : fonts)
		it.second.SetLayoutReference(desc);
}



void SetLanguage(const std::string &lang)
{
	fontLanguage = lang;
	for(auto &it : fonts)
		it.second.SetLanguage(lang);
}
