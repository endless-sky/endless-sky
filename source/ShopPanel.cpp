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
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "Sale.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"
#include "WrappedText.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

using namespace std;

namespace {
	const int ICON_TILE = 62;
	const int ICON_COLS = 4;
	const double ICON_SIZE = ICON_TILE - 8;
}



ShopPanel::ShopPanel(PlayerInfo &player, bool isOutfitter)
	: player(player), day(player.GetDate().DaysSinceEpoch()),
	planet(player.GetPlanet()), playerShip(player.Flagship()),
	categories(GameData::Categories(isOutfitter ? "outfit" : "ship")),
	collapsed(player.Collapsed(isOutfitter ? "outfitter" : "shipyard"))
{
	if(playerShip)
		playerShips.insert(playerShip);
	SetIsFullScreen(true);
	SetInterruptible(false);
}



void ShopPanel::Step()
{
	// If the player has acquired a second ship for the first time, explain to
	// them how to reorder the ships in their fleet.
	if(player.Ships().size() > 1)
		DoHelp("multiple ships");
	// Perform autoscroll to bring item details into view.
	if(scrollDetailsIntoView && mainDetailHeight > 0)
	{
		int mainTopY = Screen::Top();
		int mainBottomY = Screen::Bottom() - 40;
		double selectedBottomY = selectedTopY + TileSize() + mainDetailHeight;
		// Scroll up until the bottoms match.
		if(selectedBottomY > mainBottomY)
			DoScroll(max(-30., mainBottomY - selectedBottomY));
		// Scroll down until the bottoms or the tops match.
		else if(selectedBottomY < mainBottomY && (mainBottomY - mainTopY < selectedBottomY - selectedTopY && selectedTopY < mainTopY))
			DoScroll(min(30., min(mainTopY - selectedTopY, mainBottomY - selectedBottomY)));
		// Details are in view.
		else
			scrollDetailsIntoView = false;
	}
}



void ShopPanel::Draw()
{
	const double oldSelectedTopY = selectedTopY;
	
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Clear the list of clickable zones.
	zones.clear();
	categoryZones.clear();
	
	DrawSidebar();
	DrawButtons();
	DrawMain();
	DrawKey();
	
	shipInfo.DrawTooltips();
	outfitInfo.DrawTooltips();
	
	if(!warningType.empty())
	{
		static const int WIDTH = 250;
		static const int PAD = 10;
		const string &text = GameData::Tooltip(warningType);
		WrappedText wrap(FontSet::Get(14));
		wrap.SetWrapWidth(WIDTH - 2 * PAD);
		wrap.Wrap(text);
		
		bool isError = (warningType.back() == '!');
		const Color &textColor = *GameData::Colors().Get("medium");
		const Color &backColor = *GameData::Colors().Get(isError ? "error back" : "warning back");
		
		Point size(WIDTH, wrap.Height() + 2 * PAD);
		Point anchor = Point(warningPoint.X(), min<double>(warningPoint.Y() + size.Y(), Screen::Bottom()));
		FillShader::Fill(anchor - .5 * size, size, backColor);
		wrap.Draw(anchor - size + Point(PAD, PAD), textColor);
	}
	
	if(dragShip && dragShip->GetSprite())
	{
		static const Color selected(.8, 1.);
		const Sprite *sprite = dragShip->GetSprite();
		double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
		Point size(sprite->Width() * scale, sprite->Height() * scale);
		OutlineShader::Draw(sprite, dragPoint, size, selected);
	}

	if(sameSelectedTopY)
	{
		sameSelectedTopY = false;
		if(selectedTopY != oldSelectedTopY)
		{
			// Redraw with the same selected top (item in the same place).
			mainScroll = max(0., min(maxMainScroll, mainScroll + selectedTopY - oldSelectedTopY));
			Draw();
		}
	}
	mainScroll = min(mainScroll, maxMainScroll);
}



