/* MapSalesPanel.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MapSalesPanel.h"

#include "audio/Audio.h"
#include "CategoryList.h"
#include "CategoryType.h"
#include "Command.h"
#include "DialogPanel.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "ItemInfoDisplay.h"
#include "text/Layout.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "shader/RingShader.h"
#include "Screen.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "Swizzle.h"
#include "System.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>

using namespace std;

const double MapSalesPanel::ICON_HEIGHT = 90.;
const double MapSalesPanel::PAD = 8.;
const int MapSalesPanel::WIDTH = 270;



MapSalesPanel::MapSalesPanel(PlayerInfo &player, bool isOutfitters)
	: MapPanel(player, SHOW_SPECIAL),
	categories(GameData::GetCategory(isOutfitters ? CategoryType::OUTFIT : CategoryType::SHIP)),
	isOutfitters(isOutfitters),
	collapsed(player.Collapsed(isOutfitters ? "outfitter map" : "shipyard map")),
	loadingCircle(30.f, 10, 2.)
{
}



MapSalesPanel::MapSalesPanel(const MapPanel &panel, bool isOutfitters)
	: MapPanel(panel),
	categories(GameData::GetCategory(isOutfitters ? CategoryType::OUTFIT : CategoryType::SHIP)),
	isOutfitters(isOutfitters),
	collapsed(player.Collapsed(isOutfitters ? "outfitter map" : "shipyard map")),
	loadingCircle(30.f, 10, 2.)
{
	Audio::Pause();

	commodity = SHOW_SPECIAL;
}



void MapSalesPanel::Step()
{
	MapPanel::Step();

	loadingCircle.Step();
	// Load any and deferred thumbnails that appear in the sales.
	// This is done here instead of in the constructor because the constructor
	// does not have access to the UI stack.
	if(!hasLoadedThumbnails)
	{
		hasLoadedThumbnails = true;
		LoadCatalogThumbnails();
	}
}



void MapSalesPanel::Draw()
{
	MapPanel::Draw();

	zones.clear();
	hidPrevious = true;

	// Adjust the scroll amount if for some reason the display has changed so
	// that no items are visible.
	scroll = min(0., max(-maxScroll, scroll));

	Information info;
	DrawKey(info);
	DrawPanel();
	DrawItems();
	DrawInfo();
	FinishDrawing(isOutfitters ? "is outfitters" : "is shipyards");
}



bool MapSalesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NONE;
	if(command.Has(Command::HELP))
		DoHelp("map advanced shops", true);
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		scroll += static_cast<double>((Screen::Height() - 100) * ((key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN)));
		scroll = min(0., max(-maxScroll, scroll));
	}
	else if(key == SDLK_HOME)
		scroll = 0;
	else if(key == SDLK_END)
		scroll = -maxScroll;
	else if((key == SDLK_DOWN || key == SDLK_UP) && !zones.empty())
	{
		sound = UI::UISound::NORMAL;
		selected += (key == SDLK_DOWN) - (key == SDLK_UP);
		if(selected < 0)
			selected = zones.size() - 1;
		else if(selected > static_cast<int>(zones.size() - 1))
			selected = 0;

		Compare(compare = -1);
		Select(selected);
		ScrollTo(selected);
	}
	else if(key == 'f')
		GetUI().Push(DialogPanel::RequestString(this, &MapSalesPanel::DoFind, "Search for:"));
	else
		return MapPanel::KeyDown(key, mod, command, isNewPress);

	UI::PlaySound(sound);
	return true;
}



bool MapSalesPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return MapPanel::Click(x, y, button, clicks);

	const Interface *keyInterface = GameData::Interfaces().Get("map: sales key");
	const Rectangle keyContentBox = keyInterface->GetBox("content");
	if(x < Screen::Left() + WIDTH)
	{
		const Point point(x, y);
		const auto zone = find_if(zones.begin(), zones.end(),
			[&](const ClickZone<int> zone){ return zone.Contains(point); });

		if(zone == zones.end())
		{
			Select(selected = -1);
			Compare(compare = -1);
			UI::PlaySound(UI::UISound::NORMAL);
		}
		else if((SDL_GetModState() & KMOD_SHIFT) == 0)
		{
			Select(selected = zone->Value());
			Compare(compare = -1);
			UI::PlaySound(UI::UISound::NORMAL);
		}
		else if(zone->Value() != selected)
		{
			Compare(compare = zone->Value());
			UI::PlaySound(UI::UISound::NORMAL);
		}
	}
	else if(keyContentBox.Contains(Point(x, y)))
	{
		int clickRow = (y - keyContentBox.Top()) / 20;

		// This click was in the map key.
		if(clickRow == 2)
		{
			onlyShowSoldHere = !onlyShowSoldHere;
			onlyShowStorageHere = false;
			UI::PlaySound(UI::UISound::NORMAL);
		}
		else if(clickRow == 3)
		{
			onlyShowSoldHere = false;
			onlyShowStorageHere = !onlyShowStorageHere;
			UI::PlaySound(UI::UISound::NORMAL);
		}
		else
		{
			onlyShowSoldHere = false;
			onlyShowStorageHere = false;
			UI::PlaySound(UI::UISound::NORMAL);
		}
	}
	else
		return MapPanel::Click(x, y, button, clicks);

	return true;
}



// Check to see if the mouse is over the scrolling pane.
bool MapSalesPanel::Hover(int x, int y)
{
	isDragging = (x < Screen::Left() + WIDTH);

	return isDragging || MapPanel::Hover(x, y);
}



bool MapSalesPanel::Drag(double dx, double dy)
{
	if(isDragging)
		scroll = min(0., max(-maxScroll, scroll + dy));
	else
		return MapPanel::Drag(dx, dy);

	return true;
}



bool MapSalesPanel::Scroll(double dx, double dy)
{
	if(isDragging)
		scroll = min(0., max(-maxScroll, scroll + dy * 2.5 * Preferences::ScrollSpeed()));
	else
		return MapPanel::Scroll(dx, dy);

	return true;
}



const Swizzle *MapSalesPanel::SelectedSpriteSwizzle() const
{
	return Swizzle::None();
}



const Swizzle *MapSalesPanel::CompareSpriteSwizzle() const
{
	return Swizzle::None();
}



void MapSalesPanel::DrawKey(Information &info) const
{
	info.SetBar("full", 1.);
	if(onlyShowSoldHere)
		info.SetCondition("only sold here");
	else if(onlyShowStorageHere)
		info.SetCondition("only stored here");

	GameData::Interfaces().Get("map: sales key")->Draw(info);
}



void MapSalesPanel::DrawPanel() const
{
	const Color &back = *GameData::Colors().Get("map side panel background");
	FillShader::Fill(
		Point(Screen::Left() + WIDTH * .5, 0.),
		Point(WIDTH, Screen::Height()),
		back);

	Panel::DrawEdgeSprite(SpriteSet::Get("ui/right edge"), Screen::Left() + WIDTH);
}



void MapSalesPanel::DrawInfo() const
{
	if(selected >= 0)
	{
		const Sprite *left = SpriteSet::Get("ui/left edge");
		const Sprite *bottom = SpriteSet::Get(compare >= 0 ? "ui/bottom edges" : "ui/bottom edge");
		const Sprite *box = SpriteSet::Get(compare >= 0 ? "ui/thumb boxes" : "ui/thumb box");

		const ItemInfoDisplay &selectedInfo = SelectedInfo();
		const ItemInfoDisplay &compareInfo = CompareInfo();
		int height = max<int>(selectedInfo.AttributesHeight(), box->Height());
		int width = selectedInfo.PanelWidth();
		if(compare >= 0)
		{
			height = max(height, compareInfo.AttributesHeight());
			width += box->Width() + compareInfo.PanelWidth();
		}

		const Color &back = *GameData::Colors().Get("map side panel background");
		Point size(width, height);
		Point topLeft(Screen::Right() - size.X(), Screen::Top());
		FillShader::Fill(Rectangle::FromCorner(topLeft, size), back);

		Point leftPos = topLeft + Point(
			-.5 * left->Width(),
			size.Y() - .5 * left->Height());
		SpriteShader::Draw(left, leftPos);
		// The top left corner of the bottom sprite should be 10 x units right
		// of the bottom left corner of the left edge sprite.
		Point bottomPos = leftPos + Point(
			10. + .5 * (bottom->Width() - left->Width()),
			.5 * (left->Height() + bottom->Height()));
		SpriteShader::Draw(bottom, bottomPos);

		if(compare >= 0)
		{
			compareInfo.DrawAttributes(topLeft);
			topLeft.X() += compareInfo.PanelWidth() + box->Width();

			SpriteShader::Draw(box, topLeft + Point(-50., 100.));
			DrawSprite(topLeft + Point(-95., 5.), SelectedSprite(), SelectedSpriteSwizzle());
			DrawSprite(topLeft + Point(-95., 105.), CompareSprite(), CompareSpriteSwizzle());
		}
		else
		{
			SpriteShader::Draw(box, topLeft + Point(-60., 50.));
			DrawSprite(topLeft + Point(-95., 5.), SelectedSprite(), SelectedSpriteSwizzle());
		}
		selectedInfo.DrawAttributes(topLeft);
	}
}



bool MapSalesPanel::DrawHeader(Point &corner, const string &category)
{
	bool hide = collapsed.contains(category);
	if(!hidPrevious)
		corner.Y() += 50.;
	hidPrevious = hide;

	const Sprite *arrow = SpriteSet::Get(hide ? "ui/collapsed" : "ui/expanded");
	SpriteShader::Draw(arrow, corner + Point(15., 25.));

	const Color &textColor = *GameData::Colors().Get(hide ? "medium" : "bright");
	const Font &bigFont = FontSet::Get(18);
	bigFont.Draw(category, corner + Point(30., 15.), textColor);
	AddZone(Rectangle::FromCorner(corner, Point(WIDTH, 40.)), [this, category](){ ClickCategory(category); });
	corner.Y() += 40.;

	return hide;
}



void MapSalesPanel::DrawSprite(const Point &corner, const Sprite *sprite, const Swizzle *swizzle) const
{
	if(!sprite)
		return;
	Point iconOffset(.5 * ICON_HEIGHT, .5 * ICON_HEIGHT);
	if(sprite->IsLoaded())
	{
		double scale = min(.5, min((ICON_HEIGHT - 2.) / sprite->Height(), (ICON_HEIGHT - 2.) / sprite->Width()));

		// No swizzle was specified, so default to the player swizzle.
		if(!swizzle)
			swizzle = GameData::PlayerGovernment()->GetSwizzle();
		SpriteShader::Draw(sprite, corner + iconOffset, scale, swizzle);
	}
	else
		loadingCircle.Draw(corner + iconOffset);
}



void MapSalesPanel::Draw(Point &corner, const Sprite *sprite, const Swizzle *swizzle, bool isForSale,
		bool isSelected, const string &name, const string &variantName,
		const string &price, const string &info, const string &storage)
{
	const Font &font = FontSet::Get(14);
	const Color &selectionColor = *GameData::Colors().Get("item selected");

	// Set the padding so the text takes the same height overall,
	// regardless of whether it's three lines of text or four.
	const auto pad = storage.empty() && variantName.empty() ? PAD : (PAD * 2. / 3.);
	const auto lines = storage.empty() && variantName.empty() ? 3 : 4;
	Point nameOffset(ICON_HEIGHT, .5 * (ICON_HEIGHT - (lines - 1) * pad - lines * font.Height()));
	Point priceOffset(ICON_HEIGHT, nameOffset.Y() + font.Height() + pad);
	Point infoOffset(ICON_HEIGHT, priceOffset.Y() + font.Height() + pad);
	Point storageOffset(ICON_HEIGHT, infoOffset.Y() + font.Height() + pad);
	Point variantOffset = priceOffset;
	if(!variantName.empty())
	{
		priceOffset = infoOffset;
		infoOffset = storageOffset;
	}
	Point blockSize(WIDTH, ICON_HEIGHT);

	if(corner.Y() < Screen::Bottom() && corner.Y() + ICON_HEIGHT >= Screen::Top())
	{
		if(isSelected)
			FillShader::Fill(Rectangle::FromCorner(corner, blockSize), selectionColor);

		DrawSprite(corner, sprite, swizzle);

		const Color &mediumColor = *GameData::Colors().Get("medium");
		const Color &dimColor = *GameData::Colors().Get("dim");
		const Color textColor = isForSale ? mediumColor : storage.empty()
			? dimColor : Color::Combine(.5f, mediumColor, .5f, dimColor);
		auto layout = Layout(static_cast<int>(WIDTH - ICON_HEIGHT - 1), Truncate::BACK);
		font.Draw({name, layout}, corner + nameOffset, textColor);
		if(!variantName.empty())
			font.Draw({"\t" + variantName, layout}, corner + variantOffset, textColor);
		font.Draw({price, layout}, corner + priceOffset, textColor);
		font.Draw({info, layout}, corner + infoOffset, textColor);
		if(!storage.empty())
			font.Draw({storage, layout}, corner + storageOffset, textColor);
	}
	zones.emplace_back(corner + .5 * blockSize, blockSize, zones.size());
	corner.Y() += ICON_HEIGHT;
}



void MapSalesPanel::DoFind(const string &text)
{
	int index = FindItem(text);
	if(index >= 0 && index < static_cast<int>(zones.size()))
	{
		Compare(compare = -1);
		Select(selected = index);
		ScrollTo(index);
	}
}



void MapSalesPanel::ScrollTo(int index)
{
	if(index < 0 || index >= static_cast<int>(zones.size()))
		return;

	const ClickZone<int> &it = zones[selected];
	if(it.Bottom() > Screen::Bottom())
		scroll += Screen::Bottom() - it.Bottom();
	if(it.Top() < Screen::Top())
		scroll += Screen::Top() - it.Top();
}



void MapSalesPanel::ClickCategory(const string &name)
{
	bool isHidden = collapsed.contains(name);
	if(SDL_GetModState() & KMOD_SHIFT)
	{
		// If the shift key is held down, hide or show all categories.
		if(isHidden)
			collapsed.clear();
		else
			for(const auto &category : categories)
				collapsed.insert(category.Name());
	}
	else if(isHidden)
		collapsed.erase(name);
	else
		collapsed.insert(name);
}
