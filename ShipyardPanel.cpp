/* ShipyardPanel.cpp
Michael Zahniser, 24 Jan 2014

Function definitions for the ShipyardPanel class.
*/

#include "ShipyardPanel.h"

#include "GameData.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <algorithm>

using namespace std;

namespace {
	static const int SIDE_WIDTH = 250;
	static const int TILE_SIZE = 250;
	
	// Draw the given ship at the given location, zoomed so it will fit within
	// one cell of the grid.
	void DrawShip(const Ship &ship, const Point &center, bool isSelected)
	{
		const Sprite *sprite = ship.GetSprite().GetSprite();
		const Sprite *back = SpriteSet::Get(
			isSelected ? "ui/shipyard selected" : "ui/shipyard unselected");
		SpriteShader::Draw(back, center);
		// Make sure the ship sprite leaves 10 pixels padding all around.
		float zoomSize = TILE_SIZE - 20.f;
		
		// Draw the ship name.
		const string &name = ship.Name().empty() ? ship.ModelName() : ship.Name();
		const Font &font = FontSet::Get(14);
		Point offset(-.5f * font.Width(name), -.5f * TILE_SIZE + 10.f);
		font.Draw(name, center + offset, Color(.8, 0.).Get());
		
		float zoom = min(.5f, zoomSize / max(sprite->Width(), sprite->Height()));
		SpriteShader::Draw(sprite, center, zoom);
	}
}



ShipyardPanel::ShipyardPanel(const GameData &data, PlayerInfo &player)
	: Panel(true), data(data), player(player),
	playerShip(player.GetShip()), selectedShip(nullptr),
	mainScroll(0), sideScroll(0)
{
	if(playerShip)
		playerShipInfo.Update(*playerShip);
}



void ShipyardPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Font &font = FontSet::Get(14);
	Color bright(.8, 0.);
	Color dim(.5, 0.);
	
	// First, draw the side panel.
	FillShader::Fill(
		Point((Screen::Width() - SIDE_WIDTH) * .5, 0.),
		Point(SIDE_WIDTH, Screen::Height()),
		Color(.1, 1.).Get());
	FillShader::Fill(
		Point(Screen::Width() * .5 - SIDE_WIDTH, 0.),
		Point(1, Screen::Height()),
		Color(.2, 1.).Get());
	
	static const string YOURS = "Your Ships:";
	Point yoursPoint(
		(Screen::Width() - SIDE_WIDTH - font.Width(YOURS)) / 2,
		Screen::Height() / -2 + 10 - sideScroll);
	font.Draw(YOURS, yoursPoint, bright.Get());
	
	Point point(
		(Screen::Width() - SIDE_WIDTH) / 2,
		(Screen::Height() - SIDE_WIDTH) / -2 - sideScroll + 40);
	for(shared_ptr<Ship> ship : player.Ships())
	{
		bool isSelected = (ship.get() == playerShip);
		DrawShip(*ship, point, isSelected);
		
		if(isSelected)
		{
			Point offset(SIDE_WIDTH / -2, TILE_SIZE / 2);
			
			playerShipInfo.DrawSale(point + offset);
			point.Y() += playerShipInfo.SaleHeight();
		}
		point.Y() += TILE_SIZE;
	}
	maxSideScroll = point.Y() + sideScroll - (Screen::Height()) / 2 + 70 - TILE_SIZE / 2;
	maxSideScroll = max(0, maxSideScroll);
	
	// The last 70 pixels on the end of the side panel are for the buttons:
	FillShader::Fill(
		Point((Screen::Width() - SIDE_WIDTH) / 2, Screen::Height() / 2 - 35),
		Point(SIDE_WIDTH, 70), Color(.2, 1.).Get());
	FillShader::Fill(
		Point((Screen::Width() - SIDE_WIDTH) / 2, Screen::Height() / 2 - 70),
		Point(SIDE_WIDTH, 1), Color(.3, 1.).Get());
	point.Set(
		Screen::Width() / 2 - SIDE_WIDTH + 10,
		Screen::Height() / 2 - 65);
	font.Draw("You have:", point, dim.Get());
	string credits = to_string(player.Accounts().Credits()) + " credits";
	point.X() += (SIDE_WIDTH - 20) - font.Width(credits);
	font.Draw(credits, point, bright.Get());
	
	const Font &bigFont = FontSet::Get(18);
	
	Point buyCenter(Screen::Width() / 2 - 210, Screen::Height() / 2 - 25);
	FillShader::Fill(buyCenter, Point(60, 30), Color(.1, 1.).Get());
	bigFont.Draw("Buy",
		buyCenter - .5 * Point(bigFont.Width("Buy"), bigFont.Height()),
		selectedShip ? bright.Get() : dim.Get());
	
	Point sellCenter(Screen::Width() / 2 - 130, Screen::Height() / 2 - 25);
	FillShader::Fill(sellCenter, Point(60, 30), Color(.1, 1.).Get());
	bigFont.Draw("Sell",
		sellCenter - .5 * Point(bigFont.Width("Sell"), bigFont.Height()),
		playerShip ? bright.Get() : dim.Get());
	
	Point leaveCenter(Screen::Width() / 2 - 45, Screen::Height() / 2 - 25);
	FillShader::Fill(leaveCenter, Point(70, 30), Color(.1, 1.).Get());
	bigFont.Draw("Leave",
		leaveCenter - .5 * Point(bigFont.Width("Leave"), bigFont.Height()),
		bright.Get());
	
	// Draw all the available ships.
	// First, figure out how many colums we can draw.
	int mainWidth = (Screen::Width() - SIDE_WIDTH - 1);
	int columns = mainWidth / TILE_SIZE;
	int columnWidth = mainWidth / columns;
	
	zones.clear();
	Point begin(
		(Screen::Width() - columnWidth) / -2,
		(Screen::Height() - TILE_SIZE) / -2 - mainScroll);
	point = begin;
	float endX = Screen::Width() * .5f - (SIDE_WIDTH + 1);
	double nextY = begin.Y() + TILE_SIZE;
	for(const auto &it : data.Ships())
	{
		bool isSelected = (&it.second == selectedShip);
		DrawShip(it.second, point, isSelected);
		zones.emplace_back(point.X(), point.Y(), columnWidth / 2, TILE_SIZE / 2,
			&it.second);
		
		if(isSelected)
		{
			Color color(.2, 1.);
			
			float before = point.X() - TILE_SIZE / 2 - Screen::Width() * -.5;
			FillShader::Fill(Point(Screen::Width() * -.5 + .5 * before, point.Y() + 121.),
				Point(before, 1.), color.Get());
			
			float after = endX - (point.X() + TILE_SIZE / 2);
			FillShader::Fill(Point(endX - .5 * after, point.Y() + 121.),
				Point(after, 1.), color.Get());
			
			// The center of the display needs to be between these two values:
			int panelAndAHalf = (selectedShipInfo.PanelWidth() * 3) / 2;
			double minX = Screen::Width() / -2 + panelAndAHalf;
			double maxX = Screen::Width() / -2 + mainWidth - panelAndAHalf;
			Point center(
				max(minX, min(maxX, point.X())) - selectedShipInfo.PanelWidth() / 2,
				point.Y() + TILE_SIZE / 2);
			Point offset(selectedShipInfo.PanelWidth(), 0.);
			
			selectedShipInfo.DrawDescription(center - offset);
			selectedShipInfo.DrawAttributes(center);
			selectedShipInfo.DrawOutfits(center + offset);
			
			nextY += selectedShipInfo.MaximumHeight();
		}
		
		point.X() += columnWidth;
		if(point.X() >= endX)
		{
			point.X() = begin.X();
			point.Y() = nextY;
			nextY += TILE_SIZE;
		}
	}
	
	// What amount would mainScroll have to equal to make nextY equal the
	// bottom of the screen?
	maxMainScroll = nextY + mainScroll - Screen::Height() / 2 - TILE_SIZE / 2;
	maxMainScroll = max(0, maxMainScroll);
}



// Only override the ones you need; the default action is to return false.
bool ShipyardPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == SDLK_ESCAPE)
		Pop(this);
	
	return true;
}



bool ShipyardPanel::Click(int x, int y)
{
	if(x >= Screen::Width() / 2 - SIDE_WIDTH && y >= Screen::Height() / 2 - 70)
	{
		x -= Screen::Width() / 2 - SIDE_WIDTH;
		if(x < 80)
		{
			// TODO: buy ship.
		}
		else if(x < 160)
		{
			// TODO: sell ship;
		}
		else
			Pop(this);
		
		return true;
	}
	
	dragMain = (x < Screen::Width() / 2 - SIDE_WIDTH);
	for(const ClickZone &zone : zones)
		if(zone.Contains(x, y))
		{
			selectedShip = zone.GetShip();
			selectedShipInfo.Update(*selectedShip);
		}
		
	return true;
}



bool ShipyardPanel::Drag(int dx, int dy)
{
	int &scroll = dragMain ? mainScroll : sideScroll;
	const int &maximum = dragMain ? maxMainScroll : maxSideScroll;
	scroll -= dy;
	if(scroll < 0)
		scroll = 0;
	else if(scroll > maximum)
		scroll = maximum;
	
	return true;
}



ShipyardPanel::ClickZone::ClickZone(int x, int y, int rx, int ry, const Ship *ship)
	: left(x - rx), top(y - ry), right(x + rx), bottom(y + ry), ship(ship)
{
}



bool ShipyardPanel::ClickZone::Contains(int x, int y) const
{
	return (x >= left && x < right && y >= top && y < bottom);
}



const Ship *ShipyardPanel::ClickZone::GetShip() const
{
	return ship;
}
