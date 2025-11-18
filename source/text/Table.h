/* Table.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../Color.h"
#include "Layout.h"
#include "../Point.h"

#include <string>
#include <vector>

class DisplayText;
class Font;
class Rectangle;



// Helper class for drawing text formatted in a table, where each column of the
// table is aligned left, right, or centered. This also handles spacing in
// between table rows, underlines, selection highlights, etc.
class Table {
public:
	Table();

	// Set the column positions. If no columns are set, the Table will draw a
	// list (one column of text, left aligned).
	void Clear();
	void AddColumn(int x, Layout layout = {});

	// Set the font size. Default is 14 pixels.
	void SetFontSize(int size);
	// Set the row height. Default is 20 pixels.
	void SetRowHeight(int height) noexcept;
	// Set the width of the highlight area. If the underline has not been set,
	// this will also set the width of the underline.
	void SetHighlight(int startX, int endX) noexcept;
	// Set the X range of the underline. If the highlight has not been set, this
	// will also set the width of the highlight.
	void SetUnderline(int startX, int endX) noexcept;

	// Begin drawing at the given position. Each time text is drawn, it fills a
	// new column until all columns have been filled. Then, the Y position is
	// increased based on the row height, and a new row begins.
	void DrawAt(const Point &point) const;

	// Set the color for drawing text and underlines.
	void SetColor(const Color &color) const;

	// Advance to the next field without drawing anything.
	void Advance(int fields = 1) const;

	// Draw a single text field, and move on to the next one.
	void Draw(const char *text) const;
	void Draw(const std::string &text) const;
	// If a color is given, this field is drawn using that color, but the
	// previously set color will be used for future fields.
	void Draw(const char *text, const Color &color) const;
	void Draw(const std::string &text, const Color &color) const;
	void Draw(double value) const;
	void Draw(double value, const Color &color) const;
	// Use the width & alignment associated with the text (instead of the column's).
	void DrawCustom(const DisplayText &text) const;
	void DrawCustom(const DisplayText &text, const Color &color) const;

	// Draw two columns as a pair with opposite alignments. If needed, truncate the given
	// column based on the width of the non-truncated column's value.
	void DrawTruncatedPair(const std::string &left, const Color &leftColor, const std::string &right,
		const Color &rightColor, Truncate strategy, bool truncateRightColumn) const;

	// Draw an underline under the text for the current row.
	void DrawUnderline() const;
	void DrawUnderline(const Color &color) const;

	// Highlight the current row.
	void DrawHighlight() const;
	void DrawHighlight(const Color &color) const;

	// Shift the draw position down by the given amount. This usually should not
	// be called in the middle of a row, or the fields will not line up.
	void DrawGap(int y) const;

	// Get the point that should be passed to DrawAt() to start the next row at
	// the given location.
	Point GetPoint() const;

	// Get the center and size of the current row. This can be used to define
	// what screen region constitutes a mouse click on this particular row.
	Point GetCenterPoint() const;
	Point GetRowSize() const;
	Rectangle GetRowBounds() const;


private:
	// Starting position for drawing a column is:
	// point + Point(offset + align * font.Width(text), 0.)
	// So, "align" will either be 0, -.5, or -1.
	class Column {
	public:
		Column(double offset, Layout layout) noexcept;

		double offset = 0.;
		Layout layout;
	};


private:
	void Draw(const std::string &text, const Layout *special, const Color &color) const;


private:
	// The current draw position.
	mutable Point point;
	// The column being drawn.
	mutable std::vector<Column>::const_iterator it;
	// The color to use on the next draw call.
	mutable Color color;

	const Font *font = nullptr;
	Point rowSize;
	Point center;
	Point lineSize;
	Point lineOff;

	std::vector<Column> columns;
};
