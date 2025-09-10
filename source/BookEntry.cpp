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



// Text constructor.
BookEntry::Item::Item(const string &text)
	: text(text)
{
}



// Image constructor.
BookEntry::Item::Item(const Sprite *scene)
	: scene(scene)
{
}



BookEntry::Item BookEntry::Item::Read(const DataNode &node, int startAt)
{
	if(node.Size() - startAt == 2 && node.Token(startAt) == "scene")
		return Item(SpriteSet::Get(node.Token(startAt + 1)));

	string text;
	for(int i = startAt; i < node.Size(); ++i)
	{
		if(!text.empty())
			text += "\n\t";
		text += node.Token(i);
	}
	return Item(text);
}



BookEntry::Item BookEntry::Item::Instantiate(const map<string, string> &subs) const
{
	// Perform requested substitutions on the text of this node and return a new variant.
	if(scene)
		return Item(scene);
	return Item(Format::Replace(text, subs));
}



void BookEntry::Item::Save(DataWriter &out) const
{
	if(scene)
		out.Write("scene", scene->Name());
	else
	{
		// Break the text up into paragraphs.
		for(const string &line : Format::Split(text, "\n\t"))
			out.Write(line);
	}
}



int BookEntry::Item::Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const
{
	if(scene)
	{
		const Point offset(scene->Width() / 2, scene->Height() / 2);
		SpriteShader::Draw(scene, topLeft + offset);
		return scene->Height();
	}

	wrap.Wrap(text);
	wrap.Draw(topLeft, color);
	return wrap.Height();
}



bool BookEntry::Item::Empty() const
{
	return !scene && text.empty();
}



bool BookEntry::Empty() const
{
	if(items.empty())
		return true;
	for(const Item &item : items)
		if(!item.Empty())
			return false;
	return true;
}



void BookEntry::Append(const Item &item)
{
	if(!item.Empty())
		items.emplace_back(item);
}



void BookEntry::Read(const DataNode &node, int startAt)
{
	// Note: Like Dialog::ParseTextNode, BookEntry::Read will consume all remaining Tokens of this node
	// as well as all remaining tokens of the children of this node.

	// First, consume the rest of this first node:
	if(startAt <= node.Size())
		Append(Item::Read(node, startAt));

	// Then continue with its child nodes:
	for(const DataNode &child : node)
		Append(Item::Read(child));
}



void BookEntry::Add(const BookEntry &other)
{
	for(const Item &item : other.items)
		Append(item);
}



BookEntry BookEntry::Instantiate(const map<string, string> &subs) const
{
	BookEntry newEntry;
	for(const Item &item : items)
		newEntry.Append(item.Instantiate(subs));
	return newEntry;
}



void BookEntry::Save(DataWriter &out) const
{
	for(const Item &item : items)
	{
		out.BeginChild();
		{
			item.Save(out);
		}
		out.EndChild();
	}
}



int BookEntry::Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const
{
	// offset will track the total height
	Point offset;
	for(auto &item : items)
	{
		int y = item.Draw(topLeft + offset, wrap, color);
		offset.Y() += y;
	}
	return offset.Y();
}
