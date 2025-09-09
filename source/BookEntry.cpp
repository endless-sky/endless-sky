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
#include "Dialog.h"
#include "text/Format.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"


#include "Logger.h"

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
	Logger::LogError("\t\t\t\t\tnew Item, text: " + text);
}

// Image constructor.
BookEntry::Item::Item(const Sprite *scene)
	: scene(scene)
{
	Logger::LogError("\t\t\t\t\tnew Item, scene->Name(): " + scene->Name());
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
	else
	{
		wrap.Wrap(text);
		wrap.Draw(topLeft, color);
		return wrap.Height();
	}
}

void BookEntry::Read(const DataNode &node, const int startAt)
{
	Logger::LogError("\t\t\t\tBookEntry.Read(): [remaining:" + to_string(node.Size() - startAt) + ", startAt:" + to_string(startAt)
		+ ", node.Size():" + to_string(node.Size()) + ", Token(startAt):" + node.Token(startAt) + "]");
	if(node.Size() - startAt == 2 && node.Token(startAt) == "scene")
		items.emplace_back(Item(SpriteSet::Get(node.Token(startAt + 1))));
	else
	{
		string text;
		Dialog::ParseTextNode(node, startAt, text);
		items.emplace_back(Item(text));
	}
}

void BookEntry::Add(const BookEntry &other)
{
	for(const Item &item : other.items)
		items.emplace_back(item);
}

BookEntry BookEntry::Instantiate(const map<string, string> &subs) const
{
	BookEntry newEntry;
	for(const Item &item : items)
		newEntry.items.emplace_back(item.Instantiate(subs));
	return newEntry;
}

void BookEntry::Save(DataWriter &out, const int day, const int month, const int year) const
{
	// Note: in order to properly read sprites back in, we must repeat the day/month/year to distinguish
	// text from sprites.
	for(const Item &item : items)
	{
		out.Write(day, month, year);
		out.BeginChild();
		{
			item.Save(out);
		}
		out.EndChild();
	}
}

void BookEntry::Save(DataWriter &out, const string &topic, const string &heading) const
{
	// Note: in order to properly read sprites back in, we must repeat the day/month/year to distinguish
	// text from sprites.
	for(const Item &item : items)
	{
		out.Write(topic, heading);
		out.BeginChild();
		{
			item.Save(out);
		}
		out.EndChild();
	}
}

void BookEntry::Save(DataWriter &out, const string &book, const string &topic, const string &heading) const
{
	for(const Item &item : items)
	{
		out.Write(book, topic, heading);
		item.Save(out);
	}
}

void BookEntry::Save(DataWriter &out, const string &book) const
{
	for(const Item &item : items)
	{
		out.Write(book);
		item.Save(out);
	}
}

int BookEntry::Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const
{
	// offset will track the total height
	Point offset = Point();
	// Logger::LogError("\t\tEntry.Draw(): topLeft.Y() + offset.Y():" + to_string(topLeft.Y() + offset.Y()));
	for(auto &item : items)
	{
		int y = item.Draw(topLeft + offset, wrap, color);
		// Logger::LogError("\t\t\tItem.Draw() returned:" + to_string(y));
		offset.Y() += y;
	}
	// Logger::LogError("\t\tEntry.Draw(): offset.Y():" + to_string(offset.Y()));
	return offset.Y();
}
