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
#include "Preferences.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <SDL2/SDL.h>

using namespace std;



ShopPanel::ShopPanel(PlayerInfo &player, const vector<string> &categories)
	: player(player), planet(player.GetPlanet()), playerShip(player.Flagship()), categories(categories)
{
	if(playerShip)
		playerShips.insert(playerShip);
	SetIsFullScreen(true);
	SetInterruptible(false);
	detailsInWithMain = Preferences::Has("Details with tiles in shop");
}



void ShopPanel::Step()
{
	// Perform autoscroll to bring item details into view.
	if(scrollDetailsIntoView && selectedBottomY > 0.)
	{
		double offY = Screen::Bottom() - selectedBottomY;
		if(offY < 0.)
			DoScroll(max(-30., offY));
		else
			scrollDetailsIntoView = false;
	}
}



void ShopPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Clear the list of clickable zones.
	zones.clear();
	categoryZones.clear();
	
	DrawSidebars();
	DrawMain();
	DrawButtons();
	
	shipInfo.DrawTooltips();
	outfitInfo.DrawTooltips();
	
	if(dragShip && dragShip->GetSprite())
	{
		static const Color selected(.8, 1.);
		const Sprite *sprite = dragShip->GetSprite();
		double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
		Point size(sprite->Width() * scale, sprite->Height() * scale);
		OutlineShader::Draw(sprite, dragPoint, size, selected);
	}
}



