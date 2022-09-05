/* MapDetailPanel.cpp
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

#include "MapDetailPanel.h"

#include "text/alignment.hpp"
#include "Angle.h"
#include "Color.h"
#include "Command.h"
#include "CoreStartData.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "FillShader.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Preferences.h"
#include "Radar.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Trade.h"
#include "text/truncate.hpp"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>
#include <vector>

using namespace std;

namespace {
	// Convert the angle between two vectors into a sortable angle, i.e an angle
	// plus a length that is used as a tie-breaker.
	pair<double, double> SortAngle(const Point &reference, const Point &point)
	{
		// Rotate the given point by the reference amount.
		Point rotated(reference.Dot(point), reference.Cross(point));

		// This will be the tiebreaker value: the length, squared.
		double length = rotated.Dot(rotated);
		// Calculate the angle, but rotated 180 degrees so that the discontinuity
		// comes at the reference angle rather than directly opposite it.
		double angle = atan2(-rotated.Y(), -rotated.X());

		// Special case: collinear with the reference vector. If the point is
		// a longer vector than the reference, it's the very best angle.
		// Otherwise, it is the very worst angle. (Note: this also is applied if
		// the angle is opposite (angle == 0) but then it's a no-op.)
		if(!rotated.Y())
			angle = copysign(angle, rotated.X() - reference.Dot(reference));

		// Return the angle, plus the length as a tie-breaker.
		return make_pair(angle, length);
	}
}

double MapDetailPanel::scroll = 0.;



MapDetailPanel::MapDetailPanel(PlayerInfo &player, const System *system)
	: MapPanel(player, system ? MapPanel::SHOW_REPUTATION : player.MapColoring(), system)
{
}



MapDetailPanel::MapDetailPanel(const MapPanel &panel)
	: MapPanel(panel)
{
	// Use whatever map coloring is specified in the PlayerInfo.
	commodity = player.MapColoring();
}



void MapDetailPanel::Step()
{
	MapPanel::Step();

	if(selectedSystem != shownSystem)
		GeneratePlanetCards(*selectedSystem);
	if(!player.GetPlanet())
		DoHelp("map");
	if(GetUI()->IsTop(this) && player.GetPlanet() && player.GetDate() >= player.StartData().GetDate() + 12)
		DoHelp("map advanced ports");
}



void MapDetailPanel::Draw()
{
	MapPanel::Draw();

	DrawInfo();
	DrawOrbits();
	DrawKey();
}



bool MapDetailPanel::Scroll(double dx, double dy)
{
	Point point = UI::GetMouse();
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	if(maxScroll && point.X() < Screen::Left() + planetCardInterface->GetValue("width")
		&& point.Y() > Screen::Top() + mapInterface->GetValue("planet starting Y")
		&& point.Y() < Screen::Bottom() - mapInterface->GetValue("planet max bottom Y"))
	{
		double scrollSpeed = mapInterface->GetValue("planet scroll speed");
		if(dy > 0.)
			SetScroll(scroll - dy * scrollSpeed);
		else if(dy < 0. && scroll - dy * scrollSpeed < maxScroll)
			SetScroll(scroll - dy * scrollSpeed);

		return true;
	}
	return MapPanel::Scroll(dx, dy);
}



double MapDetailPanel::GetScroll()
{
	return scroll;
}



// Only override the ones you need; the default action is to return false.
bool MapDetailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if((key == SDLK_TAB || command.Has(Command::JUMP)) && player.Flagship())
	{
		// Clear the selected planet, if any.
		selectedPlanet = nullptr;
		SetScroll(0.);
		// Toggle to the next link connected to the "source" system. If the
		// shift key is down, the source is the end of the travel plan; otherwise
		// it is one step before the end.
		vector<const System *> &plan = player.TravelPlan();
		const System *source = plan.empty() ? player.GetSystem() : plan.front();
		const System *next = nullptr;
		Point previousUnit = Point(0., -1.);
		if(!plan.empty() && !(mod & KMOD_SHIFT))
		{
			previousUnit = plan.front()->Position();
			plan.erase(plan.begin());
			next = source;
			source = plan.empty() ? player.GetSystem() : plan.front();
			previousUnit = (previousUnit - source->Position()).Unit();
		}
		Point here = source->Position();
		const System *original = next;

		// Depending on whether the flagship has a jump drive, the possible links
		// we can travel along are different:
		bool hasJumpDrive = player.Flagship()->Attributes().Get("jump drive");
		const set<const System *> &links = hasJumpDrive ? source->JumpNeighbors(player.Flagship()->JumpRange()) : source->Links();

		// For each link we can travel from this system, check whether the link
		// is closer to the current angle (while still being larger) than any
		// link we have seen so far.
		auto bestAngle = make_pair(4., 0.);
		for(const System *it : links)
		{
			// Skip the currently selected link, if any. Also skip links to
			// systems the player has not seen, and skip hyperspace links if the
			// player has not visited either end of them.
			if(it == original)
				continue;
			if(!player.HasSeen(*it))
				continue;
			if(!(hasJumpDrive || player.HasVisited(*it) || player.HasVisited(*source)))
				continue;

			// Generate a sortable angle with vector length as a tiebreaker.
			// Otherwise if two systems are in exactly the same direction it is
			// not well defined which one comes first.
			auto angle = SortAngle(previousUnit, it->Position() - here);
			if(angle < bestAngle)
			{
				next = it;
				bestAngle = angle;
			}
		}
		if(next)
		{
			plan.insert(plan.begin(), next);
			Select(next);
		}
	}
	else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && player.HasTravelPlan())
	{
		vector<const System *> &plan = player.TravelPlan();
		plan.erase(plan.begin());
		Select(plan.empty() ? player.GetSystem() : plan.front());
	}
	else if(key == SDLK_DOWN)
	{
		if(commodity < 0 || commodity == 9)
			SetCommodity(0);
		else
			SetCommodity(commodity + 1);
	}
	else if(key == SDLK_UP)
	{
		if(commodity <= 0)
			SetCommodity(9);
		else
			SetCommodity(commodity - 1);
	}
	else
		return MapPanel::KeyDown(key, mod, command, isNewPress);

	return true;
}



bool MapDetailPanel::Click(int x, int y, int clicks)
{
	bool yInPlanetCards = (y < tradeY && y > governmentY);
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const double planetCardWidth = planetCardInterface->GetValue("width");
	if(x < Screen::Left() + 160 && !yInPlanetCards)
	{
		// The player clicked in the left-hand interface. This could be the system
		// name, the system government, a planet box, the commodity listing, or nothing.
		if(y >= tradeY && y < tradeY + 200)
		{
			// The player clicked on a tradable commodity. Color the map by its price.
			SetCommodity((y - tradeY) / 20);
			return true;
		}
		// Clicking the system name activates the view of the player's reputation with various governments.
		else if(y < governmentY)
			SetCommodity(SHOW_REPUTATION);
		// Clicking the government name activates the view of system / planet ownership.
		else if(y >= governmentY && y < governmentY + 20)
			SetCommodity(SHOW_GOVERNMENT);
	}
	else if(x < Screen::Left() + planetCardWidth && yInPlanetCards)
	{
		const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
		bool clickedArrow = (maxScroll && x > Screen::Left() + planetCardWidth - mapInterface->GetValue("arrow offset") - 5.);
		if(clickedArrow)
		{
			const double planetCardHeight = planetCardInterface->GetValue("height");
			bool arrowUp = (y < governmentY + planetCardHeight && !planetCards.front().IsShown());
			bool arrowDown = (!arrowUp && y > tradeY - planetCardHeight && !planetCards.back().IsShown());
			scroll += (arrowUp ? -planetCardHeight : arrowDown ? planetCardHeight : 0.);
		}
		else
		{
			for(auto &card : planetCards)
			{
				MapPlanetCard::ClickAction clickAction = card.Click(x, y, clicks);
				if(clickAction == MapPlanetCard::ClickAction::GOTO_SHIPYARD)
				{
					GetUI()->Pop(this);
					GetUI()->Push(new MapShipyardPanel(*this, true));
					break;
				}
				else if(clickAction == MapPlanetCard::ClickAction::GOTO_OUTFITTER)
				{
					GetUI()->Pop(this);
					GetUI()->Push(new MapOutfitterPanel(*this, true));
					break;
				}
				// Then this is the planet selected.
				else if(clickAction != MapPlanetCard::ClickAction::NONE)
				{
					selectedPlanet = card.GetPlanet();
					if(clickAction != MapPlanetCard::ClickAction::SELECTED)
						SetCommodity(static_cast<int>(clickAction));
				}
			}
		}
		return true;
	}
	else if(x >= Screen::Right() - 240 && y <= Screen::Top() + 270)
	{
		// The player has clicked within the "orbits" scene.
		// Select the nearest planet to the click point.
		Point click = Point(x, y);
		selectedPlanet = nullptr;
		double distance = numeric_limits<double>::infinity();
		for(const auto &it : planets)
		{
			double d = click.Distance(it.second);
			if(d < distance)
			{
				distance = d;
				selectedPlanet = it.first;
			}
		}
		if(selectedPlanet && player.Flagship())
			player.SetTravelDestination(selectedPlanet);

		return true;
	}

	// The click was not on an interface element, so check if it was on a system.
	MapPanel::Click(x, y, clicks);
	// If the system just changed, the selected planet is no longer valid.
	if(selectedPlanet && !selectedPlanet->IsInSystem(selectedSystem))
		selectedPlanet = nullptr;
	return true;
}



bool MapDetailPanel::RClick(int x, int y)
{
	if(!Preferences::Has("System map sends move orders"))
		return true;
	// TODO: rewrite the map panels to be driven from interfaces.txt so these XY
	// positions aren't hard-coded.
	else if(x >= Screen::Right() - 240 && y >= Screen::Top() + 10 && y <= Screen::Top() + 270)
	{
		// Only handle clicks on the actual orbits element, rather than the whole UI region.
		// (Note: this isn't perfect, and the clickable area extends into the angled sides a bit.)
		const Point orbitCenter(Screen::TopRight() + Point(-120., 160.));
		auto uiClick = Point(x, y) - orbitCenter;
		if(uiClick.Length() > 130)
			return true;

		// Only issue movement orders if the player is in-flight.
		if(player.GetPlanet())
			GetUI()->Push(new Dialog("You cannot issue fleet movement orders while docked."));
		else if(!player.HasVisited(*selectedSystem))
			GetUI()->Push(new Dialog("You must visit this system before you can send your fleet there."));
		else
			player.SetEscortDestination(selectedSystem, uiClick / scale);
	}

	return true;
}



void MapDetailPanel::GeneratePlanetCards(const System &system)
{
	set<const Planet *> shown;

	planetCards.clear();
	SetScroll(0.);
	unsigned number = 0;
	for(const StellarObject &object : system.Objects())
		if(object.HasSprite() && object.HasValidPlanet())
		{
			// The same "planet" may appear multiple times in one system,
			// providing multiple landing and departure points (e.g. ringworlds).
			const Planet *planet = object.GetPlanet();
			if(planet->IsWormhole() || !planet->IsAccessible(player.Flagship()) || shown.count(planet))
				continue;

			planetCards.emplace_back(object, number, player.HasVisited(*planet));
			shown.insert(planet);
			++number;
		}
	shownSystem = &system;
}



// Draw the legend, correlating between a system's color and the value of the
// selected "commodity," which may be reputation level, outfitter size, etc.
void MapDetailPanel::DrawKey()
{
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);

	Point pos = Screen::TopRight() + Point(-110., 310.);
	Point headerOff(-5., -.5 * font.Height());
	Point textOff(10., -.5 * font.Height());

	static const string HEADER[] = {
		"Trade prices:",
		"Ships for sale:",
		"Outfits for sale:",
		"You have visited:",
		"", // Special should never be active in this mode.
		"Government:",
		"System:"
	};
	const string &header = HEADER[-min(0, max(-6, commodity))];
	font.Draw(header, pos + headerOff, medium);
	pos.Y() += 20.;

	if(commodity >= 0)
	{
		// Each system is colored by the selected commodity's price. Draw
		// four distinct colors and the price each color represents.
		const vector<Trade::Commodity> &commodities = GameData::Commodities();
		const auto &range = commodities[commodity];
		if(static_cast<unsigned>(commodity) >= commodities.size())
			return;

		for(int i = 0; i <= 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(i * (2. / 3.) - 1.));
			int price = range.low + ((range.high - range.low) * i) / 3;
			font.Draw(to_string(price), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity >= SHOW_OUTFITTER)
	{
		// Each system is colored by the number of outfits for sale.
		static const string LABEL[2][4] = {
			{"None", "1", "5", "10+"},
			{"None", "1", "30", "60+"}};
		static const double VALUE[4] = {-1., 0., .5, 1.};

		for(int i = 0; i < 4; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
			font.Draw(LABEL[commodity == SHOW_OUTFITTER][i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_VISITED)
	{
		static const string LABEL[3] = {
			"All planets",
			"Some",
			"None"
		};
		for(int i = 0; i < 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(1 - i));
			font.Draw(LABEL[i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_GOVERNMENT)
	{
		// Each system is colored by the government of the system. Only the
		// four largest visible governments are labeled in the legend.
		vector<pair<double, const Government *>> distances;
		for(const auto &it : closeGovernments)
			distances.emplace_back(it.second, it.first);
		sort(distances.begin(), distances.end());
		for(unsigned i = 0; i < 4 && i < distances.size(); ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, GovernmentColor(distances[i].second));
			font.Draw(distances[i].second->GetName(), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_REPUTATION)
	{
		// Each system is colored in accordance with the player's reputation
		// with its owning government. The specific colors associated with a
		// given reputation (0.1, 100, and 10000) are shown for each sign.
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(1e-1, true, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(1e2, true, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(1e4, true, false));
		font.Draw("Friendly", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;

		RingShader::Draw(pos, OUTER, INNER, ReputationColor(-1e-1, false, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(-1e2, false, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(-1e4, false, false));
		font.Draw("Hostile", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;

		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, false));
		font.Draw("Restricted", pos + textOff, dim);
		pos.Y() += 20.;

		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, true));
		font.Draw("Dominated", pos + textOff, dim);
		pos.Y() += 20.;
	}

	RingShader::Draw(pos, OUTER, INNER, UninhabitedColor());
	font.Draw("Uninhabited", pos + textOff, dim);
	pos.Y() += 20.;

	RingShader::Draw(pos, OUTER, INNER, UnexploredColor());
	font.Draw("Unexplored", pos + textOff, dim);
}



// Draw the various information displays: system name & government, planetary
// details, trade prices, and details about the selected object.
void MapDetailPanel::DrawInfo()
{
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");

	const Color &back = *GameData::Colors().Get("map side panel background");

	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	double planetHeight = planetCardInterface->GetValue("height");
	double planetWidth = planetCardInterface->GetValue("width");
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	double startingY = mapInterface->GetValue("planet starting Y");
	double bottomY = mapInterface->GetValue("planet max bottom Y");

	bool hasVisited = player.HasVisited(*selectedSystem);

	// Draw the panel for the planets. If the system was not visited, no planets will be shown.
	Point size(planetWidth, min((Screen::Height() - bottomY - startingY),
		(hasVisited ? planetCards.size() : 0.) * planetHeight));
	// This needs to fill from the start of the screen.
	FillShader::Fill(Screen::TopLeft() + Point(size.X() / 2., size.Y() / 2. + startingY / 2.),
		size + Point(0., startingY), back);

	// Edges:
	Point pos(Screen::Left(), Screen::Top() + startingY);
	const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
	Point edgePos = pos + Point(.5 * size.X(), size.Y());
	Point bottomOff(-30., .5 * bottom->Height());
	SpriteShader::Draw(bottom, edgePos + bottomOff);

	const Sprite *right = SpriteSet::Get("ui/right edge");
	Point rightOff(.5 * (size.X() + right->Width()), -right->Height() / 2.);
	SpriteShader::Draw(right, edgePos + rightOff);

	const double startingX = mapInterface->GetValue("starting X");
	Point uiPoint(Screen::Left() + startingX, Screen::Top() + startingY);

	// Draw the basic information for visitable planets in this system.
	if(hasVisited)
	{
		maxScroll = 0.;
		bool drawArrows = false;
		for(auto &card : planetCards)
		{
			// This updates the location of the card so it needs to be called before AvailableSpace().
			bool wasDrawn = card.DrawIfFits(uiPoint);
			const double availableSpace = card.AvailableSpace();
			// Fit another planet, if we can, also give scrolling freedom to reach the planets at the end.
			if(wasDrawn)
				uiPoint.Y() += availableSpace;
			else
				drawArrows = true;

			// Do this all of the time so we can scroll if an element is partially shown.
			maxScroll += (planetHeight - availableSpace);
		}

		if(drawArrows)
		{
			const double arrowOffset = mapInterface->GetValue("arrow offset");
			// Draw the pointers to go up and down by a planet.
			if(!planetCards.front().IsShown())
				PointerShader::Draw(Point(Screen::Left() + planetWidth - arrowOffset,
					Screen::Top() + startingY + arrowOffset), Point(0., -1.), 10.f, 10.f, 5.f, medium);
			if(!planetCards.back().IsShown())
				PointerShader::Draw(uiPoint + Point(planetWidth - startingX - arrowOffset, -arrowOffset),
					Point(0., 1.), 10.f, 10.f, 5.f, medium);
		}
	}

	const double startingGovernmentY = mapInterface->GetValue("government top Y");
	const double textMargin = mapInterface->GetValue("text margin");
	uiPoint = Point(Screen::Left() + textMargin, Screen::Top() + startingGovernmentY);

	// Draw the information for the government of this system at the top.
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");
	SpriteShader::Draw(systemSprite, uiPoint + Point(systemSprite->Width() / 2. - textMargin, 0.));

	const Font &font = FontSet::Get(14);
	string systemName = player.KnowsName(*selectedSystem) ?
		selectedSystem->Name() : "Unexplored System";
	const auto alignLeft = Layout(140, Truncate::BACK);
	font.Draw({systemName, alignLeft}, uiPoint + Point(0., -7.), medium);

	governmentY = uiPoint.Y() + textMargin;
	string gov = player.HasVisited(*selectedSystem) ?
		selectedSystem->GetGovernment()->GetName() : "Unknown Government";
	font.Draw({gov, alignLeft}, uiPoint + Point(0., 13.), (commodity == SHOW_GOVERNMENT) ? medium : dim);
	if(commodity == SHOW_GOVERNMENT)
		PointerShader::Draw(uiPoint + Point(0., 20.), Point(1., 0.),
			10.f, 10.f, 0.f, medium);

	const double relativeTradeY = mapInterface->GetValue("relative trade Y after planet");
	uiPoint = Point(Screen::Left() + startingX, edgePos.Y() + relativeTradeY);
	const double tradeHeight = mapInterface->GetValue("trade height");
	tradeY = uiPoint.Y() - tradeHeight / 2.;

	// Trade sprite goes after the rest.
	const Sprite *tradeSprite = SpriteSet::Get("ui/map trade");
	SpriteShader::Draw(tradeSprite, uiPoint);

	// Adapt the coordinates for the text (the sprite is drawn from a center coordinate).
	uiPoint.X() -= (tradeSprite->Width() / 2. - textMargin);
	uiPoint.Y() -= (tradeSprite->Height() / 2. - textMargin);
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		bool isSelected = false;
		if(static_cast<unsigned>(this->commodity) < GameData::Commodities().size())
			isSelected = (&commodity == &GameData::Commodities()[this->commodity]);
		const Color &color = isSelected ? medium : dim;

		font.Draw(commodity.name, uiPoint, color);

		string price;

		bool hasVisited = player.HasVisited(*selectedSystem);
		if(hasVisited && selectedSystem->IsInhabited(player.Flagship()))
		{
			int value = selectedSystem->Trade(commodity.name);
			int localValue = (player.GetSystem() ? player.GetSystem()->Trade(commodity.name) : 0);
			// Don't "compare" prices if the current system is uninhabited and
			// thus has no prices to compare to.
			bool noCompare = (!player.GetSystem() || !player.GetSystem()->IsInhabited(player.Flagship()));
			if(!value)
				price = "----";
			else if(noCompare || player.GetSystem() == selectedSystem || !localValue)
				price = to_string(value);
			else
			{
				value -= localValue;
				price += "(";
				if(value > 0)
					price += '+';
				price += to_string(value);
				price += ")";
			}
		}
		else
			price = (hasVisited ? "n/a" : "?");

		const auto alignRight = Layout(140, Alignment::RIGHT, Truncate::BACK);
		font.Draw({price, alignRight}, uiPoint, color);

		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10.f, 10.f, 0.f, color);

		uiPoint.Y() += 20.;
	}

	if(selectedPlanet && !selectedPlanet->Description().empty()
			&& player.HasVisited(*selectedPlanet) && !selectedPlanet->IsWormhole())
	{
		static const int X_OFFSET = 240;
		static const int WIDTH = 500;
		const Sprite *panelSprite = SpriteSet::Get("ui/description panel");
		Point pos(Screen::Right() - X_OFFSET - .5f * panelSprite->Width(),
			Screen::Top() + .5f * panelSprite->Height());
		SpriteShader::Draw(panelSprite, pos);

		WrappedText text(font);
		text.SetAlignment(Alignment::JUSTIFIED);
		text.SetWrapWidth(WIDTH - 20);
		text.Wrap(selectedPlanet->Description());
		text.Draw(Point(Screen::Right() - X_OFFSET - WIDTH, Screen::Top() + 20), medium);
	}

	DrawButtons("is ports");
}



// Draw the planet orbits in the currently selected system, on the current day.
void MapDetailPanel::DrawOrbits()
{
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits and key");
	SpriteShader::Draw(orbitSprite, Screen::TopRight() + .5 * Point(-orbitSprite->Width(), orbitSprite->Height()));
	Point orbitCenter = Screen::TopRight() + Point(-120., 160.);

	if(!player.HasVisited(*selectedSystem))
		return;

	const Font &font = FontSet::Get(14);

	// Figure out what the largest orbit in this system is.
	double maxDistance = 0.;
	for(const StellarObject &object : selectedSystem->Objects())
	{
		double distance = object.Distance();
		int activeParent = object.Parent();
		while(activeParent >= 0)
		{
			distance += selectedSystem->Objects()[activeParent].Distance();
			activeParent = selectedSystem->Objects()[activeParent].Parent();
		}
		maxDistance = max(maxDistance, distance);
	}
	// 2400 -> 120.
	scale = .03;
	maxDistance *= scale;

	if(maxDistance > 115.)
		scale *= 115. / maxDistance;

	// Draw the orbits.
	static const Color habitColor[7] = {
		Color(.4, .2, .2, 1.),
		Color(.3, .3, 0., 1.),
		Color(0., .4, 0., 1.),
		Color(0., .3, .4, 1.),
		Color(.1, .2, .5, 1.),
		Color(.2, .2, .2, 1.),
		Color(1., 1., 1., 1.)
	};
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;

		Point parentPos;
		int habit = 5;
		if(object.Parent() >= 0)
			parentPos = selectedSystem->Objects()[object.Parent()].Position();
		else
		{
			double warmth = object.Distance() / selectedSystem->HabitableZone();
			habit = (warmth > .5) + (warmth > .8) + (warmth > 1.2) + (warmth > 2.0);
		}

		double radius = object.Distance() * scale;
		RingShader::Draw(orbitCenter + parentPos * scale,
			radius + .7, radius - .7,
			habitColor[habit]);
	}

	// Draw the planets themselves.
	planets.clear();
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;

		Point pos = orbitCenter + object.Position() * scale;
		if(object.HasValidPlanet() && object.GetPlanet()->IsAccessible(player.Flagship()))
			planets[object.GetPlanet()] = pos;

		const float *rgb = Radar::GetColor(object.RadarType(player.Flagship())).Get();
		// Darken and saturate the color, and make it opaque.
		Color color(max(0.f, rgb[0] * 1.2f - .2f), max(0.f, rgb[1] * 1.2f - .2f), max(0.f, rgb[2] * 1.2f - .2f), 1.f);
		RingShader::Draw(pos, object.Radius() * scale + 1., 0.f, color);
	}

	// If the player has a pending order for escorts to move to a new system, draw it.
	if(player.HasEscortDestination())
	{
		auto pendingOrder = player.GetEscortDestination();
		if(pendingOrder.first == selectedSystem)
		{
			// Draw an X (to mark the spot, of course).
			auto uiPoint = (pendingOrder.second * scale) + orbitCenter;
			const Color *color = GameData::Colors().Get("map orbits fleet destination");
			// TODO: Add a "batch pointershader" method that takes the shape description, a count, and a reference point+orientation
			// Use that method below and in Engine for drawing target reticles.
			auto a = Angle{45.};
			auto inc = Angle{90.};

			PointerShader::Bind();
			for(int i = 0; i < 4; ++i)
			{
				PointerShader::Add(uiPoint, a.Unit(), 6.f, 6.f, -3.f, *color);
				a += inc;
			}
			PointerShader::Unbind();
		}
	}

	// Draw the selection ring on top of everything else.
	for(const StellarObject &object : selectedSystem->Objects())
		if(selectedPlanet && object.GetPlanet() == selectedPlanet)
			RingShader::Draw(orbitCenter + object.Position() * scale,
				object.Radius() * scale + 5., object.Radius() * scale + 4.,
				habitColor[6]);

	// Draw the name of the selected planet.
	const string &name = selectedPlanet ? selectedPlanet->Name() : selectedSystem->Name();
	Point namePos(Screen::Right() - 190., Screen::Top() + 7.);
	font.Draw({name, {180, Alignment::CENTER, Truncate::BACK}},
		namePos, *GameData::Colors().Get("medium"));
}



// Set the commodity coloring, and update the player info as well.
void MapDetailPanel::SetCommodity(int index)
{
	commodity = index;
	player.SetMapColoring(commodity);
}



void MapDetailPanel::SetScroll(double newScroll)
{
	MapDetailPanel::scroll = max(0., newScroll);
}
