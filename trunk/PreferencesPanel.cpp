/* PreferencesPanel.cpp
Michael Zahniser, 20 Feb 2014

Function definitions for the PreferencesPanel class.
*/

#include "PreferencesPanel.h"

#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"

#include <GL/glew.h>

#include <cmath>
#include <map>

using namespace std;



PreferencesPanel::PreferencesPanel(GameData &data)
	: data(data), editing(-1), selected(0), firstY(0)
{
}



// Draw this panel.
void PreferencesPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	Color dim(.1, 0.);
	Color medium(.5, 0.);
	Color bright(.8, 0.);
	
	const Font &font = FontSet::Get(14);
	
	Point pos(0., -10. * (Key::END - Key::MENU) - 30.);
	{
		Point dOff(-100 - font.Width("Default"), 0.);
		font.Draw("Default", pos + dOff, bright.Get());
		
		Point cOff(-20 - font.Width("Key"), 0.);
		font.Draw("Key", pos + cOff, bright.Get());
		
		font.Draw("Action", pos, bright.Get());
		
		FillShader::Fill(pos + Point(0., 20.), Point(400., 1.), bright.Get());
		pos.Y() += 30.;
	}
	
	firstY = pos.Y();
	
	Point sOff(0., .5 * font.Height() + 20. * selected);
	FillShader::Fill(pos + sOff, Point(400., 20.), dim.Get());
	
	if(editing != -1)
	{
		Point eOff(-40., .5 * font.Height() + 20. * editing);
		FillShader::Fill(pos + eOff, Point(60., 20.), dim.Get());
	}
	
	// Check for conflicts.
	Color red(.5, 0., 0., 0.);
	map<int, int> count;
	for(Key::Command c = Key::MENU; c != Key::END; c = static_cast<Key::Command>(c + 1))
		++count[data.Keys().Get(c)];
	
	for(Key::Command c = Key::MENU; c != Key::END; c = static_cast<Key::Command>(c + 1))
	{
		// Mark conflicts.
		if(count[data.Keys().Get(c)] > 1)
		{
			Point eOff(-40., .5 * font.Height());
			FillShader::Fill(pos + eOff, Point(60., 20.), red.Get());
		}
		
		string current = SDL_GetKeyName(static_cast<SDLKey>(data.Keys().Get(c)));
		string def = SDL_GetKeyName(static_cast<SDLKey>(data.DefaultKeys().Get(c)));
		
		Point dOff(-100 - font.Width(def), 0);
		font.Draw(def, pos + dOff, (current == def ? dim : medium).Get());
		
		Point cOff(-20 - font.Width(current), 0);
		font.Draw(current, pos + cOff, bright.Get());
		
		font.Draw(Key::Description(c), pos, medium.Get());
		pos.Y() += 20.;
	}
	
	Color black(0., 1.);
	Point button = pos + Point(150., 40.);
	FillShader::Fill(button, Point(100., 30.), medium.Get());
	FillShader::Fill(button, Point(98., 28.), black.Get());
	string back = "Back to Menu";
	font.Draw(back, button - .5 * Point(font.Width(back), font.Height()), bright.Get());
	buttonX = button.X();
	buttonY = button.Y();
}



bool PreferencesPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(editing != -1)
	{
		data.Keys().Set(static_cast<Key::Command>(editing), key);
		editing = -1;
		return true;
	}
	
	if(key == SDLK_DOWN)
		selected += (selected != Key::END - 1);
	else if(key == SDLK_UP)
		selected -= (selected != Key::MENU);
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'm' || key == data.Keys().Get(Key::MENU))
		Exit();
	
	return true;
}



bool PreferencesPanel::Click(int x, int y)
{
	if(abs(x - buttonX) < 50 && abs(y - buttonY) < 15)
	{
		Exit();
		return true;
	}
	
	y -= firstY;
	if(y < 0)
		return false;
	
	y /= 20;
	if(y >= Key::END)
		return false;
	
	selected = y;
	if(x >= -70 && x < -10)
		editing = selected;
	else if(x >= -150 && x < -90)
	{
		Key::Command command = static_cast<Key::Command>(selected);
		data.Keys().Set(command, data.DefaultKeys().Get(command));
	}
	
	return true;
}



void PreferencesPanel::Exit()
{
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	data.Keys().Save(keysPath);

	Pop(this);
}
