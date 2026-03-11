/* LoadoutsPanel.cpp
Copyright (c) 2026 by Noelle Devonshire

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "LoadoutsPanel.h"

#include "DataWriter.h"
#include "DialogPanel.h"
#include "Files.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Loadout.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "UI.h"

#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Layout.h"
#include "shader/OutlineShader.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Table.h"

#include <iostream>
#include <ranges>

using namespace std;

namespace {
	constexpr double SETTINGS_MARGINS = 10;

	constexpr int ICON_TILE = 62;
	constexpr float ICON_SIZE = ICON_TILE - 8;
	constexpr int ROW_SIZE = 16;

	const string SHIP_OUTLINES = "Ship outlines in shops";
	const Color SELECTED_COLOR(.8f, 1.f);
	const Color UNSELECTED_COLOR(.4f, 1.f);

	bool IsShipPresent(const shared_ptr<Ship>& ship, const Planet *planet)
	{
		return ship->GetPlanet() == planet;
	}
}



LoadoutsPanel::LoadoutsPanel(PlayerInfo &player, set<Ship*> &playerShips, Sale<Outfit> &outfitter, const int day)
	: player(player),
	playerShips(&playerShips),
	outfitter(&outfitter),
	loadoutPanelUi(GameData::Interfaces().Get("outfitter loadouts panel")),
	loadoutPanelOverlay(GameData::Interfaces().Get("outfitter loadouts overlay")),
	day(day),
	lastClicked(player.Flagship()),
	active(*GameData::Colors().Get("active")),
	bigFont(FontSet::Get(18)),
	smallFont(FontSet::Get(14)),
	loadoutsBox(loadoutPanelUi->GetBox("loadouts")),
	selectedBox(loadoutPanelUi->GetBox("selected")),
	removedBox(loadoutPanelUi->GetBox("removed")),
	settingsBox(loadoutPanelUi->GetBox("settings")),
	cargoBox(loadoutPanelUi->GetBox("cargo")),
	creditsBox(loadoutPanelUi->GetBox("credits")),
	tooltipBox(loadoutPanelUi->GetBox("tooltip")),
	deleteBox(loadoutPanelUi->GetBox("delete")),
	saveBox(loadoutPanelUi->GetBox("save")),
	openBox(loadoutPanelUi->GetBox("open")),
	applyBox(loadoutPanelUi->GetBox("apply")),
	backgroundColor(*GameData::Colors().Get("panel background")),
	dimColor(*GameData::Colors().Get("dim")),
	mediumColor(*GameData::Colors().Get("medium")),
	brightColor(*GameData::Colors().Get("bright")),
	errorColor(*GameData::Colors().Get("heat")),
	faintColor(*GameData::Colors().Get("faint")),
	tooltip(tooltipBox.Width(), Alignment::LEFT, Tooltip::Direction::DOWN_RIGHT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	shipsTooltip(250, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
{
	loadoutScroll.SetDisplaySize(loadoutsBox.Height());
	dataScroll.SetDisplaySize(selectedBox.Height());
	removeScroll.SetDisplaySize(removedBox.Height());

	// Calculate the number of ship rows ahead of time to determine what extension and whether to use a scrollbar.
	const Planet *planet = player.GetPlanet();
	int count = 0;
	rows = 1;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		count += IsShipPresent(ship, planet);
		if(count > ROW_SIZE)
		{
			count -= ROW_SIZE;
			rows++;
		}
	}
	shipsBox = loadoutPanelUi->GetBox("ships" + (rows > 3 ? "3" : to_string(rows)));
	if(rows > 3)
	{
		shipScroll.SetDisplaySize(shipsBox.Height());
		shipScroll.SetMaxValue(max(0., ICON_TILE * rows + 5.));
	}
	buffer = make_unique<RenderBuffer>(Point(shipsBox.Width(), shipsBox.Height()));

	SetInterruptible(false);

	LoadLoadouts();
	RefreshLoadoutsBox();
	RefreshLoadoutData();

	// Calculate the button locations in the settings area.
	// Top row.
	const double boxWidth = (settingsBox.Width() - SETTINGS_MARGINS * 4) / 3;
	double currentSpace = settingsBox.Left();
	double leftBound, rightBound;
	auto nextTop = [&]() -> void {
		leftBound = currentSpace += SETTINGS_MARGINS;
		rightBound = currentSpace += boxWidth;
	};

	nextTop();
	handToHandBox = Rectangle::WithCorners({leftBound, settingsBox.Top() + SETTINGS_MARGINS},
		{rightBound, settingsBox.Center().Y() - SETTINGS_MARGINS / 2});
	nextTop();
	uniqueBox = Rectangle::WithCorners({leftBound, settingsBox.Top() + SETTINGS_MARGINS},
		{rightBound, settingsBox.Center().Y() - SETTINGS_MARGINS / 2});
	nextTop();
	enforceBox = Rectangle::WithCorners({leftBound, settingsBox.Top() + SETTINGS_MARGINS},
		{rightBound, settingsBox.Center().Y() - SETTINGS_MARGINS / 2});

	// Bottom row.
	constexpr double optionBoxWidth = 100;
	const double remainder = settingsBox.Width() - SETTINGS_MARGINS * 5 - optionBoxWidth * 3;
	currentSpace = settingsBox.Left();
	leftBound = currentSpace += SETTINGS_MARGINS;
	rightBound = currentSpace += remainder;
	auto nextBottom = [&]() -> void {
		leftBound = currentSpace += SETTINGS_MARGINS;
		rightBound = currentSpace += optionBoxWidth;
	};

	moveUnequippedBox = Rectangle::WithCorners({leftBound, settingsBox.Center().Y() + SETTINGS_MARGINS / 2},
		{rightBound, settingsBox.Bottom() - SETTINGS_MARGINS});
	nextBottom();
	toStorageBox = Rectangle::WithCorners({leftBound, settingsBox.Center().Y() + SETTINGS_MARGINS / 2},
		{rightBound, settingsBox.Bottom() - SETTINGS_MARGINS});
	nextBottom();
	toCargoBox = Rectangle::WithCorners({leftBound, settingsBox.Center().Y() + SETTINGS_MARGINS / 2},
		{rightBound, settingsBox.Bottom() - SETTINGS_MARGINS});
	nextBottom();
	toOutfitterBox = Rectangle::WithCorners({leftBound, settingsBox.Center().Y() + SETTINGS_MARGINS / 2},
		{rightBound, settingsBox.Bottom() - SETTINGS_MARGINS});

	// Associate the different boxes with tooltip keys.
	tooltipKeys[&loadoutsBox] = "loadouts";
	tooltipKeys[&selectedBox] = "selected";
	tooltipKeys[&removedBox] = "removed";
	tooltipKeys[&cargoBox] = "cargo";
	tooltipKeys[&creditsBox] = "credits";
	tooltipKeys[&uniqueBox] = "unique";
	tooltipKeys[&handToHandBox] = "hand";
	tooltipKeys[&enforceBox] = "enforce";
	tooltipKeys[&moveUnequippedBox] = "move_";
	tooltipKeys[&toStorageBox] = "_tostorage";
	tooltipKeys[&toCargoBox] = "_tocargo";
	tooltipKeys[&toOutfitterBox] = "_tooutfitter";
	tooltipKeys[&deleteBox] = "delete";
	tooltipKeys[&saveBox] = "save";
	tooltipKeys[&openBox] = "open";
	tooltipKeys[&applyBox] = "apply";
}



LoadoutsPanel::~LoadoutsPanel()
= default;



void LoadoutsPanel::Step()
{
	loadoutScroll.Step();
	dataScroll.Step();
	removeScroll.Step();
	shipScroll.Step();
}



void LoadoutsPanel::Draw()
{
	DrawBackdrop();

	Information info;
	if(selectedLoadout)
		info.SetCondition("can delete");
	if(!playerShips->empty())
		info.SetCondition("can save");
	if(selectedLoadout && !playerShips->empty())
		info.SetCondition("can apply");

	info.SetCondition("row" + (rows > 3 ? "3" : to_string(rows)));
	loadoutPanelUi->Draw(info, this);

	DrawShipsModule();
	DrawLoadoutsModule();
	DrawSelectedModule();
	DrawRemovingModule();
	DrawSettingsModule();
	DrawAccountingModule();

	const Interface *overlay = GameData::Interfaces().Get("outfitter loadouts overlay");
	overlay->Draw(info, this);

	DrawScrollbars();

	// Draw tooltips for the button being hovered over.
	if(const string tip = GameData::Tooltip(string("loadout: ") + hoveredTooltip); !tip.empty())
	{
		tooltip.IncrementCount();
		if(tooltip.ShouldDraw())
		{
			tooltip.SetZone(tooltipBox);
			tooltip.SetText(tip, true);
			tooltip.Draw();
		}
	}
	else
		tooltip.DecrementCount();

	if(draggedShip && isDraggingShip && draggedShip->GetSprite())
	{
		const Sprite *sprite = draggedShip->GetSprite();
		const float scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
		if(Preferences::Has(SHIP_OUTLINES))
		{
			const Point size(sprite->Width() * scale, sprite->Height() * scale);
			OutlineShader::Draw(sprite, dragShipPoint, size, SELECTED_COLOR);
		}
		else
		{
			const Swizzle *swizzle = draggedShip->CustomSwizzle()
				? draggedShip->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
			SpriteShader::Draw(sprite, dragShipPoint, scale, swizzle);
		}
	}

	// Draw tooltip for the ship being hovered over.
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
}



bool LoadoutsPanel::Click(const int x, const int y, const MouseButton button, const int clicks)
{
	loadoutDrag = false;
	dataDrag = false;
	removeDrag = false;
	shipScrollDrag = false;

	if(loadoutScroll.Scrollable() && loadoutScrollBar.SyncClick(loadoutScroll, x, y, button, clicks))
	{
		loadoutDrag = true;
		return true;
	}

	if(dataScroll.Scrollable() && dataScrollBar.SyncClick(dataScroll, x, y, button, clicks))
	{
		dataDrag = true;
		return true;
	}

	if(removeScroll.Scrollable() && removeScrollBar.SyncClick(removeScroll, x, y, button, clicks))
	{
		removeDrag = true;
		return true;
	}

	if(shipScroll.Scrollable() && shipScrollBar.SyncClick(shipScroll, x, y, button, clicks))
	{
		shipScrollDrag = true;
		return true;
	}

	// Was click inside loadouts list?
	if(loadoutHovered && hoveredIndex != -1)
	{
		const int previous = selected;
		selected = hoveredIndex;
		if(selected != previous)
			RefreshLoadoutData();
		return true;
	}

	// Check for ship clicked.
	const Point mouse(x, y);
	draggedShip = nullptr;
	if(shipsBox.Contains(mouse))
		for(const ClickZone<const Ship *> &zone : shipZones)
			if(zone.Contains(mouse))
			{
				const Ship *clickedShip = zone.Value();
				for(const shared_ptr<Ship> &ship : player.Ships())
					if(ship.get() == clickedShip)
					{
						draggedShip = ship.get();
						dragShipPoint.Set(x, y);
						ShipSelect(draggedShip, clicks);
						break;
					}
				return true;
			}

	return false;
}



bool LoadoutsPanel::Hover(const int x, const int y)
{
	loadoutHovered = false;
	dataHovered = false;
	removeHovered = false;
	shipHovered = false;

	const Point mouse(x, y);
	hoveredIndex = -1;
	if(loadoutsBox.Contains(mouse))
	{
		loadoutHovered = true;
		if(const int index = (loadoutScroll.AnimatedValue() + mouse.Y() - loadoutsBox.Top()) / 20;
			static_cast<unsigned>(index) < visibleLoadouts.size())
		{
			hoveredIndex = index;
		}
	}
	else if(selectedBox.Contains(mouse))
		dataHovered = true;
	else if(removedBox.Contains(mouse))
		removeHovered = true;
	else if(shipsBox.Contains(mouse))
		shipHovered = true;

	loadoutScrollBar.Hover(x, y);
	dataScrollBar.Hover(x, y);
	removeScrollBar.Hover(x, y);
	shipScrollBar.Hover(x, y);

	hoveredTooltip = "";
	for(const auto &[rectangle, key] : tooltipKeys)
	{
		if(rectangle->Contains(mouse))
		{
			hoveredTooltip = key;
			break;
		}
	}
	return true;
}



// Allow dragging of lists.
bool LoadoutsPanel::Drag(const double dx, const double dy)
{
	if(loadoutDrag)
	{
		if(loadoutScroll.Scrollable() && loadoutScrollBar.SyncDrag(loadoutScroll, dx, dy))
			return true;

		loadoutScroll.Set(loadoutScroll - dy);
		return true;
	}
	if(dataDrag)
	{
		if(dataScroll.Scrollable() && dataScrollBar.SyncDrag(dataScroll, dx, dy))
			return true;

		dataScroll.Set(dataScroll - dy);
		return true;
	}
	if(removeDrag)
	{
		if(removeScroll.Scrollable() && removeScrollBar.SyncDrag(removeScroll, dx, dy))
			return true;

		removeScroll.Set(removeScroll - dy);
		return true;
	}
	if(shipScrollDrag)
	{
		if(shipScroll.Scrollable() && shipScrollBar.SyncDrag(shipScroll, dx, dy))
			return true;

		shipScroll.Set(shipScroll - dy);
		return true;
	}
	if(draggedShip)
	{
		isDraggingShip = true;
		dragShipPoint += Point(dx, dy);
		for(const ClickZone<const Ship *> &zone : shipZones)
			if(zone.Contains(dragShipPoint))
				if(zone.Value() != draggedShip)
				{
					int dragIndex = -1;
					int dropIndex = -1;
					for(unsigned i = 0; i < player.Ships().size(); ++i)
					{
						const Ship *ship = &*player.Ships()[i];
						if(ship == draggedShip)
							dragIndex = i;
						if(ship == zone.Value())
							dropIndex = i;
					}
					if(dragIndex >= 0 && dropIndex >= 0)
						player.ReorderShip(dragIndex, dropIndex);
				}
		return true;
	}
	return false;
}



// The scroll wheel can be used to scroll hovered lists.
bool LoadoutsPanel::Scroll(const double dx, const double dy)
{
	const bool wasDragLoadout = loadoutDrag;
	loadoutDrag = loadoutHovered;
	const bool wasDragData = dataDrag;
	dataDrag = dataHovered;
	const bool wasDragRemove = removeDrag;
	removeDrag = removeHovered;
	const bool wasDragShipsScroll = shipScrollDrag;
	shipScrollDrag = shipHovered;

	const bool didDrag = Drag(0., dy * Preferences::ScrollSpeed());

	loadoutDrag = wasDragLoadout;
	dataDrag = wasDragData;
	removeDrag = wasDragRemove;
	shipScrollDrag = wasDragShipsScroll;
	return didDrag;
}



bool LoadoutsPanel::KeyDown(const SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	// Delete.
	if(key == 'e' && selectedLoadout)
	{
		hoveredTooltip = "";
		GetUI().Push(DialogPanel::CallFunctionIfOk(this, &LoadoutsPanel::DeleteLoadout,
			"Are you sure you want to delete the selected loadout, \"" + selectedLoadout->Name() + "\"?"));
		return true;
	}
	// Save.
	if(key == 's' && !playerShips->empty())
	{
		hoveredTooltip = "";
		nameToConfirm.clear();
		const Ship *ship = *playerShips->begin();
		GetUI().Push(DialogPanel::RequestString(this, &LoadoutsPanel::SaveLoadout,
			"Creating a loadout from your ship \"" + ship->GivenName() + "\".\nWhat do you want to name it?"));
		return true;
	}
	// Apply.
	if(key == 'a' && selectedLoadout && !playerShips->empty())
	{
		hoveredTooltip = "";
		GetUI().Push(DialogPanel::CallFunctionIfOk(this, &LoadoutsPanel::ApplyLoadout,
			"Are you sure you want to apply the loadout \"" + selectedLoadout->Name() + "\" to your selected ship(s)?"));
		return true;
	}
	// Done.
	if(key == 'l' || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		GetUI().Pop(this);
		return true;
	}
	// Open folder.
	if(key == 'o')
	{
		Files::OpenUserLoadoutsFolder();
		return true;
	}
	return false;
}



bool LoadoutsPanel::Release(int x, int y, const MouseButton button)
{
	if(draggedShip && button == MouseButton::LEFT)
	{
		draggedShip = nullptr;
		isDraggingShip = false;
		return true;
	}
	return false;
}



void LoadoutsPanel::LoadLoadouts()
{
	// Should these be cached or loaded/discarded as needed?
	loadouts.clear();

	for(const auto &path : Files::List(Files::Loadouts()))
	{
		// Skip any files that aren't text files.
		if(path.extension() != ".txt")
			continue;

		// Skip any files that didn't result in a valid loadout.
		if(Loadout *loaded = Loadout::Load(path))
			loadouts.push_back(loaded);
	}
}



void LoadoutsPanel::CheckLoadoutSelected()
{
	try {
		selectedLoadout = visibleLoadouts.at(selected);
	}
	catch([[maybe_unused]] const out_of_range &ex)
	{
		selectedLoadout = nullptr;
	}
}



void LoadoutsPanel::RefreshLoadoutsBox()
{
	visibleLoadouts.clear();
	vector<string> selectedModels;
	if(enforceShipTypes)
	{
		for(const Ship *ship : *playerShips)
		{
			if(ranges::find(selectedModels, ship->TrueModelName()) == selectedModels.end())
			{
				selectedModels.push_back(ship->TrueModelName());
			}
		}
	}

	for(Loadout *loadout : loadouts)
	{
		// Filter mismatched loadouts if setting enabled.
		if(enforceShipTypes && ranges::find(selectedModels, loadout->ShipModel()) == selectedModels.end()) continue;

		visibleLoadouts.push_back(loadout);
	}
	loadoutScroll.SetMaxValue(max(0., 20. * visibleLoadouts.size() + 10.));

	const Loadout *previous = selectedLoadout;
	CheckLoadoutSelected();
	if(previous != selectedLoadout)
		RefreshLoadoutData();
}



void LoadoutsPanel::RefreshLoadoutData()
{
	loadoutListings.clear();
	storageChange = 0;
	cargoChange = 0;
	loadoutSales = 0;
	CheckLoadoutSelected();

	if(selectedLoadout)
	{

		vector<Ship*> filteredShips;
		for(Ship *ship : *playerShips)
		{

			// Filter mismatched ships if setting enabled.
			if(enforceShipTypes && ship->TrueModelName() != selectedLoadout->ShipModel()) continue;

			filteredShips.push_back(ship);
		}

		for(const auto &[outfit, amount] : selectedLoadout->Outfits())
			if(outfit->IsDefined() && !outfit->Category().empty() && !outfit->DisplayName().empty())
			{
				// Filter unique outfits if setting is set.
				if(!includeUnique && outfit->Category() == "Unique") continue;

				// Filter hand-to-hand outfits if setting is set.
				if(!includeHandToHand && outfit->Category() == "Hand to Hand") continue;

				const int amountToSource = amount * filteredShips.size();
				int stillNeeded = amountToSource;
				int equipped = 0;
				for(const Ship *ship : filteredShips)
				{

					const int onShip = ship->OutfitCount(outfit);
					if(onShip >= stillNeeded)
					{
						equipped += stillNeeded;
						stillNeeded = 0;
						break;
					}
					equipped += onShip;
					stillNeeded -= onShip;
				}

				int cargo;
				if(const int inCargo = player.Cargo().Get(outfit); inCargo >= stillNeeded)
				{
					cargo = stillNeeded;
					stillNeeded = 0;
				}
				else {
					cargo = inCargo;
					stillNeeded -= inCargo;
				}
				cargoChange -= outfit->Mass() * cargo;

				int storage;
				if(const int inStorage = player.Storage().Get(outfit); inStorage >= stillNeeded)
				{
					storage = stillNeeded;
					stillNeeded = 0;
				}
				else {
					storage = inStorage;
					stillNeeded -= inStorage;
				}
				storageChange -= outfit->Mass() * storage;

				const int stock = outfitter->Has(outfit) ?
					stillNeeded : min<int>(stillNeeded, max<int>(0, player.Stock(outfit)));
				loadoutSales += player.StockDepreciation().Value(outfit, day, stock);
				stillNeeded -= stock;

				const auto sources = new OutfitSources();
				sources->byLoadout = to_string(amount) +
					(amount != amountToSource ? "(" + to_string(amountToSource) + ")" : "");
				sources->installed = equipped > 0 ? to_string(equipped) : "-";
				sources->inCargo = cargo > 0 ? to_string(cargo) : "-";
				sources->inStorage = storage > 0 ? to_string(storage) : "-";
				sources->fromOutfitter = stock > 0 ? to_string(stock) : "-";
				sources->stillRequired = stillNeeded > 0 ? to_string(stillNeeded) : "-";
				loadoutListings[outfit->Category()][outfit] = sources;
			}

		double dataScrollRoom = loadoutListings.size() * 30. + 10;
		for(const auto &content : loadoutListings | views::values)
			dataScrollRoom += content.size() * 20.;
		dataScroll.SetMaxValue(max(0., dataScrollRoom));

		removalListings.clear();
		map<const Outfit*, int> toRemove;
		for(const Ship *ship : filteredShips)
		{
			for(pair<const Outfit*, int> outfit : ship->Outfits())
			{
				// Filter unique outfits if setting is set,
				if(!includeUnique && outfit.first->Category() == "Unique") continue;

				// Filter hand-to-hand outfits if setting is set.
				if(!includeHandToHand && outfit.first->Category() == "Hand to Hand") continue;

				toRemove[outfit.first] += outfit.second;
			}
		}
		for(pair<const Outfit*, int> outfit : selectedLoadout->Outfits())
		{
			if(toRemove.contains(outfit.first))
			{
				if(const int toInstall = outfit.second * filteredShips.size(); toRemove[outfit.first] > toInstall)
					toRemove[outfit.first] -= toInstall;
				else
					toRemove.erase(outfit.first);
			}
		}

		for(const auto &[outfit, amount] : toRemove)
			if(outfit->IsDefined() && !outfit->Category().empty() && !outfit->DisplayName().empty())
			{
				removalListings[outfit->Category()][outfit] = new string(to_string(amount));
				switch(loadoutDestination)
				{
					case CARGO:
						cargoChange += amount * outfit->Mass();
						break;
					case STORAGE:
						storageChange += amount * outfit->Mass();
						break;
					default:
						loadoutSales -= player.FleetDepreciation().Value(outfit, day, amount);
						break;
				}
			}

		double removeScrollRoom = removalListings.size() * 30. + 10;
		for(const auto &content : removalListings | views::values)
			removeScrollRoom += content.size() * 20.;
		removeScroll.SetMaxValue(max(0., removeScrollRoom));
	}
}



void LoadoutsPanel::SaveLoadout(const string &name)
{
	if(!playerShips->empty())
	{

		const Ship *ship = *playerShips->begin();
		if(Loadout::Exists(name) && name != nameToConfirm)
		{
			nameToConfirm = name;
			GetUI().Push(DialogPanel::RequestString(this, &LoadoutsPanel::SaveLoadout, "Warning: \"" + name
			+ "\" is being used for an existing snapshot.\nOverwrite it?", name));
		}
		else {
			map<const Outfit*, int> outfits;
			for(auto [outfit, amount] : ship->Outfits())
			{
				// Filter unique outfits if setting is set.
				if(!includeUnique && outfit->Category() == "Unique") continue;

				// Filter hand-to-hand outfits if setting is set.
				if(!includeHandToHand && outfit->Category() == "Hand to Hand") continue;

				outfits[outfit] = amount;
			}
			if(const auto copied = new Loadout(name, ship->TrueModelName(), outfits);
				copied->Save())
			{
				if(nameToConfirm == name)
					erase_if(loadouts, [name](const Loadout *p)
					{
						return p->Name() == name;
					});
				loadouts.push_back(copied);
				RefreshLoadoutsBox();
				GetUI().Push(DialogPanel::Info("The loadout \"" + name + "\" was successfully created."));
			}
			else
				GetUI().Push(DialogPanel::Info("Could not create a file for the loadout \"" + name + "\"."));
		}
	}
	else {
		// This shouldn't be reachable.
		GetUI().Push(DialogPanel::Info("You do not have any ships selected."));
	}
}



void LoadoutsPanel::DeleteLoadout()
{
	if(selectedLoadout)
	{
		if(!selectedLoadout->Delete())
			GetUI().Push(DialogPanel::Info("Failed to delete loadout \"" + selectedLoadout->Name() + "\"."));
		else {
			GetUI().Push(DialogPanel::Info("The loadout \"" +
				selectedLoadout->Name() + "\" was successfully deleted."));
			loadouts.erase(ranges::remove(loadouts, selectedLoadout).begin(), loadouts.end());
			visibleLoadouts.erase(ranges::remove(visibleLoadouts, selectedLoadout).begin(),
				visibleLoadouts.end());
		}
		RefreshLoadoutsBox();
	}
}



void LoadoutsPanel::ApplyLoadout()
{
	// Keep track of outfits that couldn't be added or removed.
	map<Ship*, vector<string>> errors;

	// We will want to add everything to cargo after ships are equipped appropriately to better guarantee proper
	// Interaction with things like outfit/cargo expansions, as well as outfits being removed from cargo for the
	// Incoming loadout. So, cache the results and apply cargo changes after everything else.
	map<const Outfit*, int> toCargo;

	// First, remove all outfits on all ships to guarantee as much cash as possible.
	for(Ship *ship : *playerShips)
	{

		// Skip ships not belonging to the loadout if the enforce option is toggled.
		if(enforceShipTypes && ship->TrueModelName() != selectedLoadout->ShipModel()) continue;

		// Because outfits must be removed from a ship using AddOutfit to keep attributes correct,
		// Copy over a list of outfits to remove manually to bypass possible concurrent modification issues.
		vector<const Outfit*> installed;
		for(pair<const Outfit*, int> outfit : ship->Outfits())
		{
			// Filter unique outfits if setting is set
			if(!includeUnique && outfit.first->Category() == "Unique") continue;

			// Filter hand-to-hand outfits if setting is set.
			if(!includeHandToHand && outfit.first->Category() == "Hand to Hand") continue;

			installed.push_back(outfit.first);
		}

		// Sort by greater outfit space required to ensure outfit expansions can be removed last.
		ranges::stable_sort(installed, [](const Outfit *lhs, const Outfit *rhs)
		{
			return lhs->Get("outfit space") < rhs->Get("outfit space");
		});
		// Note: Since outfit space is stored as a negative number to denote how it affects your ship once installed,
		// Sorts have to be reversed from what's expected. Therefore, since we want the "greater" outfit space required
		// To be sorted first on the list, use "less than".

		// Sort by lesser outfit space on ammunition specifically so that missiles are removed before racks
		ranges::stable_sort(installed, [](const Outfit *lhs, const Outfit *rhs)
		{
			const bool leftAmmo = lhs->Category() == "Ammunition";
			const bool rightAmmo = rhs->Category() == "Ammunition";
			if(leftAmmo && !rightAmmo) return true;
			if(!leftAmmo && rightAmmo) return false;
			if(leftAmmo)
				return lhs->Get("outfit space") > rhs->Get("outfit space");
			return false;
		});

		for(const Outfit *outfit : installed)
		{
			// Do not remove outfits that are wanted on the loadout.
			int wanted = 0;
			if(selectedLoadout->Outfits().contains(outfit))
				wanted = selectedLoadout->Outfits()[outfit];
			const int equipped = ship->OutfitCount(outfit);
			if(equipped <= wanted)
				continue;
			const int toRemove = equipped - wanted;

			const int amount = ship->Attributes().CanAdd(*outfit, -toRemove);
			if(-amount != toRemove)
				errors[ship].push_back("	Could not remove " + to_string(toRemove + amount) +
					" \"" + outfit->DisplayName() + "\".");
			if(amount < 0)
			{
				// Uninstall the outfit.
				ship->AddOutfit(outfit, amount);
				// Adjust hired crew counts.
				if(const int required = outfit->Get("required crew"))
					ship->AddCrew(required * amount);
				ship->Recharge();

				// Send the outfit to the appropriate destination.
				const int toAdd = -amount;
				switch(loadoutDestination)
				{
					case STORAGE:
						player.Storage().Add(outfit, toAdd);
						break;
					case CARGO:
						toCargo[outfit] += toAdd;
						break;
					default:
						const int64_t price = player.FleetDepreciation().Value(outfit, day, toAdd);
						player.Accounts().AddCredits(price);
						player.AddStock(outfit, toAdd);
						break;
				}
			}
		}
	}


	// Then, install outfits as appropriate on all ships.
	for(Ship *ship : *playerShips)
	{

		// Skip ships not belonging to the loadout if the enforce option is toggled
		if(enforceShipTypes && ship->TrueModelName() != selectedLoadout->ShipModel()) continue;

		// Just in case the loadout, for whatever reason, can't be fully applied, apply some sorts to the list.
		// Convert the map to a list of vectors so arbitrary sorts and filters can be used.
		vector<pair<const Outfit*, int>> outfits;
		for(pair<const Outfit*, int> outfit : selectedLoadout->Outfits())
		{

			// Filter unique outfits if setting is set
			if(!includeUnique && outfit.first->Category() == "Unique") continue;

			// Filter hand-to-hand outfits if setting is set
			if(!includeHandToHand && outfit.first->Category() == "Hand to Hand") continue;

			outfits.push_back(outfit);
		}

		// Sort once by outfit space required.
		ranges::stable_sort(outfits, [](const pair<const Outfit*, int> &lhs,
			const pair<const Outfit*, int> &rhs)
		{
			return lhs.first->Get("outfit space") > rhs.first->Get("outfit space");
		});

		// Then sort by an arbitrary category order.
		enum SortCategory {Systems, Power, Engines, Weapons, Other};
		auto getSort = [](const string &category)
		{
			if(category == "Systems") return Systems;
			if(category == "Power") return Power;
			if(category == "Engines") return Engines;
			if(category == "Guns" || category == "Turrets" || category == "Secondary Weapons") return Weapons;
			return Other;
		};
		ranges::stable_sort(outfits, [&getSort](const pair<const Outfit*, int> &lhs,
			const pair<const Outfit*, int> &rhs)
		{
			return getSort(lhs.first->Category()) < getSort(rhs.first->Category());
		});

		// Sort by greater outfit space on ammunition so that racks are added before ammo.
		ranges::stable_sort(outfits, [](const pair<const Outfit*, int> &lhs,
			const pair<const Outfit*, int> &rhs)
		{
			const bool leftAmmo = lhs.first->Category() == "Ammunition";
			const bool rightAmmo = rhs.first->Category() == "Ammunition";
			if(leftAmmo && !rightAmmo) return false;
			if(!leftAmmo && rightAmmo) return true;
			if(leftAmmo)
				return lhs.first->Get("outfit space") < rhs.first->Get("outfit space");
			return false;
		});

		// Finally, actually apply the outfits.
		for(auto [outfit, toInstall] : outfits)
		{
			// Add fewer outfits if some are already installed.
			toInstall -= ship->OutfitCount(outfit);
			if(toInstall > 0)
			{
				bool errorEncountered = false;
				int actual = ship->Attributes().CanAdd(*outfit, toInstall);
				if(actual != toInstall)
				{
					errorEncountered = true;
					errors[ship].push_back("	Could not install \"" + outfit->DisplayName() + "\".");
					int diff = toInstall - actual;
					errors[ship].push_back("		Cannot fit " + to_string(diff) + (diff > 1 ? "units." : "unit."));
				}
				if(actual > 0)
				{
					// Fill first from any stockpiles in storage.
					const int fromStorage = player.Storage().Remove(outfit, actual);
					actual -= fromStorage;
					// Then from cargo.
					const int fromCargo = player.Cargo().Remove(outfit, actual);
					actual -= fromCargo;
					// Then, buy at appropriately depreciated price.
					int available = outfitter->Has(outfit) ?
						actual : min<int>(actual, max<int>(0, player.Stock(outfit)));
					int notAfford = 0;
					if(available > 0)
					{
						int64_t price = player.StockDepreciation().Value(outfit, day, available);
						// If necessary, trim the amount down to what the player can afford.
						while(price > player.Accounts().Credits() && available > 0)
						{
							++notAfford;
							if(--available > 0)
								price = player.StockDepreciation().Value(outfit, day, available);
						}
						if(notAfford > 0)
						{
							if(!errorEncountered)
								errors[ship].push_back("	Could not install \"" + outfit->DisplayName() + "\".");
							errors[ship].push_back("		Could not afford " + to_string(notAfford) +
								(notAfford > 1 ? " units." : " unit."));
						}
						if(available > 0)
						{
							player.Accounts().AddCredits(-price);
							player.AddStock(outfit, -available);
						}
					}
					actual -= available;
					ship->AddOutfit(outfit, available + fromStorage + fromCargo);
					if(actual > 0 && notAfford == 0)
					{
						if(!errorEncountered)
							errors[ship].push_back("	Could not install \"" + outfit->DisplayName() + "\".");
						errors[ship].push_back("		" + to_string(actual) + (actual > 1 ? " units " : " unit ") +
							"not in stock.");
					}
				}
			}
		}
	}


	// Finally, send any outfits needed to cargo until capacity is reached, and the rest to storage.
	for(auto [outfit, amount] : toCargo)
	{
		amount -= player.Cargo().Add(outfit, amount);
		if(amount > 0)
			player.Storage().Add(outfit, amount);
	}


	// Describe errors, if any, to the player.
	if(errors.empty())
		GetUI().Push(DialogPanel::Info("The loadout \"" + selectedLoadout->Name() +
			"\" was successfully applied to all applicable selected ships."));
	else {
		string message;
		for(const auto &[ship, errorList] : errors)
		{
			message += ship->GivenName() + "\n";
			for(const string &error : errorList)
				message += error + "\n";
		}
		GetUI().Push(DialogPanel::Info(message));
	}

	RefreshLoadoutData();
}

void LoadoutsPanel::DrawShipsModule()
{
	shipZones.clear();
	shipName.clear();
	warningType.clear();

	// The buffer affects where UI thinks the mouse is, so grab this ahead of time.
	const Point mouse = UI::GetMouse();
	// The buffer draws at different coordinates than the game, so create an offset.
	const Point offset(shipsBox.Left() - buffer->Left(), shipsBox.Top() - buffer->Top());

	auto target = buffer->SetTarget();
	const Point topLeft(buffer->Left() + 5 + ICON_TILE / 2, buffer->Top() + 5 + ICON_TILE / 2);
	Point drawPoint(topLeft - Point(0, shipScroll));
	const Planet *planet = player.GetPlanet();
	const Sprite *selectedShip = SpriteSet::Get("ui/icon selected");
	const Sprite *unselectedShip = SpriteSet::Get("ui/icon unselected");
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip any ships that are "absent" for whatever reason.
		if(!IsShipPresent(ship, planet))
			continue;

		if(drawPoint.X() > buffer->Right())
		{
			drawPoint.X() = topLeft.X();
			drawPoint.Y() += ICON_TILE;
			if(drawPoint.Y() - ICON_SIZE / 2 > buffer->Bottom())
				break;
		}

		if(drawPoint.Y() + ICON_SIZE / 2 > buffer->Top())
		{
			const bool isSelected = playerShips->contains(ship.get());
			SpriteShader::Draw(isSelected ? selectedShip : unselectedShip, drawPoint);
			// Unlike ShopPanel, highlight ships based on apply or presets being hovered,
			// as well as the status of the ship type enforce setting.
			if(ShouldHighlight(ship.get(), mouse, isSelected))
				SpriteShader::Draw(selectedShip, drawPoint);

			if(const Sprite *sprite = ship->GetSprite())
			{
				const float scale = ICON_SIZE / max(sprite->Width(), sprite->Height());
				if(Preferences::Has(SHIP_OUTLINES))
				{
					Point size(sprite->Width() * scale, sprite->Height() * scale);
					OutlineShader::Draw(sprite, drawPoint, size, isSelected ? SELECTED_COLOR : UNSELECTED_COLOR);
				}
				else
				{
					const Swizzle *swizzle = ship->CustomSwizzle() ?
						ship->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
					SpriteShader::Draw(sprite, drawPoint, scale, swizzle);
				}
			}

			shipZones.emplace_back(drawPoint + offset, Point(ICON_TILE, ICON_TILE), ship.get());

			if(shipZones.back().Contains(mouse) && shipsBox.Contains(mouse))
			{
				shipName = ship->GivenName() + (ship->IsParked() ? "\n" + GameData::Tooltip("parked") : "");
				shipsTooltip.SetZone(shipZones.back());
			}
			const auto flightChecks = player.FlightCheck();

			if(const auto checkIt = flightChecks.find(ship); checkIt != flightChecks.end())
			{
				const string &check = checkIt->second.front();
				const Sprite *icon = SpriteSet::Get(check.back() == '!' ? "ui/error" : "ui/warning");
				SpriteShader::Draw(icon, drawPoint + .5 * Point(ICON_TILE - icon->Width(), ICON_TILE - icon->Height()));
				if(shipZones.back().Contains(mouse))
					warningType = check;
			}

			if(ship->IsParked())
			{
				static const Point CORNER = .35 * Point(ICON_TILE, ICON_TILE);
				FillShader::Fill(drawPoint + CORNER, Point(6., 6.), backgroundColor);
				FillShader::Fill(drawPoint + CORNER, Point(4., 4.), isSelected ? brightColor : mediumColor);
			}
		}

		drawPoint.X() += ICON_TILE;
	}

	target.Deactivate();
	buffer->Draw(shipsBox.Center(), shipsBox.Dimensions(), {0, 0});

}


void LoadoutsPanel::DrawLoadoutsModule() const
{
	FillShader::Fill(loadoutsBox, backgroundColor);

	int index = loadoutScroll.AnimatedValue() / 20.;
	int y = loadoutsBox.Top() - loadoutScroll.AnimatedValue() + 20 * index; // + 10
	const int endY = loadoutsBox.Bottom();

	// Y offset to center the text in a 20-pixel high row.
	const double fontOff = .5 * (20 - smallFont.Height());

	for( ; y < endY && static_cast<unsigned>(index) < visibleLoadouts.size(); y += 20, ++index)
	{
		const Loadout *item = visibleLoadouts[index];

		// Check if this is the selected row.
		if(index == selected)
			FillShader::Fill(
				Point(loadoutsBox.Center().X(), y + 10.), Point(loadoutsBox.Width(), 20.), dimColor);

		Point pos(loadoutsBox.Left() + 10, y + fontOff);
		smallFont.Draw(DisplayText(item->Name(),
			Layout(static_cast<int>(loadoutsBox.Width()) - 25, Alignment::LEFT, Truncate::BACK)), pos, brightColor);
	}
}



void LoadoutsPanel::DrawSelectedModule()
{
	FillShader::Fill(selectedBox, backgroundColor);

	if(selectedLoadout)
	{
		Table table;
		// Use 10-pixel margins on both sides, then split the box by 70 pixels per number column, remainder to outfit
		const int boxWidth = static_cast<int>(selectedBox.Width());
		const int available = boxWidth - 20;
		constexpr int columnSize = 70;
		table.AddColumn(10, Layout(available - columnSize * 6));
		table.AddColumn(boxWidth - 10 - columnSize * 5, Layout(columnSize, Alignment::RIGHT));
		table.AddColumn(boxWidth - 10 - columnSize * 4, Layout(columnSize, Alignment::RIGHT));
		table.AddColumn(boxWidth - 10 - columnSize * 3, Layout(columnSize, Alignment::RIGHT));
		table.AddColumn(boxWidth - 10 - columnSize * 2, Layout(columnSize, Alignment::RIGHT));
		table.AddColumn(boxWidth - 10 - columnSize, Layout(columnSize, Alignment::RIGHT));
		table.AddColumn(boxWidth - 10, Layout(columnSize, Alignment::RIGHT));

		table.SetHighlight(0, boxWidth);
		const int topCutoff = selectedBox.Top() - 15;
		table.DrawAt(Point(selectedBox.Left(), topCutoff));
		double currentY = selectedBox.Top() - dataScroll.AnimatedValue();
		const int bottomCutoff = selectedBox.Bottom() - 5;

		// Align with the scroll bar progress by creating a table gap proportional to animated value.
		bool firstDraw = false;
		auto addProgress = [&](const double amount) -> void {
			currentY += amount;
			if(!firstDraw && currentY >= topCutoff)
			{
				table.DrawGap(currentY - topCutoff);
				firstDraw = true;
			}
		};

		for(const auto &[category, content] : loadoutListings)
		{
			if(currentY >= bottomCutoff) break;
			if(firstDraw && currentY >= topCutoff)
			{
				// Empty rows between categories.
				table.DrawGap(10);
			}
			addProgress(10.);

			if(currentY >= bottomCutoff) break;
			if(currentY >= topCutoff)
			{
				// Category listings.
				table.Draw(category + ":", mediumColor);
				// Empty data cells on category listings.
				for(int i = 0; i < 6; ++i) table.Draw("");
			}
			addProgress(20.);

			for(auto [outfit, sources] : content)
			{
				if(currentY >= bottomCutoff) break;
				if(currentY >= topCutoff)
				{
					// Outfit name.
					table.Draw(outfit->DisplayName(), mediumColor);

					// Amount required.
					table.Draw(sources->byLoadout, brightColor);

					// Amount currently equipped.
					string value = sources->installed;
					table.Draw(value, value == "-" ? dimColor : brightColor);

					// Amount from cargo.
					value = sources->inCargo;
					table.Draw(value, value == "-" ? dimColor : brightColor);

					// Amount from storage.
					value = sources->inStorage;
					table.Draw(value, value == "-" ? dimColor : brightColor);

					// Amount purchased.
					value = sources->fromOutfitter;
					table.Draw(value, value == "-" ? dimColor : brightColor);

					// Draw the amount unable to source in red, if exists.
					value = sources->stillRequired;
					table.Draw(value, value == "-" ? dimColor : errorColor);
				}
				addProgress(20.);
			}
		}
	}
}



void LoadoutsPanel::DrawRemovingModule()
{
	FillShader::Fill(removedBox, backgroundColor);

	if(selectedLoadout)
	{
		Table table;

		// Use 10-pixel margins on both sides, then split the box by 90% for text, then 10% for the number
		const int available = removedBox.Width() - 20;
		table.AddColumn(10, Layout(available * .9));
		table.AddColumn(removedBox.Width() - 10, Layout(available * 0.1, Alignment::RIGHT));

		table.SetHighlight(0, removedBox.Width());
		const int topCutoff = removedBox.Top() - 15;
		table.DrawAt(Point(removedBox.Left(), topCutoff));
		double currentY = selectedBox.Top() - removeScroll.AnimatedValue();
		const int bottomCutoff = removedBox.Bottom() - 5;

		// Align with the scroll bar progress by creating a table gap proportional to animated value.
		bool firstDraw = false;
		auto addProgress = [&](const double amount) -> void {
			currentY += amount;
			if(!firstDraw && currentY >= topCutoff)
			{
				table.DrawGap(currentY - topCutoff);
				firstDraw = true;
			}
		};

		for(const auto &[category, content] : removalListings)
		{

			if(currentY >= bottomCutoff) break;
			if(firstDraw && currentY >= topCutoff)
			{
				// 10px margin between categories.
				table.DrawGap(10);
			}
			addProgress(10.);

			if(currentY >= bottomCutoff) break;
			if(currentY >= topCutoff)
			{
				// Category listings.
				table.Draw(category + ":", mediumColor);
				table.Draw("");
			}
			addProgress(20.);

			// All outfits in the category.
			for(auto [outfit, amount] : content)
			{
				if(currentY >= bottomCutoff) break;
				if(currentY >= topCutoff)
				{
					table.Draw(outfit->DisplayName(), mediumColor);
					table.Draw(*amount, brightColor);
				}
				addProgress(20.);
			}
		}
	}
}



void LoadoutsPanel::DrawSettingsModule()
{
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};
	const double fontOff = .5 * smallFont.Height();

	// Top row.
	AddZone(handToHandBox, [this] {
		includeHandToHand = !includeHandToHand;
		RefreshLoadoutData();
	});
	FillShader::Fill(handToHandBox, faintColor);
	SpriteShader::Draw(box[includeHandToHand ? 1 : 0],
		{handToHandBox.Right() - SETTINGS_MARGINS, handToHandBox.Center().Y()});
	smallFont.Draw({"Include Hand to Hand"},
		{handToHandBox.Left() + SETTINGS_MARGINS / 2, handToHandBox.Center().Y() - fontOff}, brightColor);

	AddZone(uniqueBox, [this] {
		includeUnique = !includeUnique;
		RefreshLoadoutData();
	});
	FillShader::Fill(uniqueBox, faintColor);
	SpriteShader::Draw(box[includeUnique ? 1 : 0],
		{uniqueBox.Right() - SETTINGS_MARGINS, uniqueBox.Center().Y()});
	smallFont.Draw({"Include Uniques"},
		{uniqueBox.Left() + SETTINGS_MARGINS / 2, uniqueBox.Center().Y() - fontOff}, brightColor);

	AddZone(enforceBox, [this] {
		enforceShipTypes = !enforceShipTypes;
		RefreshLoadoutsBox();
		RefreshLoadoutData();
	});
	FillShader::Fill(enforceBox, faintColor);
	SpriteShader::Draw(box[enforceShipTypes ? 1 : 0],
		{enforceBox.Right() - SETTINGS_MARGINS, enforceBox.Center().Y()});
	smallFont.Draw({"Strict Ship Match"},
		{enforceBox.Left() + SETTINGS_MARGINS / 2, enforceBox.Center().Y() - fontOff}, brightColor);

	// Bottom row.
	AddZone(moveUnequippedBox, [this] {
		int index = loadoutDestination;
		if(++index >= destinationsSize) index = 0;
		loadoutDestination = static_cast<LoadoutDestination>(index);
		RefreshLoadoutData();
	});
	FillShader::Fill(moveUnequippedBox, faintColor);
	smallFont.Draw({"Unequipped outfits to:"}, {moveUnequippedBox.Left() + SETTINGS_MARGINS / 2,
		moveUnequippedBox.Center().Y() - fontOff}, brightColor);

	AddZone(toStorageBox, [this] {
		loadoutDestination = STORAGE;
		RefreshLoadoutData();
	});
	FillShader::Fill(toStorageBox, faintColor);
	SpriteShader::Draw(box[loadoutDestination == STORAGE ? 1 : 0],
		{toStorageBox.Right() - SETTINGS_MARGINS, toStorageBox.Center().Y()});
	smallFont.Draw({"Storage"},
		{toStorageBox.Left() + SETTINGS_MARGINS / 2, toStorageBox.Center().Y() - fontOff}, brightColor);

	AddZone(toCargoBox, [this] {
		loadoutDestination = CARGO;
		RefreshLoadoutData();
	});
	FillShader::Fill(toCargoBox, faintColor);
	SpriteShader::Draw(box[loadoutDestination == CARGO ? 1 : 0],
		{toCargoBox.Right() - SETTINGS_MARGINS, toCargoBox.Center().Y()});
	smallFont.Draw({"Cargo"},
		{toCargoBox.Left() + SETTINGS_MARGINS / 2, toCargoBox.Center().Y() - fontOff}, brightColor);

	AddZone(toOutfitterBox, [this] {
		loadoutDestination = OUTFITTER;
		RefreshLoadoutData();
	});
	FillShader::Fill(toOutfitterBox, faintColor);
	SpriteShader::Draw(box[loadoutDestination == OUTFITTER ? 1 : 0],
		{toOutfitterBox.Right() - SETTINGS_MARGINS, toOutfitterBox.Center().Y()});
	smallFont.Draw({"Sold"},
		{toOutfitterBox.Left() + SETTINGS_MARGINS / 2, toOutfitterBox.Center().Y() - fontOff}, brightColor);
}



void LoadoutsPanel::DrawAccountingModule() const
{
	if(selectedLoadout)
	{
		constexpr int margins = 5;
		const double fontOff = .5 * smallFont.Height();
		smallFont.Draw(DisplayText("Cargo Free:",
			Layout(cargoBox.Width() - margins * 2, Alignment::LEFT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() - 20. - fontOff), mediumColor);
		smallFont.Draw(DisplayText(to_string(player.Cargo().Free()) + " / " + to_string(player.Cargo().Size()),
			Layout(cargoBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() - 20. - fontOff), brightColor);

		smallFont.Draw(DisplayText("Cargo Change:",
			Layout(cargoBox.Width() - margins * 2, Alignment::LEFT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() - fontOff), mediumColor);
		string cargoString = to_string(cargoChange);
		cargoString.erase(cargoString.find_last_not_of('0') + 1, string::npos);
		cargoString.erase(cargoString.find_last_not_of('.') + 1, string::npos);
		smallFont.Draw(DisplayText(cargoString,
			Layout(cargoBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() - fontOff), brightColor);

		smallFont.Draw(DisplayText("Storage Change:",
			Layout(cargoBox.Width() - margins * 2, Alignment::LEFT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() + 20. - fontOff), mediumColor);
		string storageString = to_string(storageChange);
		storageString.erase(storageString.find_last_not_of('0') + 1, string::npos);
		storageString.erase(storageString.find_last_not_of('.') + 1, string::npos);
		smallFont.Draw(DisplayText(storageString,
			Layout(cargoBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(cargoBox.Left() + margins, cargoBox.Center().Y() + 20. - fontOff), brightColor);

		smallFont.Draw(DisplayText("You have:",
			Layout(creditsBox.Width() - margins * 2, Alignment::LEFT)),
			Point(creditsBox.Left() + margins, creditsBox.Center().Y() - 20. - fontOff), mediumColor);
		smallFont.Draw(DisplayText(Format::CreditString(player.Accounts().Credits()),
			Layout(creditsBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(creditsBox.Left(), creditsBox.Center().Y() - 20. - fontOff), brightColor);

		smallFont.Draw(DisplayText("Net cost:",
			Layout(creditsBox.Width() - margins * 2, Alignment::LEFT)),
			Point(creditsBox.Left() + margins, creditsBox.Center().Y() - fontOff), mediumColor);
		smallFont.Draw(DisplayText(Format::CreditString(loadoutSales),
			Layout(creditsBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(creditsBox.Left(), creditsBox.Center().Y() - fontOff), brightColor);

		smallFont.Draw(DisplayText("Result:",
			Layout(creditsBox.Width() - margins * 2, Alignment::LEFT)),
			Point(creditsBox.Left() + margins, creditsBox.Center().Y() + 20. - fontOff), mediumColor);
		smallFont.Draw(DisplayText(Format::CreditString(player.Accounts().Credits() - loadoutSales),
			Layout(creditsBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(creditsBox.Left(), creditsBox.Center().Y() + 20. - fontOff), brightColor);

	}
}



void LoadoutsPanel::DrawScrollbars()
{
	if(shipScroll.Scrollable())
		shipScrollBar.SyncDraw(shipScroll, shipsBox.TopRight() + Point{0., 10.},
			shipsBox.BottomRight() - Point{0., 10.});

	if(loadoutScroll.Scrollable())
		loadoutScrollBar.SyncDraw(loadoutScroll, loadoutsBox.TopRight() + Point{0., 10.},
			loadoutsBox.BottomRight() - Point{0., 10.});

	if(dataScroll.Scrollable())
		dataScrollBar.SyncDraw(dataScroll, selectedBox.TopRight() + Point{0., 10.},
			selectedBox.BottomRight() - Point{0., 10.});

	if(removeScroll.Scrollable())
		removeScrollBar.SyncDraw(removeScroll, removedBox.TopRight() + Point{0., 10.},
			removedBox.BottomRight() - Point{0., 10.});
}



bool LoadoutsPanel::ShouldHighlight(const Ship *ship, const Point mouse, const bool shipIsSelected) const
{
	if(loadoutHovered && hoveredIndex != -1 && cmp_less(hoveredIndex, visibleLoadouts.size()) && visibleLoadouts[hoveredIndex])
		return visibleLoadouts[hoveredIndex]->ShipModel() == ship->TrueModelName();
	if(shipIsSelected && selectedLoadout && applyBox.Contains(mouse))
		return enforceShipTypes ? selectedLoadout->ShipModel() == ship->TrueModelName() : true;
	return false;
}



// Code mostly copied from ShopPanel.cpp. Should I maybe inherit instead?
void LoadoutsPanel::ShipSelect(Ship *ship, const int clicks)
{
	const bool shift = (SDL_GetModState() & KMOD_SHIFT);
	const bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));

	if(shift)
	{
		bool on = false;
		const Planet *here = player.GetPlanet();
		for(const shared_ptr<Ship> &other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(!IsShipPresent(other, here))
				continue;

			if(other.get() == ship || other.get() == lastClicked)
				on = !on;
			else if(on)
				playerShips->insert(other.get());
		}
	}
	else if(!control)
	{
		playerShips->clear();
		if(clicks > 1)
			for(const shared_ptr<Ship> &it : player.Ships())
			{
				if(!IsShipPresent(it, player.GetPlanet()))
					continue;
				if(it.get() != ship && it->Imitates(*ship))
					playerShips->insert(it.get());
			}
	}
	else
	{
		if(clicks > 1)
		{
			vector<Ship *> similarShips;
			// If the ship isn't selected now, it was selected at the beginning of the whole "double click" action,
			// because the first click was handled normally.
			bool unselect = !playerShips->contains(ship);
			for(const shared_ptr<Ship> &it : player.Ships())
			{
				if(!IsShipPresent(it, player.GetPlanet()))
					continue;
				if(it.get() != ship && it->Imitates(*ship))
				{
					similarShips.push_back(it.get());
					unselect &= playerShips->contains(it.get());
				}
			}
			for(Ship *it : similarShips)
			{
				if(unselect)
					playerShips->erase(it);
				else
					playerShips->insert(it);
			}
			if(unselect && find(similarShips.begin(), similarShips.end(), lastClicked) != similarShips.end())
			{
				lastClicked = playerShips->empty() ? nullptr : *playerShips->begin();
				return;
			}
		}
		else if(playerShips->contains(ship))
		{
			playerShips->erase(ship);
			if(lastClicked == ship)
				lastClicked = playerShips->empty() ? nullptr : *playerShips->begin();
			return;
		}
	}

	lastClicked = ship;
	playerShips->insert(lastClicked);
	RefreshLoadoutsBox();
	RefreshLoadoutData();
}
