/* LogbookPanel.h
Copyright (c) 2017 by Michael Zahniser

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

#include "MapPanel.h"

#include "BookEntry.h"
#include "ClickZone.h"
#include "OrderedMap.h"
#include "PlayerInfo.h"

#include <string>
#include <vector>



// A user interface panel that displays the entries in the player's logbook.
class LogbookPanel : public MapPanel {
public:
	explicit LogbookPanel(PlayerInfo &player);

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Event handlers.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;


private:
	enum class EntryType {
		NORMAL = 0,
		STORYLINE,
		BOOK,
		ARC,
		CHAPTER
	};

	class Entry {
	public:
		Entry(EntryType type, const std::string &heading, const BookEntry &body);
		Entry(EntryType type, const PlayerInfo::StorylineProgress &progress);

		EntryType type;
		std::string heading;
		std::string subheading;
		const BookEntry &body;
	};

	enum class PageType {
		DATE = 0,
		SPECIAL,
		STORYLINE,
	};

	class Page {
	public:
		explicit Page(PageType type) : type(type) {}

		PageType type;
		std::vector<Entry> entries;
	};

	// A section is a mapping of subcategory to page.
	// Subcategories are the expanded selection of entries under a category.
	using Section = OrderedMap<std::string, Page>;
	// A selection is a category and subcategory to display the book entries of.
	using Selection = std::pair<std::string, std::string>;


private:
	void CreateSections();
	void SelectEntry(const BookEntry &entry);

	void DrawSelectedEntry() const;
	void DrawLogbook();

	std::vector<Selection> AvailableSelections(bool visibleOnly = true) const;


private:
	// Whether the scenes shown by logbook entries have been preloaded yet.
	bool hasLoadedScenes = false;

	// A mapping of category to section. These are what are selectable on the left-most side of the panel.
	// If a section has no subcategories to expand, then the section should only have one subcategory with
	// the same name as the section.
	OrderedMap<std::string, Section> sections;
	// The current page selection.
	Selection selection;
	// The currently selected book entry to display the related systems of.
	const BookEntry *selectedEntry = nullptr;
	// The system from the selected entry that is currently being centered on.
	const System *centeredSystem = nullptr;

	std::vector<ClickZone<Selection>> selectionZones;
	std::vector<ClickZone<const BookEntry *>> logZones;

	Point hoverPoint;

	// Current scroll:
	double categoryScroll = 0.;
	double scroll = 0.;
	mutable double maxCategoryScroll = 0.;
	mutable double maxScroll = 0.;
};
