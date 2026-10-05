/* Industry.cpp
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

#include "Industry.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Facility.h"
#include "Set.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// How many status reports each facility remembers.
	const size_t MAX_REPORTS = 5;

	const map<string, int> EMPTY;

	// Saturation below this is forgotten.
	const double MIN_SATURATION = .5;
}



int64_t Industry::DayReport::Net() const
{
	return sales - upkeep - purchases - freight - tax;
}



int Industry::Holding::Capacity() const
{
	return type->Storage() * count;
}



int Industry::Holding::Stock(const string &commodity) const
{
	auto it = stock.find(commodity);
	return it == stock.end() ? 0 : it->second;
}



int Industry::Holding::OutputStock() const
{
	int total = 0;
	for(const auto &[commodity, amount] : type->Outputs())
		total += Stock(commodity);
	return total;
}



bool Industry::Holding::Uses(const string &commodity) const
{
	for(const auto &[name, amount] : type->Inputs())
		if(name == commodity)
			return true;
	return false;
}



bool Industry::Holding::Makes(const string &commodity) const
{
	for(const auto &[name, amount] : type->Outputs())
		if(name == commodity)
			return true;
	return false;
}



void Industry::Holding::AddReport(const string &date, const string &text)
{
	reports.emplace_back(date, text);
	if(reports.size() > MAX_REPORTS)
		reports.erase(reports.begin(), reports.end() - MAX_REPORTS);
}



void Industry::Load(const DataNode &node, const Set<Facility> &facilities)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "warehouse" && child.Size() >= 2)
		{
			map<string, int> &warehouse = warehouses[child.Token(1)];
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
					warehouse[grand.Token(0)] = max(0, static_cast<int>(grand.Value(1)));
			continue;
		}
		if(key == "saturation" && child.Size() >= 4)
		{
			const double value = child.Value(3);
			if(value >= MIN_SATURATION)
				saturation[{child.Token(1), child.Token(2)}] = value;
			continue;
		}
		if(key == "route" && child.Size() >= 2)
		{
			Route route;
			route.commodity = child.Token(1);
			for(const DataNode &grand : child)
			{
				const string &routeKey = grand.Token(0);
				bool hasValue = grand.Size() >= 2;
				if(routeKey == "from" && hasValue)
					route.from = grand.Token(1);
				else if(routeKey == "to" && hasValue)
					route.to = grand.Token(1);
				else if(routeKey == "tons" && hasValue)
					route.tons = clamp(static_cast<int>(grand.Value(1)), 1, MAX_ROUTE_TONS);
				else if(routeKey == "buy")
					route.buy = true;
				else if(routeKey == "sell")
					route.sell = true;
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
			routes.push_back(std::move(route));
			continue;
		}
		if(key != "facility" || child.Size() < 2)
		{
			child.PrintTrace("Skipping unrecognized attribute:");
			continue;
		}
		const Facility *type = facilities.Find(child.Token(1));
		if(!type || !type->IsDefined())
		{
			child.PrintTrace("Skipping facility of undefined type:");
			continue;
		}

		Holding holding;
		holding.type = type;
		for(const DataNode &grand : child)
		{
			const string &holdingKey = grand.Token(0);
			bool hasValue = grand.Size() >= 2;
			if(holdingKey == "planet" && hasValue)
				holding.planet = grand.Token(1);
			else if(holdingKey == "count" && hasValue)
				holding.count = max(1, static_cast<int>(grand.Value(1)));
			else if(holdingKey == "stock" && grand.Size() >= 3)
				holding.stock[grand.Token(1)] = max(0, static_cast<int>(grand.Value(2)));
			// Saves from before production chains had a single output stockpile.
			else if(holdingKey == "stockpile" && hasValue && !type->Outputs().empty())
				holding.stock[type->Outputs().front().first] = max(0, static_cast<int>(grand.Value(1)));
			else if(holdingKey == "auto sell")
				holding.autoSell = true;
			else if(holdingKey == "report" && grand.Size() >= 3)
				holding.AddReport(grand.Token(1), grand.Token(2));
			else
				grand.PrintTrace("Skipping unrecognized attribute:");
		}
		if(holding.planet.empty())
			child.PrintTrace("Skipping facility with no planet:");
		else if(!Find(*type, holding.planet))
			holdings.push_back(std::move(holding));
	}
}



void Industry::Save(DataWriter &out) const
{
	bool hasWarehouseStock = false;
	for(const auto &[planet, warehouse] : warehouses)
		for(const auto &[commodity, tons] : warehouse)
			hasWarehouseStock |= (tons > 0);
	if(holdings.empty() && routes.empty() && !hasWarehouseStock && saturation.empty())
		return;

	out.Write("industry");
	out.BeginChild();
	{
		for(const Holding &holding : holdings)
		{
			out.Write("facility", holding.type->TrueName());
			out.BeginChild();
			{
				out.Write("planet", holding.planet);
				if(holding.count > 1)
					out.Write("count", holding.count);
				for(const auto &[commodity, amount] : holding.stock)
					if(amount)
						out.Write("stock", commodity, amount);
				if(holding.autoSell)
					out.Write("auto sell");
				for(const auto &[date, text] : holding.reports)
					out.Write("report", date, text);
			}
			out.EndChild();
		}
		for(const auto &[planet, warehouse] : warehouses)
		{
			bool isEmpty = true;
			for(const auto &[commodity, tons] : warehouse)
				isEmpty &= (tons <= 0);
			if(isEmpty)
				continue;
			out.Write("warehouse", planet);
			out.BeginChild();
			{
				for(const auto &[commodity, tons] : warehouse)
					if(tons > 0)
						out.Write(commodity, tons);
			}
			out.EndChild();
		}
		for(const auto &[market, value] : saturation)
			out.Write("saturation", market.first, market.second, round(value * 10.) / 10.);
		for(const Route &route : routes)
		{
			out.Write("route", route.commodity);
			out.BeginChild();
			{
				out.Write("from", route.from);
				out.Write("to", route.to);
				out.Write("tons", route.tons);
				if(route.buy)
					out.Write("buy");
				if(route.sell)
					out.Write("sell");
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



const vector<Industry::Holding> &Industry::Holdings() const
{
	return holdings;
}



vector<Industry::Holding> &Industry::Holdings()
{
	return holdings;
}



Industry::Holding *Industry::Find(const Facility &type, const string &planet)
{
	for(Holding &holding : holdings)
		if(holding.type == &type && holding.planet == planet)
			return &holding;
	return nullptr;
}



const Industry::Holding *Industry::Find(const Facility &type, const string &planet) const
{
	return const_cast<Industry *>(this)->Find(type, planet);
}



void Industry::Build(const Facility &type, const string &planet)
{
	Holding *existing = Find(type, planet);
	if(existing)
		++existing->count;
	else
	{
		Holding holding;
		holding.type = &type;
		holding.planet = planet;
		holdings.push_back(std::move(holding));
	}
}



Industry::DayReport Industry::AdvanceDay(int64_t credits, World *world)
{
	// Markets recover from yesterday's sales.
	for(auto it = saturation.begin(); it != saturation.end(); )
	{
		it->second *= SATURATION_DECAY;
		if(it->second < MIN_SATURATION)
			it = saturation.erase(it);
		else
			++it;
	}

	DayReport report;
	for(Holding &holding : holdings)
		Produce(holding, credits, report);

	if(world)
	{
		for(Route &route : routes)
			RunRoute(route, credits, report, *world);

		for(Holding &holding : holdings)
		{
			if(!holding.autoSell)
				continue;
			for(const auto &[commodity, amount] : holding.type->Outputs())
			{
				int &stored = holding.stock[commodity];
				const int price = world->Price(holding.planet, commodity);
				if(stored <= 0 || price <= 0)
					continue;
				credits += Sell(holding.planet, commodity, stored, price, report, *world);
				stored = 0;
			}
		}
	}

	// The day's profit is taxed.
	report.profit = report.sales - report.upkeep - report.freight - report.purchases;
	report.tax = Tax(report.profit);
	lastReport = report;
	return report;
}



const vector<Industry::TaxBracket> &Industry::TaxBrackets()
{
	static const vector<TaxBracket> BRACKETS = {
		{0, 0.},
		{5000, .1},
		{25000, .25},
		{100000, .4}
	};
	return BRACKETS;
}



int64_t Industry::Tax(int64_t profit)
{
	const vector<TaxBracket> &brackets = TaxBrackets();
	double tax = 0.;
	for(size_t i = 0; i < brackets.size(); ++i)
	{
		const int64_t top = (i + 1 < brackets.size()) ? brackets[i + 1].from : profit;
		if(profit > brackets[i].from)
			tax += (min(profit, top) - brackets[i].from) * brackets[i].rate;
	}
	return llround(tax);
}



double Industry::Saturation(const string &planet, const string &commodity) const
{
	auto it = saturation.find({planet, commodity});
	return it == saturation.end() ? 0. : it->second;
}



const map<pair<string, string>, double> &Industry::Saturations() const
{
	return saturation;
}



int64_t Industry::SaleValue(const string &planet, const string &commodity, int tons, int price) const
{
	if(tons <= 0 || price <= 0)
		return 0;
	// Integrate price / (1 + s / depth) over the tons sold, as each one adds to the saturation.
	const double current = MARKET_DEPTH + Saturation(planet, commodity);
	return llround(price * MARKET_DEPTH * log((current + tons) / current));
}



const Industry::DayReport &Industry::LastReport() const
{
	return lastReport;
}



int Industry::WarehouseCapacity(const string &planet) const
{
	int capacity = 0;
	for(const Holding &holding : holdings)
		if(holding.planet == planet)
			capacity += holding.type->Warehouse() * holding.count;
	return capacity;
}



int Industry::WarehouseUsed(const string &planet) const
{
	int used = 0;
	for(const auto &[commodity, tons] : Warehouse(planet))
		used += tons;
	return used;
}



const map<string, int> &Industry::Warehouse(const string &planet) const
{
	auto it = warehouses.find(planet);
	return it == warehouses.end() ? EMPTY : it->second;
}



int Industry::Store(const string &planet, const string &commodity, int tons)
{
	tons = max(0, min(tons, WarehouseCapacity(planet) - WarehouseUsed(planet)));
	if(tons)
		warehouses[planet][commodity] += tons;
	return tons;
}



int Industry::Retrieve(const string &planet, const string &commodity, int tons)
{
	auto it = warehouses.find(planet);
	if(it == warehouses.end())
		return 0;
	int &stored = it->second[commodity];
	tons = max(0, min(tons, stored));
	stored -= tons;
	return tons;
}



const vector<Industry::Route> &Industry::Routes() const
{
	return routes;
}



vector<Industry::Route> &Industry::Routes()
{
	return routes;
}



void Industry::AddRoute(const Route &route)
{
	routes.push_back(route);
	routes.back().tons = clamp(routes.back().tons, 1, MAX_ROUTE_TONS);
}



void Industry::RemoveRoute(size_t index)
{
	if(index < routes.size())
		routes.erase(routes.begin() + index);
}



int Industry::Collect(Holding &holding, const string &commodity, int space)
{
	auto it = holding.stock.find(commodity);
	if(it == holding.stock.end())
		return 0;
	int amount = max(0, min(it->second, space));
	it->second -= amount;
	return amount;
}



int Industry::Supply(Holding &holding, const string &commodity, int available)
{
	int &stored = holding.stock[commodity];
	int amount = max(0, min(available, holding.Capacity() - stored));
	stored += amount;
	return amount;
}



void Industry::Produce(Holding &holding, int64_t &credits, DayReport &report)
{
	const Facility &type = *holding.type;
	const int64_t upkeep = type.Upkeep() * holding.count;
	if(upkeep > credits)
	{
		holding.status = Status::NO_CREDITS;
		return;
	}
	credits -= upkeep;
	report.upkeep += upkeep;

	// A facility with nothing to make (such as a warehouse) is always running.
	if(type.Outputs().empty())
	{
		holding.status = Status::RUNNING;
		return;
	}

	// Each built copy of the facility runs once a day, as long as there are
	// enough inputs for it and none of the outputs is already full.
	const int capacity = holding.Capacity();
	int runs = holding.count;
	for(const auto &[commodity, amount] : type.Inputs())
		runs = min(runs, holding.Stock(commodity) / amount);
	bool isFull = false;
	for(const auto &[commodity, amount] : type.Outputs())
		isFull |= (holding.Stock(commodity) >= capacity);

	if(isFull)
		holding.status = Status::STORAGE_FULL;
	else if(runs <= 0)
		holding.status = Status::NO_INPUTS;
	else
	{
		holding.status = Status::RUNNING;
		for(const auto &[commodity, amount] : type.Inputs())
			holding.stock[commodity] -= amount * runs;
		for(const auto &[commodity, amount] : type.Outputs())
		{
			int &stored = holding.stock[commodity];
			stored = min(capacity, stored + amount * runs);
		}
	}
}



void Industry::RunRoute(Route &route, int64_t &credits, DayReport &report, World &world)
{
	route.lastMoved = 0;
	route.lastCost = 0;
	if(route.from == route.to)
	{
		route.lastNote = "The source and destination are the same.";
		return;
	}
	const int jumps = world.Jumps(route.from, route.to);
	if(jumps < 0)
	{
		route.lastNote = "There is no hyperspace route between them.";
		return;
	}
	const int64_t perTon = FREIGHT_BASE + FREIGHT_PER_JUMP * jumps;
	const string &commodity = route.commodity;

	// How much room is there at the destination, and how much is on hand at the source?
	int space = max(0, WarehouseCapacity(route.to) - WarehouseUsed(route.to));
	for(const Holding &holding : holdings)
		if(holding.planet == route.to && holding.Uses(commodity))
			space += max(0, holding.Capacity() - holding.Stock(commodity));
	int available = 0;
	auto warehouse = warehouses.find(route.from);
	if(warehouse != warehouses.end())
		available += max(0, warehouse->second[commodity]);
	for(const Holding &holding : holdings)
		if(holding.planet == route.from && holding.Makes(commodity))
			available += holding.Stock(commodity);
	const int buyPrice = route.buy ? world.Price(route.from, commodity) : 0;
	const int sellPrice = route.sell ? world.Price(route.to, commodity) : 0;

	int tons = route.tons;
	if(!sellPrice)
		tons = min(tons, space);
	if(!buyPrice)
		tons = min(tons, available);
	// Goods on hand only cost freight. Goods that must be bought cost their price as well.
	const int onHand = min(tons, available);
	if(credits < onHand * perTon)
		tons = static_cast<int>(credits / perTon);
	else if(tons > onHand)
		tons = onHand + static_cast<int>(min<int64_t>(tons - onHand,
			(credits - onHand * perTon) / (perTon + buyPrice)));

	if(tons <= 0)
	{
		if(!sellPrice && space <= 0)
			route.lastNote = "There is nowhere to put it at the destination.";
		else if(!buyPrice && available <= 0)
			route.lastNote = "There is nothing to ship at the source.";
		else
			route.lastNote = "You cannot afford the freight.";
		return;
	}

	// Take the goods from the source: facility outputs first, then the warehouse,
	// then the market.
	int remaining = tons;
	for(Holding &holding : holdings)
		if(remaining > 0 && holding.planet == route.from && holding.Makes(commodity))
			remaining -= Collect(holding, commodity, remaining);
	if(remaining > 0)
		remaining -= Retrieve(route.from, commodity, remaining);
	const int bought = remaining;
	if(bought > 0)
	{
		world.Trade(route.from, commodity, bought);
		const int64_t cost = static_cast<int64_t>(bought) * buyPrice;
		credits -= cost;
		report.purchases += cost;
	}

	// Deliver them: facilities that use them first, then the warehouse, then the market.
	remaining = tons;
	for(Holding &holding : holdings)
		if(remaining > 0 && holding.planet == route.to && holding.Uses(commodity))
			remaining -= Supply(holding, commodity, remaining);
	if(remaining > 0)
		remaining -= Store(route.to, commodity, remaining);
	const int sold = remaining;
	if(sold > 0)
		credits += Sell(route.to, commodity, sold, sellPrice, report, world);

	const int64_t freight = tons * perTon;
	credits -= freight;
	report.freight += freight;
	route.lastMoved = tons;
	route.lastCost = freight;
	route.lastNote = "Moved " + to_string(tons) + " tons";
	if(bought > 0)
		route.lastNote += ", " + to_string(bought) + " of them bought";
	if(sold > 0)
		route.lastNote += ", " + to_string(sold) + " of them sold";
	route.lastNote += ".";
}



int64_t Industry::Sell(const string &planet, const string &commodity, int tons, int price,
	DayReport &report, World &world)
{
	const int64_t income = SaleValue(planet, commodity, tons, price);
	saturation[{planet, commodity}] += tons;
	world.Trade(planet, commodity, -tons);
	report.sales += income;
	report.saturationLoss += static_cast<int64_t>(price) * tons - income;
	return income;
}
