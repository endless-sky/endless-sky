/* SystemPanel.cpp
Michael Zahniser, 26 Dec 2013

Function definitions for the SystemPanel class.
*/

#include "SystemPanel.h"

#include "Angle.h"
#include "Color.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "SpriteSet.h"
#include "FontSet.h"
#include "Font.h"
#include "Screen.h"
#include "DotShader.h"
#include "NamePanel.h"
#include "PlanetPanel.h"
#include "UI.h"

#include <string>

using namespace std;



SystemPanel::SystemPanel(System &system, Set<Planet> &planets)
	: system(system), planets(planets), selected(nullptr), paused(false)
{
	tm t;
	memset(&t, 0, sizeof(t));
	t.tm_hour = 12;
	t.tm_mday = 1;
	t.tm_mon = 1;
	t.tm_year = 3013 - 1900;
	now = mktime(&t);
}



// Move the state of this panel forward one game step. This will only be
// called on the front-most panel, so if there are things like animations
// that should work on panels behind that one, update them in Draw().
void SystemPanel::Step(bool isActive)
{
	if(!paused && isActive)
		now += 60 * 60;
}



// Draw this panel.
void SystemPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	draw.Clear();
	drawn.clear();
	Draw(system.root);
	draw.Draw();
	
	const Font &font = FontSet::Get(14);
	double y = Screen::Height() * -.5;
	double x = Screen::Width() * -.5;
	for(const auto &it : system.trade)
	{
		const string &name = it.first;
		float trade = system.TradeRange(name);
		float color[4] = {
			(trade >= 0.f) ? 1.f : (1.f + trade),
			0.f,
			(trade <= 0.f) ? 1.f : (1.f - trade),
			1.f};
		color[1] = min(color[0], color[2]);
		Color shade(color[0], color[1], color[2], color[3]);
		DotShader::Draw(Point(x + 10., y + 10.), 6., 2., shade);
		font.Draw(name, Point(x + 20., y + 10. - .5 * font.Height()), shade);
		y += 20.;
	}
}



// Only override the ones you need; the default action is to return false.
bool SystemPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'r')
	{
		selected = nullptr;
		drawn.clear();
		system.Randomize();
	}
	else if(key == 'h')
		position = Point();
	else if(key == 'p')
		paused = !paused;
	else if(key == 's')
	{
		selected = nullptr;
		drawn.clear();
		system.Sol();
	}
	else if(key == SDLK_RETURN && selected)
	{
		if(selected->planet.empty())
			GetUI()->Push(new NamePanel(selected->planet));
		else
		{
			Planet &planet = *planets.Get(selected->planet);
			if(planet.name.empty())
				planet.name = selected->planet;
			GetUI()->Push(new PlanetPanel(planet));
		}
	}
	else if(key == SDLK_ESCAPE)
		GetUI()->Pop(this);
	
	return true;
}



bool SystemPanel::Click(int x, int y)
{
	selected = nullptr;
	Point click(x, y);
	for(const auto &it : drawn)
	{
		double d = click.Distance(it.second);
		if(d < SpriteSet::Get(it.first->sprite)->Width() * .5)
			selected = const_cast<System::Object *>(it.first);
	}
	return true;
}



bool SystemPanel::Drag(int dx, int dy)
{
	position += Point(dx, dy);
	return true;
}



void SystemPanel::Draw(const System::Object &object, Point center) const
{
	Point off(Screen::Width() * .5 - 300., Screen::Height() * .5 - 300.);
	static const Color white(1., 1., 1., 1.);
	static const double scale = .1;
	static const Color color[5] = {
		Color(1., 0., 0., 1.),
		Color(1., 1., 0., 1.),
		Color(0., 1., 0., 1.),
		Color(0., 1., 1., 1.),
		Color(0., 0., 1., 1.)};
	static const Color grey(.5, .5, .5, 1.);
	
	double time = now / (60. * 60. * 24.);
	for(auto it = object.children.rbegin(); it != object.children.rend(); ++it)
	{
		Angle angle(360. * time / it->period + it->offset);
		Point pos = angle.Unit() * it->distance + center;
		
		double warmth = pos.Length() / system.habitable;
		const Color *ring = &grey;
		if(!center.X() && !center.Y())
			ring = (warmth < .5) ? &color[0] :
				(warmth < .8) ? &color[1] :
				(warmth < 1.2) ? &color[2] :
				(warmth < 2.0) ? &color[3] : &color[4];
		
		double d = it->distance * scale;
		DotShader::Draw(center * scale + off, d + .7, d - .7, *ring);
		
		Draw(*it, pos);
	}
	
	Point unit = Angle(360. * time / object.period).Unit();
	if(center.X() || center.Y())
		unit = center.Unit();
	
	if(!object.sprite.empty())
	{
		const Sprite *sprite = SpriteSet::Get(object.sprite);
		draw.Add(sprite, center + position, unit);
		drawn[&object] = center + position;
		
		if(&object == selected)
		{
			double r = sprite->Width() * .6;
			DotShader::Draw(center + position, r, r - 3., white);
		}
		
		double d = sprite->Width() * .5 * scale;
		DotShader::Draw(center * scale + off, d + 1., d - 1., white);
	}
}
