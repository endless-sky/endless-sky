/* HailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "HailPanel.h"

#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "UI.h"

using namespace std;



HailPanel::HailPanel(PlayerInfo &player, const Ship *ship)
	: player(player), ship(ship)
{
}



HailPanel::HailPanel(PlayerInfo &player, const StellarObject *planet)
	: player(player), planet(planet)
{
}



void HailPanel::Draw() const
{
	DrawBackdrop();
	
	Information interfaceInfo;
	if(ship)
	{
		bool isEnemy = GameData::GetPolitics().IsEnemy(
			GameData::PlayerGovernment(), ship->GetGovernment());
		if(isEnemy)
		{
			interfaceInfo.SetCondition("can bribe");
			interfaceInfo.SetCondition("cannot assist");
		}
		else
			interfaceInfo.SetCondition("can assist");
	}
	else
	{
		bool isEnemy = GameData::GetPolitics().IsEnemy(
			GameData::PlayerGovernment(), &player.GetSystem()->GetGovernment());
		if(isEnemy)
			interfaceInfo.SetCondition("can bribe");
		else
			interfaceInfo.SetCondition("cannot bribe");
		
		interfaceInfo.SetCondition("can dominate");
	}
	
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	interface->Draw(interfaceInfo);
	
	// Draw the sprite, rotated, scaled, and swizzled as necessary.
	int swizzle = ship ? ship->GetGovernment()->GetSwizzle() : 0;
	const Animation &animation = ship ? ship->GetSprite() : planet->GetSprite();
	uint32_t tex = animation.GetSprite()->Texture();
	
	float pos[2] = {-170.f, -10.f};
	
	Point unit = ship ? 2. * ship->Unit() : planet->Position().Unit();
	double zoom = min(1., 200. / max(animation.Width(), animation.Height()));
	Point uw = unit * (animation.Width() * zoom);
	Point uh = unit * (animation.Height() * zoom);
	float tr[4] = {
		static_cast<float>(-uw.Y()), static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()), static_cast<float>(-uh.Y())};
	
	SpriteShader::Bind();
	SpriteShader::Add(tex, tex, pos, tr, swizzle);
	SpriteShader::Unbind();
}



bool HailPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == 'd')
		GetUI()->Pop(this);
	else if(key == 'a' || key == 't' || key == 'h')
	{
		// TODO: handle dominating planets and asking for help from ships.
	}
	else if(key == 'b' || key == 'o')
	{
		// TODO: handle bribes to planets and ships.
	}
	
	return true;
}



bool HailPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key != '\0')
			return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	return true;
}
