/* PlanetPanel.cpp
Michael Zahniser, 30 Dec 2013

Function definitions for the PlanetPanel GUI.
*/

#include "PlanetPanel.h"

#include "DirIt.h"
#include "DrawList.h"
#include "Font.h"
#include "FontSet.h"
#include "shift.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "WrappedText.h"

#include <map>
#include <string>

using namespace std;

namespace {
	DrawList draw;
	
	const Sprite *ui = nullptr;
	map<string, const Sprite *> gallery;
	
	void LoadSprites()
	{
		if(ui)
			return;
		
		ui = SpriteSet::Get("ui/planet dialog.png");
		
		DirIt it("../images/land/");
		while(true)
		{
			string path = it.Next();
			if(path.empty())
				break;
			if(path.length() < 4 || path.substr(path.length() - 4) != ".jpg")
				continue;
			
			path = path.substr(10);
			gallery[path] = SpriteSet::Get(path);
		}
		
		SpriteSet::Finish();
	}
}



PlanetPanel::PlanetPanel(Planet &planet)
	: planet(planet)
{
	if(planet.description.empty())
		planet.description += '\n';
	LoadSprites();
}



// Draw this panel.
void PlanetPanel::Draw() const
{
	draw.Clear();
	
	if(planet.landscape.empty())
	{
		int i = 0;
		for(const auto &it : gallery)
		{
			int x = ((i % 15) - 7) * (720 / 16 + 3) + 1;
			int y = (i / 15 - 7) * (360 / 16. + 2.5) - 130;
			draw.Add(it.second, Point(x, y), 1. / 16., Point(0., -1.));
			++i;
		}
	}
	else
		draw.Add(gallery[planet.landscape], Point(0., -141.));
	
	draw.Add(ui, Point());
	
	draw.Draw();
	
	const Font &font = FontSet::Get(14);
	WrappedText text;
	text.SetFont(font);
	text.SetAlignment(WrappedText::JUSTIFIED);
	text.SetWrapWidth(480);
	text.Wrap(planet.description);
	
	const float textColor[4] = {.8, .8, .8, 1.};
	for(const WrappedText::Word &word : text.Words())
	{
		Point pos(word.X() - 240., word.Y() + 80.);
		font.Draw(word.String(), pos, textColor);
	}
}



bool PlanetPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if((key >= ' ' && key <= '~') || key == '\t')
	{
		bool isShifted = (mod & KMOD_SHIFT);
		planet.description.insert(planet.description.length() - 1, 1,
			isShifted ? SHIFT[key] : key);
	}
	else if(key == SDLK_RETURN)
		planet.description.insert(planet.description.length() - 1, 1, '\n');
	else if(planet.description.size() > 1 && (key == SDLK_DELETE || key == SDLK_BACKSPACE))
		planet.description.erase(planet.description.length() - 2, 1);
	else if(key == SDLK_ESCAPE)
		Pop(this);
	return true;
}



bool PlanetPanel::Click(int x, int y)
{
	if(y >= -320 && y < 40 && x >= -360 && x <= 360)
	{
		if(planet.landscape.empty())
		{
			x += 360;
			y += 320;
			x /= 48;
			y /= 25;
			int i = x + y * 15;
		
			for(const auto &it : gallery)
			{
				if(!i--)
				{
					planet.landscape = it.first;
					break;
				}
			}
		}
		else
			planet.landscape.clear();
	}
	return true;
}



bool PlanetPanel::RClick(int x, int y)
{
	return true;
}
