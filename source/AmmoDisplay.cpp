/* AmmoDisplay.cpp
Copyright (c) 2022 by warp-core, 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AmmoDisplay.h"

#include "Color.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Rectangle.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

using namespace std;



AmmoDisplay::AmmoDisplay(PlayerInfo &player)
	: player(player)
{
}



void AmmoDisplay::Update(const Ship &flagship)
{
	ammo.clear();
	for(const auto &it : flagship.Weapons())
	{
		const Outfit *secWeapon = it.GetOutfit();
		if(!secWeapon || !secWeapon->Icon() || ammo.find(secWeapon) != ammo.end())
			continue;

		double ammoCount = -1.;
		// TODO: if a weapon has both an ammo requirement and consumes fuel, the ammo display will only show the ammo,
		// not the fuel, so the weapon may not fire because it is out of fuel even though it still has ammo,
		// and will not show a '0' in the ammo display.
		if(secWeapon->Ammo())
			ammoCount = flagship.OutfitCount(secWeapon->Ammo());
		else if(secWeapon->FiringFuel())
		{
			double remaining = flagship.Fuel()
				* flagship.Attributes().Get("fuel capacity");
			ammoCount = remaining / secWeapon->FiringFuel();
		}
		ammo[secWeapon] = ammoCount;
	}
}



void AmmoDisplay::Draw(const Rectangle &ammoBox, const Point &iconDim) const
{
	const Font &font = FontSet::Get(14);
	ammoIconZones.clear();

	const double &ammoIconWidth = iconDim.X();
	const double &ammoIconHeight = iconDim.Y();
	// Pad the ammo list by the same amount on all four sides.
	double ammoPad = .5 * (ammoBox.Width() - ammoIconWidth);
	const Sprite *selectedSprite = SpriteSet::Get("ui/ammo selected");
	const Sprite *unselectedSprite = SpriteSet::Get("ui/ammo unselected");
	const Color &selectedColor = *GameData::Colors().Get("bright");
	const Color &unselectedColor = *GameData::Colors().Get("dim");

	// This is the bottom left corner of the ammo display.
	auto pos = Point(ammoBox.Left() + ammoPad, ammoBox.Bottom() - ammoPad);
	// These offsets are relative to that corner.
	auto boxOff = Point(ammoIconWidth - .5 * selectedSprite->Width(), .5 * ammoIconHeight);
	auto textOff = Point(ammoIconWidth - .5 * ammoIconHeight, .5 * (ammoIconHeight - font.Height()));
	auto iconOff = Point(.5 * ammoIconHeight, .5 * ammoIconHeight);
	const double iconCenterX = (ammoBox.Right() + ammoBox.Left()) / 2.;
	for(const auto &it : ammo)
	{
		pos.Y() -= ammoIconHeight;
		if(pos.Y() < ammoBox.Top() + ammoPad)
			break;

		const auto &playerSelectedWeapons = player.SelectedSecondaryWeapons();
		bool isSelected = (playerSelectedWeapons.find(it.first) != playerSelectedWeapons.end());

		SpriteShader::Draw(it.first->Icon(), pos + iconOff);
		SpriteShader::Draw(isSelected ? selectedSprite : unselectedSprite, pos + boxOff);

		auto iconCenter = Point(iconCenterX, pos.Y() + ammoIconHeight / 2.);
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



bool AmmoDisplay::Click(const Point &clickPoint, bool control)
{
	for(const auto &it : ammoIconZones)
		if(it.Contains(clickPoint))
		{
			if(!control)
				player.DeselectAllSecondaries();
			player.ToggleAnySecondary(it.Value());
			return true;
		}
	return false;
}



bool AmmoDisplay::Click(const Rectangle &clickBox)
{
	bool reselected = false;
	for(const auto &it : ammoIconZones)
		if(it.Overlaps(clickBox))
		{
			if(!reselected)
			{
				reselected = true;
				player.DeselectAllSecondaries();
			}
			player.ToggleAnySecondary(it.Value());
		}
	return reselected;
}

