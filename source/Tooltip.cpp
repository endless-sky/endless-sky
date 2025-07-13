/* Tooltip.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Tooltip.h"

#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Point.h"
#include "Screen.h"

using namespace std;

namespace {
	// Only show tooltips if the mouse has hovered in one place for this amount
	// of time.
	const int HOVER_TIME = 60;
}



Tooltip::Tooltip(int width, Alignment alignment, Direction direction, Corner corner)
	: width(width), direction(direction), corner(corner),
	normal(GameData::Colors().Get("tooltip background")),
	warning(GameData::Colors().Get("warning back")),
	error(GameData::Colors().Get("error back")),
	fontColor(GameData::Colors().Get("medium"))
{
	text.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the tooltip box.
	text.SetWrapWidth(width - 20);
	text.SetAlignment(alignment);
}



void Tooltip::IncrementCount()
{
	if(hoverCount < HOVER_TIME)
		++hoverCount;
}



void Tooltip::DecrementCount()
{
	if(hoverCount)
		--hoverCount;
}



void Tooltip::ResetCount()
{
	hoverCount = 0;
}



bool Tooltip::ShouldDraw() const
{
	return hoverCount >= HOVER_TIME;
}



void Tooltip::SetZone(const Point &center, const Point &dimensions)
{
	zone = Rectangle(center, dimensions);
}



void Tooltip::SetZone(const Rectangle &zone)
{
	this->zone = zone;
}



void Tooltip::SetText(const string &text)
{
	// Reset the wrap width each time we set text in case the WrappedText
	// was previously shrunk to the size of the text.
	this->text.SetWrapWidth(width);
	this->text.Wrap(text);
}



bool Tooltip::HasText() const
{
	return text.Height();
}



void Tooltip::Clear()
{
	text.Wrap("");
}



void Tooltip::SetState(State state)
{
	this->state = state;
}



void Tooltip::Shrink()
{
	const string &rawText = text.GetText();
	int longest = text.LongestLineWidth();
	if(longest < text.WrapWidth())
	{
		text.SetWrapWidth(longest);
		text.Wrap(rawText);
	}
}



void Tooltip::Draw(bool forceDraw) const
{
	if((!forceDraw && !ShouldDraw()) || !HasText())
		return;

	Point point;
	if(corner == Corner::TOP_LEFT)
		point = zone.TopLeft();
	else if(corner == Corner::TOP_RIGHT)
		point = zone.TopRight();
	else if(corner == Corner::BOTTOM_LEFT)
		point = zone.BottomLeft();
	else if(corner == Corner::BOTTOM_RIGHT)
		point = zone.BottomRight();
	Draw(point);
}



void Tooltip::Draw(const Point &point) const
{
	Point textSize(text.WrapWidth(), text.Height() - text.ParagraphBreak());
	Point boxSize = textSize + Point(20., 20.);
	Rectangle box = Rectangle::FromCorner(point, boxSize);

	// The default box has a direction of DOWN_RIGHT, so shift the box left
	// or up accordingly with the chosen direction.
	if(direction == Direction::UP_LEFT || direction == Direction::DOWN_LEFT)
		box -= Point(boxSize.X(), 0);
	if(direction == Direction::UP_LEFT || direction == Direction::UP_RIGHT)
		box -= Point(0, boxSize.Y());

	// If the chosen point would push the box off-screen, adjust the box's position.
	// Instead of nudging the box by the amount of overflow, flip it over one of its axes
	// Doing this ensures we don't overlap any useful information hat would be covered by
	// simply nudging the box.
	if(box.Top() < Screen::Top())
		box += Point(0, boxSize.Y());
	if(box.Left() < Screen::Left())
		box += Point(boxSize.X(), 0);
	if(box.Right() > Screen::Right())
		box -= Point(boxSize.X(), 0);
	if(box.Bottom() > Screen::Bottom())
		box -= Point(0, boxSize.Y());

	// Determine the background color that should be used given the current state
	// of this tooltip.
	const Color *background = nullptr;
	if(state == State::NORMAL)
		background = normal;
	else if(state == State::WARNING)
		background = warning;
	else
		background = error;

	FillShader::Fill(box.Center(), boxSize, *background);
	text.Draw(box.TopLeft() + Point(10., 10.), *fontColor);
}
