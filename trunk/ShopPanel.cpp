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

#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "ShipInfoDisplay.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

using namespace std;



ShopPanel::ShopPanel(PlayerInfo &player, const vector<string> &categories)
	: player(player), planet(player.GetPlanet()),
	playerShip(player.GetShip()), selectedShip(nullptr), selectedOutfit(nullptr),
	mainScroll(0), sideScroll(0), dragMain(true), 
	mainDetailHeight(0), sideDetailHeight(0), categories(categories)
{
	SetIsFullScreen(true);
}



void ShopPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Clear the list of clickable zones.
	zones.clear();
	
	DrawSidebar();
	DrawButtons();
	DrawMain();
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
		Screen::Right() - SIDE_WIDTH / 2,
		Screen::Top() + SIDE_WIDTH / 2 - sideScroll + 40);
	for(shared_ptr<Ship> ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(ship->GetSystem() != player.GetSystem())
			continue;
		
		bool isSelected = (ship.get() == playerShip);
		DrawShip(*ship, point, isSelected);
		zones.emplace_back(point.X(), point.Y(), SHIP_SIZE / 2, SHIP_SIZE / 2, ship.get());
		
		if(isSelected)
		{
			Point offset(SIDE_WIDTH / -2, SHIP_SIZE / 2);
			sideDetailHeight = DrawPlayerShipInfo(point + offset);
			point.Y() += sideDetailHeight;
		}
		point.Y() += SHIP_SIZE;
	}
	maxSideScroll = point.Y() + sideScroll - Screen::Bottom() + 70 - SHIP_SIZE / 2;
	maxSideScroll = max(0, maxSideScroll);
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
	string buy = (selectedOutfit && player.Cargo().Get(selectedOutfit)) ? "Install" : "Buy";
	bigFont.Draw(buy,
		buyCenter - .5 * Point(bigFont.Width(buy), bigFont.Height()),
		CanBuy() ? bright : dim);
	
	Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), Color(.1, 1.));
	bigFont.Draw("Sell",
		sellCenter - .5 * Point(bigFont.Width("Sell"), bigFont.Height()),
		CanSell() ? bright : dim);
	
	Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), Color(.1, 1.));
	bigFont.Draw("Leave",
		leaveCenter - .5 * Point(bigFont.Width("Leave"), bigFont.Height()),
		bright);
	
	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		font.Draw(mod, buyCenter + Point(-.5 * modWidth, 10.), dim);
		font.Draw(mod, sellCenter + Point(-.5 * modWidth, 10.), dim);	
	}
}



void ShopPanel::DrawMain() const
{
	const Font &bigFont = FontSet::Get(18);
	Color bright = *GameData::Colors().Get("bright");
	mainDetailHeight = 0;
	
	// Draw all the available ships.
	// First, figure out how many colums we can draw.
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
			if(!DrawItem(name, point))
				continue;
			isEmpty = false;
			
			bool isSelected = (selectedShip && GameData::Ships().Get(name) == selectedShip)
				|| (selectedOutfit && GameData::Outfits().Get(name) == selectedOutfit);
			
			if(isSelected)
			{
				Color color = *GameData::Colors().Get("dim");
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
	
	float zoom = min(.5f, zoomSize / max(sprite->Width(), sprite->Height()));
	int swizzle = player.GetSwizzle();
	
	SpriteShader::Draw(sprite, center, zoom, swizzle);
}


// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == 'l' && FlightCheck())
	{
		player.UpdateCargoCapacities();
		GetUI()->Pop(this);
	}
	else if(key == 'b')
	{
		int modifier = Modifier();
		for(int i = 0; i < modifier && CanBuy(); ++i)
			Buy();
	}
	else if(key == 's')
	{
		int modifier = Modifier();
		for(int i = 0; i < modifier && CanSell(); ++i)
			Sell();
	}
	else if(key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN)
	{
		// Handle arrow keys to move the selection.
		if(!dragMain)
			SideSelect(key == SDLK_RIGHT || key == SDLK_DOWN);
		else if(key == SDLK_LEFT)
			MainLeft();
		else if(key == SDLK_RIGHT)
			MainRight();
		else if(key == SDLK_UP)
			MainUp();
		else if(key == SDLK_DOWN)
			MainDown();
		
		return true;
	}
	else if(key == SDLK_PAGEUP)
		return Drag(0, Screen::Bottom());
	else if(key == SDLK_PAGEDOWN)
		return Drag(0, Screen::Top());
	else
		return false;
	
	return true;
}



