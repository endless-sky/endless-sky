/* SpaceportPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpaceportPanel.h"

#include "Color.h"
#include "Font.h"
#include "FontSet.h"

using namespace std;



SpaceportPanel::SpaceportPanel(const string &description)
{
	SetTrapAllEvents(false);
	
	text.SetFont(FontSet::Get(14));
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(description);
}



void SpaceportPanel::Draw() const
{
	Color textColor(.8);
	
	const Font &font = FontSet::Get(14);
	for(const WrappedText::Word &word : text.Words())
	{
		Point pos(word.X() - 300., word.Y() + 80.);
		font.Draw(word.String(), pos, textColor.Get());
	}
}