void ShopPanel::DrawSidebar()
{
	const Font &font = FontSet::Get(14);
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	sideDetailHeight = 0;
	
	// Fill in the background.
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH / 2, 0.),
		Point(SIDE_WIDTH, Screen::Height()),
		*GameData::Colors().Get("panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH, 0.),
		Point(1, Screen::Height()),
		*GameData::Colors().Get("shop side panel background"));
	
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
	for(const shared_ptr<Ship> &ship : player.Ships())
		shipsHere += !(ship->GetSystem() != player.GetSystem() || ship->IsDisabled());
	if(shipsHere < 4)
		point.X() += .5 * ICON_TILE * (4 - shipsHere);
	
	// Check whether flight check tooltips should be shown.
	Point mouse = GetUI()->GetMouse();
	warningType.clear();
	
	static const Color selected(.8, 1.);
	static const Color unselected(.4, 1.);
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;
	
		if(point.X() > Screen::Right())
		{
			point.X() -= ICON_TILE * ICON_COLS;
			point.Y() += ICON_TILE;
		}
		
		bool isSelected = playerShips.count(ship.get());
		const Sprite *background = SpriteSet::Get(isSelected ? "ui/icon selected" : "ui/icon unselected");
		SpriteShader::Draw(background, point);
		// If this is one of the selected ships, check if the currently hovered
		// button (if any) applies to it. If so, brighten the background.
		if(isSelected && ShouldHighlight(ship.get()))
			SpriteShader::Draw(background, point);
		
		const Sprite *sprite = ship->GetSprite();
		if(sprite)
		{
			double scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
			Point size(sprite->Width() * scale, sprite->Height() * scale);
			OutlineShader::Draw(sprite, point, size, isSelected ? selected : unselected);
		}
		
		zones.emplace_back(point, Point(ICON_TILE, ICON_TILE), ship.get());
		
		string check = ship->FlightCheck();
		if(!check.empty())
		{
			const Sprite *icon = SpriteSet::Get(check.back() == '!' ? "ui/error" : "ui/warning");
			SpriteShader::Draw(icon, point + .5 * Point(ICON_TILE - icon->Width(), ICON_TILE - icon->Height()));
			if(zones.back().Contains(mouse))
			{
				warningType = check;
				warningPoint = zones.back().TopLeft();
			}
		}
		
		point.X() += ICON_TILE;
	}
	point.Y() += ICON_TILE;
	
	if(playerShip)
	{
		point.Y() += SHIP_SIZE / 2;
		point.X() = Screen::Right() - SIDE_WIDTH / 2;
		DrawShip(*playerShip, point, true);
		
		Point offset(SIDE_WIDTH / -2, SHIP_SIZE / 2);
		sideDetailHeight = DrawPlayerShipInfo(point + offset);
		point.Y() += sideDetailHeight + SHIP_SIZE / 2;
	}
	else if(player.Cargo().Size())
	{
		point.X() = Screen::Right() - SIDE_WIDTH + 10;
		font.Draw("cargo space:", point, medium);
		
		string space = Format::Number(player.Cargo().Free()) + " / " + Format::Number(player.Cargo().Size());
		Point right(Screen::Right() - font.Width(space) - 10, point.Y());
		font.Draw(space, right, bright);
		point.Y() += 20.;
	}
	maxSideScroll = max(0., point.Y() + sideScroll - Screen::Bottom() + BUTTON_HEIGHT);
	
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(sideScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10, Screen::Bottom() - 80),
		Point(0., 1.), 10., 10., 5., Color(sideScroll < maxSideScroll ? .8 : .2, 0.));
}



