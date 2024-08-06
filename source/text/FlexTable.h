/* FlexTable.h
Copyright (c) 2024 by tibetiroka

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
#include "FontSet.h"
#include "layout.hpp"
#include "../Point.h"
#include "../Rectangle.h"
#include "WrappedText.h"

#include <string>
#include <vector>



// Helper class for drawing wrapped text formatted in a table, where each column of the
// table is aligned left, right, or centered. This also handles spacing in
// between table rows, underlines, selection highlights, etc.
class FlexTable {
public:
	class Column;
	// A single table cell. Cells use their column's formatting values.
	// By default, each cell fits tightly to the contained text, and are aligned
	// by the column setting custom wrap widths, depending on its configuration.
	class Cell {
	public:
		explicit Cell(Column *column, const std::string &text = "");

		void SetText(const std::string &text);
		const std::string &Text() const;

		// The minimum width of the cell, based on the previous wrap width.
		// This is not an absolute minimum; rather, it indicates how much of the previously configured space is actually used.
		int MinWidth() const;
		// The width and height of the cell, including padding.
		int Width() const;
		int Height() const;

		// Set the font. Default is the size 14 font.
		void SetFont(const Font *font);
		const Font *GetFont() const;

		// Extra height reserved at the top or bottom of the cell.
		// Gaps are synchronized between all cells in a row.
		void SetTopGap(int gap);
		void SetBottomGap(int gap);
		int GetTopGap() const;
		int GetBottomGap() const;

		// Configures cell decorations. These affect the entire cell, not just the space occupied by text.
		void SetHighlight(bool highlight = true);
		void SetUnderline(bool underline = true);

		// Sets the colors for highlights, underlines and text.
		// Setting highlight or underline-related colors automatically enables highlighting/underlining.
		void SetTextColor(const Color &textColor);
		void SetHighlightedTextColor(const Color &textColor);
		void SetHighlightColor(const Color &highlightColor);
		void SetUnderlineColor(const Color &underlineColor);

	private:
		int GetRow() const;
		// Makes the cell span the entire row. This should only be called within the first column.
		void SpanRow(bool span = true);
		bool SpansRow() const;

		// The optimal width of the column (the most space the text inside can take up).
		int OptimalFlexWidth() const;
		void SetWrapWidth(int width);

		void UpdateLayout();

		// Draws the cell centered on the given point.
		void Draw(const Point &position) const;


	private:
		Column *column;

		// Cells can span the entire column of a table.
		// They are only present in the first column; the rest store empty cells
		// that are skipped when drawing.
		bool spansRow = false;

		WrappedText wrappedText{};
		std::string text;
		int wrapWidth;
		int topGap = 0;
		int bottomGap = 0;
		bool highlight = false;
		bool underline = false;

		const Color *highlightColor;
		const Color *underlineColor;
		const Color *textColor;
		const Color *highlightedTextColor;

		const Font *font;

		friend class Column;
		friend class FlexTable;
	};

	class Column {
	public:
		explicit Column(FlexTable *table);

		// Gets the cell in the specified row.
		const Cell &Row(int row) const;
		Cell &Row(int row);
		int RowCount() const;

		// Gets the width of the column (the width of the widest cell in the column).
		// This ignores cells that span across columns.
		int Width() const;
		// The width required for this column to best fit in the table,
		// accounting for the amount of text contained in each cell.
		int OptimalFlexWidth() const;

		void SetAlignment(Alignment alignment);
		void SetTruncate(Truncate truncate);

		// A flexing column can extend beyond the width of the text it contains.
		void SetFlex(bool flex = true);
		bool CanFlex() const;

		// Sets whether cells decorate the spacing after the column.
		void SetDecorateGap(bool decorate);
		bool DecoratesGap() const;


	private:
		// Fits each cell to the given width by setting their padding values.
		// This also sets the column's width to this value.
		void FitToWidth(int width);
		// Sets the wrap width of each cell to the column's minimum width.
		void Pack();
		void UpdateLayout();

		// Removes all cells from this column.
		void Clear();
		Cell &AddRow(const std::string &text);


	private:
		Alignment alignment = Alignment::LEFT;
		Truncate truncate = Truncate::NONE;
		bool canFlex = true;

		bool decorateGap = true;

		std::vector<Cell> cells;
		FlexTable *table;

		friend class FlexTable;
	};

	enum class FlexStrategy {
		FIRST, // Prioritizes the first column. This column receives its requested width first, and holds all empty space.
		LAST,
		EVEN, // Distributes the available space evenly between the columns that request it.
		INDIVIDUAL // Flexes each cell separately, instead of in columns. This reduces the table's size,
					// but can be jarring if the columns are misaligned.
	};


public:
	explicit FlexTable(int width = 0, int columns = 0);

	// Since we are keeping references to tables and columns, we need custom copy behaviour.
	FlexTable(const FlexTable &other) noexcept;
	FlexTable &operator=(const FlexTable &other);

	// Removes all rows from the table, but preserves columns.
	void Clear();
	void AddColumn();

	int Columns() const;
	const Column &GetColumn(int index) const;
	Column &GetColumn(int index);
	int Rows() const;

	int Width() const;
	void SetWidth(int width);
	// Gets the height of the table until the bottom of the specified row.
	int Height(int untilRow = -1);

	// Gets the mouse hitbox of a row. This hitbox includes gaps and paddings, but not the spacing between rows.
	// The anchor point must be the same as in Draw().
	Rectangle GetRowHitbox(int targetRow, const Point &anchor);
	Rectangle GetCellHitbox(int row, int column, const Point &anchor);

	const Cell &GetCell(int row, int column) const;
	Cell &GetCell(int row, int column);

	// Fills a row with individual cells. Any extra arguments are discarded,
	// and missing ones are substituted with empty cells.
	// Returns a pointer to the first cell in the row, or nullptr if there are no columns in the table.
	Cell *FillRow(const std::initializer_list<std::string> &cellTexts = {});
	Cell *FillRow(const std::initializer_list<std::pair<std::string, const Color &>> &cellTexts);
	// Fills a row with a single cell. This cell spans across every column.
	// Returns a pointer to the cell, or nullptr if there are no columns in the table.
	Cell *FillUnifiedRow(const std::string &text, const Color &color);
	// Removes the last n rows of the table.
	void PopRow(int amount = 1);

	// Configures the number of empty pixels between each table row (not text rows within a single cell).
	void SetRowSpacing(int spacing);
	int GetRowSpacing() const;
	// Configures the number of empty pixels between each table column.
	// This is ignored around empty columns.
	void SetColumnSpacing(int spacing);
	int GetColumnSpacing() const;

	// Highlights the row containing the specific point, and turns off highlighting for every other cell.
	// Returns the index of the row containing the point, or -1 if it's not in the table.
	// If allowFirstRow is false, the first row is ignored.
	// The anchor point must be the same as in Draw().
	int SetRowHighlight(const Point &point, const Point &anchor, bool allowFirstRow = false);

	// Draws the table, and returns a point under the table for other draw operations.
	// The anchor point is the center of the top edge of the table.
	Point Draw(const Point &anchor);

	FlexStrategy GetFlexStrategy() const;
	void SetFlexStrategy(FlexStrategy strategy);

private:
	// The width of an average column in the table, without spacing.
	int AverageColumnWidth() const;

	// Marks that the layout of the table has to be recomputed.
	void Invalidate();
	void UpdateLayout();

	// Finds the row that corresponds to the given offset, supporting negative values and wrap-arounds.
	void ClampRow(int &row) const;
	void ClampColumn(int &column) const;


private:
	int width;
	int rowSpacing = 0;
	int columnSpacing = 10;

	FlexStrategy flexStrategy = FlexStrategy::FIRST;

	bool valid = true;

	std::vector<Column> columns;
	int rowCount = 0;

	friend class Column;
};
