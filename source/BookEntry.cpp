/* BookEntry.cpp
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

#include "BookEntry.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/WrappedText.h"

using namespace std;



// Note: A BookEntry exists potentially in advance of having taken effect and being placed into
// the logbook, e.g. when it exists as merely a potential outcome of a given GameAction.
// When the GameAction is triggered, the Instantiate method will be called to perform necessary
// substitutions on the text at that time.
BookEntry::BookEntry()
{
}



bool BookEntry::Empty() const
{
	if(items.empty())
		return true;
	for(const ItemType &item : items)
		if(!std::holds_alternative<std::monostate>(item))
			return false;
	return true;
}



void BookEntry::Read(const DataNode &node, int startAt)
{
	// Note: Like Dialog::ParseTextNode, BookEntry::Read will consume all remaining Tokens of this node
	// as well as all remaining tokens of the children of this node.

	if(startAt <= node.Size())
		AppendItem(ReadItem(node, startAt));

	for(const DataNode &child : node)
		AppendItem(ReadItem(child));
}



void BookEntry::Add(const BookEntry &other)
{
	for(const ItemType &item : other.items)
		AppendItem(item);
}



BookEntry BookEntry::Instantiate(const map<string, string> &subs) const
{
	BookEntry newEntry;
	for(const ItemType &item : items)
	{
		// Perform requested substitutions on the text of this node and return a new variant.
		if(holds_alternative<string>(item))
			newEntry.items.emplace_back(Format::Replace(std::get<string>(item), subs));
		else
			newEntry.items.emplace_back(item);
	}
	return newEntry;
}



void BookEntry::Save(DataWriter &out) const
{
	for(const ItemType &item : items)
	{
		out.BeginChild();
		{
			// Break the text up into paragraphs.
			if(holds_alternative<string>(item))
				for(const string &line : Format::Split(std::get<string>(item), "\n\t"))
					out.Write(line);
			else
				out.Write("scene", std::get<const Sprite *>(item)->Name());
		}
		out.EndChild();
	}
}



int BookEntry::Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const
{
	Point drawPoint = topLeft;
	for(const ItemType &item : items)
	{
		if(holds_alternative<string>(item))
		{
			wrap.Wrap(std::get<string>(item));
			wrap.Draw(drawPoint, color);
			drawPoint.Y() += wrap.Height();
		}
		else
		{
			const Sprite *scene = std::get<const Sprite *>(item);
			const Point sceneOffset(scene->Width() / 2, scene->Height() / 2);
			SpriteShader::Draw(scene, drawPoint + sceneOffset);
			drawPoint.Y() += scene->Height();
		}
	}
	return drawPoint.Y() - topLeft.Y();
}



void BookEntry::AppendItem(const ItemType &item)
{
	// Skip empty strings.
	if(holds_alternative<string>(item) && std::get<string>(item).empty())
		return;

	if(!std::holds_alternative<std::monostate>(item))
		items.emplace_back(item);
}



BookEntry::ItemType BookEntry::ReadItem(const DataNode &node, int startAt)
{
	if(node.Size() - startAt == 2 && node.Token(startAt) == "scene")
		return SpriteSet::Get(node.Token(startAt + 1));

	string text;
	for(int i = startAt; i < node.Size(); ++i)
	{
		if(!text.empty())
			text += "\n\t";
		text += node.Token(i);
	}
	return text;
}
