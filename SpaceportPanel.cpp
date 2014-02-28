/* SpaceportPanel.cpp
Michael Zahniser, 11 Jan 2014

Function definitions for the SpaceportPanel class.
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