bool ShopPanel::Click(int x, int y)
{
	// Handle clicks on the buttons.
	if(x >= Screen::Width() / 2 - SIDE_WIDTH && y >= Screen::Height() / 2 - 70)
	{
		x -= Screen::Width() / 2 - SIDE_WIDTH;
		if(x < 80)
			KeyDown(SDLK_b, KMOD_NONE);
		else if(x < 160)
			KeyDown(SDLK_s, KMOD_NONE);
		else
			KeyDown(SDLK_l, KMOD_NONE);
		
		return true;
	}
	
	// Handle clicks anywhere else by checking if they fell into any of the
	// active click zones (main panel or side panel).
	dragMain = (x < Screen::Right() - SHIP_SIZE);
	for(const ClickZone &zone : zones)
		if(zone.Contains(x, y))
		{
			if(zone.GetShip())
			{
				// Is the ship that was clicked one of the player's?
				for(const shared_ptr<Ship> &ship : player.Ships())
					if(ship.get() == zone.GetShip())
					{
						playerShip = ship.get();
						return true;
					}
				
				selectedShip = zone.GetShip();
				return true;
			}
			else
			{
				selectedOutfit = zone.GetOutfit();
				return true;
			}
		}
		
	return true;
}



bool ShopPanel::Drag(int dx, int dy)
{
	int &scroll = dragMain ? mainScroll : sideScroll;
	const int &maximum = dragMain ? maxMainScroll : maxSideScroll;
	
	scroll = max(0, min(maximum, scroll - dy));
	return true;
}



bool ShopPanel::Scroll(int dx, int dy)
{
	return Drag(dx, dy * 50);
}



ShopPanel::ClickZone::ClickZone(int x, int y, int rx, int ry, const Ship *ship)
	: left(x - rx), top(y - ry), right(x + rx), bottom(y + ry), ship(ship), outfit(nullptr)
{
}



ShopPanel::ClickZone::ClickZone(int x, int y, int rx, int ry, const Outfit *outfit)
	: left(x - rx), top(y - ry), right(x + rx), bottom(y + ry), ship(nullptr), outfit(outfit)
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



void ShopPanel::SideSelect(bool next)
{
	vector<shared_ptr<Ship>>::const_iterator it = player.Ships().begin();
	for( ; it != player.Ships().end(); ++it)
	{
		// Skip any ships that are "absent" for whatever reason.
		if((*it)->GetSystem() != player.GetSystem())
			continue;
		
		if((*it).get() == playerShip)
			break;
	}
	// Bail out if there are no ships to choose from.
	if(it == player.Ships().end())
		return;
	
	int previousY = 0;
	for(vector<ClickZone>::const_iterator it = zones.begin(); it != zones.end(); ++it)
		if(it->GetShip() == playerShip)
		{
			previousY = it->CenterY();
			break;
		}
	
	if(!next)
	{
		while(true)
		{
			if(it == player.Ships().begin())
				it = player.Ships().end();
			--it;
			
			if((*it)->GetSystem() == player.GetSystem())
				break;
		}
	}
	else
	{
		while(true)
		{
			++it;
			if(it == player.Ships().end())
				it = player.Ships().begin();
			
			if((*it)->GetSystem() == player.GetSystem())
				break;
		}
	}
	playerShip = &**it;
	
	// Scroll to this ship.
	int newY = 0;
	for(vector<ClickZone>::const_iterator it = zones.begin(); it != zones.end(); ++it)
		if(it->GetShip() == playerShip)
		{
			newY = it->CenterY();
			break;
		}
	int delta = newY - previousY;
	if(delta > 0)
		delta -= sideDetailHeight;
	sideScroll += delta;
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