void ShopPanel::DrawSidebars() const
{
	const Font &font = FontSet::Get(14);
	Color bright = *GameData::Colors().Get("bright");
	sideDetailHeight = 0;
	
	// Fill in the background.
	FillShader::Fill(
		Point(Screen::Right() - SideWidth() / 2, 0.),
		Point(SideWidth(), Screen::Height()),
		COLOR_DETAILS_BG);
	// A bright line separating the side and main panels.
	FillShader::Fill(
		Point(Screen::Right() - SideWidth(), 0.),
		Point(1, Screen::Height()),
		COLOR_DIVIDERS);
	
	// Draw this string, centered in the side panel:
	static const string YOURS = "Your Fleet:";
	Point yoursPoint(
		Screen::Right() - SideWidth() / 2 - font.Width(YOURS) / 2,
		Screen::Top() + 10 - sideScroll);
	font.Draw(YOURS, yoursPoint, bright);
	
	// Start below the "Your Ships" label, and draw them.
	Point point(
		Screen::Right() - SideWidth() + ICON_TILE / 2,
		Screen::Top() - sideScroll + ICON_TILE / 2 + 40);	
	
	// Recalculate shipsHere
	shipsHere = 0;
	for(shared_ptr<Ship> ship : player.Ships())
		shipsHere += ShipIsHere(ship);
	
	if(true) // Always show the ship outlines, so that storage and cargo will be available.
	{
		static const Color selected(.8, 1.);
		static const Color unselected(.6, 1.);
		static const Color parked(.25, 1.);
		static const Color orange(1.0, 0.5, 0.3, 0.);
		static const Color red(1.0, 0., 0., 0.);

		bool anyFailingFlightCheck= false;
		bool anyMissingHyperdrive = false;
		
		// Draw tiles for all the player's ships.
		for(shared_ptr<Ship> ship : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(!ShipIsHere(ship))
				continue;
		
			if(point.X() > Screen::Right())
			{
				point.X() -= ICON_TILE * IconCols();
				point.Y() += ICON_TILE;
			}
			
			bool isSelected = playerShips.count(ship.get());
			const Sprite *background = SpriteSet::Get(isSelected ? "ui/icon selected" : "ui/icon unselected");
			SpriteShader::Draw(background, point);
			
			// Get the color of the ship's outline based on its status. 
			Color shipStatusColor;
			if (ship->IsParked())
				shipStatusColor = parked;
			else if (!ship->PassesFlightCheck())
			{
				shipStatusColor = red;
				anyFailingFlightCheck = true;
			}
			else if (!ship->HasHyperdrive() && !ship->CanBeCarried())
			{
				shipStatusColor = orange;
				anyMissingHyperdrive = true;
			}
			else if (isSelected)
				shipStatusColor = selected;
			else 
				shipStatusColor = unselected;
			
			const Sprite *sprite = ship->GetSprite();
			if(sprite)
			{
				// Draw the ship's outline
				double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
				Point size(sprite->Width() * scale, sprite->Height() * scale);
				OutlineShader::Draw(sprite, point, size, shipStatusColor);
			}
			
			// Mark the "primary selected ship" playerShip with some small framing lines.
			if (ship.get() == playerShip)
			{
				FillShader::Fill(point - Point(ICON_TILE / 2 - 8 , ICON_TILE / 2 - 3), 
					Point(10,1), bright);
				FillShader::Fill(point - Point(ICON_TILE / 2 - 3 , ICON_TILE / 2 - 8), 
					Point(1,10), bright);
				FillShader::Fill(point + Point(ICON_TILE / 2 - 8 , ICON_TILE / 2 - 3), 
					Point(10,1), bright);
				FillShader::Fill(point + Point(ICON_TILE / 2 - 3 , ICON_TILE / 2 - 8), 
					Point(1,10), bright);
				FillShader::Fill(point - Point(-(ICON_TILE / 2 - 8) , ICON_TILE / 2 - 3), 
					Point(10,1), bright);
				FillShader::Fill(point - Point(-(ICON_TILE / 2 - 3) , ICON_TILE / 2 - 8), 
					Point(1,10), bright);
				FillShader::Fill(point + Point(-(ICON_TILE / 2 - 8) , ICON_TILE / 2 - 3), 
					Point(10,1), bright);
				FillShader::Fill(point + Point(-(ICON_TILE / 2 - 3) , ICON_TILE / 2 - 8), 
					Point(1,10), bright);
			}
			// Make the click-zone: 
			zones.emplace_back(point, Point(ICON_TILE-2, ICON_TILE-2), ship.get());
		
			point.X() += ICON_TILE;
		}
		point.Y() += ICON_TILE/2;
		
		if (shipsHere > 1)
		{
			static const string str = "park/unpark ships with 'P'";
			font.Draw(str, Point(Screen::Right() - SideWidth()/2 - font.Width(str)/2, point.Y()), parked);
			point.Y() += 20;
		}
		
		{ // Always display this.
			static const string str = "to select cargo hold, use ctrl-click ";
			font.Draw(str, Point(Screen::Right() - SideWidth()/2 - font.Width(str)/2, point.Y()), parked);
			point.Y() += 20;
		}
		{
			static const string str = "to un-select all ships.";
			font.Draw(str, Point(Screen::Right() - SideWidth()/2 - font.Width(str)/2, point.Y()), parked);
			point.Y() += 20;			
		}

		// If any ships have a status problem draw a label explaining what their outline color means.
		if (anyFailingFlightCheck)
		{
			static const string str = "[ship(s) failing flight-check]";
			font.Draw(str, Point(Screen::Right() - SideWidth()/2 - font.Width(str)/2, point.Y()), red);
			point.Y() += 20;
		}
		if (anyMissingHyperdrive)
		{
			static const string str = "[ship(s) missing hyperdrive]";
			font.Draw(str, Point(Screen::Right() - SideWidth()/2 - font.Width(str)/2, point.Y()), orange);
			point.Y() += 20;
		}
		point.Y() += ICON_TILE/2;
	}
	

	// If player ship details are in their own panel, draw a background and scroll buttons for it.  
	if (PlayerShipWidth())
	{
		//Background
		FillShader::Fill( 
			Point(Screen::Right() - SideWidth() - DetailsWidth() / 2, 0.),
			Point(DetailsWidth() - 2, Screen::Height()),
			COLOR_DETAILS_BG);
		//Thin dividing line.
		FillShader::Fill(
			Point(Screen::Right() - SideWidth() - DetailsWidth(), 0.),
			Point(1, Screen::Height()),
			COLOR_DIVIDERS);
		// Top scroll arrow
		PointerShader::Draw(Point(Screen::Right() - SideWidth() - 10, Screen::Top() + 10),
			Point(0., -1.), 10., 10., 5., Color(playerShipScroll > 0 ? .8 : .2, 0.));
		// Bottom scroll arrow
		PointerShader::Draw(Point(Screen::Right() - SideWidth() - 10, Screen::Bottom() - 10),
			Point(0., 1.), 10., 10., 5., Color(playerShipScroll < maxPlayerShipScroll ? .8 : .2, 0.));
		
		// A label "Selected Ship: "
		string label = playerShip ? "Selected Ship:" : "Your Fleet's Cargo:";
		Point labelPoint(
			Screen::Right() - SideWidth() - PlayerShipWidth() / 2 -  font.Width(label) / 2,
			Screen::Top() + 10 - playerShipScroll);
		font.Draw(label, labelPoint, bright);
		
	}
	
	// Draw the player's flagship image.  
	// The details drawn below this image are up to the subclass.
	if (!PlayerShipWidth())
	{
		point.X() = Screen::Right() - SideWidth() / 2;
		if (playerShip)
		{
			point.Y() += SHIP_SIZE / 2;
			DrawShip(*playerShip, point, true);
			Point offset(SideWidth() / -2, SHIP_SIZE / 2);
			sideDetailHeight = DrawPlayerShipInfo(point + offset);	
		}
		else
			sideDetailHeight = DrawCargoHoldInfo(point + Point(SideWidth()/ -2, 0));
		point.Y() += sideDetailHeight + SHIP_SIZE / 2;
	}
	else 
	{
		Point playerShipPoint(Screen::Right() - SideWidth() - PlayerShipWidth() / 2, Screen::Top() + SHIP_SIZE / 2 + 40 - playerShipScroll);
		int playerShipDetailsHeight;
		if (playerShip) 
		{
			DrawShip(*playerShip, playerShipPoint, true);
			playerShipDetailsHeight = DrawPlayerShipInfo(playerShipPoint + Point(PlayerShipWidth() / -2, SHIP_SIZE / 2));			
		}
		else 
			playerShipDetailsHeight = DrawCargoHoldInfo(playerShipPoint + Point(PlayerShipWidth() / -2, SHIP_SIZE / -2));
		// Update max scroll for this panel.
		maxPlayerShipScroll = max(0, 40 + SHIP_SIZE + playerShipDetailsHeight - Screen::Height());
	}

	
	maxSideScroll = point.Y() + sideScroll - Screen::Bottom() + BUTTON_HEIGHT;
	maxSideScroll = max(0, maxSideScroll);
	
	// Draw the scroll buttons for the side panel.
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(sideScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Bottom() - 80),
		Point(0., 1.), 10., 10., 5., Color(sideScroll < maxSideScroll ? .8 : .2, 0.));

	// If the details panel is also on the side, draw the background shader and 
	// the scroll buttons for that here. 
	if (!detailsInWithMain)
	{
		//Background
		FillShader::Fill( 
			Point(Screen::Right() - SideWidth() - PlayerShipWidth() - DetailsWidth() / 2, 0.),
			Point(DetailsWidth() - 2, Screen::Height()),
			Color(.1, 1.));
		//Thin dividing line.
		FillShader::Fill(
			Point(Screen::Right() - SideWidth() - PlayerShipWidth() - DetailsWidth(), 0.),
			Point(1, Screen::Height()),
			Color(.2, 1.));
		// Top scroll arrow
		PointerShader::Draw(Point(Screen::Right() - SideWidth() - PlayerShipWidth() - 10, Screen::Top() + 10),
			Point(0., -1.), 10., 10., 5., Color(detailsScroll > 0 ? .8 : .2, 0.));
		// Bottom scroll arrow
		PointerShader::Draw(Point(Screen::Right() - SideWidth() - PlayerShipWidth() - 10, Screen::Bottom() - 10),
			Point(0., 1.), 10., 10., 5., Color(detailsScroll < maxDetailsScroll ? .8 : .2, 0.));
	}
}



