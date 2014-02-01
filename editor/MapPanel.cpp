/* MapPanel.cpp
Michael Zahniser, 28 Dec 2013

Function definitions for the MapPanel class.
*/

#include "MapPanel.h"

#include "DataFile.h"
#include "DotShader.h"
#include "Font.h"
#include "FontSet.h"
#include "LineShader.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "SystemPanel.h"

#include <fstream>

using namespace std;



MapPanel::MapPanel(const char *path)
	: mapPath(path), selected(nullptr), commodity("Food")
{
	DataFile mapData(mapPath);
	
	for(const DataFile::Node &node : mapData)
	{
		if(node.Token(0) == "system" && node.Size() >= 2)
			systems.Get(node.Token(1))->Load(node, systems);
		else if(node.Token(0) == "planet" && node.Size() >= 2)
			planets.Get(node.Token(1))->Load(node);
		else
			unrecognized.push_back(node);
	}
	
	SpriteSet::Get("ui/galaxy.jpg");
	SpriteSet::Finish();
}



MapPanel::~MapPanel()
{
	ofstream out(mapPath);
	for(const auto &it : systems)
		it.second.Write(out);
	for(const auto &it : planets)
		it.second.Write(out);
	
	// Now, write everything we did not recognize.
	for(const DataFile::Node &node : unrecognized)
		node.Write(out);
}



// Draw this panel.
void MapPanel::Draw() const
{
	const Sprite *galaxy = SpriteSet::Get("ui/galaxy.jpg");
	SpriteShader::Draw(galaxy, position);
	
	float white[] = {1., 1., 1., 1.};
	float grey[] = {.5, .5, .5, .25};
	if(selected)
		DotShader::Draw(selected->pos + position, 100., 98., grey);
	
	for(const auto &it : systems)
	{
		const System *system = &it.second;
		
		for(const System *link : system->links)
			if(link > system)
			{
				Point from = system->pos + position;
				Point to = link->pos + position;
				Point unit = (from - to).Unit() * 7.;
				from -= unit;
				to += unit;
				LineShader::Draw(from, to, 1.2, grey);
			}
	}
	for(const auto &it : systems)
	{
		float trade = it.second.TradeRange(commodity);
		float color[4] = {
			(trade >= 0.f) ? 1.f : (1.f + trade),
			0.f,
			(trade <= 0.f) ? 1.f : (1.f - trade),
			1.f};
		color[1] = min(color[0], color[2]);
		if(&it.second != selected)
		{
			color[0] *= .5f;
			color[1] *= .5f;
			color[2] *= .5f;
			color[3] *= .25f;
		}
		DotShader::Draw(it.second.pos + position, 6., 3.5, color);
	}
	
	const Font &font = FontSet::Get(14);
	Point offset(6., -.5 * font.Height());
	for(const auto &it : systems)
		font.Draw(it.second.name, it.second.pos + offset + position,
			(&it.second == selected) ? white : grey);
	
	double y = Screen::Height() * -.5;
	double x = Screen::Width() * -.5;
	
	for(const auto &it : systems.begin()->second.trade)
	{
		const string &name = it.first;
		float trade = selected ? selected->TradeRange(name) : 0.f;
		float color[4] = {
			(trade >= 0.f) ? 1.f : (1.f + trade),
			0.f,
			(trade <= 0.f) ? 1.f : (1.f - trade),
			1.f};
		color[1] = min(color[0], color[2]);
		if(name != commodity)
		{
			color[0] *= .5f;
			color[1] *= .5f;
			color[2] *= .5f;
			color[3] = .25f;
		}
		DotShader::Draw(Point(x + 10., y + 10.), 6., 3.5, color);
		font.Draw(name, Point(x + 20., y + 10. - .5 * font.Height()), color);
		y += 20.;
	}
}



// Only override the ones you need; the default action is to return false.
bool MapPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'f')
		commodity = "Food";
	else if(key == 'c')
		commodity = "Clothing";
	else if(key == 'm')
		commodity = "Metal";
	else if(key == 'p')
		commodity = "Plastic";
	else if(key == 'q')
		commodity = "Equipment";
	else if(key == 'd')
		commodity = "Medical";
	else if(key == 'e')
		commodity = "Electronics";
	else if(key == 'i')
		commodity = "Industrial";
	else if(key == 'h')
		commodity = "Heavy Metals";
	else if(key == 'l')
		commodity = "Luxury Goods";
	else if(key == SDLK_RETURN && selected)
		Push(new SystemPanel(*selected, planets));
	return true;
}



bool MapPanel::Click(int x, int y)
{
	selected = nullptr;
	
	Point click(x, y);
	click -= position;
	for(const auto &it : systems)
	{
		if(it.second.pos.Distance(click) <= 6.)
			selected = systems.Get(it.first);
	}
	return true;
}



bool MapPanel::RClick(int x, int y)
{
	if(!selected)
		return Click(x, y);
	
	Point click(x, y);
	click -= position;
	for(const auto &it : systems)
	{
		if(it.second.pos.Distance(click) <= 6.)
		{
			systems.Get(it.first)->ToggleLink(selected);
			break;
		}
	}
	return true;
}



bool MapPanel::Drag(int dx, int dy)
{
	if(selected)
		selected->pos += Point(dx, dy);
	else
		position += Point(dx, dy);
	
	return true;
}
