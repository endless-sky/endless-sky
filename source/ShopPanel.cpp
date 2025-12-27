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

#include "text/Alignment.h"
#include "CategoryList.h"
#include "CategoryType.h"
#include "Color.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Mission.h"
#include "shader/OutlineShader.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "Sale.h"
#include "Screen.h"
#include "ScrollBar.h"
#include "ScrollVar.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "UI.h"

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

	constexpr auto ScrollbarMaybeUpdate = [](const auto &op, ScrollBar &scrollbar,
		ScrollVar<double> &scroll, bool animate)
	{
		if(!op(scrollbar))
			return false;
		scrollbar.SyncInto(scroll, animate ? 5 : 0);
		return true;
	};
}



ShopPanel::ShopPanel(PlayerInfo &player, bool isOutfitter)
	: player(player), day(player.GetDate().DaysSinceEpoch()),
	planet(player.GetPlanet()), isOutfitter(isOutfitter), playerShip(player.Flagship()),
	categories(GameData::GetCategory(isOutfitter ? CategoryType::OUTFIT : CategoryType::SHIP)),
	collapsed(player.Collapsed(isOutfitter ? "outfitter" : "shipyard")),
	shipsTooltip(250, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	creditsTooltip(250, Alignment::LEFT, Tooltip::Direction::UP_LEFT, Tooltip::Corner::TOP_RIGHT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	buttonsTooltip(250, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	hover(*GameData::Colors().Get("hover")),
	active(*GameData::Colors().Get("active")),
	inactive(*GameData::Colors().Get("inactive")),
	back(*GameData::Colors().Get("panel background"))
{
	if(playerShip)
		playerShips.insert(playerShip);
	SetIsFullScreen(true);
	SetInterruptible(false);
}



void ShopPanel::Step()
{
	if(!checkedHelp && GetUI()->IsTop(this) && player.Ships().size() > 1)
	{
		if(DoHelp("multiple ships"))
		{
			// Nothing to do here, just don't want to execute the other branch.
		}
		else if(!Preferences::Has("help: shop with multiple ships"))
		{
			set<string> modelNames;
			for(const auto &it : player.Ships())
			{
				if(!CanShowInSidebar(*it, player.GetPlanet()))
					continue;
				if(modelNames.contains(it->DisplayModelName()))
				{
					DoHelp("shop with multiple ships");
					break;
				}
				modelNames.insert(it->DisplayModelName());
			}
		}
		checkedHelp = true;
	}
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

	// Draw the Find button. Note: buttonZones are cleared in DrawButtons.
	const Point findCenter = Screen::BottomRight() - Point(580, 20);
	const Sprite *findIcon = SpriteSet::Get(hoverButton == 'f' ? "ui/find selected" : "ui/find unselected");
	SpriteShader::Draw(findIcon, findCenter);
	buttonZones.emplace_back(Rectangle(findCenter, {findIcon->Width(), findIcon->Height()}), 'f');
	static const string FIND = "_Find";

	shipInfo.DrawTooltips();
	outfitInfo.DrawTooltips();

	if(!shipName.empty())
	{
		string text = shipName;
		if(!warningType.empty())
			text += "\n" + GameData::Tooltip(warningType);
		shipsTooltip.SetText(text, true);
		shipsTooltip.SetBackgroundColor(GameData::Colors().Get(warningType.empty() ? "tooltip background"
			: (warningType.back() == '!' ? "error back" : "warning back")));
		shipsTooltip.Draw(true);
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
			const Swizzle *swizzle = dragShip->CustomSwizzle()
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



void ShopPanel::UpdateTooltipActivation()
{
	shipsTooltip.UpdateActivationCount();
	creditsTooltip.UpdateActivationCount();
	buttonsTooltip.UpdateActivationCount();
}



void ShopPanel::DrawShip(const Ship &ship, const Point &center, bool isSelected)
{
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/shipyard selected" : "ui/shipyard unselected");
	SpriteShader::Draw(back, center);

	const Sprite *thumbnail = ship.Thumbnail();
	const Sprite *sprite = ship.GetSprite();
	const Swizzle *swizzle = ship.CustomSwizzle() ? ship.CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
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
	const string &name = ship.GivenName().empty() ? ship.DisplayModelName() : ship.GivenName();
	Point offset(-SIDEBAR_CONTENT / 2, -.5f * SHIP_SIZE + 10.f);
	font.Draw({name, {SIDEBAR_CONTENT, Alignment::CENTER, Truncate::MIDDLE}},
		center + offset, *GameData::Colors().Get("bright"));
}



void ShopPanel::CheckForMissions(Mission::Location location) const
{
	if(!GetUI()->IsTop(this))
		return;

	Mission *mission = player.MissionToOffer(location);
	if(mission)
		mission->Do(Mission::OFFER, player, GetUI());
	else
		player.HandleBlockedMissions(location, GetUI());
}



int ShopPanel::VisibilityCheckboxesSize() const
{
	return 0;
}



bool ShopPanel::ShouldHighlight(const Ship *ship)
{
	return (hoverButton == 's' || hoverButton == 'r');
}



// Only override the ones you need; the default action is to return false.
bool ShopPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'l' || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		if(!isOutfitter)
			player.UpdateCargoCapacities();
		GetUI()->Pop(this);
		UI::PlaySound(UI::UISound::NORMAL);
	}
	else if(command.Has(Command::HELP))
	{
		if(player.Ships().size() > 1)
		{
			if(isOutfitter)
				DoHelp("outfitter with multiple ships", true);

			set<string> modelNames;
			for(const auto &it : player.Ships())
			{
				if(!CanShowInSidebar(*it, player.GetPlanet()))
					continue;
				if(modelNames.contains(it->DisplayModelName()))
				{
					DoHelp("shop with multiple ships", true);
					break;
				}
				modelNames.insert(it->DisplayModelName());
			}

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
	else if(key == 'k' || (key == 'p' && (mod & KMOD_SHIFT)))
	{
		const Ship *flagship = player.Flagship();
		bool anyEscortUnparked = any_of(playerShips.begin(), playerShips.end(),
			[&](const Ship *ship){ return ship != flagship && !ship->IsParked() && !ship->IsDisabled(); });
		for(const Ship *ship : playerShips)
			if(ship != flagship)
				player.ParkShip(ship, anyEscortUnparked);
	}
	else if(key >= '0' && key <= '9')
	{
		int group = key - '0';
		if(mod & (KMOD_CTRL | KMOD_GUI))
			player.SetEscortGroup(group, &playerShips);
		else if(mod & KMOD_SHIFT)
		{
			// If every single ship in this group is already selected, shift
			// plus the group number means to deselect all those ships.
			set<Ship *> added = player.GetEscortGroup(group);
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
			set<Ship *> wanted = player.GetEscortGroup(group);

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
	{
		TransactionResult result = HandleShortcuts(key);
		if(result.HasMessage())
			GetUI()->Push(new Dialog(result.Message()));
		else if(isOutfitter)
		{
			// Ship-based updates to cargo are handled when leaving.
			// Ship-based selection changes are asynchronous, and handled by ShipyardPanel.
			player.UpdateCargoCapacities();
		}
	}

	CheckSelection();

	return true;
}



bool ShopPanel::Click(int x, int y, MouseButton button, int clicks)
{
	auto ScrollbarClick = [x, y, button, clicks](ScrollBar &scrollbar, ScrollVar<double> &scroll)
	{
		return ScrollbarMaybeUpdate([x, y, button, clicks](ScrollBar &scrollbar)
			{
				return scrollbar.Click(x, y, button, clicks);
			}, scrollbar, scroll, true);
	};
	if(ScrollbarClick(mainScrollbar, mainScroll)
			|| ScrollbarClick(sidebarScrollbar, sidebarScroll)
			|| ScrollbarClick(infobarScrollbar, infobarScroll))
		return true;

	if(button != MouseButton::LEFT)
		return false;

	dragShip = nullptr;

	// Handle clicks on the buttons.
	char zoneButton = CheckButton(x, y);
	if(zoneButton)
		return DoKey(zoneButton);

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
			const Ship *clickedShip = zone.Value();
			for(const shared_ptr<Ship> &ship : player.Ships())
				if(ship.get() == clickedShip)
				{
					dragShip = ship.get();
					dragPoint.Set(x, y);
					SideSelect(dragShip, clicks);
					break;
				}

			return true;
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



bool ShopPanel::Release(int x, int y, MouseButton button)
{
	if(button != MouseButton::LEFT)
		return false;

	dragShip = nullptr;
	isDraggingShip = false;
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
		auto best = next(zones.begin(), index);
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
	// onlyOwned represents that `outfit` is being transferred from Cargo or Storage.

	// If the player is attempting to install an outfit from cargo, storage, or that they just
	// sold to the shop, then ignore its license requirement, if any. (Otherwise there
	// would be no way to use or transfer license-restricted outfits between ships.)
	bool owned = player.Cargo().Get(outfit) || player.Storage().Get(outfit);
	if((owned && onlyOwned) || player.Stock(outfit) > 0)
		return 0;

	const Sale<Outfit> &available = player.GetPlanet()->OutfitterStock();

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

	sidebarScroll.Step();

	// Fill in the background.
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, 0.),
		Point(SIDEBAR_WIDTH, Screen::Height()),
		*GameData::Colors().Get("panel background"));
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
		if(sprite)
		{
			float scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
			if(Preferences::Has(SHIP_OUTLINES))
			{
				Point size(sprite->Width() * scale, sprite->Height() * scale);
				OutlineShader::Draw(sprite, point, size, isSelected ? selected : unselected);
			}
			else
			{
				const Swizzle *swizzle = ship->CustomSwizzle() ? ship->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
				SpriteShader::Draw(sprite, point, scale, swizzle);
			}
		}

		shipZones.emplace_back(point, Point(ICON_TILE, ICON_TILE), ship.get());

		if(mouse.Y() < Screen::Bottom() - ButtonPanelHeight() && shipZones.back().Contains(mouse))
		{
			shipName = ship->GivenName() + (ship->IsParked() ? "\n" + GameData::Tooltip("parked") : "");
			shipsTooltip.SetZone(shipZones.back());
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

	if(playerShip)
	{
		point.Y() += SHIP_SIZE / 2;
		point.X() = (Screen::Right() - SIDEBAR_CONTENT / 2) - SIDEBAR_PADDING;
		DrawShip(*playerShip, point, true);

		Point offset(SIDEBAR_CONTENT / -2, SHIP_SIZE / 2);
		const int detailHeight = DrawPlayerShipInfo(point + offset);
		point.Y() += detailHeight + SHIP_SIZE / 2;
	}
	sidebarScroll.SetDisplaySize(Screen::Height() - ButtonPanelHeight());
	sidebarScroll.SetMaxValue(max(0., point.Y() + sidebarScroll.AnimatedValue() - Screen::Bottom() + Screen::Height()));

	if(sidebarScroll.Scrollable())
	{
		Point top(Screen::Right() - 3, Screen::Top() + 10);
		Point bottom(Screen::Right() - 3, Screen::Bottom() - 80);

		sidebarScrollbar.SyncDraw(sidebarScroll, top, bottom);
	}
}



void ShopPanel::DrawDetailsSidebar()
{
	// Fill in the background.
	const Color &line = *GameData::Colors().Get("dim");
	const Color &back = *GameData::Colors().Get("shop info panel background");

	infobarScroll.Step();

	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH - INFOBAR_WIDTH, 0.),
		Point(1., Screen::Height()),
		line);
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH - INFOBAR_WIDTH / 2, 0.),
		Point(INFOBAR_WIDTH - 1., Screen::Height()),
		back);

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



void ShopPanel::DrawMain()
{
	const Font &bigFont = FontSet::Get(18);
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

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
	// bottom of the screen? (Also leave space for the "key" at the bottom,
	// and a small (10px) amount of space between the last item and the bottom
	// of the screen.)
	mainScroll.SetDisplaySize(Screen::Height());
	mainScroll.SetMaxValue(max(0., nextY + mainScroll.AnimatedValue() - Screen::Height() / 2 - TILE_SIZE / 2 +
		VisibilityCheckboxesSize() + 10.) + Screen::Height());

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



void ShopPanel::SideSelect(Ship *ship, int clicks)
{
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));

	if(shift)
	{
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
				playerShips.insert(other.get());
		}
	}
	else if(!control)
	{
		playerShips.clear();
		if(clicks > 1)
			for(const shared_ptr<Ship> &it : player.Ships())
			{
				if(!CanShowInSidebar(*it, player.GetPlanet()))
					continue;
				if(it.get() != ship && it->Imitates(*ship))
					playerShips.insert(it.get());
			}
	}
	else
	{
		if(clicks > 1)
		{
			vector<Ship *> similarShips;
			// If the ship isn't selected now, it was selected at the beginning of the whole "double click" action,
			// because the first click was handled normally.
			bool unselect = !playerShips.contains(ship);
			for(const shared_ptr<Ship> &it : player.Ships())
			{
				if(!CanShowInSidebar(*it, player.GetPlanet()))
					continue;
				if(it.get() != ship && it->Imitates(*ship))
				{
					similarShips.push_back(it.get());
					unselect &= playerShips.contains(it.get());
				}
			}
			for(Ship *it : similarShips)
			{
				if(unselect)
					playerShips.erase(it);
				else
					playerShips.insert(it);
			}
			if(unselect && find(similarShips.begin(), similarShips.end(), playerShip) != similarShips.end())
			{
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
				CheckSelection();
				return;
			}
		}
		else if(playerShips.contains(ship))
		{
			playerShips.erase(ship);
			if(playerShip == ship)
				playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
			CheckSelection();
			return;
		}
	}

	playerShip = ship;
	playerShips.insert(playerShip);
	CheckSelection();
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



void ShopPanel::DrawButton(const string &name, const Rectangle &buttonShape, bool isActive,
	bool hovering, char keyCode)
{
	const Font &bigFont = FontSet::Get(18);
	const Color *color = !isActive ? &inactive : hovering ? &hover : &active;

	FillShader::Fill(buttonShape, back);
	bigFont.Draw(name, buttonShape.Center() - .5 * Point(bigFont.Width(name), bigFont.Height()), *color);

	// Add this button to the buttonZones:
	buttonZones.emplace_back(buttonShape, keyCode);
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



// Check if the given point is within the button zone, and if so return the letter of the button.
// Returns '\0' if the click is not within the panel, and ' ' if it's within the panel but not on a button.
char ShopPanel::CheckButton(int x, int y)
{
	const Point clickPoint(x, y);

	// Check all the buttonZones.
	for(const ClickZone<char> zone : buttonZones)
		if(zone.Contains(clickPoint))
			return zone.Value();

	if(x < Screen::Right() - SIDEBAR_WIDTH || y < Screen::Bottom() - ButtonPanelHeight())
		return '\0';

	// Returning space here ensures that hover text for the ship info panel is suppressed.
	return ' ';
}
