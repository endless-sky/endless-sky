/* LogbookEntry.h
Copyright (c) 2025 by xobes

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

#include "Color.h"
#include "DataWriter.h"
#include "image/Sprite.h"
#include "text/WrappedText.h"

#include <string>


using namespace std;

// Implement a collection of text and image nodes which form a singular Logbook entry
class BookEntry {
public:
	class Item {
		public:
			explicit Item(const string &text);
			explicit Item(const Sprite *scene);
			void Save(DataWriter &out) const;
		private:
			const Sprite *scene = nullptr;
			string text;
	};


public:
	BookEntry();

	void AddItem(Item &item);

	void Save(DataWriter &out) const;
	void Instantiate(map<string, string> &subs);
	// Returns height.
	int Draw(const Point &topLeft, const Font &font, Alignment alignment, int width, const Color &color) const;

	vector<Item> &items;
};
