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
#include <functional>
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
// one, and the daily production that turns inputs into outputs.
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
		// Add a status report, forgetting the oldest one if there are too many.
		void AddReport(const std::string &date, const std::string &text);
	};

	// The result of a day of production, in credits.
	struct DayReport {
		int64_t upkeep = 0;
		int64_t sales = 0;
	};

	// Sell the given tons of a holding's output on the local market, returning
	// the credits earned, or 0 if it cannot be sold there.
	using Seller = std::function<int64_t(const Holding &holding, const std::string &commodity, int tons)>;


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
	// Run one day of production for every facility. Upkeep is only paid (and
	// the facility only runs) while the credits on hand cover it. The caller
	// must apply the returned report to the player's account.
	DayReport AdvanceDay(int64_t credits, const Seller &sell = nullptr);

	// Take up to the given number of tons of a commodity out of a holding's stock.
	// Returns how many tons were actually taken.
	static int Collect(Holding &holding, const std::string &commodity, int space);
	// Add up to the given number of tons of a commodity to a holding's stock,
	// limited by its capacity. Returns how many tons were actually added.
	static int Supply(Holding &holding, const std::string &commodity, int available);


private:
	std::vector<Holding> holdings;
};
