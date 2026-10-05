/* IndustryPanel.cpp
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

#include "IndustryPanel.h"

#include "text/Alignment.h"
#include "CargoHold.h"
#include "Color.h"
#include "Command.h"
#include "DialogPanel.h"
#include "Facility.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Industry.h"
#include "Interface.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Port.h"
#include "Rectangle.h"
#include "Screen.h"
#include "System.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <map>
#include <set>

using namespace std;

namespace {
	const double PAD = 10.;
	const double LINE = 18.;
	const double ROW = 20.;
	const double LIST_WIDTH = 170.;
	const Point BUTTON_SIZE(68., 22.);
	const Point ARROW_SIZE(18., 16.);
	const double BUTTON_GAP = 6.;
	const int ROUTE_ROWS = 2;

	enum RouteField {
		COMMODITY,
		FROM,
		TO,
		TONS,
		BUY,
		SELL
	};

	// Join a list of phrases with commas.
	string Join(const vector<string> &parts)
	{
		string result;
		for(const string &part : parts)
			result += (result.empty() ? "" : ", ") + part;
		return result;
	}

	// For example, "2 Metal, 1 Plastic".
	string AmountsString(const Facility::Amounts &amounts, int count)
	{
		vector<string> parts;
		for(const auto &[commodity, amount] : amounts)
			parts.push_back(to_string(amount * count) + " " + commodity);
		return Join(parts);
	}

	string StatusString(Industry::Status status)
	{
		switch(status)
		{
			case Industry::Status::STARTING:
				return "Starting up";
			case Industry::Status::RUNNING:
				return "Running";
			case Industry::Status::NO_INPUTS:
				return "Idle: needs inputs";
			case Industry::Status::STORAGE_FULL:
				return "Idle: storage full";
			case Industry::Status::NO_CREDITS:
				return "Idle: can't pay upkeep";
		}
		return "";
	}

	string PlanetName(const string &trueName)
	{
		const Planet *planet = GameData::Planets().Find(trueName);
		return (planet && planet->IsValid()) ? planet->DisplayName() : trueName;
	}

	// The market price of a commodity on a planet, or 0 if it has no market.
	int MarketPrice(const string &planetName, const string &commodity)
	{
		const Planet *planet = GameData::Planets().Find(planetName);
		if(!planet || !planet->IsValid() || !planet->IsInhabited()
				|| !planet->GetPort().HasService(Port::ServicesType::Trading))
			return 0;
		const System *system = planet->GetSystem();
		return (system && system->HasTrade()) ? max(0, system->Trade(commodity)) : 0;
	}

	// For example, "about 412 per ton (83% of 497)".
	string PayoutString(const Industry &industry, const string &planet, const string &commodity, int tons)
	{
		const int price = MarketPrice(planet, commodity);
		if(price <= 0 || tons <= 0)
			return "";
		const int64_t value = industry.SaleValue(planet, commodity, tons, price);
		const int64_t perTon = value / tons;
		return "about " + Format::CreditString(perTon, false) + " per ton ("
			+ to_string(static_cast<int>(100 * perTon / price)) + "% of " + Format::CreditString(price, false) + ")";
	}

	const Rectangle ContentBox()
	{
		return GameData::Interfaces().Get(Screen::Width() < 1280 ? "planet (small screen)" : "planet")
			->GetBox("content");
	}

	int VisibleRows(const Rectangle &box, double top)
	{
		return max(1, static_cast<int>((box.Bottom() - 2. * LINE - top) / ROW));
	}

	// Move to the next or previous entry in a list, wrapping around.
	string Cycle(const vector<string> &options, const string &current, int step)
	{
		if(options.empty())
			return current;
		auto it = find(options.begin(), options.end(), current);
		int index = (it == options.end()) ? 0 : static_cast<int>(it - options.begin()) + step;
		const int size = static_cast<int>(options.size());
		return options[((index % size) + size) % size];
	}
}



IndustryPanel::IndustryPanel(PlayerInfo &player, const Planet &planet)
	: player(player), planet(planet)
{
	// Let the planet panel underneath keep handling its own buttons and keys.
	SetTrapAllEvents(false);
	SetInterruptible(false);
	if(Rows().empty())
		view = View::OVERVIEW;
}



bool IndustryPanel::IsAvailable(const PlayerInfo &player, const Planet &planet)
{
	// Uninhabited worlds have no services to deny, so outposts can be built there.
	if(planet.IsInhabited() && !planet.CanUseServices())
		return false;
	return !player.GetIndustry().Holdings().empty() || !Facilities(player, planet).empty();
}



void IndustryPanel::Draw()
{
	ClearZones();
	if(view == View::PLANET && Rows().empty())
		view = View::ROUTES;

	if(view == View::PLANET)
		DrawPlanetView();
	else if(view == View::ROUTES)
		DrawRoutes();
	else if(view == View::FINANCES)
		DrawFinances();
	else
		DrawOverview();

	// The bottom line shows the result of the last action, or a reminder of the keys.
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Point bottom(box.Left() + PAD, box.Bottom() - LINE);
	string hint;
	if(view == View::PLANET)
		hint = "E: build    U: supply    C: collect    A: auto-sell    Tab: freight";
	else if(view == View::ROUTES)
		hint = "R: new route    X: delete    -/+: tons    Tab: finances";
	else if(view == View::FINANCES)
		hint = "Tab: all holdings";
	else
		hint = "Up/Down: scroll    Tab: this planet";
	if(!status.empty())
		font.Draw(status, bottom, *GameData::Colors().Get("bright"));
	else
		font.Draw(hint, bottom, *GameData::Colors().Get("dim"));
}



bool IndustryPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_TAB)
	{
		if(view == View::PLANET)
			view = View::ROUTES;
		else if(view == View::ROUTES)
			view = View::FINANCES;
		else if(view == View::FINANCES)
			view = View::OVERVIEW;
		else
			view = Rows().empty() ? View::ROUTES : View::PLANET;
		scroll = 0;
		status.clear();
		return true;
	}

	const int step = (key == SDLK_UP ? -1 : key == SDLK_DOWN ? 1 : 0);
	if(view == View::FINANCES)
		return false;
	if(view == View::OVERVIEW)
	{
		if(!step)
			return false;
		scroll = max(0, scroll + step);
	}
	else if(view == View::ROUTES)
	{
		if(step)
			selectedRoute = max(0, selectedRoute + step);
		else if(key == 'r')
			NewRoute();
		else if(key == 'x' || key == SDLK_DELETE)
			DeleteRoute();
		else if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
			ChangeRoute(TONS, -1);
		else if(key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS)
			ChangeRoute(TONS, 1);
		else
			return false;
	}
	else if(step)
	{
		selectedRow = max(0, selectedRow + step);
		status.clear();
	}
	else if(key == 'e')
		Build();
	else if(key == 'u')
		IsWarehouseSelected() ? StoreCargo() : Supply();
	else if(key == 'c')
		IsWarehouseSelected() ? LoadWarehouse() : Collect();
	else if(key == 'a')
		ToggleAutoSell();
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER)
	{
		const Facility *facility = Selected();
		if(IsWarehouseSelected())
			LoadWarehouse();
		else if(facility && player.GetIndustry().Find(*facility, planet.TrueName()))
			Collect();
		else
			Build();
	}
	else
		return false;

	return true;
}



bool IndustryPanel::Scroll(double dx, double dy)
{
	if(dy)
		scroll = max(0, scroll + (dy > 0. ? -1 : 1));
	return true;
}



vector<const Facility *> IndustryPanel::Facilities(const PlayerInfo &player, const Planet &planet)
{
	vector<const Facility *> result;
	const bool hasStation = HasStationInSystem(player, planet);
	for(const auto &it : GameData::Facilities())
	{
		const Facility &facility = it.second;
		if(!facility.IsDefined())
			continue;
		if(player.GetIndustry().Find(facility, planet.TrueName()))
		{
			result.push_back(&facility);
			continue;
		}
		// Some facilities have to be unlocked before they can be built.
		const string &requirement = facility.Requirement();
		if(!requirement.empty() && player.Conditions().Get(requirement) <= 0)
			continue;
		// A station can be founded from any planet in a system that doesn't have one yet.
		if(facility.IsStation() ? !hasStation
				: facility.CanBuildOn(planet.TrueName(), planet.Attributes(), planet.IsInhabited()))
			result.push_back(&facility);
	}
	// Stations go first, so they are easy to find.
	stable_partition(result.begin(), result.end(), [](const Facility *facility) { return facility->IsStation(); });
	return result;
}



bool IndustryPanel::HasStationInSystem(const PlayerInfo &player, const Planet &planet)
{
	for(const Industry::Holding &holding : player.GetIndustry().Holdings())
		if(holding.type->IsStation())
		{
			const Planet *station = GameData::Planets().Find(holding.planet);
			if(station && station->GetSystem() == planet.GetSystem())
				return true;
		}
	return false;
}



vector<const Facility *> IndustryPanel::Rows() const
{
	vector<const Facility *> rows = Facilities(player, planet);
	if(HasWarehouse())
		rows.insert(rows.begin(), nullptr);
	return rows;
}



bool IndustryPanel::HasWarehouse() const
{
	const Industry &industry = player.GetIndustry();
	return industry.WarehouseCapacity(planet.TrueName()) > 0 || industry.WarehouseUsed(planet.TrueName()) > 0;
}



const Facility *IndustryPanel::Selected() const
{
	const vector<const Facility *> rows = Rows();
	if(rows.empty())
		return nullptr;
	return rows[clamp(selectedRow, 0, static_cast<int>(rows.size()) - 1)];
}



bool IndustryPanel::IsWarehouseSelected() const
{
	return HasWarehouse() && !Selected();
}



void IndustryPanel::DrawPlanetView()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &faint = *GameData::Colors().Get("faint");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const vector<const Facility *> rows = Rows();
	selectedRow = clamp(selectedRow, 0, static_cast<int>(rows.size()) - 1);
	const Industry &industry = player.GetIndustry();

	font.Draw("Industry on " + planet.DisplayName(), box.TopLeft() + Point(PAD, 0.), bright);

	// List on the left. Scroll it to keep the selection visible.
	const double listTop = box.Top() + LINE + 6.;
	const int visible = VisibleRows(box, listTop);
	scroll = clamp(scroll, max(0, selectedRow - visible + 1), selectedRow);
	for(int i = scroll; i < static_cast<int>(rows.size()) && i < scroll + visible; ++i)
	{
		const Point corner(box.Left(), listTop + (i - scroll) * ROW);
		const Rectangle row = Rectangle::FromCorner(corner, Point(LIST_WIDTH, ROW));
		if(i == selectedRow)
			FillShader::Fill(row, faint);
		string label = "Warehouse";
		if(rows[i])
		{
			const Industry::Holding *holding = industry.Find(*rows[i], planet.TrueName());
			label = rows[i]->TrueName() + (holding ? " x" + to_string(holding->count) : "");
		}
		font.Draw(label, corner + Point(PAD, .5 * (ROW - font.Height())), i == selectedRow ? bright : medium);
		AddZone(row, [this, i]() { selectedRow = i; status.clear(); });
	}

	const double left = box.Left() + LIST_WIDTH + PAD;
	if(!rows[selectedRow])
	{
		DrawWarehouse(left, listTop);
		return;
	}

	// Details of the selected facility on the right.
	const Facility &facility = *rows[selectedRow];
	const Industry::Holding *holding = industry.Find(facility, planet.TrueName());
	const int count = holding ? holding->count : 1;
	const double width = box.Right() - PAD - left;
	Point pos(left, listTop);
	auto line = [&font, &pos](const string &text, const Color &color)
	{
		font.Draw(text, pos, color);
		pos.Y() += LINE;
	};

	line(facility.TrueName() + (holding ? " x" + to_string(count) : ""), bright);
	if(holding)
		line("Status: " + StatusString(holding->status), medium);
	line("Cost: " + Format::CreditString(facility.Cost(), false) + (holding ? " to expand" : ""), medium);
	if(facility.Upkeep())
		line("Upkeep: " + Format::CreditString(facility.Upkeep() * count, false) + " per day", medium);
	if(!facility.Inputs().empty())
		line("Uses: " + AmountsString(facility.Inputs(), count) + " per day", medium);
	if(!facility.Outputs().empty())
		line("Makes: " + AmountsString(facility.Outputs(), count) + " per day", medium);
	if(facility.Warehouse())
		line("Warehouse: " + Format::MassString(facility.Warehouse() * count), medium);

	WrappedText text(font);
	text.SetWrapWidth(width);
	if(holding && !facility.Outputs().empty())
	{
		vector<string> parts;
		for(const Facility::Amounts *amounts : {&facility.Inputs(), &facility.Outputs()})
			for(const auto &[commodity, amount] : *amounts)
				parts.push_back(commodity + " " + to_string(holding->Stock(commodity))
					+ "/" + to_string(holding->Capacity()));
		string stock = "Stock: " + Join(parts) + ".";
		if(holding->autoSell && !facility.Outputs().empty())
		{
			const auto &[commodity, amount] = facility.Outputs().front();
			const string payout = PayoutString(industry, planet.TrueName(), commodity, amount * count);
			stock += payout.empty() ? " Output is auto-sold." : " Auto-selling " + commodity + " pays " + payout + ".";
		}
		text.Wrap(stock);
	}
	else if(!holding)
		text.Wrap(facility.IsStation() ? facility.Description()
			+ " It will be founded in orbit in the " + planet.GetSystem()->DisplayName() + " system."
			: facility.Description());
	else
		text.Wrap("");
	pos.Y() += 4.;
	text.Draw(pos, medium);
	pos.Y() += text.Height();

	// The latest report from this facility, if there is room for it above the buttons.
	const double buttonsTop = box.Bottom() - LINE - BUTTON_SIZE.Y() - 8.;
	if(holding && !holding->reports.empty())
	{
		const auto &[date, report] = holding->reports.back();
		text.Wrap(date + ": " + report);
		if(pos.Y() + text.Height() <= buttonsTop)
			text.Draw(pos, medium);
	}

	// Action buttons along the bottom of the details.
	Point corner(left, buttonsTop);
	const CargoHold &cargo = player.Cargo();
	const bool canAfford = player.Accounts().Credits() >= facility.Cost();
	DrawButton(corner, holding ? "Expand" : facility.IsStation() ? "Found" : "Build", canAfford,
		[this]() { Build(); });
	if(holding)
	{
		bool canSupply = false;
		for(const auto &[commodity, amount] : facility.Inputs())
			canSupply |= (cargo.Get(commodity) > 0 && holding->Stock(commodity) < holding->Capacity());
		if(!facility.Inputs().empty())
			DrawButton(corner, "Supply", canSupply, [this]() { Supply(); });
		if(!facility.Outputs().empty())
		{
			DrawButton(corner, "Collect", holding->OutputStock() > 0 && cargo.Free() > 0, [this]() { Collect(); });
			DrawButton(corner, holding->autoSell ? "Sell: on" : "Sell: off", CanAutoSell(),
				[this]() { ToggleAutoSell(); });
		}
	}
}



void IndustryPanel::DrawWarehouse(double left, double top)
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Industry &industry = player.GetIndustry();
	const string &here = planet.TrueName();

	Point pos(left, top);
	font.Draw("Warehouse", pos, bright);
	pos.Y() += LINE;
	font.Draw("Space used: " + to_string(industry.WarehouseUsed(here)) + " / "
		+ Format::MassString(industry.WarehouseCapacity(here)), pos, medium);
	pos.Y() += LINE + 4.;

	vector<string> parts;
	for(const auto &[commodity, tons] : industry.Warehouse(here))
		if(tons > 0)
			parts.push_back(commodity + " " + to_string(tons));
	WrappedText text(font);
	text.SetWrapWidth(box.Right() - PAD - left);
	text.Wrap(parts.empty() ? "Empty. Store cargo here, or have freight routes deliver to it."
		: "Contents: " + Join(parts) + ".");
	text.Draw(pos, medium);

	const CargoHold &cargo = player.Cargo();
	Point corner(left, box.Bottom() - LINE - BUTTON_SIZE.Y() - 8.);
	DrawButton(corner, "Store", !cargo.Commodities().empty()
		&& industry.WarehouseUsed(here) < industry.WarehouseCapacity(here), [this]() { StoreCargo(); });
	DrawButton(corner, "Load", !parts.empty() && cargo.Free() > 0, [this]() { LoadWarehouse(); });
}



void IndustryPanel::DrawRoutes()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &faint = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	font.Draw("Freight routes", box.TopLeft() + Point(PAD, 0.), bright);
	const string rates = "Freight: " + to_string(Industry::FREIGHT_BASE) + " + "
		+ to_string(Industry::FREIGHT_PER_JUMP) + " per jump, per ton";
	font.Draw(rates, Point(box.Right() - PAD - font.Width(rates), box.Top()), medium);

	const vector<Industry::Route> &routes = player.GetIndustry().Routes();
	const double listTop = box.Top() + LINE + 6.;
	Point buttons(box.Left() + PAD, box.Bottom() - LINE - BUTTON_SIZE.Y() - 8.);
	if(routes.empty())
	{
		WrappedText text(font);
		text.SetWrapWidth(box.Width() - 2. * PAD);
		text.Wrap("You have no freight routes. A route moves goods every day: from your facilities or"
			" warehouse at one planet, or bought on its market, to facilities that need them, a warehouse,"
			" or the market at another. New routes start here.");
		text.Draw(Point(box.Left() + PAD, listTop), medium);
		DrawButton(buttons, "New", true, [this]() { NewRoute(); });
		return;
	}

	// The list of routes, scrolled to keep the selection visible.
	selectedRoute = clamp(selectedRoute, 0, static_cast<int>(routes.size()) - 1);
	scroll = clamp(scroll, max(0, selectedRoute - ROUTE_ROWS + 1), selectedRoute);
	for(int i = scroll; i < static_cast<int>(routes.size()) && i < scroll + ROUTE_ROWS; ++i)
	{
		const Industry::Route &route = routes[i];
		const Point corner(box.Left(), listTop + (i - scroll) * ROW);
		const Rectangle row = Rectangle::FromCorner(corner, Point(box.Width(), ROW));
		if(i == selectedRoute)
			FillShader::Fill(row, faint);
		const string label = to_string(route.tons) + " " + route.commodity + ": "
			+ PlanetName(route.from) + " to " + PlanetName(route.to);
		font.Draw(label, corner + Point(PAD, .5 * (ROW - font.Height())), i == selectedRoute ? bright : medium);
		const string moved = to_string(route.lastMoved) + " t";
		font.Draw(moved, corner + Point(box.Width() - PAD - font.Width(moved), .5 * (ROW - font.Height())), dim);
		AddZone(row, [this, i]() { selectedRoute = i; });
	}

	// Editable fields of the selected route.
	const Industry::Route &route = routes[selectedRoute];
	double y = listTop + ROUTE_ROWS * ROW + 6.;
	const double arrowLeft = box.Left() + 100.;
	const double arrowRight = box.Left() + 290.;
	auto field = [&](const string &label, const string &value, int fieldIndex)
	{
		font.Draw(label, Point(box.Left() + PAD, y), medium);
		Point corner(arrowLeft, y - 1.);
		const Rectangle left = Rectangle::FromCorner(corner, ARROW_SIZE);
		const Rectangle right = Rectangle::FromCorner(Point(arrowRight, y - 1.), ARROW_SIZE);
		for(const Rectangle *arrow : {&left, &right})
		{
			FillShader::Fill(*arrow, dim);
			const string symbol = (arrow == &left) ? "<" : ">";
			font.Draw(symbol, arrow->Center() - .5 * Point(font.Width(symbol), font.Height()), bright);
		}
		AddZone(left, [this, fieldIndex]() { ChangeRoute(fieldIndex, -1); });
		AddZone(right, [this, fieldIndex]() { ChangeRoute(fieldIndex, 1); });
		const double center = .5 * (arrowLeft + ARROW_SIZE.X() + arrowRight);
		font.Draw(value, Point(center - .5 * font.Width(value), y), bright);
	};
	auto toggle = [&](const string &label, bool on, int fieldIndex)
	{
		const Rectangle area = Rectangle::FromCorner(Point(box.Right() - PAD - 90., y - 3.), Point(90., ROW));
		FillShader::Fill(area, on ? dim : faint);
		const string text = label + (on ? ": on" : ": off");
		font.Draw(text, area.Center() - .5 * Point(font.Width(text), font.Height()), on ? bright : medium);
		AddZone(area, [this, fieldIndex]() { ChangeRoute(fieldIndex, 1); });
	};
	field("Carry", route.commodity, COMMODITY);
	y += LINE;
	field("From", PlanetName(route.from), FROM);
	toggle("Buy", route.buy, BUY);
	y += LINE;
	field("To", PlanetName(route.to), TO);
	toggle("Sell", route.sell, SELL);
	y += LINE;
	field("Tons a day", to_string(route.tons), TONS);
	y += LINE;
	string last = route.lastNote;
	if(route.lastCost)
		last += " Freight: " + Format::CreditString(route.lastCost, false) + ".";
	font.Draw(last, Point(box.Left() + PAD, y), medium);
	y += LINE;
	const Industry &industry = player.GetIndustry();
	string market;
	if(route.sell)
	{
		const string payout = PayoutString(industry, route.to, route.commodity, route.tons);
		market = payout.empty() ? "There is no market for " + route.commodity + " at the destination."
			: "Selling " + to_string(route.tons) + " tons there pays " + payout + ".";
	}
	else if(route.buy)
	{
		const int price = MarketPrice(route.from, route.commodity);
		market = price ? "Buying costs " + Format::CreditString(price, false) + " per ton at the source."
			: "There is no market for " + route.commodity + " at the source.";
	}
	if(!market.empty())
		font.Draw(market, Point(box.Left() + PAD, y), dim);

	DrawButton(buttons, "New", true, [this]() { NewRoute(); });
	DrawButton(buttons, "Delete", true, [this]() { DeleteRoute(); });
}



void IndustryPanel::DrawFinances()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	const Industry &industry = player.GetIndustry();
	const Industry::DayReport &report = industry.LastReport();

	font.Draw("Finances", box.TopLeft() + Point(PAD, 0.), bright);
	const string yesterday = "Your industry yesterday";
	font.Draw(yesterday, Point(box.Right() - PAD - font.Width(yesterday), box.Top()), medium);

	// Two columns of figures from the most recent day.
	const double top = box.Top() + LINE + 6.;
	const double middle = box.Left() + .5 * box.Width();
	auto entry = [&](int column, int row, const string &label, int64_t value, bool isCost)
	{
		const double left = column ? middle + PAD : box.Left() + PAD;
		const double right = column ? box.Right() - PAD : middle - PAD;
		const double y = top + row * LINE;
		font.Draw(label, Point(left, y), medium);
		const string amount = (isCost && value ? "-" : "") + Format::CreditString(value, false);
		font.Draw(amount, Point(right - font.Width(amount), y), bright);
	};
	entry(0, 0, "Sales", report.sales, false);
	entry(0, 1, "Upkeep", report.upkeep, true);
	entry(0, 2, "Freight", report.freight, true);
	entry(0, 3, "Goods bought", report.purchases, true);
	entry(1, 0, "Profit", report.profit, false);
	entry(1, 1, "Industry tax", report.tax, true);
	entry(1, 2, "Net result", report.Net(), false);
	entry(1, 3, "Lost to saturation", report.saturationLoss, false);

	// How taxes and saturation are calculated.
	vector<string> brackets;
	const vector<Industry::TaxBracket> &taxes = Industry::TaxBrackets();
	for(size_t i = 0; i < taxes.size(); ++i)
	{
		const string rate = to_string(static_cast<int>(taxes[i].rate * 100. + .5)) + "%";
		if(i + 1 < taxes.size())
			brackets.push_back(rate + " up to " + Format::CreditString(taxes[i + 1].from, false));
		else
			brackets.push_back(rate + " above that");
	}
	WrappedText text(font);
	text.SetWrapWidth(box.Width() - 2. * PAD);
	Point pos(box.Left() + PAD, top + 4 * LINE + 6.);
	const double bottom = box.Bottom() - LINE;
	auto paragraph = [&](const string &str, const Color &color)
	{
		text.Wrap(str);
		if(pos.Y() + text.Height() > bottom)
			return;
		text.Draw(pos, color);
		pos.Y() += text.Height();
	};
	paragraph("Each day's profit is taxed: " + Join(brackets) + ".", medium);
	paragraph("Each ton your industry sells on a market pays price / (1 + saturation / "
		+ to_string(static_cast<int>(Industry::MARKET_DEPTH)) + "), and adds a ton of saturation."
		" Saturation halves every day.", medium);

	// The most saturated markets.
	vector<pair<double, string>> markets;
	for(const auto &[market, value] : industry.Saturations())
		markets.emplace_back(value, market.second + " at " + PlanetName(market.first) + ": "
			+ to_string(static_cast<int>(100. / (1. + value / Industry::MARKET_DEPTH))) + "%");
	sort(markets.rbegin(), markets.rend());
	vector<string> worst;
	for(size_t i = 0; i < markets.size() && i < 3; ++i)
		worst.push_back(markets[i].second);
	paragraph(worst.empty() ? "None of your markets are saturated." : "Current payout: " + Join(worst) + ".", dim);
}



void IndustryPanel::DrawOverview()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const vector<Industry::Holding> &holdings = player.GetIndustry().Holdings();
	int64_t upkeep = 0;
	for(const Industry::Holding &holding : holdings)
		upkeep += holding.type->Upkeep() * holding.count;

	font.Draw("All holdings", box.TopLeft() + Point(PAD, 0.), bright);
	const string total = "Upkeep: " + Format::CreditString(upkeep, false) + " per day";
	font.Draw(total, Point(box.Right() - PAD - font.Width(total), box.Top()), medium);

	if(holdings.empty())
	{
		font.Draw("You don't own any facilities yet.", box.TopLeft() + Point(PAD, LINE + 6.), medium);
		return;
	}

	const double columns[] = {box.Left() + PAD, box.Left() + 130., box.Left() + 300.};
	double y = box.Top() + LINE + 6.;
	font.Draw("Planet", Point(columns[0], y), dim);
	font.Draw("Facility", Point(columns[1], y), dim);
	font.Draw("Status", Point(columns[2], y), dim);
	const string goodsHeading = "Goods";
	font.Draw(goodsHeading, Point(box.Right() - PAD - font.Width(goodsHeading), y), dim);
	y += ROW;

	const int rows = VisibleRows(box, y);
	scroll = clamp(scroll, 0, max(0, static_cast<int>(holdings.size()) - rows));
	for(int i = scroll; i < static_cast<int>(holdings.size()) && i < scroll + rows; ++i)
	{
		const Industry::Holding &holding = holdings[i];
		const Color &color = (holding.planet == planet.TrueName()) ? bright : medium;
		font.Draw(PlanetName(holding.planet), Point(columns[0], y), color);
		font.Draw(holding.type->TrueName() + " x" + to_string(holding.count), Point(columns[1], y), color);
		font.Draw(StatusString(holding.status), Point(columns[2], y), color);
		const string goods = holding.autoSell ? "auto-sold" : Format::MassString(holding.OutputStock());
		font.Draw(goods, Point(box.Right() - PAD - font.Width(goods), y), color);
		y += ROW;
	}
}



void IndustryPanel::DrawButton(Point &corner, const string &label, bool enabled, function<void()> action)
{
	const Font &font = FontSet::Get(14);
	const Rectangle area = Rectangle::FromCorner(corner, BUTTON_SIZE);
	FillShader::Fill(area, *GameData::Colors().Get(enabled ? "dim" : "faint"));
	font.Draw(label, area.Center() - .5 * Point(font.Width(label), font.Height()),
		*GameData::Colors().Get(enabled ? "bright" : "dim"));
	AddZone(area, action);
	corner.X() += BUTTON_SIZE.X() + BUTTON_GAP;
}



void IndustryPanel::Build()
{
	const Facility *facility = Selected();
	if(!facility)
		return;
	const int64_t missing = facility->Cost() - player.Accounts().Credits();
	if(missing > 0)
	{
		status = "You need " + Format::CreditString(missing, false) + " more.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	Industry &industry = player.GetIndustry();
	if(facility->IsStation() && !industry.Find(*facility, planet.TrueName()))
	{
		pendingStation = facility;
		GetUI().Push(DialogPanel::RequestString(this, &IndustryPanel::FoundStation,
			"What would you like to call your new station?", DefaultStationName()));
		return;
	}
	player.Accounts().AddCredits(-facility->Cost());
	industry.Build(*facility, planet.TrueName());
	const int count = industry.Find(*facility, planet.TrueName())->count;
	status = (count == 1 ? "Built a " + facility->TrueName() + " on " + planet.DisplayName()
		: "Expanded your " + facility->TrueName() + " to " + to_string(count) + " units") + ".";
	Messages::Add({status, GameData::MessageCategories().Get("normal")});
	UI::PlaySound(UI::UISound::NORMAL);
}



void IndustryPanel::FoundStation(const string &name)
{
	const Facility *facility = pendingStation;
	pendingStation = nullptr;
	if(!facility)
		return;
	if(!PlayerInfo::IsValidStationName(name))
	{
		status = "That name is already taken, or cannot be used.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	if(player.Accounts().Credits() < facility->Cost() || !player.FoundStation(*facility, name))
		return;
	player.Accounts().AddCredits(-facility->Cost());
	status = "Founded " + name + ". You will see it in orbit when you take off.";
	Messages::Add({"Founded " + name + " in the " + planet.GetSystem()->DisplayName() + " system.",
		GameData::MessageCategories().Get("normal")});
	UI::PlaySound(UI::UISound::NORMAL);
}



void IndustryPanel::Supply()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding)
		return;
	if(facility->Inputs().empty())
	{
		status = "This facility doesn't need any supplies.";
		return;
	}

	CargoHold &cargo = player.Cargo();
	vector<string> supplied;
	vector<string> missing;
	for(const auto &[commodity, amount] : facility->Inputs())
	{
		const int amountSupplied = Industry::Supply(*holding, commodity, cargo.Get(commodity));
		if(amountSupplied)
		{
			// Remove the cost basis of the goods along with the goods themselves.
			player.AdjustBasis(commodity, -player.GetBasis(commodity, amountSupplied));
			cargo.Remove(commodity, amountSupplied);
			supplied.push_back(Format::CargoString(amountSupplied, commodity));
		}
		else if(holding->Stock(commodity) < holding->Capacity())
			missing.push_back(commodity);
	}
	if(!supplied.empty())
	{
		status = "Supplied " + Join(supplied) + ".";
		UI::PlaySound(UI::UISound::NORMAL);
	}
	else
	{
		status = missing.empty() ? "Input storage is already full."
			: "Your cargo hold has no " + Join(missing) + ".";
		UI::PlaySound(UI::UISound::FAILURE);
	}
}



void IndustryPanel::Collect()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding)
		return;

	CargoHold &cargo = player.Cargo();
	if(!holding->OutputStock())
		status = "Nothing to collect yet. Check back in a few days.";
	else if(cargo.Free() <= 0)
		status = "Your fleet has no free cargo space.";
	else
	{
		vector<string> loaded;
		for(const auto &[commodity, amount] : facility->Outputs())
		{
			const int taken = Industry::Collect(*holding, commodity, cargo.Free());
			if(taken)
			{
				cargo.Add(commodity, taken);
				loaded.push_back(Format::CargoString(taken, commodity));
			}
		}
		status = "Loaded " + Join(loaded) + " into your cargo hold.";
		UI::PlaySound(UI::UISound::NORMAL);
		return;
	}
	UI::PlaySound(UI::UISound::FAILURE);
}



void IndustryPanel::ToggleAutoSell()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding || facility->Outputs().empty())
		return;
	if(!CanAutoSell())
	{
		status = "There is no market here to sell to.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	holding->autoSell = !holding->autoSell;
	status = holding->autoSell ? "Output will be sold to the local market every day."
		: "Output will be stored for you to collect.";
	UI::PlaySound(UI::UISound::NORMAL);
}



bool IndustryPanel::CanAutoSell() const
{
	const System *system = planet.GetSystem();
	return planet.IsInhabited() && planet.GetPort().HasService(Port::ServicesType::Trading)
		&& system && system->HasTrade();
}



void IndustryPanel::StoreCargo()
{
	Industry &industry = player.GetIndustry();
	CargoHold &cargo = player.Cargo();
	int total = 0;
	// Copy the list, since storing goods changes it.
	const map<string, int> commodities = cargo.Commodities();
	for(const auto &[commodity, tons] : commodities)
	{
		const int stored = industry.Store(planet.TrueName(), commodity, tons);
		if(!stored)
			continue;
		player.AdjustBasis(commodity, -player.GetBasis(commodity, stored));
		cargo.Remove(commodity, stored);
		total += stored;
	}
	status = total ? "Stored " + Format::MassString(total) + " of cargo in the warehouse."
		: "There is nothing to store, or the warehouse is full.";
	UI::PlaySound(total ? UI::UISound::NORMAL : UI::UISound::FAILURE);
}



void IndustryPanel::LoadWarehouse()
{
	Industry &industry = player.GetIndustry();
	CargoHold &cargo = player.Cargo();
	int total = 0;
	const map<string, int> contents = industry.Warehouse(planet.TrueName());
	for(const auto &[commodity, tons] : contents)
	{
		const int taken = industry.Retrieve(planet.TrueName(), commodity, min(tons, cargo.Free()));
		if(!taken)
			continue;
		cargo.Add(commodity, taken);
		total += taken;
	}
	status = total ? "Loaded " + Format::MassString(total) + " from the warehouse."
		: "The warehouse is empty, or your fleet has no free cargo space.";
	UI::PlaySound(total ? UI::UISound::NORMAL : UI::UISound::FAILURE);
}



void IndustryPanel::NewRoute()
{
	Industry::Route route;
	route.from = planet.TrueName();
	// Default to carrying something made here, to the first other place the player owns.
	route.commodity = "Metal";
	for(const Industry::Holding &holding : player.GetIndustry().Holdings())
		if(holding.planet == planet.TrueName() && !holding.type->Outputs().empty())
		{
			route.commodity = holding.type->Outputs().front().first;
			break;
		}
	route.to = route.from;
	for(const string &location : Locations())
		if(location != route.from)
		{
			route.to = location;
			break;
		}
	player.GetIndustry().AddRoute(route);
	selectedRoute = static_cast<int>(player.GetIndustry().Routes().size()) - 1;
	status = "Added a route. Click the arrows to change it.";
	UI::PlaySound(UI::UISound::NORMAL);
}



void IndustryPanel::DeleteRoute()
{
	Industry &industry = player.GetIndustry();
	if(industry.Routes().empty())
		return;
	selectedRoute = clamp(selectedRoute, 0, static_cast<int>(industry.Routes().size()) - 1);
	industry.RemoveRoute(selectedRoute);
	status = "Deleted the route.";
	UI::PlaySound(UI::UISound::NORMAL);
}



void IndustryPanel::ChangeRoute(int field, int step)
{
	vector<Industry::Route> &routes = player.GetIndustry().Routes();
	if(routes.empty())
		return;
	selectedRoute = clamp(selectedRoute, 0, static_cast<int>(routes.size()) - 1);
	Industry::Route &route = routes[selectedRoute];
	if(field == COMMODITY)
		route.commodity = Cycle(Commodities(), route.commodity, step);
	else if(field == FROM)
		route.from = Cycle(Locations(), route.from, step);
	else if(field == TO)
		route.to = Cycle(Locations(), route.to, step);
	else if(field == TONS)
		route.tons = clamp(route.tons + step * (route.tons < 10 ? 1 : 5), 1, Industry::MAX_ROUTE_TONS);
	else if(field == BUY)
		route.buy = !route.buy;
	else if(field == SELL)
		route.sell = !route.sell;
	status.clear();
}



vector<string> IndustryPanel::Locations() const
{
	set<string> names = {planet.TrueName()};
	for(const Industry::Holding &holding : player.GetIndustry().Holdings())
		names.insert(holding.planet);
	vector<string> result(names.begin(), names.end());
	sort(result.begin(), result.end(), [](const string &a, const string &b) { return PlanetName(a) < PlanetName(b); });
	return result;
}



vector<string> IndustryPanel::Commodities()
{
	vector<string> result;
	for(const Trade::Commodity &commodity : GameData::Commodities())
		result.push_back(commodity.name);
	// Special commodities are only offered if some facility makes them.
	set<string> produced;
	for(const auto &it : GameData::Facilities())
		for(const auto &[commodity, amount] : it.second.Outputs())
			produced.insert(commodity);
	for(const Trade::Commodity &commodity : GameData::SpecialCommodities())
		if(produced.contains(commodity.name))
			result.push_back(commodity.name);
	return result;
}



string IndustryPanel::DefaultStationName() const
{
	const string base = planet.GetSystem()->DisplayName() + " Station";
	if(PlayerInfo::IsValidStationName(base))
		return base;
	for(int i = 2; i < 10; ++i)
		if(PlayerInfo::IsValidStationName(base + " " + to_string(i)))
			return base + " " + to_string(i);
	return "";
}