void ShopPanel::DrawButtons()
{
	// The last 70 pixels on the end of the side panel are for the buttons:
	Point buttonSize(SIDE_WIDTH, BUTTON_HEIGHT);
	FillShader::Fill(Screen::BottomRight() - .5 * buttonSize, buttonSize, *GameData::Colors().Get("shop side panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDE_WIDTH / 2, Screen::Bottom() - BUTTON_HEIGHT),
		Point(SIDE_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));
	
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("panel background");
	
	Point point(
		Screen::Right() - SIDE_WIDTH + 10,
		Screen::Bottom() - 65);
	font.Draw("You have:", point, dim);
	
	string credits = Format::Credits(player.Accounts().Credits()) + " credits";
	point.X() += (SIDE_WIDTH - 20) - font.Width(credits);
	font.Draw(credits, point, bright);
	
	const Font &bigFont = FontSet::Get(18);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");
	
	Point buyCenter = Screen::BottomRight() - Point(210, 25);
	FillShader::Fill(buyCenter, Point(60, 30), back);
	string BUY = (playerShip && selectedOutfit && player.Cargo().Get(selectedOutfit)) ? "_Install" : "_Buy";
	bigFont.Draw(BUY,
		buyCenter - .5 * Point(bigFont.Width(BUY), bigFont.Height()),
		CanBuy() ? hoverButton == 'b' ? hover : active : inactive);
	
	Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), back);
	static const string SELL = "_Sell";
	bigFont.Draw(SELL,
		sellCenter - .5 * Point(bigFont.Width(SELL), bigFont.Height()),
		CanSell() ? hoverButton == 's' ? hover : active : inactive);
	
	Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), back);
	static const string LEAVE = "_Leave";
	bigFont.Draw(LEAVE,
		leaveCenter - .5 * Point(bigFont.Width(LEAVE), bigFont.Height()),
		hoverButton == 'l' ? hover : active);
	
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