void ShopPanel::DrawButtons() const
{
	// The last 70 pixels on the end of the side panel are for the buttons:
	Point buttonSize(SideWidth(), BUTTON_HEIGHT);
	FillShader::Fill(Screen::BottomRight() - .5 * buttonSize, buttonSize, Color(.2, 1.));
	FillShader::Fill(
		Point(Screen::Right() - SideWidth() / 2, Screen::Bottom() - (BUTTON_HEIGHT/2)),
		Point(SideWidth(), 70), COLOR_DIVIDERS);
	FillShader::Fill(
		Point(Screen::Right() - SideWidth() / 2, Screen::Bottom() - BUTTON_HEIGHT),
		Point(SideWidth(), 1), COLOR_BUTTONS_BG);
	
	const Font &font = FontSet::Get(14);
	Color bright = *GameData::Colors().Get("bright");
	Color dim = *GameData::Colors().Get("medium");
	
	Point point(
		Screen::Right() - SideWidth() + 10,
		Screen::Bottom() - 65);
	font.Draw("You have:", point, dim);
	
	string credits = Format::Number(player.Accounts().Credits()) + " credits";
	point.X() += (SideWidth() - 20) - font.Width(credits);
	font.Draw(credits, point, bright);
	
	const Font &bigFont = FontSet::Get(18);
	
	Point buyCenter = Screen::BottomRight() - Point(210, 25);
	FillShader::Fill(buyCenter, Point(60, 30), COLOR_DETAILS_BG);
	string BUY = (playerShip && selectedOutfit && player.Cargo().GetOutfitCount(selectedOutfit)) ? "_Install" : "_Buy";
	bigFont.Draw(BUY,
		buyCenter - .5 * Point(bigFont.Width(BUY), bigFont.Height()),
		CanBuy() ? bright : dim);
	
	Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), COLOR_DETAILS_BG);
	static const string SELL = "_Sell";
	bigFont.Draw(SELL,
		sellCenter - .5 * Point(bigFont.Width(SELL), bigFont.Height()),
		CanSell() ? bright : dim);
	
	Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), COLOR_DETAILS_BG);
	static const string LEAVE = "_Leave";
	bigFont.Draw(LEAVE,
		leaveCenter - .5 * Point(bigFont.Width(LEAVE), bigFont.Height()),
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
	Color dim = *GameData::Colors().Get("dim");
	Color bright = *GameData::Colors().Get("bright");
	mainDetailHeight = 0;
	int sideWidth = SideWidth() + DetailsWidth() + PlayerShipWidth();
	
	// Draw all the available ships.
	// First, figure out how many columns we can draw.
	const int TILE_SIZE = TileSize();
	int mainWidth = (Screen::Width() - sideWidth - 1);
	int columns = mainWidth / TILE_SIZE;
	int columnWidth = mainWidth / columns;
	
	Point begin(
		(Screen::Width() - columnWidth) / -2,
		(Screen::Height() - TILE_SIZE) / -2 - mainScroll);
	Point point = begin;
	float endX = Screen::Right() - (sideWidth + 1);
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
		
		bool isCollapsed = collapsed.count(category);
		bool isEmpty = true;
		for(const string &name : it->second)
		{
			if(!HasItem(name))
				continue;
			isEmpty = false;
			if(isCollapsed)
				break;
			
			int drawResult = DrawItem(name, point, scrollY);
			
			if(drawResult == SELECTED)
			{
				Color color(.2, 0.);
				int dy = DividerOffset();
				
				// Draw thin lines on the right and left sides, under the selected item. 
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
				selectedBottomY = nextY - TILE_SIZE / 2;
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
			Point size(bigFont.Width(category), bigFont.Height());
			categoryZones.emplace_back(Point(Screen::Left(), side.Y()) + .5 * size, size, category);
			bigFont.Draw(category, side, isCollapsed ? dim : bright);
			
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
	
	PointerShader::Draw(Point(Screen::Right() - 10 - sideWidth, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(mainScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10 - sideWidth, Screen::Bottom() - 10),
		Point(0., 1.), 10., 10., 5., Color(mainScroll < maxMainScroll ? .8 : .2, 0.));
}



void ShopPanel::DrawShip(const Ship &ship, const Point &center, bool isSelected) const
{
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
	
	const Sprite *sprite = ship.GetSprite();
	if(sprite)
	{
		float zoom = min(1.f, zoomSize / max(sprite->Width(), sprite->Height()));
		int swizzle = GameData::PlayerGovernment()->GetSwizzle();
		
		SpriteShader::Draw(sprite, center, zoom, swizzle);
	}
}



void ShopPanel::FailSell() const
{
}



bool ShopPanel::CanSellMultiple() const
{
	return true;
}



void ShopPanel::ToggleParked() const 
{
	bool park = false;
	// If at least one selected ship is not already parked, park all selected ships.
	// Otherwise unpark them all.
	for(auto it : playerShips)
		if (!it->IsParked())
		{
			park = true;
			break;
		}
	for(auto it : playerShips)
		if (it != player.Flagship())
			it->SetIsParked(park);
}



// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	scrollDetailsIntoView = false;
	if((key == 'l' || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)))) && FlightCheck())
	{
		player.UpdateCargoCapacities();
		GetUI()->Pop(this);
	}
	else if(key == 'b' || key == 'i')
	{
		if(!CanBuy())
			FailBuy();
		else
		{
			Buy();
			player.UpdateCargoCapacities();
		}
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
			player.UpdateCargoCapacities();
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
			SideSelect(-IconCols());
		return true;
	}
	else if(key == SDLK_DOWN)
	{
		if(dragMain)
			MainDown();
		else
			SideSelect(IconCols());
		return true;
	}
	else if(key == SDLK_PAGEUP)
		return DoScroll(Screen::Bottom());
	else if(key == SDLK_PAGEDOWN)
		return DoScroll(Screen::Top());
	else if(key == 'p')
		ToggleParked();
	else
		return false;
	
	return true;
}



