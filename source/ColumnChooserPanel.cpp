/* ColumnChooserPanel.cpp
Copyright (c) 2017 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ColumnChooserPanel.h"

#include "text/alignment.hpp"
#include "Command.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "InfoPanelState.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "LogbookPanel.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace std;

ColumnChooserPanel::ColumnChooserPanel(InfoPanelState *panelState)
	: panelState(panelState)
{
	SetInterruptible(false);
}



void ColumnChooserPanel::Draw()
{
	if(GetUI()->IsTop(this))
		DrawBackdrop();

	zones.clear();

	Information info;
	const Interface *columnChooser = GameData::Interfaces().Get("columns menu");
	info.SetCondition("columns menu open");
	columnChooser->Draw(info, this);

	const Font &font = FontSet::Get(14);
	const Color *color[] = {GameData::Colors().Get("medium"), GameData::Colors().Get("bright")};
	const Sprite *box[] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};
	const string columns[][2] = {
		{"ship", "Ship name"},
		{"model", "Ship model"},
		{"system", "Current system"},
		{"shields", "Shield strength"},
		{"hull", "Hull integrity"},
		{"fuel", "Fuel"},
		{"combat", "Combat prowess"},
		{"crew", "Crew"},
		{"cargo", "Free cargo space"},
	};

	// Table table;
	// table.AddColumn(
	// 	pluginListClip->Left() + box[0]->Width(),
	// 	Layout(pluginListBox.Width() - box[0]->Width(), Truncate::MIDDLE)
	// );

	Point topLeft(270., -280.);
	const Point boxSize = Point(box[0]->Width(), box[0]->Height());
	static const Point ROW_ADVANCE(0., 20.);
	const set<const string> visibleColumns = panelState->VisibleColumns();
	for(const string * const column : columns)
	{
		bool visible = (visibleColumns.find(column[0]) != visibleColumns.end());

		const Sprite *sprite = box[visible];
		Rectangle spriteBounds = Rectangle::FromCorner(topLeft, boxSize);
		SpriteShader::Draw(sprite, spriteBounds.Center());

		Point textPos = topLeft + Point(box[0]->Width(), 2.);
		font.Draw(column[1], textPos, *color[visible]);

		zones.emplace_back(ClickZone(Rectangle::FromCorner(topLeft, Point(220., ROW_ADVANCE.Y())), column[0]));

		topLeft = topLeft + ROW_ADVANCE;
	}
}



bool ColumnChooserPanel::AllowsFastForward() const noexcept
{
	return true;
}



bool ColumnChooserPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'a')
		GetUI()->Pop(this);
	else
		return false;

	return true;
}



bool ColumnChooserPanel::Click(int x, int y, int /* clicks */)
{
	Point mouse = Point(x, y);
	for(auto &zone : zones)
		if(zone.Contains(mouse))
		{
			// toggle checkbox
			panelState->ToggleColumn(zone.Value());

			return true;
		}

	return false;
}



void ColumnChooserPanel::DrawTooltip(const string &text, const Point &hoverPoint)
{
	if(text == "")
		return;

	const int WIDTH = 250;
	const int PAD = 10;
	WrappedText wrap(FontSet::Get(14));
	wrap.SetWrapWidth(WIDTH - 2 * PAD);
	wrap.Wrap(text);
	int longest = wrap.LongestLineWidth();
	if(longest < wrap.WrapWidth())
	{
		wrap.SetWrapWidth(longest);
		wrap.Wrap(text);
	}

	const Color *backColor = GameData::Colors().Get("tooltip background");
	const Color *textColor = GameData::Colors().Get("medium");
	Point textSize(wrap.WrapWidth() + 2 * PAD, wrap.Height() + 2 * PAD - wrap.ParagraphBreak());
	Point anchor = hoverPoint + Point(0., textSize.Y());
	FillShader::Fill(anchor - .5 * textSize, textSize, *backColor);
	wrap.Draw(anchor - textSize + Point(PAD, PAD), *textColor);
}



bool ColumnChooserPanel::Hover(int x, int y)
{
	return Hover(Point(x, y));
}



bool ColumnChooserPanel::Hover(const Point &point)
{
	hoverPoint = point;
	hoverIndex = -1;

	return true;
}



bool ColumnChooserPanel::Scroll(double /* dx */, double dy)
{
	return Scroll(dy * -.1 * Preferences::ScrollSpeed());
}



bool ColumnChooserPanel::ScrollAbsolute(int scroll)
{
	// int maxScroll = columns.size() - LINES_PER_PAGE;
	// int newScroll = max(0, min<int>(maxScroll, scroll));
	// if(panelState.Scroll() == newScroll)
	// 	return false;

	// panelState.SetScroll(newScroll);

	return true;
}



// Adjust the scroll by the given amount. Return true if it changed.
bool ColumnChooserPanel::Scroll(int distance)
{
	// return ScrollAbsolute(panelState.Scroll() + distance);
	return true;
}
