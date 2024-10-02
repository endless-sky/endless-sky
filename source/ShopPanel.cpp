/* ShopPanel.cpp
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

#include "ShopPanel.h"

#include "GamePad.h"
#include "GamepadCursor.h"
#include "Rectangle.h"
#include "text/alignment.hpp"
#include "CategoryTypes.h"
#include "Color.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Mission.h"
#include "OutlineShader.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "Sale.h"
#include "Screen.h"
#include "ScrollBar.h"
#include "ScrollVar.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "SpriteShader.h"
#include "text/truncate.hpp"
#include "UI.h"
#include "text/WrappedText.h"

#include "opengl.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;

namespace {
	const string SHIP_OUTLINES = "Ship outlines in shops";

	constexpr int ICON_TILE = 62;
	constexpr int ICON_COLS = 4;
	constexpr float ICON_SIZE = ICON_TILE - 8;

	bool CanShowInSidebar(const Ship &ship, const Planet *here)
	{
		return ship.GetPlanet() == here;
	}

	const int HOVER_TIME = 60;

	void DrawTooltip(const string &text, const Point &hoverPoint, const Color &textColor, const Color &backColor)
	{
		constexpr int WIDTH = 250;
		constexpr int PAD = 10;
		WrappedText wrap(FontSet::Get(14));
		wrap.SetWrapWidth(WIDTH - 2 * PAD);
		wrap.Wrap(text);
		int longest = wrap.LongestLineWidth();
		if(longest < wrap.WrapWidth())
		{
			wrap.SetWrapWidth(longest);
			wrap.Wrap(text);
		}

		Point textSize(wrap.WrapWidth() + 2 * PAD, wrap.Height() + 2 * PAD - wrap.ParagraphBreak());
		Point anchor = Point(hoverPoint.X(), min<double>(hoverPoint.Y() + textSize.Y(), Screen::Bottom()));
		FillShader::Fill(anchor - .5 * textSize, textSize, backColor);
		wrap.Draw(anchor - textSize + Point(PAD, PAD), textColor);
	}


	constexpr auto ScrollbarMaybeUpdate = [](const auto &op, ScrollBar &scrollbar,
		ScrollVar<double> &scroll, bool animate)
	{
		if(!op(scrollbar))
			return false;
		scrollbar.SyncInto(scroll, animate ? 5 : 0);
		return true;
	};

	// disposition menu options
	const string INSTALL_IN_SHIP = "Install in ship";
	const string MOVE_TO_CARGO = "Move to cargo";
	const string MOVE_TO_STORAGE = "Move to storage";
}

static const unsigned int LONG_CLICK_DURATION = 500; // milliseconds

ShopPanel::ShopPanel(PlayerInfo &player, bool isOutfitter)
	: player(player), day(player.GetDate().DaysSinceEpoch()),
	planet(player.GetPlanet()), isOutfitter(isOutfitter), playerShip(player.Flagship()),
	categories(GameData::GetCategory(isOutfitter ? CategoryType::OUTFIT : CategoryType::SHIP)),
	collapsed(player.Collapsed(isOutfitter ? "outfitter" : "shipyard"))
{
	if(playerShip)
		playerShips.insert(playerShip);
	SetIsFullScreen(true);
	SetInterruptible(false);

	selected_quantity = std::make_shared<Dropdown>();
	selected_quantity->SetAlign(Dropdown::LEFT);
	selected_quantity->SetFontSize(14);
	selected_quantity->SetPadding(0);
	selected_quantity->SetOptions({"1", "10", "100", "1000"});

	outfit_disposition = std::make_shared<Dropdown>();
	outfit_disposition->SetVisible(isOutfitter);
	outfit_disposition->SetAlign(Dropdown::LEFT);
	outfit_disposition->SetFontSize(14);
	outfit_disposition->SetPadding(0);
	outfit_disposition->SetOptions({INSTALL_IN_SHIP, MOVE_TO_CARGO, MOVE_TO_STORAGE});
	outfit_disposition->SetCallback([this](int, const std::string& value) { this->DispositionChanged(value); });

	AddChild(selected_quantity);
	AddChild(outfit_disposition);

	// Default pane is main, which does its own higlighting, so disable the
	// cursor. Set the default button though for when the side pane is set
	// TODO: need a better way to find this.
	const Point buyCenter = Screen::BottomRight() - Point(210, 25);
	GamepadCursor::SetPosition(buyCenter);
	GamepadCursor::SetEnabled(false);
}



void ShopPanel::Step()
{
	// If the player has acquired a second ship for the first time, explain to
	// them how to reorder the ships in their fleet.
	if(player.Ships().size() > 1)
		DoHelp("multiple ships");
}



void ShopPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	// These get added by both DrawMain and DrawDetailsSidebar, so clear them here.
	categoryZones.clear();
	DrawMain();
	DrawShipsSidebar();
	DrawDetailsSidebar();
	DrawButtons();
	DrawKey();

	shipInfo.DrawTooltips();
	outfitInfo.DrawTooltips();

	if(!shipName.empty())
	{
		string text = shipName;
		if(!warningType.empty())
			text += "\n" + GameData::Tooltip(warningType);
		const Color &textColor = *GameData::Colors().Get("medium");
		const Color &backColor = *GameData::Colors().Get(warningType.empty() ? "tooltip background"
					: (warningType.back() == '!' ? "error back" : "warning back"));
		DrawTooltip(text, hoverPoint, textColor, backColor);
	}

	if(dragShip && isDraggingShip && dragShip->GetSprite())
	{
		const Sprite *sprite = dragShip->GetSprite();
		float scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
		if(Preferences::Has(SHIP_OUTLINES))
		{
			static const Color selected(.8f, 1.f);
			Point size(sprite->Width() * scale, sprite->Height() * scale);
			OutlineShader::Draw(sprite, dragPoint, size, selected);
		}
		else
		{
			int swizzle = dragShip->CustomSwizzle() >= 0
				? dragShip->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
			SpriteShader::Draw(sprite, dragPoint, scale, swizzle);
		}
	}

	// Check to see if we need to scroll things onto the screen.
	if(delayedAutoScroll)
	{
		delayedAutoScroll = false;
		const auto selected = Selected();
		if(selected != zones.end())
			MainAutoScroll(selected);
	}
}



void ShopPanel::DrawShip(const Ship &ship, const Point &center, bool isSelected)
{
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/shipyard selected" : "ui/shipyard unselected");
	SpriteShader::Draw(back, center);

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

	// Draw the ship name.
	const Font &font = FontSet::Get(14);
	const string &name = ship.Name().empty() ? ship.DisplayModelName() : ship.Name();
	Point offset(-SIDEBAR_CONTENT / 2, -.5f * SHIP_SIZE + 10.f);
	font.Draw({name, {SIDEBAR_CONTENT, Alignment::CENTER, Truncate::MIDDLE}},
		center + offset, *GameData::Colors().Get("bright"));
}



void ShopPanel::CheckForMissions(Mission::Location location)
{
	if(!GetUI()->IsTop(this))
		return;

	Mission *mission = player.MissionToOffer(location);
	if(mission)
		mission->Do(Mission::OFFER, player, GetUI());
	else
		player.HandleBlockedMissions(location, GetUI());
}



void ShopPanel::FailSell(bool toStorage) const
{
}



bool ShopPanel::CanSellMultiple() const
{
	return true;
}



// Helper function for UI buttons to determine if the selected item is
// already owned. Affects if "Install" is shown for already owned items
// or if "Buy" is shown for items not yet owned.
//
// If we are buying into cargo, then items in cargo don't count as already
// owned, but they count as "already installed" in cargo.
bool ShopPanel::IsAlreadyOwned() const
{
	return (playerShip && selectedOutfit && player.Cargo().Get(selectedOutfit))
		|| player.Storage().Get(selectedOutfit);
}



bool ShopPanel::ShouldHighlight(const Ship *ship)
{
	return (hoverButton == 's');
}



void ShopPanel::DrawKey()
{
}



int ShopPanel::VisibilityCheckboxesSize() const
{
	return 0;
}



void ShopPanel::ToggleForSale()
{
	CheckSelection();
	delayedAutoScroll = true;
}



void ShopPanel::ToggleStorage()
{
	CheckSelection();
	delayedAutoScroll = true;
}



void ShopPanel::ToggleCargo()
{
	CheckSelection();
	delayedAutoScroll = true;
}



// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool toStorage = planet && planet->HasOutfitter() && (key == 'r' || key == 'u');
	if(key == 'l' || key == 'd' || key == SDLK_ESCAPE || key == SDLK_AC_BACK
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		if(!isOutfitter)
			player.UpdateCargoCapacities();
		GetUI()->Pop(this);
	}
	else if(command.Has(Command::HELP))
	{
		if(player.Ships().size() > 1)
		{
			if(isOutfitter)
				DoHelp("outfitter with multiple ships", true);
			DoHelp("multiple ships", true);
		}
		if(isOutfitter)
		{
			DoHelp("uninstalling and storage", true);
			DoHelp("cargo management", true);
			DoHelp("outfitter", true);
		}
	}
	else if(command.Has(Command::MAP))
	{
		if(isOutfitter)
			GetUI()->Push(new MapOutfitterPanel(player));
		else
			GetUI()->Push(new MapShipyardPanel(player));
	}
	else if(key == 'b' || key == 'i' || key == 'c')
	{
		const auto result = CanBuy(key == 'i' || key == 'c');
		if(result)
		{
			Buy(key == 'i' || key == 'c');
			// Ship-based updates to cargo are handled when leaving.
			// Ship-based selection changes are asynchronous, and handled by ShipyardPanel.
			if(isOutfitter)
			{
				player.UpdateCargoCapacities();
				CheckSelection();
			}
		}
		else if(result.HasMessage())
			GetUI()->Push(new Dialog(result.Message()));
	}
	else if(key == 's' || toStorage)
	{
		if (outfit_disposition->GetSelected() == MOVE_TO_STORAGE)
			toStorage = true;
		if(!CanSell(toStorage))
			FailSell(toStorage);
		else
		{
			int modifier = CanSellMultiple() ? stoi(selected_quantity->GetSelected()) : 1;
			for(int i = 0; i < modifier && CanSell(toStorage); ++i)
				Sell(toStorage);
			if(isOutfitter)
			{
				player.UpdateCargoCapacities();
				CheckSelection();
			}
		}
	}
	else if(key == SDLK_LEFT)
	{
		if(activePane != ShopPane::Sidebar)
			MainLeft();
		else
			SideSelect(-1);
		return true;
	}
	else if(key == SDLK_RIGHT)
	{
		if(activePane != ShopPane::Sidebar)
			MainRight();
		else
			SideSelect(1);
		return true;
	}
	else if(key == SDLK_UP)
	{
		if(activePane != ShopPane::Sidebar)
			MainUp();
		else
			SideSelect(-4);
		return true;
	}
	else if(key == SDLK_DOWN)
	{
		if(activePane != ShopPane::Sidebar)
			MainDown();
		else
			SideSelect(4);
		return true;
	}
	else if(key == SDLK_PAGEUP)
		return DoScroll(Screen::Bottom());
	else if(key == SDLK_PAGEDOWN)
		return DoScroll(Screen::Top());
	else if(key == SDLK_HOME)
		return SetScrollToTop();
	else if(key == SDLK_END)
		return SetScrollToBottom();
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

			const Planet *here = player.GetPlanet();
			for(Ship *ship : added)
				if(CanShowInSidebar(*ship, here))
					playerShips.insert(ship);

			if(!playerShips.contains(playerShip))
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		}
		else
		{
			// Change the selection to the desired ships, if they are landed here.
			playerShips.clear();
			set<Ship *> wanted = player.GetGroup(group);

			const Planet *here = player.GetPlanet();
			for(Ship *ship : wanted)
				if(CanShowInSidebar(*ship, here))
					playerShips.insert(ship);

			if(!playerShips.contains(playerShip))
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		}
	}
	else if(key == SDLK_TAB)
		activePane = (activePane == ShopPane::Main ? ShopPane::Sidebar : ShopPane::Main);
	else if(key == 'f')
		GetUI()->Push(new Dialog(this, &ShopPanel::DoFind, "Search for:"));
	else
		return false;

	return true;
}



bool ShopPanel::ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive)
{
	// treat left joystick like arrow keys, right joystick like navigation keys.
	// Fallback to the default zone-navigation behavior on the side pane.
	if(activePane == ShopPane::Main)
	{
		if(axis == SDL_CONTROLLER_AXIS_LEFTX)
			return KeyDown(positive ? SDLK_RIGHT : SDLK_LEFT, 0, Command(), true);
		else if(axis == SDL_CONTROLLER_AXIS_LEFTY)
			return KeyDown(positive ? SDLK_DOWN : SDLK_UP, 0, Command(), true);
	}
	else if(activePane == ShopPane::Info)
	{
		if(axis == SDL_CONTROLLER_AXIS_LEFTX ||
		   axis == SDL_CONTROLLER_AXIS_LEFTY)
			return true; // prevent event from doing normal button selection logic
	}
	else if(activePane == ShopPane::Sidebar)
	{
		if(axis == SDL_CONTROLLER_AXIS_LEFTX ||
		   axis == SDL_CONTROLLER_AXIS_LEFTY)
		{
			// do not rely on default zone-based cursor handling, as this class
			// uses its own zones (also confusingly named "Zone")
			std::vector<Point> options = GetUI()->ZonePositions();
			for(auto& z: shipZones)
			{
				// only add zones in the right side panel that are visible on the
				// screen
				Rectangle sidePane(Point(Screen::Right() - SIDEBAR_WIDTH/2.0, 0), Point(SIDEBAR_WIDTH, Screen::Height()));
				if(sidePane.Contains(z.Center()))
					options.push_back(z.Center());
			}
			Point oldPos = GamepadCursor::Position();
			GamepadCursor::MoveDir(GamePad::LeftStick(), options);

			if(isDraggingShip)
			{
				const Point& dp = GamepadCursor::Position() - oldPos;
				Drag(dp.X(), dp.Y());
			}

			return true;
		}
	}

	if(axis == SDL_CONTROLLER_AXIS_RIGHTX)
	{
		if(positive && activePane == ShopPane::Main)
		{
			activePane = ShopPane::Info;
			GamepadCursor::SetEnabled(false);
		}
		else if(activePane == ShopPane::Info)
		{
			if(positive)
			{
				activePane = ShopPane::Sidebar;
				GamepadCursor::SetEnabled(true);
			}
			else
			{
				activePane = ShopPane::Main;
				GamepadCursor::SetEnabled(false);
			}
		}
		else if(!positive && activePane == ShopPane::Sidebar)
		{
			activePane = ShopPane::Info;
			GamepadCursor::SetEnabled(false);
		}
		return true;
	}
	return false;
}



bool ShopPanel::ControllerButtonDown(SDL_GameControllerButton button)
{
	if(button == SDL_CONTROLLER_BUTTON_GUIDE)
		return KeyDown(SDLK_ESCAPE, 0, Command(), true);
	if(activePane == ShopPane::Main)
	{
		if(button == SDL_CONTROLLER_BUTTON_A)
		{
			activePane = ShopPane::Sidebar;
			// switch to the sidebar, and highlight the buy button
			// TODO: need a better way to find this.
			const Point buyCenter = Screen::BottomRight() - Point(210, 25);
			GamepadCursor::SetPosition(buyCenter);
			return true;
		}
	}
	else if(activePane == ShopPane::Sidebar)
	{
		if(isDraggingShip)
		{
			// Any button ends the dragging operation
			const Point& p = GamepadCursor::Position();
			return Release(p.X(), p.Y());
		}

		if(button == SDL_CONTROLLER_BUTTON_A)
		{
			// Act like a short click
			const Point& p = GamepadCursor::Position();
			bool ret = Click(p.X(), p.Y(), 1);
			Release(p.X(), p.Y());
			return ret;
		}
		else if(button == SDL_CONTROLLER_BUTTON_B)
		{
			// Act like a long click
			const Point& p = GamepadCursor::Position();
			bool ret = Click(p.X(), p.Y(), 1);
			lastShipClickTime = SDL_GetTicks() - LONG_CLICK_DURATION - 1;
			Release(p.X(), p.Y());
			return ret;
		}
		else if(button == SDL_CONTROLLER_BUTTON_X)
		{
			// Act like a double click
			const Point& p = GamepadCursor::Position();
			return Click(p.X(), p.Y(), 2);
		}
		else if(button == SDL_CONTROLLER_BUTTON_Y)
		{
			// If we have a ship selected, act like the start of a drag and drop
			const Point& p = GamepadCursor::Position();
			for(const auto &zone : shipZones)
			{
				// floating point comparison ok here.
				if(zone.Center().X() == p.X() && zone.Center().Y() == p.Y())
				{
					Click(p.X(), p.Y(), 1);
					return Drag(10, -10);
				}
			}
		}
	}
	return false;
}



bool ShopPanel::Click(int x, int y, int clicks)
{
	dragShip = nullptr;
	char button = CheckButton(x, y);
	if(button)
		return DoKey(button);

	auto ScrollbarClick = [x, y, clicks](ScrollBar &scrollbar, ScrollVar<double> &scroll)
	{
		return ScrollbarMaybeUpdate([x, y, clicks](ScrollBar &scrollbar)
			{
				return scrollbar.Click(x, y, clicks);
			}, scrollbar, scroll, true);
	};
	if(ScrollbarClick(mainScrollbar, mainScroll)
			|| ScrollbarClick(sidebarScrollbar, sidebarScroll)
			|| ScrollbarClick(infobarScrollbar, infobarScroll))
		return true;

	const Point clickPoint(x, y);

	// Check for clicks in the category labels.
	for(const ClickZone<string> &zone : categoryZones)
		if(zone.Contains(clickPoint))
		{
			bool toggleAll = (SDL_GetModState() & KMOD_SHIFT);
			auto it = collapsed.find(zone.Value());
			if(it == collapsed.end())
			{
				if(toggleAll)
				{
					selectedShip = nullptr;
					selectedOutfit = nullptr;
					for(const auto &category : categories)
						collapsed.insert(category.Name());
				}
				else
				{
					collapsed.insert(zone.Value());
					CategoryAdvance(zone.Value());
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

	// Check for clicks in the main zones.
	for(const Zone &zone : zones)
		if(zone.Contains(clickPoint))
		{
			if(zone.GetShip())
				selectedShip = zone.GetShip();
			else
				selectedOutfit = zone.GetOutfit();

			previousX = zone.Center().X();

			return true;
		}

	// Check for clicks in the sidebar zones.
	for(const ClickZone<const Ship *> &zone : shipZones)
		if(zone.Contains(clickPoint))
		{
			if(clicks > 1)
			{
				bool foundSelectedShip = false;
				// select all contiguous ships that are the same type as the
				// ship that was double-clicked.
				std::set<Ship*> playerShipsToSelect;
				for(const shared_ptr<Ship> &ship : player.Ships())
				{
					// Skip any ships that are "absent" for whatever reason.
					if(!CanShowInSidebar(*ship, player.GetPlanet()))
						continue;
					if(ship.get() == zone.Value())
					{
						foundSelectedShip = true;
						SideSelect(ship.get());
					}
					if(ship->TrueModelName() == zone.Value()->TrueModelName())
						playerShipsToSelect.insert(ship.get());
					else if(!foundSelectedShip) // not the correct range
						playerShipsToSelect.clear();
					else
						break; // we hit the end of the correct range.
				}
				if(foundSelectedShip)
				{
					playerShips = playerShipsToSelect;
					return true;
				}
			}
			else
			{
				lastShipClickTime = SDL_GetTicks();

				// Is the ship that was clicked one of the player's?
				for(const shared_ptr<Ship> &ship : player.Ships())
					if(ship.get() == zone.Value())
					{
						dragShip = ship.get();
						dragPoint.Set(x, y);
						SideSelect(dragShip);
						return true;
					}
			}
		}


	return true;
}



bool ShopPanel::Hover(int x, int y)
{
	mainScrollbar.Hover(x, y);
	infobarScrollbar.Hover(x, y);
	sidebarScrollbar.Hover(x, y);

	hoverPoint = Point(x, y);
	// Check that the point is not in the button area.
	hoverButton = CheckButton(x, y);
	if(hoverButton)
	{
		shipInfo.ClearHover();
		outfitInfo.ClearHover();
	}
	else
	{
		shipInfo.Hover(hoverPoint);
		outfitInfo.Hover(hoverPoint);
	}

	activePane = ShopPane::Main;
	if(x > Screen::Right() - SIDEBAR_WIDTH)
		activePane = ShopPane::Sidebar;
	else if(x > Screen::Right() - SIDE_WIDTH)
		activePane = ShopPane::Info;
	return true;
}



bool ShopPanel::Drag(double dx, double dy)
{
	if(dragShip)
	{
		isDraggingShip = true;
		dragPoint += Point(dx, dy);
		for(const ClickZone<const Ship *> &zone : shipZones)
			if(zone.Contains(dragPoint))
				if(zone.Value() != dragShip)
				{
					int dragIndex = -1;
					int dropIndex = -1;
					for(unsigned i = 0; i < player.Ships().size(); ++i)
					{
						const Ship *ship = &*player.Ships()[i];
						if(ship == dragShip)
							dragIndex = i;
						if(ship == zone.Value())
							dropIndex = i;
					}
					if(dragIndex >= 0 && dropIndex >= 0)
						player.ReorderShip(dragIndex, dropIndex);
				}
	}
	else
	{
		auto scrollbarInterceptSpec = [dx, dy](ScrollBar &scrollbar, ScrollVar<double> &scroll) {
			scrollbar.SyncFrom(scroll, scrollbar.from, scrollbar.to, false);
			return ScrollbarMaybeUpdate([dx, dy](ScrollBar &scrollbar)
				{
					return scrollbar.Drag(dx, dy);
				}, scrollbar, scroll, false);
		};
		if(!scrollbarInterceptSpec(mainScrollbar, mainScroll)
				&& !scrollbarInterceptSpec(sidebarScrollbar, sidebarScroll)
				&& !scrollbarInterceptSpec(infobarScrollbar, infobarScroll))
			DoScroll(dy, 0);
	}

	return true;
}



bool ShopPanel::Release(int x, int y)
{
	if (isDraggingShip)
	{
		dragShip = nullptr;
		isDraggingShip = false;
	}
	else if (lastShipClickTime != static_cast<unsigned int>(-1))
	{
		// what ship did we unclick on?
		for(auto &zone : shipZones)
		{
			if(zone.Contains(Point(x, y)) && zone.Value() == playerShip)
			{
				if (SDL_GetTicks() - lastShipClickTime < LONG_CLICK_DURATION)
				{
					// normal click. Remove everything but the current ship
					if (playerShip)
					{
						playerShips.clear();
						playerShips.insert(playerShip);
						outfit_disposition->SetSelected(INSTALL_IN_SHIP);
					}
				}
				else
				{
					// long click. if the ship was already there, remove it
					if (shipToRemoveIfLongClick)
					{
						playerShips.erase(shipToRemoveIfLongClick);
						if (shipToRemoveIfLongClick == playerShip)
						{
							if (!playerShips.empty())
								playerShip = *playerShips.begin();
							else
								playerShip = nullptr;
						}
					}
				}
				break;
			}
		}
	}
	lastShipClickTime = -1;
	shipToRemoveIfLongClick = nullptr;
	return true;
}



bool ShopPanel::Scroll(double dx, double dy)
{
	return DoScroll(dy * 2.5 * Preferences::ScrollSpeed());
}



void ShopPanel::DoFind(const string &text)
{
	int index = FindItem(text);
	if(index >= 0 && index < static_cast<int>(zones.size()))
	{
		auto best = std::next(zones.begin(), index);
		if(best->GetShip())
			selectedShip = best->GetShip();
		else
			selectedOutfit = best->GetOutfit();

		previousX = best->Center().X();

		MainAutoScroll(best);
	}
}



int64_t ShopPanel::LicenseCost(const Outfit *outfit, bool onlyOwned) const
{
	// If the player is attempting to install an outfit from cargo, storage, or that they just
	// sold to the shop, then ignore its license requirement, if any. (Otherwise there
	// would be no way to use or transfer license-restricted outfits between ships.)
	bool owned = (player.Cargo().Get(outfit) && playerShip) || player.Storage().Get(outfit);
	if((owned && onlyOwned) || player.Stock(outfit) > 0)
		return 0;

	const Sale<Outfit> &available = player.GetPlanet()->Outfitter();

	int64_t cost = 0;
	for(const string &name : outfit->Licenses())
		if(!player.HasLicense(name))
		{
			const Outfit *license = GameData::Outfits().Find(name + " License");
			if(!license || !license->Cost() || !available.Has(license))
				return -1;
			cost += license->Cost();
		}
	return cost;
}



ShopPanel::Zone::Zone(Point center, Point size, const Ship *ship)
	: ClickZone(center, size, ship)
{
}



ShopPanel::Zone::Zone(Point center, Point size, const Outfit *outfit)
	: ClickZone(center, size, nullptr), outfit(outfit)
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



void ShopPanel::DrawShipsSidebar()
{
	const Font &font = FontSet::Get(14);
	const Color &dark = *GameData::Colors().Get("dark");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &bg = *GameData::Colors().Get("panel background");
	const Color &bgSelected = *GameData::Colors().Get("panel background selected");

	sidebarScroll.Step();

	// Fill in the background.
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, 0.),
		Point(SIDEBAR_WIDTH, Screen::Height()),
		activePane == ShopPane::Sidebar ? bgSelected : bg);
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH, 0.),
		Point(1, Screen::Height()),
		*GameData::Colors().Get("shop side panel background"));

	// Draw this string, centered in the side panel:
	static const string YOURS = "Your Ships:";
	Point yoursPoint(Screen::Right() - SIDEBAR_WIDTH, Screen::Top() + 10 - sidebarScroll.AnimatedValue());
	font.Draw({YOURS, {SIDEBAR_WIDTH, Alignment::CENTER}}, yoursPoint, bright);

	// Start below the "Your Ships" label, and draw them.
	Point point(
		Screen::Right() - SIDEBAR_CONTENT / 2 - SIDEBAR_PADDING - 93,
		Screen::Top() + SIDEBAR_CONTENT / 2 - sidebarScroll.AnimatedValue() + 40 - 93);

	const Planet *here = player.GetPlanet();
	int shipsHere = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
		shipsHere += CanShowInSidebar(*ship, here);
	if(shipsHere < 4)
		point.X() += .5 * ICON_TILE * (4 - shipsHere);

	// Check whether flight check tooltips should be shown.
	const auto flightChecks = player.FlightCheck();
	Point mouse = UI::GetMouse();
	warningType.clear();
	shipName.clear();
	shipZones.clear();

	static const Color selected(.8f, 1.f);
	static const Color unselected(.4f, 1.f);
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(!CanShowInSidebar(*ship, here))
			continue;

		if(point.X() > Screen::Right())
		{
			point.X() -= ICON_TILE * ICON_COLS;
			point.Y() += ICON_TILE;
		}

		bool isSelected = playerShips.contains(ship.get());
		const Sprite *background = SpriteSet::Get(isSelected ? "ui/icon selected" : "ui/icon unselected");
		SpriteShader::Draw(background, point);
		// If this is one of the selected ships, check if the currently hovered
		// button (if any) applies to it. If so, brighten the background.
		if(isSelected && ShouldHighlight(ship.get()))
			SpriteShader::Draw(background, point);

		const Sprite *sprite = ship->GetSprite();
		if(sprite && (!isDraggingShip || dragShip != ship.get()))
		{
			float scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
			if(Preferences::Has(SHIP_OUTLINES))
			{
				Point size(sprite->Width() * scale, sprite->Height() * scale);
				OutlineShader::Draw(sprite, point, size, isSelected ? selected : unselected);
			}
			else
			{
				int swizzle = ship->CustomSwizzle() >= 0 ? ship->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
				SpriteShader::Draw(sprite, point, scale, swizzle);
			}
		}

		shipZones.emplace_back(point, Point(ICON_TILE/2.0, ICON_TILE/2.0), ship.get());

		if(mouse.Y() < Screen::Bottom() - BUTTON_HEIGHT && shipZones.back().Contains(mouse))
		{
			shipName = ship->Name() + (ship->IsParked() ? "\n" + GameData::Tooltip("parked") : "");
			hoverPoint = shipZones.back().TopLeft();
		}

		const auto checkIt = flightChecks.find(ship);
		if(checkIt != flightChecks.end())
		{
			const string &check = (*checkIt).second.front();
			const Sprite *icon = SpriteSet::Get(check.back() == '!' ? "ui/error" : "ui/warning");
			SpriteShader::Draw(icon, point + .5 * Point(ICON_TILE - icon->Width(), ICON_TILE - icon->Height()));
			if(shipZones.back().Contains(mouse))
				warningType = check;
		}

		if(isSelected && playerShips.size() > 1 && ship->OutfitCount(selectedOutfit))
			PointerShader::Draw(Point(point.X() - static_cast<int>(ICON_TILE / 3), point.Y()),
				Point(1., 0.), 14.f, 12.f, 0., Color(.9f, .9f, .9f, .2f));

		if(ship->IsParked())
		{
			static const Point CORNER = .35 * Point(ICON_TILE, ICON_TILE);
			FillShader::Fill(point + CORNER, Point(6., 6.), dark);
			FillShader::Fill(point + CORNER, Point(4., 4.), isSelected ? bright : medium);
		}

		point.X() += ICON_TILE;
	}
	point.Y() += ICON_TILE;

	if(outfit_disposition->GetSelected() == MOVE_TO_STORAGE)
	{
		CargoHold& storage = player.Storage();
		point.X() = Screen::Right() - SIDEBAR_WIDTH + 10;
		bool empty = true;
		if(!storage.Outfits().empty())
		{
			for (auto& kv: player.Storage().Outfits())
			{
				if (kv.second != 0)
				{
					if (empty)
					{
						empty = false;
						font.Draw("Outfits in planetary storage:", point, bright);
						point.Y() += 20.;
					}
					font.Draw(kv.first->DisplayName() + ": ", point, medium);
					string count = Format::Number(kv.second);
					font.Draw({count, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, point, bright);
					point.Y() += 20.;
				}
			}
		}
		if(empty)
		{
			font.Draw("Nothing in planetary storage", point, bright);
			point.Y() += 20.;
		}
	}
	else if(playerShip)
	{
		point.Y() += SHIP_SIZE / 2;
		point.X() = (Screen::Right() - SIDEBAR_CONTENT / 2) - SIDEBAR_PADDING;
		DrawShip(*playerShip, point, true);

		Point offset(SIDEBAR_CONTENT / -2, SHIP_SIZE / 2);
		const int detailHeight = DrawPlayerShipInfo(point + offset);
		point.Y() += detailHeight + SHIP_SIZE / 2;
	}
	else if(player.Cargo().Size())
	{
		point.X() = Screen::Right() - SIDEBAR_WIDTH + 10;
		font.Draw("cargo space:", point, medium);

		string space = Format::Number(player.Cargo().Free()) + " / " + Format::Number(player.Cargo().Size());
		font.Draw({space, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, point, bright);
		point.Y() += 20.;
	}
	sidebarScroll.SetDisplaySize(Screen::Height() - BUTTON_HEIGHT);
	sidebarScroll.SetMaxValue(max(0., point.Y() + sidebarScroll.AnimatedValue() - Screen::Bottom() + Screen::Height()));

	if(sidebarScroll.Scrollable())
	{
		Point top(Screen::Right() - 3, Screen::Top() + 10);
		Point bottom(Screen::Right() - 3, Screen::Bottom() - BUTTON_HEIGHT - 10);

		sidebarScrollbar.SyncDraw(sidebarScroll, top, bottom);
	}
}



void ShopPanel::DrawDetailsSidebar()
{
	// Fill in the background.
	const Color &line = *GameData::Colors().Get("dim");
	const Color &back = *GameData::Colors().Get("shop info panel background");
	const Color &backSelected = *GameData::Colors().Get("shop info panel background selected");

	infobarScroll.Step();

	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH - INFOBAR_WIDTH, 0.),
		Point(1., Screen::Height()),
		line);
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH - INFOBAR_WIDTH / 2, 0.),
		Point(INFOBAR_WIDTH - 1., Screen::Height()),
		activePane == ShopPane::Info ? backSelected : back);

	Point point(
		Screen::Right() - SIDE_WIDTH + INFOBAR_WIDTH / 2,
		Screen::Top() + 10 - infobarScroll.AnimatedValue());

	double heightOffset = DrawDetails(point);

	infobarScroll.SetDisplaySize(Screen::Height());
	infobarScroll.SetMaxValue(max(0., heightOffset + infobarScroll.AnimatedValue() - Screen::Bottom()) + Screen::Height());

	if(infobarScroll.Scrollable())
	{
		Point top{Screen::Right() - SIDEBAR_WIDTH - 7., Screen::Top() + 10.};
		Point bottom{Screen::Right() - SIDEBAR_WIDTH - 7., Screen::Bottom() - 10.};

		infobarScrollbar.SyncDraw(infobarScroll, top, bottom);
	}
}



void ShopPanel::DrawButtons()
{
	// The last 70 pixels on the end of the side panel are for the buttons:
	const Color &bg = *GameData::Colors().Get("shop side panel background");
	const Color &bgSelected = *GameData::Colors().Get("shop side panel background selected");

	Point buttonSize(SIDEBAR_WIDTH, BUTTON_HEIGHT);
	FillShader::Fill(Screen::BottomRight() - .5 * buttonSize, buttonSize,
		activePane == ShopPane::Sidebar ? bgSelected : bg);
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, Screen::Bottom() - BUTTON_HEIGHT),
		Point(SIDEBAR_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));

	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("panel background");

	const Point creditsPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - BUTTON_HEIGHT + 5);
	font.Draw("You have:", creditsPoint, dim);

	const auto credits = Format::CreditString(player.Accounts().Credits());
	font.Draw({credits, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, creditsPoint, bright);

	const Font &bigFont = FontSet::Get(18);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");

	const Point buyCenter = Screen::BottomRight() - Point(210, 25);
	FillShader::Fill(buyCenter, Point(60, 30), back);
	bool isOwned = IsAlreadyOwned();
	const Color *buyTextColor;
	if(!CanBuy(isOwned))
		buyTextColor = &inactive;
	else if(hoverButton == (isOwned ? 'i' : 'b'))
		buyTextColor = &hover;
	else
		buyTextColor = &active;
	string BUY = isOwned ? (playerShip ? "_Install" : "_Cargo") : "_Buy";
	char keyToPress = isOwned ? (playerShip ? 'i' : 'c') : 'b';
	bigFont.Draw(BUY,
		buyCenter - .5 * Point(bigFont.Width(BUY), bigFont.Height()),
		*buyTextColor);
	AddZone(Rectangle(buyCenter, Point(60, 30)), [this, keyToPress]() {
		KeyDown(keyToPress, 0, Command(), true);
	});

	const Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), back);
	const string SELL = outfit_disposition->GetSelected() == MOVE_TO_STORAGE ? "Move" : "_Sell";
	bigFont.Draw(SELL,
		sellCenter - .5 * Point(bigFont.Width(SELL), bigFont.Height()),
		CanSell() ? hoverButton == 's' ? hover : active : inactive);
	AddZone(Rectangle(sellCenter, Point(60, 30)), [this]() {
		KeyDown('s', 0, Command(), true);
	});

	const Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), back);
	static const string LEAVE = "_Leave";
	bigFont.Draw(LEAVE,
		leaveCenter - .5 * Point(bigFont.Width(LEAVE), bigFont.Height()),
		hoverButton == 'l' ? hover : active);
	AddZone(Rectangle(leaveCenter, Point(70, 30)), [this]() {
		KeyDown(SDLK_ESCAPE, 0, Command(), true);
	});

	const Point findCenter = Screen::BottomRight() - Point(580, 20);
	const Sprite *findIcon =
		hoverButton == 'f' ? SpriteSet::Get("ui/find selected") : SpriteSet::Get("ui/find unselected");
	SpriteShader::Draw(findIcon, findCenter);
	static const string FIND = "_Find";

	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		selected_quantity->SetSelected(to_string(modifier));
		quantity_is_modifier = true;
	}
	else if (quantity_is_modifier)
	{
		// User has released modifier keys. Reset quantity dropdown to 1x
		selected_quantity->SetSelected("1");
		quantity_is_modifier = false;
	}

	font.Draw("Quantity:", Screen::BottomRight() - Point(240, 68), dim);

	const Point sqCenter = Screen::BottomRight() - Point(150, 60);
	selected_quantity->SetPosition(Rectangle(sqCenter, {45, 20}));

	const Point odCenter = Screen::BottomRight() - Point(65, 60);
	outfit_disposition->SetPosition(Rectangle(odCenter, {110, 20}));

	// Draw the tooltip for your full number of credits.
	const Rectangle creditsBox = Rectangle::FromCorner(creditsPoint, Point(SIDEBAR_WIDTH - 20, 15));
	if(creditsBox.Contains(hoverPoint))
		hoverCount += hoverCount < HOVER_TIME;
	else if(hoverCount)
		--hoverCount;

	if(hoverCount == HOVER_TIME)
	{
		string text = Format::Number(player.Accounts().Credits()) + " credits";
		DrawTooltip(text, hoverPoint, dim, *GameData::Colors().Get("tooltip background"));
	}
}



void ShopPanel::DrawMain()
{
	const Font &bigFont = FontSet::Get(18);
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color bg = *GameData::Colors().Get("shop main panel background");
	const Color bgSelected = *GameData::Colors().Get("shop main panel background selected");

	const Sprite *collapsedArrow = SpriteSet::Get("ui/collapsed");
	const Sprite *expandedArrow = SpriteSet::Get("ui/expanded");

	mainScroll.Step();

	// Draw all the available items.
	// First, figure out how many columns we can draw.
	const int TILE_SIZE = TileSize();
	const int mainWidth = (Screen::Width() - SIDE_WIDTH - 1);
	// If the user horizontally compresses the window too far, draw nothing.
	if(mainWidth < TILE_SIZE)
		return;
	const int columns = mainWidth / TILE_SIZE;
	const int columnWidth = mainWidth / columns;

	Rectangle bgArea(Point(Screen::Left() + mainWidth/2.0, 0), Point(mainWidth, Screen::Height()));
	FillShader::Fill(bgArea.Center(), bgArea.Dimensions(), activePane == ShopPane::Main ? bgSelected : bg);

	const Point begin(
		(Screen::Width() - columnWidth) / -2,
		(Screen::Height() - TILE_SIZE) / -2 - mainScroll.AnimatedValue());
	Point point = begin;
	const float endX = Screen::Right() - (SIDE_WIDTH + 1);
	double nextY = begin.Y() + TILE_SIZE;
	zones.clear();
	for(const auto &cat : categories)
	{
		const string &category = cat.Name();
		map<string, vector<string>>::const_iterator it = catalog.find(category);
		if(it == catalog.end())
			continue;

		// This should never happen, but bail out if we don't know what planet
		// we are on (meaning there's no way to know what items are for sale).
		if(!planet)
			break;

		Point side(Screen::Left() + 5., point.Y() - TILE_SIZE / 2 + 10);
		point.Y() += bigFont.Height() + 20;
		nextY += bigFont.Height() + 20;

		bool isCollapsed = collapsed.contains(category);
		bool isEmpty = true;
		for(const string &name : it->second)
		{
			if(!HasItem(name))
				continue;
			isEmpty = false;
			if(isCollapsed)
				break;

			DrawItem(name, point);

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
			Point size(bigFont.Width(category) + 25., bigFont.Height());
			categoryZones.emplace_back(Point(Screen::Left(), side.Y()) + .5 * size, size, category);
			SpriteShader::Draw(isCollapsed ? collapsedArrow : expandedArrow, side + Point(10., 10.));
			bigFont.Draw(category, side + Point(25., 0.), isCollapsed ? dim : bright);

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
	// bottom of the screen? (Also leave space for the "key" at the bottom.)
	mainScroll.SetDisplaySize(Screen::Height());
	mainScroll.SetMaxValue(max(0., nextY + mainScroll.AnimatedValue() - Screen::Height() / 2 - TILE_SIZE / 2 +
		VisibilityCheckboxesSize() + 40.) + Screen::Height());

	if(mainScroll.Scrollable())
	{
		double scrollbarX = Screen::Right() - 7 - SIDE_WIDTH;
		Point top(scrollbarX, Screen::Top() + 10.);
		Point bottom(scrollbarX, Screen::Bottom() - 10.);

		mainScrollbar.SyncDraw(mainScroll, top, bottom);
	}
}



int ShopPanel::DrawPlayerShipInfo(const Point &point)
{
	shipInfo.Update(*playerShip, player, collapsed.contains("description"), true);
	shipInfo.DrawAttributes(point, !isOutfitter);
	const int attributesHeight = shipInfo.GetAttributesHeight(!isOutfitter);
	shipInfo.DrawOutfits(Point(point.X(), point.Y() + attributesHeight));

	return attributesHeight + shipInfo.OutfitsHeight();
}



bool ShopPanel::DoScroll(double dy, int steps)
{
	if(activePane == ShopPane::Info)
		infobarScroll.Scroll(-dy, steps);
	else if(activePane == ShopPane::Sidebar)
		sidebarScroll.Scroll(-dy, steps);
	else
		mainScroll.Scroll(-dy, steps);

	return true;
}



bool ShopPanel::SetScrollToTop()
{
	if(activePane == ShopPane::Info)
		infobarScroll = 0.;
	else if(activePane == ShopPane::Sidebar)
		sidebarScroll = 0.;
	else
		mainScroll = 0.;

	return true;
}



bool ShopPanel::SetScrollToBottom()
{
	if(activePane == ShopPane::Info)
		infobarScroll = infobarScroll.MaxValue();
	else if(activePane == ShopPane::Sidebar)
		sidebarScroll = sidebarScroll.MaxValue();
	else
		mainScroll = mainScroll.MaxValue();

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

		CheckSelection();
		return;
	}


	const Planet *here = player.GetPlanet();
	if(count < 0)
	{
		while(count)
		{
			if(it == player.Ships().begin())
				it = player.Ships().end();
			--it;

			if(CanShowInSidebar(**it, here))
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

			if(CanShowInSidebar(**it, here))
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
		lastShipClickTime = -1;
		bool on = false;
		const Planet *here = player.GetPlanet();
		for(const shared_ptr<Ship> &other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(!CanShowInSidebar(*other, here))
				continue;

			if(other.get() == ship || other.get() == playerShip)
				on = !on;
			else if(on)
			{
				playerShips.insert(other.get());
				outfit_disposition->SetSelected(INSTALL_IN_SHIP);
			}
		}
	}
	else if(!control)
	{
		// playerShips.clear(); // deferred until we know if it was a long click
	}
	else if(playerShips.contains(ship))
	{
		lastShipClickTime = -1;
		playerShips.erase(ship);
		if(playerShip == ship)
			playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		CheckSelection();
		return;
	}

	playerShip = ship;
	if (playerShips.count(playerShip))
		// Already here. remove it if this turns out to be a long click
		shipToRemoveIfLongClick = ship;
	else
	{
		playerShips.insert(playerShip);
		CheckSelection();
		outfit_disposition->SetSelected(INSTALL_IN_SHIP);
	}
}



// If selected item is offscreen, scroll just enough to put it on.
void ShopPanel::MainAutoScroll(const vector<Zone>::const_iterator &selected)
{
	const int TILE_SIZE = TileSize();
	const int topY = selected->Center().Y() - TILE_SIZE / 2;
	const int offTop = topY + Screen::Bottom();
	if(offTop < 0)
		mainScroll += offTop;
	else
	{
		const int offBottom = topY + TILE_SIZE - Screen::Bottom();
		if(offBottom > 0)
			mainScroll += offBottom;
	}
}



void ShopPanel::MainLeft()
{
	if(zones.empty())
		return;

	vector<Zone>::const_iterator it = Selected();

	if(it == zones.end() || it == zones.begin())
	{
		it = zones.end();
		--it;
		mainScroll = mainScroll.MaxValue();
	}
	else
	{
		--it;
		MainAutoScroll(it);
	}

	previousX = it->Center().X();

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



void ShopPanel::MainRight()
{
	if(zones.empty())
		return;

	vector<Zone>::const_iterator it = Selected();

	if(it == zones.end() || ++it == zones.end())
	{
		it = zones.begin();
		mainScroll = 0.;
	}
	else
		MainAutoScroll(it);

	previousX = it->Center().X();

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



void ShopPanel::MainUp()
{
	if(zones.empty())
		return;

	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Start from the first item.
	if(it == zones.end())
	{
		it = zones.begin();
		previousX = it->Center().X();
	}

	const double previousY = it->Center().Y();
	while(it != zones.begin() && it->Center().Y() == previousY)
		--it;
	if(it == zones.begin() && it->Center().Y() == previousY)
	{
		it = zones.end();
		--it;
		mainScroll = mainScroll.MaxValue();
	}
	else
		MainAutoScroll(it);

	while(it->Center().X() > previousX)
		--it;

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



void ShopPanel::MainDown()
{
	if(zones.empty())
		return;

	vector<Zone>::const_iterator it = Selected();
	// Special case: nothing is selected. Select the first item.
	if(it == zones.end())
	{
		mainScroll = 0.;
		it = zones.begin();
		selectedShip = it->GetShip();
		selectedOutfit = it->GetOutfit();
		previousX = it->Center().X();
		return;
	}

	const double previousY = it->Center().Y();
	++it;
	while(it != zones.end() && it->Center().Y() == previousY)
		++it;
	if(it == zones.end())
	{
		it = zones.begin();
		mainScroll = 0.;
	}
	else
		MainAutoScroll(it);

	// Overshoot by one in case this line is shorter than the previous one.
	const double newY = it->Center().Y();
	++it;
	while(it != zones.end() && it->Center().X() <= previousX && it->Center().Y() == newY)
		++it;
	--it;

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



// If the selected item is no longer displayed, advance selection until we find something that is.
void ShopPanel::CheckSelection()
{
	if((!selectedOutfit && !selectedShip) ||
			(selectedShip && HasItem(selectedShip->VariantName())) ||
			(selectedOutfit && HasItem(selectedOutfit->TrueName())))
		return;

	vector<Zone>::const_iterator it = Selected();
	if(it == zones.end())
		return;
	const vector<Zone>::const_iterator oldIt = it;

	// Advance to find next valid selection.
	for( ; it != zones.end(); ++it)
	{
		const Ship *ship = it->GetShip();
		const Outfit *outfit = it->GetOutfit();
		if((ship && HasItem(ship->VariantName())) || (outfit && HasItem(outfit->TrueName())))
			break;
	}

	// If that didn't work, try the other direction.
	if(it == zones.end())
	{
		it = oldIt;
		while(it != zones.begin())
		{
			--it;
			const Ship *ship = it->GetShip();
			const Outfit *outfit = it->GetOutfit();
			if((ship && HasItem(ship->VariantName())) || (outfit && HasItem(outfit->TrueName())))
			{
				++it;
				break;
			}
		}

		if(it == zones.begin())
		{
			// No displayed objects, apparently.
			selectedShip = nullptr;
			selectedOutfit = nullptr;
			return;
		}
		--it;
	}

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
	MainAutoScroll(it);
}



// The selected item's category has collapsed, to advance to next displayed item.
void ShopPanel::CategoryAdvance(const string &category)
{
	vector<Zone>::const_iterator it = Selected();
	if(it == zones.end())
		return;
	const vector<Zone>::const_iterator oldIt = it;

	// Advance to find next valid selection.
	for( ; it != zones.end(); ++it)
	{
		const Ship *ship = it->GetShip();
		const Outfit *outfit = it->GetOutfit();
		if((ship && ship->Attributes().Category() != category) || (outfit && outfit->Category() != category))
			break;
	}

	// If that didn't work, try the other direction.
	if(it == zones.end())
	{
		it = oldIt;
		while(it != zones.begin())
		{
			--it;
			const Ship *ship = it->GetShip();
			const Outfit *outfit = it->GetOutfit();
			if((ship && ship->Attributes().Category() != category) || (outfit && outfit->Category() != category))
			{
				++it;
				break;
			}
		}

		if(it == zones.begin())
		{
			// No displayed objects, apparently.
			selectedShip = nullptr;
			selectedOutfit = nullptr;
			return;
		}
		--it;
	}

	selectedShip = it->GetShip();
	selectedOutfit = it->GetOutfit();
}



// Find the currently selected item.
vector<ShopPanel::Zone>::const_iterator ShopPanel::Selected() const
{
	vector<Zone>::const_iterator it = zones.begin();
	for( ; it != zones.end(); ++it)
		if(it->GetShip() == selectedShip && it->GetOutfit() == selectedOutfit)
			break;

	return it;
}



// Check if the given point is within the button zone, and if so return the
// letter of the button (or ' ' if it's not on a button).
char ShopPanel::CheckButton(int x, int y)
{
	if(x > Screen::Right() - SIDEBAR_WIDTH - 342 && x < Screen::Right() - SIDEBAR_WIDTH - 316 &&
		y > Screen::Bottom() - 31 && y < Screen::Bottom() - 4)
		return 'f';

	if(x < Screen::Right() - SIDEBAR_WIDTH || y < Screen::Bottom() - BUTTON_HEIGHT)
		return '\0';

	if(y < Screen::Bottom() - 40 || y >= Screen::Bottom() - 10)
		return ' ';

	x -= Screen::Right() - SIDEBAR_WIDTH;
	if(x > 9 && x < 70)
	{
		if(!IsAlreadyOwned())
			return 'b';
		else
			return 'i';
	}
	else if(x > 89 && x < 150)
		return 's';
	else if(x > 169 && x < 240)
		return 'l';

	return ' ';
}



// Called when the outfit disposition dropdown has changed value
void ShopPanel::DispositionChanged(const std::string& value)
{
	// Purchasing to cargo or installing in a ship is controlled by what ships
	// are selected.
	if (value == INSTALL_IN_SHIP || value == MOVE_TO_STORAGE)
	{
		if (playerShips.empty())
		{
			SideSelect(0);
		}
	}
	else if (value == MOVE_TO_CARGO)
	{
		playerShips.clear();
		selectedShip = nullptr;
		playerShip = nullptr;
	}
}
