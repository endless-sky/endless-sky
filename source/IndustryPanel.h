/* IndustryPanel.h
Copyright (c) 2026 by Endless Sky Expanded contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Panel.h"

#include <functional>
#include <string>
#include <vector>

class Facility;
class Planet;
class PlayerInfo;
class Point;



// Planet screen panel for the player's industry, drawn in the planet
// description area like the bank. It has four views, switched with Tab:
// this planet (build, supply and collect from facilities, use the warehouse,
// found a station), freight routes, finances (taxes and market saturation),
// and an overview of all holdings.
class IndustryPanel : public Panel {
public:
	IndustryPanel(PlayerInfo &player, const Planet &planet);

	// Check whether the Industry button should be shown on this planet: there is
	// a facility that can be built or is owned here, or the player owns
	// facilities elsewhere that they may want to review.
	static bool IsAvailable(const PlayerInfo &player, const Planet &planet);

	virtual void Draw() override;


protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	enum class View {
		PLANET,
		ROUTES,
		FINANCES,
		OVERVIEW
	};


private:
	// The facility types that can be built or are owned on this planet.
	static std::vector<const Facility *> Facilities(const PlayerInfo &player, const Planet &planet);
	// Whether the player already has a station in the given planet's system.
	static bool HasStationInSystem(const PlayerInfo &player, const Planet &planet);
	// The rows of the planet view: the warehouse (as nullptr) if there is one,
	// then the facilities.
	std::vector<const Facility *> Rows() const;
	bool HasWarehouse() const;
	const Facility *Selected() const;
	bool IsWarehouseSelected() const;

	void DrawPlanetView();
	void DrawWarehouse(double left, double top);
	void DrawRoutes();
	void DrawFinances();
	void DrawOverview();
	// Draw a button and make it clickable. Moves the corner to the right.
	void DrawButton(Point &corner, const std::string &label, bool enabled, std::function<void()> action);

	// Actions on the selected facility.
	void Build();
	void FoundStation(const std::string &name);
	void Supply();
	void Collect();
	void ToggleAutoSell();
	bool CanAutoSell() const;
	// Warehouse actions.
	void StoreCargo();
	void LoadWarehouse();
	// Freight route actions.
	void NewRoute();
	void DeleteRoute();
	void ChangeRoute(int field, int step);
	// Planets that freight routes can use: wherever the player has facilities, and here.
	std::vector<std::string> Locations() const;
	// Commodities that freight routes can carry.
	static std::vector<std::string> Commodities();
	std::string DefaultStationName() const;


private:
	PlayerInfo &player;
	const Planet &planet;
	View view = View::PLANET;
	int selectedRow = 0;
	int selectedRoute = 0;
	int scroll = 0;
	std::string status;
	// The station type waiting for the player to choose a name.
	const Facility *pendingStation = nullptr;
};