bool ShopPanel::Click(int x, int y)
{
	dragShip = nullptr;
	// Handle clicks on the buttons.
	if(x >= Screen::Right() - DETAILS_WIDTH && y >= Screen::Bottom() - BUTTON_HEIGHT)
	{
		x -= Screen::Right() - DETAILS_WIDTH;
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
		if(y < Screen::Bottom() - BUTTON_HEIGHT && y >= Screen::Bottom() - BUTTON_HEIGHT - 20)
			return Scroll(0, -4);
	}
	else if((x >= Screen::Right() - SideWidth() - 20 && x < Screen::Right() - SideWidth()) 
		||  (x >= Screen::Right() - SideWidth() - DetailsWidth() - 20 && x < Screen::Right() - SideWidth() - DetailsWidth()) 
		||  (x >= Screen::Right() - SideWidth() - DetailsWidth() - PlayerShipWidth() - 20 && x < Screen::Right() - SideWidth() - DetailsWidth() - PlayerShipWidth()))
	{
		if(y < Screen::Top() + 20)
			return Scroll(0, 4);
		if(y >= Screen::Bottom() - 20)
			return Scroll(0, -4);
	}
	
	Point point(x, y);
	
	// Check for clicks in the category labels.
	for(const ClickZone<string> &zone : categoryZones)
		if(zone.Contains(point))
		{
			auto it = collapsed.find(zone.Value());
			if(it == collapsed.end())
			{
				collapsed.insert(zone.Value());
				if(selectedShip && selectedShip->Attributes().Category() == zone.Value())
					selectedShip = nullptr;
				if(selectedOutfit && selectedOutfit->Category() == zone.Value())
					selectedOutfit = nullptr;
			}
			else
				collapsed.erase(it);
			return true;
		}
	
	// Handle clicks anywhere else by checking if they fell into any of the
	// active click zones (main panel or side panel).
	for(const Zone &zone : zones)
		if(zone.Contains(point))
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
			
			scrollDetailsIntoView = true;
			// Reset selectedBottomY so that Step() waits for it to be updated
			// with the proper value computed in Draw().
			selectedBottomY = 0.;
			mainScroll = max(0., mainScroll + zone.ScrollY());
			return true;
		}
		
	return true;
}



