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
#include "GameData.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "System.h"
#include "text/WrappedText.h"

using namespace std;



bool BookEntry::IsEmpty() const
{
	return all_of(items.begin(), items.end(),
		[](const Item &item) { return holds_alternative<std::monostate>(item); });
}



void BookEntry::Load(const DataNode &node, optional<int> startAt)
{
	if(startAt.has_value() && startAt < node.Size())
		LoadSingle(node, *startAt);
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "source" && child.Size() >= 2)
			source = GameData::Systems().Get(child.Token(1));
		else if(key == "mark")
		{
			for(int i = 1; i < child.Size(); ++i)
				markSystems.insert(GameData::Systems().Get(child.Token(i)));
			for(const DataNode &grand : child)
				markSystems.insert(GameData::Systems().Get(grand.Token(0)));
		}
		else if(key == "circle")
		{
			for(int i = 1; i < child.Size(); ++i)
				circleSystems.insert(GameData::Systems().Get(child.Token(i)));
			for(const DataNode &grand : child)
				circleSystems.insert(GameData::Systems().Get(grand.Token(0)));
		}
		else
			LoadSingle(child);
	}
}



void BookEntry::Add(const BookEntry &other)
{
	items.insert(items.end(), other.items.begin(), other.items.end());
	for(const Item &item : other.items)
		if(holds_alternative<const Sprite *>(item))
			scenes.insert(std::get<const Sprite *>(item));

	markSystems.insert(other.markSystems.begin(), other.markSystems.end());
	circleSystems.insert(other.circleSystems.begin(), other.circleSystems.end());
}



BookEntry BookEntry::Instantiate(const map<string, string> &subs) const
{
	BookEntry newEntry;
	for(const Item &item : items)
	{
		// Perform requested substitutions on the text of this node and return a new variant.
		if(holds_alternative<string>(item))
			newEntry.items.emplace_back(Format::Replace(std::get<string>(item), subs));
		else
			newEntry.items.emplace_back(item);
	}
	newEntry.scenes = scenes;
	newEntry.markSystems = markSystems;
	newEntry.circleSystems = circleSystems;
	return newEntry;
}



void BookEntry::Save(DataWriter &out) const
{
	out.BeginChild();
	{
		for(const Item &item : items)
		{
			// Break the text up into paragraphs.
			if(holds_alternative<string>(item))
				for(const string &line : Format::Split(std::get<string>(item), "\n\t"))
					out.Write(line);
			else
				out.Write("scene", std::get<const Sprite *>(item)->Name());
		}
		if(source)
			out.Write("source", source->TrueName());
		if(!markSystems.empty())
		{
			out.Write("mark");
			out.BeginChild();
			{
				for(const System *system : markSystems)
					out.Write(system->TrueName());
			}
			out.EndChild();
		}
		if(!circleSystems.empty())
		{
			out.Write("circle");
			out.BeginChild();
			{
				for(const System *system : circleSystems)
					out.Write(system->TrueName());
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



const set<const Sprite *> &BookEntry::GetScenes() const
{
	return scenes;
}



int BookEntry::Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const
{
	Point drawPoint = topLeft;
	for(const Item &item : items)
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



const System *BookEntry::SourceSystem() const
{
	return source;
}



void BookEntry::SetSourceSystem(const System *system)
{
	source = system;
}



const set<const System *> &BookEntry::MarkSystems() const
{
	return markSystems;
}



const set<const System *> &BookEntry::CircleSystems() const
{
	return circleSystems;
}



bool BookEntry::HasSystems() const
{
	return source || !markSystems.empty() || !circleSystems.empty();
}



void BookEntry::LoadSingle(const DataNode &node, int startAt)
{
	if(node.Size() - startAt == 2 && node.Token(startAt) == "scene")
	{
		const Sprite *scene = SpriteSet::Get(node.Token(startAt + 1));
		items.emplace_back(scene);
		scenes.insert(scene);
	}
	else
	{
		string text;
		for(int i = startAt; i < node.Size(); ++i)
		{
			if(!text.empty())
				text += "\n\t";
			text += node.Token(i);
		}
		items.emplace_back(text);
	}
}