void ShopPanel::DrawMain()
{
	const Font &bigFont = FontSet::Get(18);
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	mainDetailHeight = 0;
	
	const Sprite *collapsedArrow = SpriteSet::Get("ui/collapsed");
	const Sprite *expandedArrow = SpriteSet::Get("ui/expanded");
	
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
		
		Point side(Screen::Left() + 5., point.Y() - TILE_SIZE / 2 + 10);
		point.Y() += bigFont.Height() + 20;
		nextY += bigFont.Height() + 20;
		
		bool isCollapsed = collapsed.count(category);
		bool isEmpty = true;
		for(const string &name : it->second)
		{
			bool isSelected = (selectedShip && GameData::Ships().Get(name) == selectedShip)
				|| (selectedOutfit && GameData::Outfits().Get(name) == selectedOutfit);
			
			if(isSelected)
				selectedTopY = point.Y() - TILE_SIZE / 2;
			
			if(!HasItem(name))
				continue;
			isEmpty = false;
			if(isCollapsed)
				break;
			
			DrawItem(name, point, scrollY);
			
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
			Point size(bigFont.Width(category) + 25., bigFont.Height());
			categoryZones.emplace_back(Point(Screen::Left(), side.Y()) + .5 * size, size, category);
			SpriteShader::Draw(isCollapsed ? collapsedArrow : expandedArrow, side + Point(10., 10.));
			bigFont.Draw(category, side + Point(25., 0.), isCollapsed ? dim : bright);
			
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
	// bottom of the screen? (Also leave space for the "key" at the bottom.)
	maxMainScroll = max(0., nextY + mainScroll - Screen::Height() / 2 - TILE_SIZE / 2 + 40.);
	
	PointerShader::Draw(Point(Screen::Right() - 10 - SIDE_WIDTH, Screen::Top() + 10),
		Point(0., -1.), 10., 10., 5., Color(mainScroll > 0 ? .8 : .2, 0.));
	PointerShader::Draw(Point(Screen::Right() - 10 - SIDE_WIDTH, Screen::Bottom() - 10),
		Point(0., 1.), 10., 10., 5., Color(mainScroll < maxMainScroll ? .8 : .2, 0.));
}



void ShopPanel::DrawShip(const Ship &ship, const Point &center, bool isSelected)
{
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/shipyard selected" : "ui/shipyard unselected");
	SpriteShader::Draw(back, center);
	
	// Draw the ship name.
	const Font &font = FontSet::Get(14);
	const string &name = ship.Name().empty() ? ship.ModelName() : font.TruncateMiddle(ship.Name(), SIDE_WIDTH - 61);
	Point offset(-.5f * font.Width(name), -.5f * SHIP_SIZE + 10.f);
	font.Draw(name, center + offset, *GameData::Colors().Get("bright"));
	
	const Sprite *thumbnail = ship.Thumbnail();
	const Sprite *sprite = ship.GetSprite();
	int swizzle = ship.CustomSwizzle() >= 0 ? ship.CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
	if(thumbnail)
		SpriteShader::Draw(thumbnail, center + Point(0., 10.), 1., swizzle);
	else if(sprite)
	{
		// Make sure the ship sprite leaves 10 pixels padding all around.
		const float zoomSize = SHIP_SIZE - 60.f;
		float zoom = min(1.f, zoomSize / max(sprite->Width(), sprite->Height()));
		SpriteShader::Draw(sprite, center, zoom, swizzle);
	}
}



void ShopPanel::FailSell(bool toCargo) const
{
}



bool ShopPanel::CanSellMultiple() const
{
	return true;
}



bool ShopPanel::ShouldHighlight(const Ship *ship)
{
	return (hoverButton == 's');
}



void ShopPanel::DrawKey()
{
}



void ShopPanel::ToggleForSale()
{
	sameSelectedTopY = true;
}



void ShopPanel::ToggleCargo()
{
	sameSelectedTopY = true;
}



// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	scrollDetailsIntoView = false;
	bool toCargo = selectedOutfit && (key == 'r' || key == 'u');
	if(key == 'l' || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		player.UpdateCargoCapacities();
		GetUI()->Pop(this);
	}
	else if(key == 'b' || (key == 'i' && selectedOutfit && player.Cargo().Get(selectedOutfit)))
	{
		if(!CanBuy())
			FailBuy();
		else
		{
			Buy(key == 'i');
			player.UpdateCargoCapacities();
		}
	}
	else if(key == 's' || toCargo)
	{
		if(!CanSell(toCargo))
			FailSell(toCargo);
		else
		{
			int modifier = CanSellMultiple() ? Modifier() : 1;
			for(int i = 0; i < modifier && CanSell(toCargo); ++i)
				Sell(toCargo);
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
	else if(key >= '0' && key <= '9')
	{
		int group = key - '0';
		if(mod & (KMOD_CTRL | KMOD_GUI))
			player.SetGroup(group, &playerShips);
		else if(mod & KMOD_SHIFT)
		{
			// If every single ship in this group is already selected, shift
			// plus the group number means to deselect all those ships.
			set<Ship *> added = player.GetGroup(group);
			bool allWereSelected = true;
			for(Ship *ship : added)
				allWereSelected &= playerShips.erase(ship);
			
			if(allWereSelected)
				added.clear();
			
			const System *here = player.GetSystem();
			for(Ship *ship : added)
				if(!ship->IsDisabled() && ship->GetSystem() == here)
					playerShips.insert(ship);
			
			if(!playerShips.count(playerShip))
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		}
		else
		{
			// Change the selection to the desired ships, if they are landed here.
			playerShips.clear();
			set<Ship *> wanted = player.GetGroup(group);
			
			const System *here = player.GetSystem();
			for(Ship *ship : wanted)
				if(!ship->IsDisabled() && ship->GetSystem() == here)
					playerShips.insert(ship);
			
			if(!playerShips.count(playerShip))
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		}
	}
	else
		return false;
	
	return true;
}



bool ShopPanel::Click(int x, int y, int clicks)
{
	dragShip = nullptr;
	// Handle clicks on the buttons.
	char button = CheckButton(x, y);
	if(button)
		return DoKey(button);
	
	// Check for clicks in the scroll arrows.
	if(x >= Screen::Right() - 20)
	{
		if(y < Screen::Top() + 20)
			return Scroll(0, 4);
		if(y < Screen::Bottom() - BUTTON_HEIGHT && y >= Screen::Bottom() - BUTTON_HEIGHT - 20)
			return Scroll(0, -4);
	}
	else if(x >= Screen::Right() - SIDE_WIDTH - 20 && x < Screen::Right() - SIDE_WIDTH)
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
			bool toggleAll = (SDL_GetModState() & KMOD_SHIFT);
			auto it = collapsed.find(zone.Value());
			if(it == collapsed.end())
			{
				if(toggleAll)
				{
					selectedShip = nullptr;
					selectedOutfit = nullptr;
					for(const string &category : categories)
						collapsed.insert(category);
				}
				else
				{
					collapsed.insert(zone.Value());
					if(selectedShip && selectedShip->Attributes().Category() == zone.Value())
						selectedShip = nullptr;
					if(selectedOutfit && selectedOutfit->Category() == zone.Value())
						selectedOutfit = nullptr;
				}
			}
			else
			{
				if(toggleAll)
					collapsed.clear();
				else
					collapsed.erase(it);
			}
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
			
			// Scroll details into view in Step() when the height is known.
			scrollDetailsIntoView = true;
			mainDetailHeight = 0;
			mainScroll = max(0., mainScroll + zone.ScrollY());
			return true;
		}
	
	return true;
}



bool ShopPanel::Hover(int x, int y)
{
	Point point(x, y);
	// Check that the point is not in the button area.
	hoverButton = CheckButton(x, y);
	if(hoverButton)
	{
		shipInfo.ClearHover();
		outfitInfo.ClearHover();
	}
	else
	{
		shipInfo.Hover(point);
		outfitInfo.Hover(point);
	}
	
	dragMain = (x < Screen::Right() - SIDE_WIDTH);
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
	return DoScroll(dy * 2.5 * Preferences::ScrollSpeed());
}



int64_t ShopPanel::LicenseCost(const Outfit *outfit) const
{
	// Don't require a license for an outfit that you have in cargo or that you
	// just sold to the outfitter. (Otherwise, there would be no way to transfer
	// a restricted plundered outfit between ships or from cargo to a ship.)
	if(player.Cargo().Get(outfit) || player.Stock(outfit) > 0)
		return 0;
	
	const Sale<Outfit> &available = player.GetPlanet()->Outfitter();
	
	int64_t cost = 0;
	for(const string &name : outfit->Licenses())
		if(!player.GetCondition("license: " + name))
		{
			const Outfit *license = GameData::Outfits().Find(name + " License");
			if(!license || !license->Cost() || !available.Has(license))
				return -1;
			cost += license->Cost();
		}
	return cost;
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
	double &scroll = dragMain ? mainScroll : sideScroll;
	const double &maximum = dragMain ? maxMainScroll : maxSideScroll;
	
	scroll = max(0., min(maximum, scroll - dy));
	
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
	
	
	const System *here = player.GetSystem();
	if(count < 0)
	{
		while(count)
		{
			if(it == player.Ships().begin())
				it = player.Ships().end();
			--it;
			
			if((*it)->GetSystem() == here && !(*it)->IsDisabled())
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
			
			if((*it)->GetSystem() == here && !(*it)->IsDisabled())
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
		const System *here = player.GetSystem();
		for(const shared_ptr<Ship> &other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(other->GetSystem() != here || other->IsDisabled())
				continue;
			
			if(other.get() == ship || other.get() == playerShip)
				on = !on;
			else if(on)
				playerShips.insert(other.get());
		}
	}	
	else if(!control)
		playerShips.clear();
	else if(playerShips.count(ship))
	{
		playerShips.erase(ship);
		if(playerShip == ship)
			playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		return;
	}
	
	playerShip = ship;
	playerShips.insert(playerShip);
	sameSelectedTopY = true;
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
	int margin = Screen::Right() - SHIP_SIZE;
	vector<Zone>::const_iterator start = zones.begin();
	while(start != zones.end() && start->Center().X() > margin)
		++start;
	
	return start;
}



// Check if the given point is within the button zone, and if so return the
// letter of the button (or ' ' if it's not on a button).
char ShopPanel::CheckButton(int x, int y)
{
	if(x < Screen::Right() - SIDE_WIDTH || y < Screen::Bottom() - BUTTON_HEIGHT)
		return '\0';
	
	if(y < Screen::Bottom() - 40 || y >= Screen::Bottom() - 10)
		return ' ';
	
	x -= Screen::Right() - SIDE_WIDTH;
	if(x < 80)
		return 'b';
	else if(x < 160)
		return 's';
	else
		return 'l';
	
	return ' ';
}
