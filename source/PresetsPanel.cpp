/* PresetsPanel.cpp
Copyright (c) 2026 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PresetsPanel.h"

#include "DataWriter.h"
#include "DialogPanel.h"
#include "Files.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Preset.h"
#include "UI.h"

#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Layout.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Table.h"

#include <iostream>
#include <ranges>

using namespace std;

namespace {
	constexpr double SETTINGS_MARGINS = 10;
}



PresetsPanel::PresetsPanel(PlayerInfo &player, set<Ship*> &playerShips, Sale<Outfit> &outfitter, const int day)
	: player(player),
	playerShips(&playerShips),
	outfitter(&outfitter),
	presetPanelUi(GameData::Interfaces().Get("outfitter presets panel")),
	day(day),
	active(*GameData::Colors().Get("active")),
	bigFont(FontSet::Get(18)),
	smallFont(FontSet::Get(14)),
	presetsBox(presetPanelUi->GetBox("presets")),
	selectedBox(presetPanelUi->GetBox("selected")),
	removedBox(presetPanelUi->GetBox("removed")),
	settingsBox(presetPanelUi->GetBox("settings")),
	cargoBox(presetPanelUi->GetBox("cargo")),
	creditsBox(presetPanelUi->GetBox("credits")),
	tooltipBox(presetPanelUi->GetBox("tooltip")),
	deleteBox(presetPanelUi->GetBox("delete")),
	saveBox(presetPanelUi->GetBox("save")),
	openBox(presetPanelUi->GetBox("open")),
	applyBox(presetPanelUi->GetBox("apply")),
	backgroundColor(*GameData::Colors().Get("panel background")),
	dimColor(*GameData::Colors().Get("dim")),
	mediumColor(*GameData::Colors().Get("medium")),
	brightColor(*GameData::Colors().Get("bright")),
	errorColor(*GameData::Colors().Get("heat")),
	faintColor(*GameData::Colors().Get("faint")),
	tooltip(tooltipBox.Width(), Alignment::LEFT, Tooltip::Direction::DOWN_RIGHT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
{
	// Box height minus 10px margins.
	presetScroll.SetDisplaySize(presetsBox.Height() - 20.);
	// Box height minus 10px margins, 20px header.
	dataScroll.SetDisplaySize(selectedBox.Height() - 40.);
	removeScroll.SetDisplaySize(removedBox.Height() - 40.);

	SetInterruptible(false);

	LoadPresets();
	RefreshPresetsBox();
	RefreshPresetData();

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
	tooltipKeys[&presetsBox] = "presets";
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



PresetsPanel::~PresetsPanel()
= default;



void PresetsPanel::Step()
{
	presetScroll.Step();
	dataScroll.Step();
	removeScroll.Step();
}



void PresetsPanel::Draw()
{
	DrawBackdrop();

	Information info;
	if(selectedPreset)
		info.SetCondition("can delete");
	if(!playerShips->empty())
		info.SetCondition("can save");
	if(selectedPreset && !playerShips->empty())
		info.SetCondition("can apply");
	presetPanelUi->Draw(info, this);
	// TODO: currently using the texture of Info Panel.
	// Will want to make a new texture and the properly positioned buttons.

	DrawPresetsModule();
	DrawSelectedModule();
	DrawRemovingModule();
	DrawSettingsModule();
	DrawAccountingModule();

	// Draw tooltips for the button being hovered over.
	if(const string tip = GameData::Tooltip(string("preset: ") + hoveredTooltip); !tip.empty())
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
}



bool PresetsPanel::Click(const int x, const int y, const MouseButton button, const int clicks)
{
	presetDrag = false;
	dataDrag = false;
	removeDrag = false;

	if(presetScroll.Scrollable() && presetScrollBar.SyncClick(presetScroll, x, y, button, clicks))
	{
		presetDrag = true;
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

	// Was click inside presets list?
	if(presetsBox.Contains(Point(x, y)))
	{
		if(const int index = (presetScroll.AnimatedValue() + y - presetsBox.Top() - 10) / 20;
			static_cast<unsigned>(index) < presets.size())
		{
			const int previous = selected;
			selected = index;
			if(selected != previous)
				RefreshPresetData();
		}
		return true;
	}

	return false;
}



bool PresetsPanel::Hover(const int x, const int y)
{
	presetHovered = false;
	dataHovered = false;
	removeHovered = false;

	if(presetsBox.Contains(Point(x, y)))
		presetHovered = true;
	else if(selectedBox.Contains(Point(x, y)))
		dataHovered = true;
	else if(removedBox.Contains(Point(x, y)))
		removeHovered = true;

	presetScrollBar.Hover(x, y);
	dataScrollBar.Hover(x, y);
	removeScrollBar.Hover(x, y);

	hoveredTooltip = "";
	for(const auto &[rectangle, key] : tooltipKeys)
	{
		if(rectangle->Contains(Point(x, y)))
		{
			hoveredTooltip = key;
			break;
		}
	}
	return true;
}



// Allow dragging of lists.
bool PresetsPanel::Drag(const double dx, const double dy)
{
	if(presetDrag)
	{
		if(presetScroll.Scrollable() && presetScrollBar.SyncDrag(presetScroll, dx, dy))
			return true;

		presetScroll.Set(presetScroll - dy);
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
	return false;
}



// The scroll wheel can be used to scroll hovered lists.
bool PresetsPanel::Scroll(const double dx, const double dy)
{
	presetDrag = presetHovered;
	dataDrag = dataHovered;
	removeDrag = removeHovered;
	return Drag(0., dy * Preferences::ScrollSpeed());
}



bool PresetsPanel::KeyDown(const SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	// Delete.
	if(key == 'l' && selectedPreset)
	{
		GetUI().Push(DialogPanel::CallFunctionIfOk(this, &PresetsPanel::DeletePreset,
			"Are you sure you want to delete the selected preset, \"" + selectedPreset->Name() + "\"?"));
		return true;
	}
	// Save.
	if(key == 's' && !playerShips->empty())
	{
		nameToConfirm.clear();
		const Ship *ship = *playerShips->begin();
		GetUI().Push(DialogPanel::RequestString(this, &PresetsPanel::SavePreset,
			"Creating a preset from your ship \"" + ship->GivenName() + "\".\nWhat do you want to name it?"));
		return true;
	}
	// Apply.
	if(key == 'a' && selectedPreset && !playerShips->empty())
	{
		GetUI().Push(DialogPanel::CallFunctionIfOk(this, &PresetsPanel::ApplyPreset,
			"Are you sure you want to apply the preset \"" + selectedPreset->Name() + "\" to your selected ship(s)?"));
		return true;
	}
	// Done.
	if(key == 'd')
	{
		GetUI().Pop(this);
		return true;
	}
	// Open folder.
	if(key == 'o')
	{
		Files::OpenUserPresetsFolder();
		return true;
	}
	return false;
}



void PresetsPanel::LoadPresets()
{
	// Should these be cached or loaded/discarded as needed?
	presets.clear();

	for(const auto &path : Files::List(Files::Presets()))
	{
		// Skip any files that aren't text files.
		if(path.extension() != ".txt")
			continue;

		// Skip any files that didn't result in a valid preset.
		if(Preset *loaded = Preset::Load(path))
			presets.push_back(loaded);
	}
}



void PresetsPanel::CheckPresetSelected()
{
	try {
		selectedPreset = visiblePresets.at(selected);
	}
	catch([[maybe_unused]] const out_of_range &ex)
	{
		selectedPreset = nullptr;
	}
}



void PresetsPanel::RefreshPresetsBox()
{
	visiblePresets.clear();
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

	for(Preset *preset : presets)
	{
		// Filter mismatched presets if setting enabled.
		if(enforceShipTypes && ranges::find(selectedModels, preset->ShipModel()) == selectedModels.end()) continue;

		visiblePresets.push_back(preset);
	}
	presetScroll.SetMaxValue(max(0., 20. * visiblePresets.size()));

	const Preset *previous = selectedPreset;
	CheckPresetSelected();
	if(previous != selectedPreset)
		RefreshPresetData();
}



void PresetsPanel::RefreshPresetData()
{
	presetListings.clear();
	storageChange = 0;
	cargoChange = 0;
	presetSales = 0;
	CheckPresetSelected();

	if(selectedPreset)
	{

		vector<Ship*> filteredShips;
		for(Ship *ship : *playerShips)
		{

			// Filter mismatched ships if setting enabled.
			if(enforceShipTypes && ship->TrueModelName() != selectedPreset->ShipModel()) continue;

			filteredShips.push_back(ship);
		}

		for(const auto &[outfit, amount] : selectedPreset->Outfits())
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
				presetSales += player.StockDepreciation().Value(outfit, day, stock);
				stillNeeded -= stock;

				const auto sources = new OutfitSources();
				sources->byPreset = to_string(amount) +
					(amount != amountToSource ? "(" + to_string(amountToSource) + ")" : "");
				sources->installed = equipped > 0 ? to_string(equipped) : "-";
				sources->inCargo = cargo > 0 ? to_string(cargo) : "-";
				sources->inStorage = storage > 0 ? to_string(storage) : "-";
				sources->fromOutfitter = stock > 0 ? to_string(stock) : "-";
				sources->stillRequired = stillNeeded > 0 ? to_string(stillNeeded) : "-";
				presetListings[outfit->Category()][outfit] = sources;
			}

		double dataScrollRoom = presetListings.size() * 30.;
		for(const auto &content : presetListings | views::values)
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
		for(pair<const Outfit*, int> outfit : selectedPreset->Outfits())
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
				switch(presetDestination)
				{
					case CARGO:
						cargoChange += amount * outfit->Mass();
						break;
					case STORAGE:
						storageChange += amount * outfit->Mass();
						break;
					default:
						presetSales -= player.FleetDepreciation().Value(outfit, day, amount);
						break;
				}
			}

		double removeScrollRoom = removalListings.size() * 30.;
		for(const auto &content : removalListings | views::values)
			removeScrollRoom += content.size() * 20.;
		removeScroll.SetMaxValue(max(0., removeScrollRoom));
	}
}



void PresetsPanel::SavePreset(const string &name)
{
	if(!playerShips->empty())
	{

		const Ship *ship = *playerShips->begin();
		if(Preset::Exists(name) && name != nameToConfirm)
		{
			nameToConfirm = name;
			GetUI().Push(DialogPanel::RequestString(this, &PresetsPanel::SavePreset, "Warning: \"" + name
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
			if(const auto copied = new Preset(name, ship->TrueModelName(), outfits);
				copied->Save())
			{
				if(nameToConfirm == name)
					erase_if(presets, [name](const Preset *p)
					{
						return p->Name() == name;
					});
				presets.push_back(copied);
				RefreshPresetsBox();
				GetUI().Push(DialogPanel::Info("The preset \"" + name + "\" was successfully created."));
			}
			else
				GetUI().Push(DialogPanel::Info("Could not create a file for the preset \"" + name + "\"."));
		}
	}
	else {
		// This shouldn't be reachable.
		GetUI().Push(DialogPanel::Info("You do not have any ships selected."));
	}
}



void PresetsPanel::DeletePreset()
{
	if(selectedPreset)
	{
		if(!selectedPreset->Delete())
			GetUI().Push(DialogPanel::Info("Failed to delete preset \"" + selectedPreset->Name() + "\"."));
		else {
			GetUI().Push(DialogPanel::Info("The preset \"" +
				selectedPreset->Name() + "\" was successfully deleted."));
			presets.erase(ranges::remove(presets, selectedPreset).begin(), presets.end());
			visiblePresets.erase(ranges::remove(visiblePresets, selectedPreset).begin(),
				visiblePresets.end());
		}
		RefreshPresetsBox();
	}
}



void PresetsPanel::ApplyPreset()
{
	// Keep track of outfits that couldn't be added or removed.
	map<Ship*, vector<string>> errors;

	// We will want to add everything to cargo after ships are equipped appropriately to better guarantee proper
	// Interaction with things like outfit/cargo expansions, as well as outfits being removed from cargo for the
	// Incoming preset. So, cache the results and apply cargo changes after everything else.
	map<const Outfit*, int> toCargo;

	// First, remove all outfits on all ships to guarantee as much cash as possible.
	for(Ship *ship : *playerShips)
	{

		// Skip ships not belonging to the preset if the enforce option is toggled.
		if(enforceShipTypes && ship->TrueModelName() != selectedPreset->ShipModel()) continue;

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
			// Do not remove outfits that are wanted on the preset.
			int wanted = 0;
			if(selectedPreset->Outfits().contains(outfit))
				wanted = selectedPreset->Outfits()[outfit];
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
				switch(presetDestination)
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

		// Skip ships not belonging to the preset if the enforce option is toggled
		if(enforceShipTypes && ship->TrueModelName() != selectedPreset->ShipModel()) continue;

		// Just in case the preset, for whatever reason, can't be fully applied, apply some sorts to the list.
		// Convert the map to a list of vectors so arbitrary sorts and filters can be used.
		vector<pair<const Outfit*, int>> outfits;
		for(pair<const Outfit*, int> outfit : selectedPreset->Outfits())
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
		GetUI().Push(DialogPanel::Info("The preset \"" + selectedPreset->Name() +
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

	RefreshPresetData();
}



void PresetsPanel::DrawPresetsModule()
{
	FillShader::Fill(presetsBox, backgroundColor);

	// Round up initial draw index at 0.2 so the text doesn't visually escape from the box, but still has wiggle room.
	const double indexRaw = (presetScroll.AnimatedValue()) / 20.;
	int index = indexRaw;
	if(indexRaw - index > 0.2) ++index;

	int y = presetsBox.Top() + 10 - presetScroll.AnimatedValue() + 20 * index;
	const int endY = presetsBox.Bottom() - 15 - 10;

	// Y offset to center the text in a 20-pixel high row.
	const double fontOff = .5 * (20 - smallFont.Height());

	for( ; y < endY && static_cast<unsigned>(index) < visiblePresets.size(); y += 20, ++index)
	{
		const Preset *item = visiblePresets[index];

		// Check if this is the selected row.
		if(index == selected)
			FillShader::Fill(
				Point(presetsBox.Center().X(), y + 10.), Point(presetsBox.Width(), 20.), dimColor);

		Point pos(presetsBox.Left() + 10, y + fontOff);
		smallFont.Draw(DisplayText(item->Name(),
			Layout(static_cast<int>(presetsBox.Width()) - 25, Alignment::LEFT, Truncate::BACK)), pos, brightColor);
	}

	if(presetScroll.Scrollable())
		presetScrollBar.SyncDraw(presetScroll, presetsBox.TopRight() + Point{0., 10.},
			presetsBox.BottomRight() - Point{0., 10.});
}



void PresetsPanel::DrawSelectedModule()
{
	FillShader::Fill(selectedBox, backgroundColor);

	if(selectedPreset)
	{
		const Point point = selectedBox.TopLeft();

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
		table.DrawAt(point);
		// Add ten pixels of padding at the top.
		table.DrawGap(10);

		table.Draw("Outfit", brightColor);
		table.Draw("Preset", brightColor);
		table.Draw("Equipped", brightColor);
		table.Draw("Cargo", brightColor);
		table.Draw("Stored", brightColor);
		table.Draw("Purchase", brightColor);
		table.Draw("Missing", brightColor);

		const int scrollableY = selectedBox.Top() + 20;
		double currentY = scrollableY - dataScroll.AnimatedValue();
		const int cutoff = selectedBox.Bottom() - 20 - 10;

		// Align with the scroll bar progress by creating a table gap proportional to animated value.
		bool firstDraw = false;
		auto addProgress = [&](const double amount) -> void {
			currentY += amount;
			if(!firstDraw && currentY >= scrollableY)
			{
				table.DrawGap(currentY - scrollableY);
				firstDraw = true;
			}
		};

		for(const auto &[category, content] : presetListings)
		{
			if(currentY >= cutoff) break;
			if(firstDraw && currentY >= scrollableY)
			{
				// Empty rows between categories.
				table.DrawGap(10);
			}
			addProgress(10.);

			if(currentY >= cutoff) break;
			if(currentY >= scrollableY)
			{
				// Category listings.
				table.Draw(category + ":", mediumColor);
				// Empty data cells on category listings.
				for(int i = 0; i < 6; ++i) table.Draw("");
			}
			addProgress(20.);

			for(auto [outfit, sources] : content)
			{
				if(currentY >= cutoff) break;
				if(currentY >= scrollableY)
				{
					// Outfit name.
					table.Draw(outfit->DisplayName(), mediumColor);

					// Amount required.
					table.Draw(sources->byPreset, brightColor);

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
		if(dataScroll.Scrollable())
			dataScrollBar.SyncDraw(dataScroll, selectedBox.TopRight() + Point{0., 10.},
				selectedBox.BottomRight() - Point{0., 10.});
	}
}



void PresetsPanel::DrawRemovingModule()
{
	FillShader::Fill(removedBox, backgroundColor);

	if(selectedPreset)
	{
		const Point point = removedBox.TopLeft();
		Table table;

		// Use 10-pixel margins on both sides, then split the box by 90% for text, then 10% for the number
		const int available = removedBox.Width() - 20;
		table.AddColumn(10, Layout(available * .9));
		table.AddColumn(removedBox.Width() - 10, Layout(available * 0.1, Alignment::RIGHT));

		table.SetHighlight(0, removedBox.Width());
		table.DrawAt(point);

		// 10px margin on top.
		table.DrawGap(10);

		// Table header.
		table.Draw("To Be Removed", brightColor);
		table.Draw("", brightColor);

		const int scrollableY = removedBox.Top() + 20;
		double currentY = scrollableY - removeScroll.AnimatedValue();
		const int cutoff = removedBox.Bottom() - 20 - 10;

		// Align with the scroll bar progress by creating a table gap proportional to animated value.
		bool firstDraw = false;
		auto addProgress = [&](const double amount) -> void {
			currentY += amount;
			if(!firstDraw && currentY >= scrollableY)
			{
				table.DrawGap(currentY - scrollableY);
				firstDraw = true;
			}
		};

		for(const auto &[category, content] : removalListings)
		{

			if(currentY >= cutoff) break;
			if(firstDraw && currentY >= scrollableY)
			{
				// 10px margin between categories.
				table.DrawGap(10);
			}
			addProgress(10.);

			if(currentY >= cutoff) break;
			if(currentY >= scrollableY)
			{
				// Category listings.
				table.Draw(category + ":", mediumColor);
				table.Draw("");
			}
			addProgress(20.);

			// All outfits in the category.
			for(auto [outfit, amount] : content)
			{
				if(currentY >= cutoff) break;
				if(currentY >= scrollableY)
				{
					table.Draw(outfit->DisplayName(), mediumColor);
					table.Draw(*amount, brightColor);
				}
				addProgress(20.);
			}
		}
		if(removeScroll.Scrollable())
			removeScrollBar.SyncDraw(removeScroll, removedBox.TopRight() + Point{0., 10.},
				removedBox.BottomRight() - Point{0., 10.});
	}
}



void PresetsPanel::DrawSettingsModule()
{
	FillShader::Fill(settingsBox, backgroundColor);
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};
	const double fontOff = .5 * smallFont.Height();

	// Top row.
	AddZone(handToHandBox, [this] {
		includeHandToHand = !includeHandToHand;
		RefreshPresetData();
	});
	FillShader::Fill(handToHandBox, faintColor);
	SpriteShader::Draw(box[includeHandToHand ? 1 : 0],
		{handToHandBox.Right() - SETTINGS_MARGINS, handToHandBox.Center().Y()});
	smallFont.Draw({"Include Hand to Hand"},
		{handToHandBox.Left() + SETTINGS_MARGINS / 2, handToHandBox.Center().Y() - fontOff}, brightColor);

	AddZone(uniqueBox, [this] {
		includeUnique = !includeUnique;
		RefreshPresetData();
	});
	FillShader::Fill(uniqueBox, faintColor);
	SpriteShader::Draw(box[includeUnique ? 1 : 0],
		{uniqueBox.Right() - SETTINGS_MARGINS, uniqueBox.Center().Y()});
	smallFont.Draw({"Include Uniques"},
		{uniqueBox.Left() + SETTINGS_MARGINS / 2, uniqueBox.Center().Y() - fontOff}, brightColor);

	AddZone(enforceBox, [this] {
		enforceShipTypes = !enforceShipTypes;
		RefreshPresetsBox();
		RefreshPresetData();
	});
	FillShader::Fill(enforceBox, faintColor);
	SpriteShader::Draw(box[enforceShipTypes ? 1 : 0],
		{enforceBox.Right() - SETTINGS_MARGINS, enforceBox.Center().Y()});
	smallFont.Draw({"Strict Ship Match"},
		{enforceBox.Left() + SETTINGS_MARGINS / 2, enforceBox.Center().Y() - fontOff}, brightColor);

	// Bottom row.
	AddZone(moveUnequippedBox, [this] {
		int index = presetDestination;
		if(++index >= destinationsSize) index = 0;
		presetDestination = static_cast<PresetDestination>(index);
		RefreshPresetData();
	});
	FillShader::Fill(moveUnequippedBox, faintColor);
	smallFont.Draw({"Unequipped outfits to:"}, {moveUnequippedBox.Left() + SETTINGS_MARGINS / 2,
		moveUnequippedBox.Center().Y() - fontOff}, brightColor);

	AddZone(toStorageBox, [this] {
		presetDestination = STORAGE;
		RefreshPresetData();
	});
	FillShader::Fill(toStorageBox, faintColor);
	SpriteShader::Draw(box[presetDestination == STORAGE ? 1 : 0],
		{toStorageBox.Right() - SETTINGS_MARGINS, toStorageBox.Center().Y()});
	smallFont.Draw({"Storage"},
		{toStorageBox.Left() + SETTINGS_MARGINS / 2, toStorageBox.Center().Y() - fontOff}, brightColor);

	AddZone(toCargoBox, [this] {
		presetDestination = CARGO;
		RefreshPresetData();
	});
	FillShader::Fill(toCargoBox, faintColor);
	SpriteShader::Draw(box[presetDestination == CARGO ? 1 : 0],
		{toCargoBox.Right() - SETTINGS_MARGINS, toCargoBox.Center().Y()});
	smallFont.Draw({"Cargo"},
		{toCargoBox.Left() + SETTINGS_MARGINS / 2, toCargoBox.Center().Y() - fontOff}, brightColor);

	AddZone(toOutfitterBox, [this] {
		presetDestination = OUTFITTER;
		RefreshPresetData();
	});
	FillShader::Fill(toOutfitterBox, faintColor);
	SpriteShader::Draw(box[presetDestination == OUTFITTER ? 1 : 0],
		{toOutfitterBox.Right() - SETTINGS_MARGINS, toOutfitterBox.Center().Y()});
	smallFont.Draw({"Sold"},
		{toOutfitterBox.Left() + SETTINGS_MARGINS / 2, toOutfitterBox.Center().Y() - fontOff}, brightColor);
}



void PresetsPanel::DrawAccountingModule() const
{
	FillShader::Fill(cargoBox, faintColor);
	FillShader::Fill(creditsBox, faintColor);

	if(selectedPreset)
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
		smallFont.Draw(DisplayText(Format::CreditString(presetSales),
			Layout(creditsBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(creditsBox.Left(), creditsBox.Center().Y() - fontOff), brightColor);

		smallFont.Draw(DisplayText("Result:",
			Layout(creditsBox.Width() - margins * 2, Alignment::LEFT)),
			Point(creditsBox.Left() + margins, creditsBox.Center().Y() + 20. - fontOff), mediumColor);
		smallFont.Draw(DisplayText(Format::CreditString(player.Accounts().Credits() - presetSales),
			Layout(creditsBox.Width() - margins * 2, Alignment::RIGHT)),
			Point(creditsBox.Left(), creditsBox.Center().Y() + 20. - fontOff), brightColor);

	}
}