bool ShopPanel::Hover(int x, int y)
{
	// Info panel hover functions.
	Point point(x, y);
	// Check that the point is not in the button area.
	if(x >= Screen::Right() - SideWidth() && y >= Screen::Bottom() - BUTTON_HEIGHT)
	{
		shipInfo.ClearHover();
		outfitInfo.ClearHover();
	}
	else
	{
		shipInfo.Hover(point);
		outfitInfo.Hover(point);
	}
	
	// Figure out which panel the point (x,y) is in.
	int cutoff = Screen::Right() - SideWidth() - PlayerShipWidth() - DetailsWidth();
	dragMain = (x < cutoff);
	
	cutoff += DetailsWidth();
	dragDetails = !dragMain && (x < cutoff);
	
	cutoff += PlayerShipWidth();
	dragPlayerShip = !dragMain && !dragDetails && (x < cutoff);
	
	return true;
}



bool ShopPanel::Drag(double dx, double dy)
{
	if(dragShip)
	{
		dragPoint += Point(dx, dy);
		for(const Zone &zone : zones)
			if(zone.Contains(dragPoint))
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
	{
		scrollDetailsIntoView = false;
		DoScroll(dy);
	}
	
	return true;
}



bool ShopPanel::Release(int x, int y)
{
	dragShip = nullptr;
	return true;
}



bool ShopPanel::Scroll(double dx, double dy)
{
	scrollDetailsIntoView = false;
	return DoScroll(dy * 50.);
}



ShopPanel::Zone::Zone(Point center, Point size, const Ship *ship, double scrollY)
	: ClickZone(center, size, ship), scrollY(scrollY)
{
}



ShopPanel::Zone::Zone(Point center, Point size, const Outfit *outfit, double scrollY)
	: ClickZone(center, size, nullptr), scrollY(scrollY), outfit(outfit)
{
}



const Ship *ShopPanel::Zone::GetShip() const
{
	return Value();
}



const Outfit *ShopPanel::Zone::GetOutfit() const
{
	return outfit;
}



double ShopPanel::Zone::ScrollY() const
{
	return scrollY;
}



bool ShopPanel::DoScroll(double dy)
{
	double &scroll = dragMain ? mainScroll : dragDetails ? detailsScroll : dragPlayerShip ? playerShipScroll : sideScroll;
	const int &maximum = dragMain ? maxMainScroll : dragDetails ? maxDetailsScroll : dragPlayerShip ? maxPlayerShipScroll : maxSideScroll;
	
	scroll = max(0., min(static_cast<double>(maximum), scroll - dy));
	
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
				break; // Don't loop around.
			--it;
			
			if(ShipIsHere(*it))
				++count;
		}
	}
	else
	{
		while(count)
		{
			++it;
			if(it == player.Ships().end())
			{
				--it;
				break; // Don't loop around.
			}
			
			if(ShipIsHere(*it))
				--count;
		}
	}
	SideSelect(&**it);
}



