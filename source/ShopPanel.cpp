/* ShopPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShopPanel.h"

#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "OutlineShader.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PointerShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <SDL2/SDL.h>

using namespace std;

namespace {
	const int ICON_TILE = 62;
	const int ICON_COLS = 4;
	const double ICON_SIZE = ICON_TILE - 8;
}



ShopPanel::ShopPanel(PlayerInfo &player, const vector<string> &categories)
	: player(player), planet(player.GetPlanet()), playerShip(player.Flagship()), categories(categories)
{
	if(playerShip)
		playerShips.insert(playerShip);
	SetIsFullScreen(true);
	SetInterruptible(false);
}



void ShopPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Clear the list of clickable zones.
	zones.clear();
	
	DrawSidebar();
	DrawButtons();
	DrawMain();
	
	if(dragShip)
	{
		static const Color selected(.8, 1.);
		const Sprite *sprite = dragShip->GetSprite().GetSprite();
		double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
		Point size(sprite->Width() * scale, sprite->Height() * scale);
		OutlineShader::Draw(sprite, dragPoint, size, selected);
	}
}



void ShopPanel::DrawSidebar() const
{
	const Font &font = FontSet::Get(14);
	Color bright = *GameData::Colors().Get("bright");
	sideDetailHeight = 0;
	
	// Fill in the background.
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH / 2, 0.),
		Point(SIDE_WIDTH, Screen::Height()),
		Color(.1, 1.));
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH, 0.),
		Point(1, Screen::Height()),
		Color(.2, 1.));
	
	// Draw this string, centered in the side panel:
	static const string YOURS = "Your Ships:";
	Point yoursPoint(
		Screen::Right() - SIDE_WIDTH / 2 - font.Width(YOURS) / 2,
		Screen::Top() + 10 - sideScroll);
	font.Draw(YOURS, yoursPoint, bright);
	
	// Start below the "Your Ships" label, and draw them.
	Point point(
		Screen::Right() - SIDE_WIDTH / 2 - 93,
		Screen::Top() + SIDE_WIDTH / 2 - sideScroll + 40 - 93);
	
	int shipsHere = 0;
	for(shared_ptr<Ship> ship : player.Ships())
		shipsHere += !(ship->GetSystem() != player.GetSystem() || ship->IsDisabled());
	
	if(shipsHere > 1)
	{
		static const Color selected(.8, 1.);
		static const Color unselected(.4, 1.);
		for(shared_ptr<Ship> ship : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
				continue;
		
			if(point.X() > Screen::Right())
			{
				point.X() -= ICON_TILE * ICON_COLS;
				point.Y() += ICON_TILE;
			}
			
			bool isSelected = (playerShips.find(ship.get()) != playerShips.end());
			const Sprite *background = SpriteSet::Get(isSelected ? "ui/icon selected" : "ui/icon unselected");
			SpriteShader::Draw(background, point);
			
			const Sprite *sprite = ship->GetSprite().GetSprite();
			double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
			Point size(sprite->Width() * scale, sprite->Height() * scale);
			OutlineShader::Draw(sprite, point, size, isSelected ? selected : unselected);
		
			zones.emplace_back(point.X(), point.Y(), ICON_TILE / 2, ICON_TILE / 2, ship.get());
		
			point.X() += ICON_TILE;
		}
		point.Y() += ICON_TILE;
	}
	point.Y() += SHIP_SIZE / 2;
	
	if(playerShip)
	{
		point.X() = Screen::Right() - SIDE_WIDTH / 2;
		DrawShip(*playerShip, point, true);
		
		Point offset(SIDE_WIDTH / -2, SHIP_SIZE / 2);
		sideDetailHeight = DrawPlayerShipInfo(point + offset);
		point.Y() += sideDetailHeight + SHIP_SIZE;
	}
	maxSideScroll = point.Y() + sideScroll - Screen::Bottom() + 70 - SHIP_SIZE / 2;
	maxSideScroll = max(0, maxSideScroll);
	
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(sideScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Bottom() - 80),
		Point(0., 1.), 10., 10., 5., Color(sideScroll < maxSideScroll ? .8 : .2, 0.));
}



void ShopPanel::DrawButtons() const
{
	// The last 70 pixels on the end of the side panel are for the buttons:
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH / 2, Screen::Bottom() - 35),
		Point(SIDE_WIDTH, 70), Color(.2, 1.));
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH / 2, Screen::Bottom() - 70),
		Point(SIDE_WIDTH, 1), Color(.3, 1.));
	
	const Font &font = FontSet::Get(14);
	Color bright = *GameData::Colors().Get("bright");
	Color dim = *GameData::Colors().Get("medium");
	
	Point point(
		Screen::Right() - SIDE_WIDTH + 10,
		Screen::Bottom() - 65);
	font.Draw("You have:", point, dim);
	
	string credits = Format::Number(player.Accounts().Credits()) + " credits";
	point.X() += (SIDE_WIDTH - 20) - font.Width(credits);
	font.Draw(credits, point, bright);
	
	const Font &bigFont = FontSet::Get(18);
	
	Point buyCenter = Screen::BottomRight() - Point(210, 25);
	FillShader::Fill(buyCenter, Point(60, 30), Color(.1, 1.));
	string buy = (selectedOutfit && player.Cargo().Get(selectedOutfit)) ? "_Install" : "_Buy";
	bigFont.Draw(buy,
		buyCenter - .5 * Point(bigFont.Width(buy), bigFont.Height()),
		CanBuy() ? bright : dim);
	
	Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), Color(.1, 1.));
	bigFont.Draw("_Sell",
		sellCenter - .5 * Point(bigFont.Width("Sell"), bigFont.Height()),
		CanSell() ? bright : dim);
	
	Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), Color(.1, 1.));
	bigFont.Draw("_Leave",
		leaveCenter - .5 * Point(bigFont.Width("Leave"), bigFont.Height()),
		bright);
	
	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		font.Draw(mod, buyCenter + Point(-.5 * modWidth, 10.), dim);
		if(CanSellMultiple())
			font.Draw(mod, sellCenter + Point(-.5 * modWidth, 10.), dim);	
	}
}



void ShopPanel::DrawMain() const
{
	const Font &bigFont = FontSet::Get(18);
	Color bright = *GameData::Colors().Get("bright");
	mainDetailHeight = 0;
	
	// Draw all the available ships.
	// First, figure out how many columns we can draw.
	const int TILE_SIZE = TileSize();
	int mainWidth = (Screen::Width() - SIDE_WIDTH - 1);
	int columns = mainWidth / TILE_SIZE;
	int columnWidth = mainWidth / columns;
	
	Point begin(
		(Screen::Width() - columnWidth) / -2,
		(Screen::Height() - TILE_SIZE) / -2 - mainScroll);
	Point point = begin;
	float endX = Screen::Right() - (SIDE_WIDTH + 1);
	double nextY = begin.Y() + TILE_SIZE;
	int scrollY = 0;
	for(const string &category : categories)
	{
		map<string, set<string>>::const_iterator it = catalog.find(category);
		if(it == catalog.end())
			continue;
		
		// This should never happen, but bail out if we don't know what planet
		// we are on (meaning there's no way to know what ships are for sale).
		if(!planet)
			break;
		
		Point side(Screen::Left() + 10., point.Y() - TILE_SIZE / 2 + 10);
		point.Y() += bigFont.Height() + 20;
		nextY += bigFont.Height() + 20;
		
		bool isEmpty = true;
		for(const string &name : it->second)
		{
			if(!DrawItem(name, point, scrollY))
				continue;
			isEmpty = false;
			
			bool isSelected = (selectedShip && GameData::Ships().Get(name) == selectedShip)
				|| (selectedOutfit && GameData::Outfits().Get(name) == selectedOutfit);
			
			if(isSelected)
			{
				Color color(.2, 0.);
				int dy = DividerOffset();
				
				float before = point.X() - TILE_SIZE / 2 - Screen::Left();
				FillShader::Fill(Point(Screen::Left() + .5 * before, point.Y() + dy),
					Point(before, 1.), color);
				
				float after = endX - (point.X() + TILE_SIZE / 2);
				FillShader::Fill(Point(endX - .5 * after, point.Y() + dy),
					Point(after, 1.), color);
				
				// The center of the display needs to be between these two values:
				int panelAndAHalf = DetailWidth() / 2;
				double minX = Screen::Left() + panelAndAHalf;
				double maxX = Screen::Left() + mainWidth - panelAndAHalf;
				Point center(
					max(minX, min(maxX, point.X())),
					point.Y() + TILE_SIZE / 2);
				
				mainDetailHeight = DrawDetails(center);
				nextY += mainDetailHeight;
			}
			
			point.X() += columnWidth;
			if(point.X() >= endX)
			{
				point.X() = begin.X();
				point.Y() = nextY;
				nextY += TILE_SIZE;
				scrollY = -mainDetailHeight;
			}
		}
		
		if(!isEmpty)
		{
			bigFont.Draw(category, side, bright);
			
			if(point.X() != begin.X())
			{
				point.X() = begin.X();
				point.Y() = nextY;
				nextY += TILE_SIZE;
				scrollY = -mainDetailHeight;
			}
			point.Y() += 40;
			nextY += 40;
		}
		else
		{
			point.Y() -= bigFont.Height() + 20;
			nextY -= bigFont.Height() + 20;
		}
	}
	// This is how much Y space was actually used.
	nextY -= 40 + TILE_SIZE;
	
	// What amount would mainScroll have to equal to make nextY equal the
	// bottom of the screen?
	maxMainScroll = nextY + mainScroll - Screen::Height() / 2 - TILE_SIZE / 2;
	maxMainScroll = max(0, maxMainScroll);
	
	PointerShader::Draw(Point(Screen::Right() - 10 - SIDE_WIDTH, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(mainScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10 - SIDE_WIDTH, Screen::Bottom() - 10),
		Point(0., 1.), 10., 10., 5., Color(mainScroll < maxMainScroll ? .8 : .2, 0.));
}



void ShopPanel::DrawShip(const Ship &ship, const Point &center, bool isSelected) const
{
	const Sprite *sprite = ship.GetSprite().GetSprite();
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/shipyard selected" : "ui/shipyard unselected");
	SpriteShader::Draw(back, center);
	// Make sure the ship sprite leaves 10 pixels padding all around.
	float zoomSize = SHIP_SIZE - 60.f;
	
	// Draw the ship name.
	const string &name = ship.Name().empty() ? ship.ModelName() : ship.Name();
	const Font &font = FontSet::Get(14);
	Point offset(-.5f * font.Width(name), -.5f * SHIP_SIZE + 10.f);
	font.Draw(name, center + offset, *GameData::Colors().Get("bright"));
	
	float zoom = min(1.f, zoomSize / max(sprite->Width(), sprite->Height()));
	int swizzle = GameData::PlayerGovernment()->GetSwizzle();
	
	SpriteShader::Draw(sprite, center, zoom, swizzle);
}



void ShopPanel::FailSell() const
{
}



bool ShopPanel::CanSellMultiple() const
{
	return true;
}



// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if((key == 'l' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)))) && FlightCheck())
	{
		player.UpdateCargoCapacities();
		GetUI()->Pop(this);
	}
	else if(key == 'b')
	{
		if(!CanBuy())
			FailBuy();
		else
			Buy();
	}
	else if(key == 's')
	{
		if(!CanSell())
			FailSell();
		else
		{
			int modifier = CanSellMultiple() ? Modifier() : 1;
			for(int i = 0; i < modifier && CanSell(); ++i)
				Sell();
		}
	}
	else if(key == SDLK_LEFT)
	{
		if(dragMain)
			MainLeft();
		else
			SideSelect(-1);
		return true;
	}
	else if(key == SDLK_RIGHT)
	{
		if(dragMain)
			MainRight();
		else
			SideSelect(1);
		return true;
	}
	else if(key == SDLK_UP)
	{
		if(dragMain)
			MainUp();
		else
			SideSelect(-4);
		return true;
	}
	else if(key == SDLK_DOWN)
	{
		if(dragMain)
			MainDown();
		else
			SideSelect(4);
		return true;
	}
	else if(key == SDLK_PAGEUP)
		return DoScroll(Screen::Bottom());
	else if(key == SDLK_PAGEDOWN)
		return DoScroll(Screen::Top());
	else
		return false;
	
	return true;
}



bool ShopPanel::Click(int x, int y)
{
	dragShip = nullptr;
	// Handle clicks on the buttons.
	if(x >= Screen::Right() - SIDE_WIDTH && y >= Screen::Bottom() - 70)
	{
		x -= Screen::Right() - SIDE_WIDTH;
		if(x < 80)
			DoKey(SDLK_b);
		else if(x < 160)
			DoKey(SDLK_s);
		else
			DoKey(SDLK_l);
		
		return true;
	}
	
	// Check for clicks in the scroll arrows.
	if(x >= Screen::Right() - 20)
	{
		if(y < Screen::Top() + 20)
			return Scroll(0, 4);
		if(y < Screen::Bottom() - 70 && y >= Screen::Bottom() - 90)
			return Scroll(0, -4);
	}
	else if(x >= Screen::Right() - SIDE_WIDTH - 20 && x < Screen::Right() - SIDE_WIDTH)
	{
		if(y < Screen::Top() + 20)
			return Scroll(0, 4);
		if(y >= Screen::Bottom() - 20)
			return Scroll(0, -4);
	}
	
	// Handle clicks anywhere else by checking if they fell into any of the
	// active click zones (main panel or side panel).
	for(const ClickZone &zone : zones)
		if(zone.Contains(x, y))
		{
			if(zone.GetShip())
			{
				// Is the ship that was clicked one of the player's?
				for(const shared_ptr<Ship> &ship : player.Ships())
					if(ship.get() == zone.GetShip())
					{
						dragShip = ship.get();
						dragPoint.Set(x, y);
						SideSelect(dragShip);
						return true;
					}
				
				selectedShip = zone.GetShip();
			}
			else
				selectedOutfit = zone.GetOutfit();
			
			mainScroll = max(0, mainScroll + zone.ScrollY());
			return true;
		}
		
	return true;
}



bool ShopPanel::Hover(int x, int y)
{
	dragMain = (x < Screen::Right() - SIDE_WIDTH);
	return true;
}



bool ShopPanel::Drag(int dx, int dy)
{
	if(dragShip)
	{
		dragPoint += Point(dx, dy);
		for(const ClickZone &zone : zones)
			if(zone.Contains(dragPoint.X(), dragPoint.Y()))
				if(zone.GetShip() && zone.GetShip()->IsYours() && zone.GetShip() != dragShip)
				{
					int dragIndex = -1;
					int dropIndex = -1;
					for(unsigned i = 0; i < player.Ships().size(); ++i)
					{
						const Ship *ship = &*player.Ships()[i];
						if(ship == dragShip)
							dragIndex = i;
						if(ship == zone.GetShip())
							dropIndex = i;
					}
					if(dragIndex >= 0 && dropIndex >= 0)
						player.ReorderShip(dragIndex, dropIndex);
				}
	}
	else
		DoScroll(dy);
	
	return true;
}



bool ShopPanel::Release(int x, int y)
{
	dragShip = nullptr;
	return true;
}



bool ShopPanel::Scroll(int dx, int dy)
{
	return DoScroll(dy * 50);
}



ShopPanel::ClickZone::ClickZone(int x, int y, int rx, int ry, const Ship *ship, int scrollY)
	: left(x - rx), top(y - ry), right(x + rx), bottom(y + ry), scrollY(scrollY),
	ship(ship), outfit(nullptr)
{
}



ShopPanel::ClickZone::ClickZone(int x, int y, int rx, int ry, const Outfit *outfit, int scrollY)
	: left(x - rx), top(y - ry), right(x + rx), bottom(y + ry), scrollY(scrollY),
	ship(nullptr), outfit(outfit)
{
}



bool ShopPanel::ClickZone::Contains(int x, int y) const
{
	return (x >= left && x < right && y >= top && y < bottom);
}



const Ship *ShopPanel::ClickZone::GetShip() const
{
	return ship;
}



const Outfit *ShopPanel::ClickZone::GetOutfit() const
{
	return outfit;
}



int ShopPanel::ClickZone::CenterX() const
{
	return (left + right) / 2;
}



int ShopPanel::ClickZone::CenterY() const
{
	return (top + bottom) / 2;
}



int ShopPanel::ClickZone::ScrollY() const
{
	return scrollY;
}



bool ShopPanel::DoScroll(int dy)
{
	int &scroll = dragMain ? mainScroll : sideScroll;
	const int &maximum = dragMain ? maxMainScroll : maxSideScroll;
	
	scroll = max(0, min(maximum, scroll - dy));
	
	return true;
}



void ShopPanel::SideSelect(int count)
{
	// Find the currently selected ship in the list.
	vector<shared_ptr<Ship>>::const_iterator it = player.Ships().begin();
	for( ; it != player.Ships().end(); ++it)
		if((*it).get() == playerShip)
			break;
	
	// Bail out if there are no ships to choose from.
	if(it == player.Ships().end())
	{
		playerShips.clear();
		playerShip = player.Flagship();
		if(playerShip)
			playerShips.insert(playerShip);
		
		return;
	}
	
	
	if(count < 0)
	{
		while(count)
		{
			if(it == player.Ships().begin())
				it = player.Ships().end();
			--it;
			
			if((*it)->GetSystem() == player.GetSystem())
				++count;
		}
	}
	else
	{
		while(count)
		{
			++it;
			if(it == player.Ships().end())
				it = player.Ships().begin();
			
			if((*it)->GetSystem() == player.GetSystem())
				--count;
		}
	}
	SideSelect(&**it);
}



void ShopPanel::SideSelect(Ship *ship)
{
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	
	if(shift)
	{
		bool on = false;
		for(shared_ptr<Ship> other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(other->GetSystem() != player.GetSystem())
				continue;
			
			if(other.get() == ship || other.get() == playerShip)
				on = !on;
			else if(on)
				playerShips.insert(other.get());
		}
	}	
	else if(!control)
		playerShips.clear();
	
	playerShip = ship;
	playerShips.insert(playerShip);
}



void ShopPanel::MainLeft()
{
	vector<ClickZone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<ClickZone>::const_iterator it = Selected();
	// Special case: nothing is selected. Go to the last item.
	if(it == zones.end())
	{
		--it;
		mainScroll += it->CenterY() - start->CenterY();
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
		return;
	}
	
	if(it == start)
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
	}
	else
	{
		int previousY = it->CenterY();
		--it;
		mainScroll += it->CenterY() - previousY;
		if(mainScroll < 0)
			mainScroll = 0;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainRight()
{
	vector<ClickZone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<ClickZone>::const_iterator it = Selected();
	// Special case: nothing is selected. Select the first item.
	if(it == zones.end())
	{
		selectedShip = start->GetShip();
		selectedOutfit = start->GetOutfit();
		return;
	}
	
	int previousY = it->CenterY();
	++it;
	if(it == zones.end())
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
	}
	else
	{
		if(it->CenterY() != previousY)
			mainScroll += it->CenterY() - previousY - mainDetailHeight;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainUp()
{
	vector<ClickZone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<ClickZone>::const_iterator it = Selected();
	// Special case: nothing is selected. Go to the last item.
	if(it == zones.end())
	{
		--it;
		mainScroll += it->CenterY() - start->CenterY();
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
		return;
	}
	
	int previousX = it->CenterX();
	int previousY = it->CenterY();
	while(it != start && it->CenterY() == previousY)
		--it;
	while(it != start && it->CenterX() > previousX)
		--it;
	
	if(it == start && it->CenterY() == previousY)
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
	}
	else
	{
		mainScroll += it->CenterY() - previousY;
		if(mainScroll < 0)
			mainScroll = 0;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainDown()
{
	vector<ClickZone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<ClickZone>::const_iterator it = Selected();
	// Special case: nothing is selected. Select the first item.
	if(it == zones.end())
	{
		selectedShip = start->GetShip();
		selectedOutfit = start->GetOutfit();
		return;
	}
	
	int previousX = it->CenterX();
	int previousY = it->CenterY();
	while(it != zones.end() && it->CenterY() == previousY)
		++it;
	if(it == zones.end())
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
		return;
	}
	
	int newY = it->CenterY();
	while(it != zones.end() && it->CenterX() <= previousX && it->CenterY() == newY)
		++it;
	--it;
	
	mainScroll += it->CenterY() - previousY - mainDetailHeight;
	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



vector<ShopPanel::ClickZone>::const_iterator ShopPanel::Selected() const
{
	// Find the object that was clicked on.
	vector<ClickZone>::const_iterator it = MainStart();
	for( ; it != zones.end(); ++it)
		if(it->GetShip() == selectedShip && it->GetOutfit() == selectedOutfit)
			break;
	
	return it;
}



vector<ShopPanel::ClickZone>::const_iterator ShopPanel::MainStart() const
{
	// Find the first non-player-ship click zone.
	int margin = Screen::Right() - SHIP_SIZE;
	vector<ClickZone>::const_iterator start = zones.begin();
	while(start != zones.end() && start->CenterX() > margin)
		++start;
	
	return start;
}
