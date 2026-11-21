/* DisplayText.cpp
Copyright (c) 2020 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DisplayText.h"

#include "../Point.h"
#include "../image/Sprite.h"
#include "../image/SpriteSet.h"

using namespace std;



DisplayText::DisplayText(const char *text, Layout layout)
	: layout(layout), text(text)
{
}



DisplayText::DisplayText(const string &text, Layout layout)
	: layout(layout), text(text)
{
}



const string &DisplayText::GetText() const noexcept
{
	return text;
}



const Layout &DisplayText::GetLayout() const noexcept
{
	return layout;
}




// Returns the width of the sprites that are included by reference in the string in &width.
// Don't call this to process sprite references before the sprites are loaded, e.g. GameLoadingPanel, it won't work.
void DisplayText::UpdateSpriteReferences()
{
	if(!spritesLoaded)
	{
		string target;
		target.reserve(text.length());

		inlineSprites.clear();

		size_t start = 0;
		size_t search = start;
		while(search < text.length())
		{
			size_t left = text.find("<sprite:", search);
			if(left == string::npos)
				break;

			size_t right = text.find('>', left);
			if(right == string::npos)
				break;

			size_t spriteLeft = left + 8;

			// Gather the path and the (optional) embossed text.
			string spritePath = text.substr(spriteLeft, right - spriteLeft);
			string embossedText;
			size_t embossed = spritePath.find(':', 0);
			if(embossed != string::npos)
			{
				embossedText = spritePath.substr(embossed + 1);
				spritePath.resize(embossed);
			}

			inlineSprites.emplace_back(SpriteSet::Get(spritePath), embossedText, Point());

			// target will skip over the entire <sprite> key, and we will leave a placeholder instead.
			target.append(text, start, left - start);
			target += SPRITE_PLACEHOLDER;
			start = ++right;
			search = start;
		}
		target.append(text, start, text.length() - start);
		spritesLoaded = true;

		this->text = target;
	}
}