void ShopPanel::SideSelect(Ship *ship)
{
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	
	// Set selected ship
	playerShip = ship;
	// Make sure selected ship is in view.
	int selectedIndex = 0;
	for(shared_ptr<Ship> ship : player.Ships())
	{
		if(ship.get() == playerShip)
			break;
		if (ShipIsHere(ship))
			++selectedIndex;
	}
	int selectedCol = selectedIndex / IconCols();
	sideScroll = min((int)sideScroll, 40 + ICON_TILE*selectedCol);
	sideScroll = max((int)sideScroll, 40 + 70 + ICON_TILE*(selectedCol+1) - Screen::Height());	
	
	if (!control)
		playerShips.clear();
	
	if(shift && playerShip != shiftSelectAnchorShip)
	{
		// Select playerShip, shiftSelectAnchorShip, and everything between them. 
		bool on = false;
		for(shared_ptr<Ship> other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(!ShipIsHere(other))
				continue;
			bool isEndpoint = other.get() == playerShip || other.get() == shiftSelectAnchorShip;
			if(isEndpoint)
				on = !on;
			if(on || isEndpoint)
				playerShips.insert(other.get());
		}
	}
	
	if(!shift)
		shiftSelectAnchorShip = playerShip;
	
	if (!shift && control && playerShips.find(playerShip) != playerShips.end())
	{
		playerShips.erase(playerShip);
		if (playerShips.empty())
			playerShip = nullptr; // Clearing playerShip allows buying / selling from cargo directly. (TODO: would be better if cargo were a selectable tile.)
	}
	else
		playerShips.insert(playerShip);
}



