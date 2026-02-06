/* ColumnChooserPanel.cpp
Copyright (c) 2024 by Endless Sky contributors

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

#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <numeric>

using namespace std;



ColumnChooserPanel::ColumnChooserPanel(const vector<PlayerInfoPanel::SortableColumn> &columns,
	InfoPanelState *panelState)
	: columns(columns), panelState(panelState)
{
	SetInterruptible(false);
}



void ColumnChooserPanel::Draw()
{
	if(GetUI().IsTop(this))
		DrawBackdrop();

	zones.clear();

	Information info;
	const Interface *columnChooser = GameData::Interfaces().Get("columns menu");
	info.SetCondition("columns menu open");
	columnChooser->Draw(info, this);

	const Font &font = FontSet::Get(14);
	const Color * const dim = GameData::Colors().Get("dim");
	const Color * const medium = GameData::Colors().Get("medium");
	const Color * const bright = GameData::Colors().Get("bright");
	const Sprite *box[] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};

	Point topLeft(270., -280.);
	static const Point ROW_ADVANCE(0., 20.);
	static const Point TEXT_OFFSET(box[0]->Width(), 2.);
	static const Point BOX_SIZE(box[0]->Width(), box[0]->Height());
	static const int PANEL_CONTENT_WIDTH = 727;
	const set<const string> &visibleColumns = panelState->VisibleColumns();
	auto isVisible = [&](string name){ return visibleColumns.find(name) != visibleColumns.end(); };
	const int availableWidth = PANEL_CONTENT_WIDTH - accumulate(columns.begin(), columns.end(), 0,
		[&](int acc, PlayerInfoPanel::SortableColumn column) {
			return acc + (isVisible(column.name) ? column.layout.width : 0);
		});
	for(PlayerInfoPanel::SortableColumn &column : columns)
	{
		Rectangle zoneBounds = Rectangle::FromCorner(topLeft, Point(220., ROW_ADVANCE.Y()));

		bool visible = isVisible(column.name);
		bool enabled = visible || column.layout.width <= availableWidth;
		bool hover = zoneBounds.Contains(hoverPoint);

		Rectangle spriteBounds = Rectangle::FromCorner(topLeft, BOX_SIZE);
		SpriteShader::Draw(box[visible], spriteBounds.Center());

		Point textPos = topLeft + TEXT_OFFSET;
		font.Draw(column.checkboxLabel, textPos, enabled ? hover ? *bright : *medium : *dim);

		if(enabled)
			zones.emplace_back(zoneBounds, column.name);

		topLeft += ROW_ADVANCE;
	}
}



bool ColumnChooserPanel::AllowsFastForward() const noexcept
{
	return true;
}



bool ColumnChooserPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'a' || key == 'n')
		GetUI().Pop(this);
	else
		return false;

	return true;
}



bool ColumnChooserPanel::Click(int x, int y, MouseButton button, int /* clicks */)
{
	Point mouse(x, y);
	for(auto &zone : zones)
		if(zone.Contains(mouse))
		{
			panelState->ToggleColumn(zone.Value());
			return true;
		}

	return false;
}



bool ColumnChooserPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	return true;
}
