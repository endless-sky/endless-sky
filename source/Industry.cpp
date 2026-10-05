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

using namespace std;

namespace {
	// How many status reports each facility remembers.
	const size_t MAX_REPORTS = 5;
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
		if(child.Token(0) != "facility" || child.Size() < 2)
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
			const string &key = grand.Token(0);
			bool hasValue = grand.Size() >= 2;
			if(key == "planet" && hasValue)
				holding.planet = grand.Token(1);
			else if(key == "count" && hasValue)
				holding.count = max(1, static_cast<int>(grand.Value(1)));
			else if(key == "stock" && grand.Size() >= 3)
				holding.stock[grand.Token(1)] = max(0, static_cast<int>(grand.Value(2)));
			// Saves from before production chains had a single output stockpile.
			else if(key == "stockpile" && hasValue && !type->Outputs().empty())
				holding.stock[type->Outputs().front().first] = max(0, static_cast<int>(grand.Value(1)));
			else if(key == "auto sell")
				holding.autoSell = true;
			else if(key == "report" && grand.Size() >= 3)
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
	if(holdings.empty())
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



Industry::DayReport Industry::AdvanceDay(int64_t credits, const Seller &sell)
{
	DayReport report;
	for(Holding &holding : holdings)
	{
		const Facility &type = *holding.type;
		const int64_t upkeep = type.Upkeep() * holding.count;
		if(upkeep > credits)
		{
			holding.status = Status::NO_CREDITS;
			continue;
		}
		credits -= upkeep;
		report.upkeep += upkeep;

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

		if(holding.autoSell && sell)
			for(const auto &[commodity, amount] : type.Outputs())
			{
				int &stored = holding.stock[commodity];
				if(stored <= 0)
					continue;
				const int64_t income = sell(holding, commodity, stored);
				if(income > 0)
				{
					stored = 0;
					credits += income;
					report.sales += income;
				}
			}
	}
	return report;
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
