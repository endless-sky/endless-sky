/* ComboList.cpp
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ComboList.h"

#include "Color.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Point.h"
#include "Rectangle.h"
#include "Screen.h"
#include "UI.h"

using namespace std;

ComboList::ComboList(Rectangle box, const std::vector<std::pair<std::string, callback>> &listElements,
	Alignment alignment, bool dimBackground, int padding, int initialIndex)
	: elements(listElements), currentIndex(initialIndex), rect(box), alignment(alignment),
		padding(padding), dimBackground(dimBackground)
{
	int comboBoxHeight = ((FontSet::Get(14).Height() + padding) * elements.size()) + padding;
	float maxVerticalHeight = rect.Bottom();  (rect.Bottom() + Screen::RawHeight() / 2);

	facingUp = maxVerticalHeight < comboBoxHeight;

	zones.clear();

	const int elementHeight = rect.Height() + 0.5;
	for(int i = 0; i < static_cast<int>(elements.size()); i++)
	{
		int verticalOffset = i * elementHeight;
		Point offset = Point(0, facingUp ? verticalOffset : -verticalOffset);
		Rectangle labelRect = Rectangle(rect.Center() + offset, rect.Dimensions());
		zones.push_back(ClickZone<int>(labelRect, i));
	}
}

void ComboList::Close()
{
	this->GetUI()->Pop(this);
}

void ComboList::Draw()
{
	if(dimBackground)
		DrawBackdrop();

	ClearZones();
	AddZone(Rectangle(Point(), Screen::Dimensions()), [this](){Close(); });

	int index = 0;
	const Font &font = FontSet::Get(14);
	const Color &dim = GameData::Colors().Get("dim")->Opaque();
	const Color &gray = GameData::Colors().Get("medium")->Opaque();
	const Color &bright = GameData::Colors().Get("bright")->Opaque();
	const Color &dark = GameData::Colors().Get("dark")->Opaque();
	const int elementHeight = rect.Height();
	for(const auto &it : elements)
	{
		auto &label = it.first;

		int verticalOffset = index * elementHeight;
		Point offset = facingUp ? Point(0, verticalOffset) : Point(0, -verticalOffset);

		Rectangle labelRect = Rectangle(rect.Center() + offset, rect.Dimensions());

		if(index == currentIndex)
		{
			FillShader::Fill(labelRect.Center(), labelRect.Dimensions(), gray);
			FillShader::Fill(labelRect.Center(), labelRect.Dimensions() - Point(padding, padding), dim);
		}
		else
		{
			FillShader::Fill(labelRect.Center(), labelRect.Dimensions(), dim);
			FillShader::Fill(labelRect.Center(), labelRect.Dimensions() - Point(padding, padding), dark);
		}


		Point textSource;
		switch(alignment)
		{
		case Alignment::LEFT:
			textSource = Point(labelRect.Left() + padding * 2, labelRect.Center().Y() - font.Height() / 2);
			break;
		case Alignment::RIGHT:
			textSource = Point(labelRect.Right() - font.Width(label) - padding * 2, labelRect.Center().Y() - font.Height() / 2);
			break;
		default:
			textSource = Point(labelRect.Center().X() - font.Width(label) / 2, labelRect.Center().Y() - font.Height() / 2);
			break;
		}
		font.Draw(label, textSource, bright);

		AddZone(labelRect, [this, it](){Close(); it.second(); });

		index++;
	}
}

bool ComboList::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_RETURN)
	{
		Close();
		elements[currentIndex].second();
	}
	else if(key == SDLK_DOWN)
	{
		currentIndex = (elements.size() + currentIndex + (facingUp ? 1 : -1)) % elements.size();
	}
	else if(key == SDLK_UP)
	{
		currentIndex = (elements.size() + currentIndex - (facingUp ? 1 : -1)) % elements.size();
	}
	else if(key == SDLK_ESCAPE)
	{
		Close();
	}
	return true;
}

bool ComboList::Hover(int x, int y)
{
	Point hoverPoint(x, y);
	for(const auto &zone : zones)
	{
		if(zone.Contains(hoverPoint))
		{
			currentIndex = zone.Value();
			return true;
		}
	}
	return false;
}
