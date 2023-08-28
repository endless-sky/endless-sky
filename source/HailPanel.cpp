/* HailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HailPanel.h"

#include "text/alignment.hpp"
#include "Dialog.h"
#include "DrawList.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Ship.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <cmath>

using namespace std;



HailPanel::HailPanel()
{
	SetInterruptible(false);
}



void HailPanel::DrawHail()
{
	DrawBackdrop();

	Information info;
	info.SetString("header", header);

	const Interface *hailUi = GameData::Interfaces().Get("hail panel");
	hailUi->Draw(info, this);

	// Draw the current message.
	WrappedText wrap;
	wrap.SetAlignment(Alignment::JUSTIFIED);
	wrap.SetWrapWidth(330);
	wrap.SetFont(FontSet::Get(14));
	wrap.Wrap(message);
	wrap.Draw(Point(-50., -50.), *GameData::Colors().Get("medium"));
}



void HailPanel::DrawIcon(const Ship &ship)
{
	// Draw the sprite, rotated, scaled, and swizzled as necessary.
	float zoom = min(2.f, 400.f / max(ship.GetSprite()->Width(), ship.GetSprite()->Height()));
	Point center(-170., -10.);
	const Angle facing = ship.Facing();

	DrawList draw;
	// Copy the ships's swizzle, animation settings, etc.
	// Also draw its fighters and weapon hardpoints.
	bool hasFighters = ship.PositionFighters();
	auto addHardpoint = [this, &draw, &center, zoom, &facing](const Hardpoint &hardpoint) -> void
	{
		if(hardpoint.GetOutfit() && hardpoint.GetOutfit()->HardpointSprite().HasSprite())
		{
			Body body(
				hardpoint.GetOutfit()->HardpointSprite(),
				center + zoom * facing.Rotate(hardpoint.GetPoint()),
				Point(),
				facing + hardpoint.GetAngle(),
				zoom);
			draw.Add(body);
		}
	};
	auto addFighter = [this, &draw, &center, zoom, &facing](const Ship::Bay &bay) -> void
	{
		if(bay.ship)
		{
			Body body(
				*bay.ship,
				center + zoom * facing.Rotate(bay.point),
				Point(),
				facing + bay.facing,
				zoom);
			draw.Add(body);
		}
	};
	if(hasFighters)
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.side == Ship::Bay::UNDER)
				addFighter(bay);
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.IsUnder())
			addHardpoint(hardpoint);
	draw.Add(Body(ship, center, Point(), facing, zoom));
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(!hardpoint.IsUnder())
			addHardpoint(hardpoint);
	if(hasFighters)
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.side == Ship::Bay::OVER)
				addFighter(bay);
	draw.Draw();
}



void HailPanel::DrawIcon(const Sprite &sprite, Angle facing)
{
	const float zoom = min(2.f, 400.f / max(sprite.Width(), sprite.Height()));
	const Point center(-170., -10.);

	DrawList draw;
	draw.Add(Body(&sprite, center, Point(), facing, zoom));
	draw.Draw();
}
