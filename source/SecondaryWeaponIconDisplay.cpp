/* SecondaryWeaponIconDisplay.cpp
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SecondaryWeaponIconDisplay.h"

#include "Color.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include "SDL2/SDL_keyboard.h"

using namespace std;



SecondaryWeaponIconDisplay::SecondaryWeaponIconDisplay(PlayerInfo &player)
	: player(player)
{

}



void SecondaryWeaponIconDisplay::Update(const Ship &flagship)
{
	Clear();
	for(const auto &it : flagship->Outfits())
	{
		const Outfit *secWeapon = it.first;
		if(!secWeapon->Icon())
			continue;

		double ammoCount = -1;
		if(secWeapon->Ammo())
			ammoCount = flagship->OutfitCount(secWeapon->Ammo());
		else if(secWeapon->FiringFuel())
		{
			double remaining = flagship->Fuel()
				* flagship->Attributes().Get("fuel capacity");
			ammoCount = remaining / secWeapon->FiringFuel();
		}
		ammo.emplace_back(secWeapon, ammoCount);
	}
}



void SecondaryWeaponIconDisplay::Clear()
{
	ammo.clear();
}



void SecondaryWeaponIconDisplay::Draw(Rectangle ammoBox, Point iconDim) const
{
	const Set<Color> &colors = GameData::Colors();
	const Font &font = FontSet::Get(14);
	ammoIconZones.clear();

	double ammoIconWidth = iconDim.X();
	double ammoIconHeight = iconDim.Y();
	// Pad the ammo list by the same amount on all four sides.
	double ammoPad = .5 * (ammoBox.Width() - ammoIconWidth);
	const Sprite *selectedSprite = SpriteSet::Get("ui/ammo selected");
	const Sprite *unselectedSprite = SpriteSet::Get("ui/ammo unselected");
	Color selectedColor = *colors.Get("bright");
	Color unselectedColor = *colors.Get("dim");

	// This is the bottom left corner of the ammo display.
	Point pos(ammoBox.Left() + ammoPad, ammoBox.Bottom() - ammoPad);
	// These offsets are relative to that corner.
	Point boxOff(ammoIconWidth - .5 * selectedSprite->Width(), .5 * ammoIconHeight);
	Point textOff(ammoIconWidth - .5 * ammoIconHeight, .5 * (ammoIconHeight - font.Height()));
	Point iconOff(.5 * ammoIconHeight, .5 * ammoIconHeight);
	double iconCenterX = (ammoBox.Right() + ammoBox.Left()) / 2.;
	for(const pair<const Outfit *, int> &it : ammo)
	{
		pos.Y() -= ammoIconHeight;
		if(pos.Y() < ammoBox.Top() + ammoPad)
			break;

		const auto &playerSelectedWeapons = player.SelectedSecondaryWeapons();
		bool isSelected = (playerSelectedWeapons.find(it.first) != playerSelectedWeapons.end());

		SpriteShader::Draw(it.first->Icon(), pos + iconOff);
		SpriteShader::Draw(isSelected ? selectedSprite : unselectedSprite, pos + boxOff);

		Point iconCenter(iconCenterX, pos.Y() + ammoIconHeight / 2.);
		ammoIconZones.emplace_back(iconCenter, iconDim, it.first);

		// Some secondary weapons may not have limited ammo. In that case, just
		// show the icon without a number.
		if(it.second < 0)
			continue;

		string amount = to_string(it.second);
		Point textPos = pos + textOff + Point(-font.Width(amount), 0.);
		font.Draw(amount, textPos, isSelected ? selectedColor : unselectedColor);
	}
}



bool SecondaryWeaponIconDisplay::Click(const Point &clickPoint)
{
	bool control = (SDL_GetModState() & KMOD_CTRL);
	for(const ClickZone<const Outfit *> &it : ammoIconZones)
		if(it.Contains(clickPoint))
		{
			if(!control)
				player.DeselectAllSecondaries();
			player.ToggleAnySecondary(it.Value());
			return true;
		}
	return false;
}

