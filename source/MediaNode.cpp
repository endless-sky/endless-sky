/* MediaNode.cpp
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

#include "MediaNode.h"

#include "DataWriter.h"
#include "text/Format.h"
#include "shader/SpriteShader.h"

using namespace std;

// Text constructor.
MediaNode::MediaNode(const string &text)
	: text(text)
{
}



// Image constructor.
MediaNode::MediaNode(const Sprite *scene)
	: scene(scene)
{
}



void MediaNode::Write(DataWriter &out) const
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



void MediaNode::FormatReplace(map<string, string> &subs)
{
	// Perform requested substitutions on the text of this node.
	if(!text.empty())
		text = Format::Replace(text, subs);
}



int MediaNode::Draw(const Point &topLeft, const Font &font, Alignment alignment, int width, const Color &color) const
{
	if(scene)
	{
		const Point offset(scene->Width() / 2, scene->Height() / 2);
		SpriteShader::Draw(scene, topLeft + offset);
		return scene->Height();
	}
	else
	{
		WrappedText wrap(font);
		wrap.SetAlignment(alignment);
		wrap.SetWrapWidth(width);
		wrap.Wrap(text);
		wrap.Draw(topLeft, color);
		return wrap.Height();
	}
}
