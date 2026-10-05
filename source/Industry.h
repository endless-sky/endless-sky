/* Industry.h
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

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;
class Facility;
template<class Type>
class Set;



// All of the industrial facilities the player owns, the goods stocked in each
// one and in their warehouses, the freight routes that move goods between
// them, and the daily production that turns inputs into outputs.
class Industry {
public:
	// What a facility did on the most recent day.
	enum class Status {
		STARTING,
		RUNNING,
		NO_INPUTS,
		STORAGE_FULL,
		NO_CREDITS
	};

	// One type of facility the player has built on a particular planet.
	struct Holding {
		const Facility *type = nullptr;
		// The true name of the planet the facility is on.
		std::string planet;
		// How many of this facility have been built here. Production, upkeep
		// and storage all scale with this.
		int count = 1;
		// Tons of each commodity (inputs and outputs) stored here.
		std::map<std::string, int> stock;
		// Whether outputs are sold to the local market every day.
		bool autoSell = false;
		Status status = Status::STARTING;
		// The most recent status reports from this facility, oldest first, as
		// pairs of date and text.
		std::vector<std::pair<std::string, std::string>> reports;

		// The most tons of any one commodity this holding can store.
		int Capacity() const;
		int Stock(const std::string &commodity) const;
		// Total tons of outputs waiting to be collected.
		int OutputStock() const;
		bool Uses(const std::string &commodity) const;
		bool Makes(const std::string &commodity) const;
		// Add a status report, forgetting the oldest one if there are too many.
		void AddReport(const std::string &date, const std::string &text);
	};

	// A standing order to move a commodity from one planet to another every day.
	// Goods are taken from the outputs of the player's facilities at the source,
	// then from the warehouse there, then (if allowed) bought on the market.
	// They are delivered to facilities at the destination that use them, then
	// to the warehouse there, then (if allowed) sold on the market.
	struct Route {
		std::string commodity;
		std::string from;
		std::string to;
		int tons = 10;
		bool buy = false;
		bool sell = false;
		// What happened on the most recent day.
		int lastMoved = 0;
		int64_t lastCost = 0;
		std::string lastNote = "Waiting for the first day.";
	};

	// The result of a day, in credits.
	struct DayReport {
		int64_t upkeep = 0;
		int64_t sales = 0;
		int64_t purchases = 0;
		int64_t freight = 0;
	};

	// Access to the rest of the universe: markets and travel distances.
	class World {
	public:
		virtual ~World() = default;
		// The price of a commodity on the given planet's market, or 0 if it
		// cannot be bought or sold there.
		virtual int Price(const std::string &planet, const std::string &commodity) const = 0;
		// Record that tons were bought (positive) or sold (negative) on the
		// given planet's market, so that its prices can react.
		virtual void Trade(const std::string &planet, const std::string &commodity, int tons) = 0;
		// The number of hyperspace jumps between two planets, or -1 if one
		// cannot be reached from the other.
		virtual int Jumps(const std::string &from, const std::string &to) const = 0;
	};

	// Freight costs per ton moved: a flat fee, plus a fee for every jump.
	static constexpr int64_t FREIGHT_BASE = 5;
	static constexpr int64_t FREIGHT_PER_JUMP = 10;
	static constexpr int MAX_ROUTE_TONS = 500;


public:
	// Load the "industry" node of a saved game.
	void Load(const DataNode &node, const Set<Facility> &facilities);
	void Save(DataWriter &out) const;

	const std::vector<Holding> &Holdings() const;
	std::vector<Holding> &Holdings();
	// Find the given type of facility on the given planet, if the player owns one.
	Holding *Find(const Facility &type, const std::string &planet);
	const Holding *Find(const Facility &type, const std::string &planet) const;

	// Add a new facility, or one more of it if the player already owns this type
	// of facility on this planet. Paying for it is up to the caller.
	void Build(const Facility &type, const std::string &planet);
	// Run one day: pay upkeep, produce, move freight, and auto-sell. Upkeep,
	// purchases and freight are only paid (and things only happen) while the
	// credits on hand cover them. The caller must apply the returned report
	// to the player's account. Without a world, there are no markets or routes.
	DayReport AdvanceDay(int64_t credits, World *world = nullptr);

	// Warehouses: shared storage for any commodity, provided by facilities.
	int WarehouseCapacity(const std::string &planet) const;
	int WarehouseUsed(const std::string &planet) const;
	const std::map<std::string, int> &Warehouse(const std::string &planet) const;
	// Add up to the given tons to a planet's warehouse; returns the tons added.
	int Store(const std::string &planet, const std::string &commodity, int tons);
	// Take up to the given tons out of a planet's warehouse; returns the tons taken.
	int Retrieve(const std::string &planet, const std::string &commodity, int tons);

	// Freight routes.
	const std::vector<Route> &Routes() const;
	std::vector<Route> &Routes();
	void AddRoute(const Route &route);
	void RemoveRoute(size_t index);

	// Take up to the given number of tons of a commodity out of a holding's stock.
	// Returns how many tons were actually taken.
	static int Collect(Holding &holding, const std::string &commodity, int space);
	// Add up to the given number of tons of a commodity to a holding's stock,
	// limited by its capacity. Returns how many tons were actually added.
	static int Supply(Holding &holding, const std::string &commodity, int available);


private:
	void Produce(Holding &holding, int64_t &credits, DayReport &report);
	void RunRoute(Route &route, int64_t &credits, DayReport &report, World &world);


private:
	std::vector<Holding> holdings;
	std::map<std::string, std::map<std::string, int>> warehouses;
	std::vector<Route> routes;
};