void ShopPanel::MainLeft()
{
	vector<Zone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Go to the last item.
	if(it == zones.end())
	{
		--it;
		mainScroll += it->Center().Y() - start->Center().Y();
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
		int previousY = it->Center().Y();
		--it;
		mainScroll += it->Center().Y() - previousY;
		if(mainScroll < 0)
			mainScroll = 0;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainRight()
{
	vector<Zone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Select the first item.
	if(it == zones.end())
	{
		selectedShip = start->GetShip();
		selectedOutfit = start->GetOutfit();
		return;
	}
	
	int previousY = it->Center().Y();
	++it;
	if(it == zones.end())
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
	}
	else
	{
		if(it->Center().Y() != previousY)
			mainScroll += it->Center().Y() - previousY - mainDetailHeight;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainUp()
{
	vector<Zone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Go to the last item.
	if(it == zones.end())
	{
		--it;
		mainScroll += it->Center().Y() - start->Center().Y();
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
		return;
	}
	
	int previousX = it->Center().X();
	int previousY = it->Center().Y();
	while(it != start && it->Center().Y() == previousY)
		--it;
	while(it != start && it->Center().X() > previousX)
		--it;
	
	if(it == start && it->Center().Y() == previousY)
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
	}
	else
	{
		mainScroll += it->Center().Y() - previousY;
		if(mainScroll < 0)
			mainScroll = 0;
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
	}
}



void ShopPanel::MainDown()
{
	vector<Zone>::const_iterator start = MainStart();
	if(start == zones.end())
		return;
	
	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Select the first item.
	if(it == zones.end())
	{
		selectedShip = start->GetShip();
		selectedOutfit = start->GetOutfit();
		return;
	}
	
	int previousX = it->Center().X();
	int previousY = it->Center().Y();
	while(it != zones.end() && it->Center().Y() == previousY)
		++it;
	if(it == zones.end())
	{
		mainScroll = 0;
		selectedShip = nullptr;
		selectedOutfit = nullptr;
		return;
	}
	
	int newY = it->Center().Y();
	while(it != zones.end() && it->Center().X() <= previousX && it->Center().Y() == newY)
		++it;
	--it;
	
	mainScroll += it->Center().Y() - previousY - mainDetailHeight;
	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



vector<ShopPanel::Zone>::const_iterator ShopPanel::Selected() const
{
	// Find the object that was clicked on.
	vector<Zone>::const_iterator it = MainStart();
	for( ; it != zones.end(); ++it)
		if(it->GetShip() == selectedShip && it->GetOutfit() == selectedOutfit)
			break;
	
	return it;
}



vector<ShopPanel::Zone>::const_iterator ShopPanel::MainStart() const
{
	// Find the first non-player-ship click zone.
	int margin = Screen::Right() - SideWidth();
	vector<Zone>::const_iterator start = zones.begin();
	while(start != zones.end() && start->Center().X() > margin)
		++start;
	
	return start;
}



bool ShopPanel::ShipIsHere(shared_ptr<Ship> ship) const 
{
	return !(ship->GetSystem() != player.GetSystem() || ship->IsDisabled());
} 



// Zero if configured not to put details on the side.
int ShopPanel::DetailsWidth() const
{
	return detailsInWithMain ? 0 : DETAILS_WIDTH;
}



// Zero if the player does not have a large fleet or the screen is to small. 
int ShopPanel::PlayerShipWidth() const
{
	if(shipsHere > 12 && (Screen::Width() > DETAILS_WIDTH * 5))
		return DETAILS_WIDTH;
	return 0;
}



int ShopPanel::SideWidth() const
{
	// If screen/fleet size favor having a separate panel for details on the players ship, then allow the side "fleet"
	// panel to grow up to 20% of the screen. 
	int defaultWidth = DETAILS_WIDTH;
	if (PlayerShipWidth() == 0)
		return defaultWidth;
	return max(defaultWidth, 2 + ICON_TILE * (Screen::Width() / (ICON_TILE * 5)));
}



int ShopPanel::IconCols() const
{
	return SideWidth() / ICON_TILE;
}
