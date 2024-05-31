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
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Preferences.h"
#include "Radar.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipJumpNavigation.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Trade.h"
#include "text/truncate.hpp"
#include "UI.h"
#include "Wormhole.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>
#include <vector>

using namespace std;

namespace {
	// Convert the angle between two vectors into a sortable angle, i.e. an angle
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
double MapDetailPanel::planetPanelHeight = 0.;



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

	if(GetUI()->IsTop(this) && player.GetPlanet() && player.GetDate() >= player.StartData().GetDate() + 12)
	{
		DoHelp("map advanced danger");
		DoHelp("map advanced ports");
	}
	if(!player.GetPlanet())
		DoHelp("map");
}



void MapDetailPanel::Draw()
{
	MapPanel::Draw();

	DrawInfo();
	DrawOrbits();
	DrawKey();
	FinishDrawing("is ports");
}



double MapDetailPanel::GetScroll()
{
	return scroll;
}



double MapDetailPanel::PlanetPanelHeight()
{
	return planetPanelHeight;
}



bool MapDetailPanel::Hover(int x, int y)
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	isPlanetViewSelected = (x < Screen::Left() + planetCardInterface->GetValue("width")
		&& y < Screen::Top() + PlanetPanelHeight());

	return isPlanetViewSelected ? true : MapPanel::Hover(x, y);
}



bool MapDetailPanel::Drag(double dx, double dy)
{
	if(isPlanetViewSelected)
	{
		SetScroll(scroll - dy);

		return true;
	}
	return MapPanel::Drag(dx, dy);
}



bool MapDetailPanel::Scroll(double dx, double dy)
{
	if(isPlanetViewSelected)
	{
		SetScroll(scroll - dy * Preferences::ScrollSpeed());

		return true;
	}
	return MapPanel::Scroll(dx, dy);
}



// Only override the ones you need; the default action is to return false.
bool MapDetailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	const double planetCardHeight = MapPlanetCard::Height();
	if(command.Has(Command::HELP))
	{
		DoHelp("map advanced danger", true);
		DoHelp("map advanced ports", true);
		if(!player.GetPlanet())
			DoHelp("map", true);
	}
	else if((key == SDLK_TAB || command.Has(Command::JUMP)) && player.Flagship())
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
		bool hasJumpDrive = player.Flagship()->JumpNavigation().HasJumpDrive();
		const set<const System *> &links = hasJumpDrive
			? source->JumpNeighbors(player.Flagship()->JumpNavigation().JumpRange()) : source->Links();

		// For each link we can travel from this system, check whether the link
		// is closer to the current angle (while still being larger) than any
		// link we have seen so far.
		auto bestAngle = make_pair(4., 0.);
		for(const System *it : links)
		{
			// Skip the currently selected link, if any, and non valid system links. Also skip links to
			// systems the player has not seen, and skip hyperspace links if the
			// player cannot view either end of them.
			if(!it->IsValid() || it == original)
				continue;
			if(!player.HasSeen(*it))
				continue;
			if(!(hasJumpDrive || player.CanView(*it) || player.CanView(*source)))
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
		if(!isPlanetViewSelected)
		{
			if(commodity < 0 || commodity == 9)
				SetCommodity(0);
			else
				SetCommodity(commodity + 1);
		}
		else
		{
			bool selectNext = false;
			for(auto &card : planetCards)
			{
				if(selectNext)
				{
					card.Select();
					double space = card.AvailableSpace();
					if(space < planetCardHeight)
						scroll += (planetCardHeight - space);
					break;
				}
				// We have this one selected, the next one will be selected instead.
				else if(card.IsSelected())
				{
					if(!planetCards.back().IsSelected())
						selectNext = true;
					card.Select(false);
				}
			}
			// If none/the last one are considered selected, it will select the first one from the list.
			if(!selectNext && !planetCards.empty())
			{
				SetScroll(0.);
				planetCards.front().Select();
			}
		}

	}
	else if(key == SDLK_UP)
	{
		if(!isPlanetViewSelected)
		{
			if(commodity <= 0)
				SetCommodity(9);
			else
				SetCommodity(commodity - 1);
		}
		else
		{
			MapPlanetCard *previousCard = &planetCards.front();
			bool anySelected = false;
			for(auto &card : planetCards)
			{
				if(card.IsSelected())
				{
					if(!planetCards.front().IsSelected())
						anySelected = true;
					previousCard->Select();
					card.Select(false);
					double space = previousCard->AvailableSpace();
					if(space < planetCardHeight)
						scroll -= (planetCardHeight - space);
					break;
				}
				previousCard = &card;
			}
			if(!anySelected && !planetCards.empty())
			{
				SetScroll(maxScroll);
				planetCards.back().Select();
			}
		}
	}
	else
		return MapPanel::KeyDown(key, mod, command, isNewPress);

	return true;
}



bool MapDetailPanel::Click(int x, int y, int clicks)
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const double planetCardWidth = planetCardInterface->GetValue("width");
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const double arrowOffset = mapInterface->GetValue("arrow x offset");
	const double planetCardHeight = MapPlanetCard::Height();
	if(x < Screen::Left() + 160)
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
		// But the bit to the left will show danger of pirate/raid fleets instead.
		else if(y < governmentY && y > governmentY - 30)
			SetCommodity(x < Screen::Left() + mapInterface->GetValue("text margin") ?
				SHOW_DANGER : SHOW_REPUTATION);
		// Clicking the government name activates the view of system / planet ownership.
		else if(y >= governmentY && y < governmentY + 25)
			SetCommodity(SHOW_GOVERNMENT);
	}
	if(y <= Screen::Top() + planetPanelHeight + 30 && x <= Screen::Left() + planetCardWidth + arrowOffset + 10)
	{
		if(maxScroll && x > Screen::Left() + planetCardWidth + arrowOffset - 10)
		{
			// The arrows are of size 10.
			const double arrowVerticalOffset = mapInterface->GetValue("arrow y offset") + 10.;
			bool arrowUp = (y < Screen::Top() + arrowVerticalOffset);
			bool arrowDown = (!arrowUp && y > Screen::Top() + planetPanelHeight - arrowVerticalOffset);
			SetScroll(scroll + (arrowUp ? -planetCardHeight : arrowDown ? planetCardHeight : 0.));
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
					if(selectedPlanet && player.Flagship())
						player.SetTravelDestination(selectedPlanet);
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
				int place = 0;
				for(auto &planetCard : planetCards)
				{
					planetCard.Select(planetCard.GetPlanet() == selectedPlanet);
					if(planetCard.IsSelected())
						SetScroll(place * planetCardHeight);
					++place;
				}
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
		else if(!player.CanView(*selectedSystem))
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
	MapPlanetCard::ResetSize();
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

	Point pos = Screen::TopRight() + Point(-130., 310.);
	Point headerOff(-5., -.5 * font.Height());
	Point textOff(10., -.5 * font.Height());

	static const string HEADER[] = {
		"Trade prices:",
		"Ships for sale:",
		"Outfits for sale:",
		"You have visited:",
		"", // Special should never be active in this mode.
		"Government:",
		"System:",
		"Danger level:"
	};
	const string &header = HEADER[-min(0, max(-7, commodity))];
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
		{
			if(!it.first)
				continue;
			distances.emplace_back(it.second, it.first);
		}
		sort(distances.begin(), distances.end());
		int drawn = 0;
		vector<pair<string, Color>> alreadyDisplayed;
		for(const auto &it : distances)
		{
			const string &displayName = it.second->GetName();
			const Color &displayColor = it.second->GetColor();
			auto foundIt = find(alreadyDisplayed.begin(), alreadyDisplayed.end(),
					make_pair(displayName, displayColor));
			if(foundIt != alreadyDisplayed.end())
				continue;
			RingShader::Draw(pos, OUTER, INNER, GovernmentColor(it.second));
			font.Draw(displayName, pos + textOff, dim);
			pos.Y() += 20.;
			alreadyDisplayed.emplace_back(displayName, displayColor);
			++drawn;
			if(drawn >= 4)
				break;
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
	else if(commodity == SHOW_DANGER)
	{
		RingShader::Draw(pos, OUTER, INNER, DangerColor(numeric_limits<double>::quiet_NaN()));
		font.Draw("None", pos + textOff, dim);
		pos.Y() += 20.;
		// Each system is colored in accordance with its danger to the player,
		// including threats from any "raid fleet" presence.
		static const string labels[4] = {"Minimal", "Low", "Moderate", "High"};
		for(int i = 0; i < 4; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, DangerColor(i / 3.));
			font.Draw(labels[i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}

	if(commodity != SHOW_DANGER)
	{
		RingShader::Draw(pos, OUTER, INNER, UninhabitedColor());
		font.Draw("Uninhabited", pos + textOff, dim);
		pos.Y() += 20.;
	}

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
	double planetCardHeight = MapPlanetCard::Height();
	double planetWidth = planetCardInterface->GetValue("width");
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	double minPlanetPanelHeight = mapInterface->GetValue("min planet panel height");
	double maxPlanetPanelHeight = mapInterface->GetValue("max planet panel height");

	const double bottomGovY = mapInterface->GetValue("government Y");
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");

	bool canView = player.CanView(*selectedSystem);

	// Draw the panel for the planets. If the system was not visited, no planets will be shown.
	const double minimumSize = max(minPlanetPanelHeight, Screen::Height() - bottomGovY - systemSprite->Height());
	planetPanelHeight = canView ? min(min(minimumSize, maxPlanetPanelHeight),
		(planetCards.size()) * planetCardHeight) : 0.;
	Point size(planetWidth, planetPanelHeight);
	// This needs to fill from the start of the screen.
	FillShader::Fill(Screen::TopLeft() + Point(size.X() / 2., size.Y() / 2.),
		size, back);

	const double startingX = mapInterface->GetValue("starting X");
	Point uiPoint(Screen::Left() + startingX, Screen::Top());

	// Draw the basic information for visitable planets in this system.
	if(canView && !planetCards.empty())
	{
		uiPoint.Y() -= GetScroll();
		maxScroll = 0.;
		for(auto &card : planetCards)
		{
			// Fit another planet, if we can, also give scrolling freedom to reach the planets at the end.
			// This updates the location of the card so it needs to be called before AvailableSpace().
			card.DrawIfFits(uiPoint);
			uiPoint.Y() += planetCardHeight;

			// Do this all of the time so we can scroll if an element is partially shown.
			maxScroll += (planetCardHeight - card.AvailableSpace());
		}

		// Edges:
		Point pos(Screen::Left(), Screen::Top());
		const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
		Point edgePos = pos + Point(.5 * size.X(), size.Y());
		Point bottomOff(-23.5, .5 * bottom->Height() - 1);
		SpriteShader::Draw(bottom, edgePos + bottomOff);

		const Sprite *right = SpriteSet::Get("ui/right edge");
		Point rightOff(.5 * (size.X() + right->Width()) - 1, -right->Height() / 2.);
		SpriteShader::Draw(right, edgePos + rightOff);

		if(maxScroll)
		{
			const double arrowOffsetX = mapInterface->GetValue("arrow x offset");
			const double arrowOffsetY = mapInterface->GetValue("arrow y offset");
			// Draw the pointers to go up and down by a planet at most.
			PointerShader::Draw(Point(Screen::Left() + planetWidth + arrowOffsetX,
				Screen::Top() + arrowOffsetY), Point(0., -1.), 10.f, 10.f, 5.f, scroll ? medium : dim);
			PointerShader::Draw(Point(Screen::Left() + planetWidth + arrowOffsetX,
				Screen::Top() - arrowOffsetY + planetPanelHeight), Point(0., 1.), 10.f, 10.f, 5.f,
				(scroll < maxScroll) ? medium : dim);
		}
	}

	const double textMargin = mapInterface->GetValue("text margin");
	uiPoint = Point(Screen::Left() + textMargin, Screen::Bottom() - bottomGovY);

	// Draw the information for the government of this system at the top of the trade sprite.
	SpriteShader::Draw(systemSprite, uiPoint + Point(systemSprite->Width() / 2. - textMargin, 0.));

	const Font &font = FontSet::Get(14);
	const Sprite *alertSprite = SpriteSet::Get(commodity == SHOW_DANGER ? "ui/red alert" : "ui/red alert grayed");
	const float alertScale = min<float>(1.f, min<double>(textMargin,
		font.Height()) / max(alertSprite->Width(), alertSprite->Height()));
	SpriteShader::Draw(alertSprite, uiPoint + Point(-textMargin / 2., -7. + font.Height() / 2.), alertScale);

	string systemName = player.KnowsName(*selectedSystem) ?
		selectedSystem->Name() : "Unexplored System";
	const auto alignLeft = Layout(145, Truncate::BACK);
	font.Draw({systemName, alignLeft}, uiPoint + Point(0., -7.), medium);

	governmentY = uiPoint.Y() + textMargin;
	string gov = canView ? selectedSystem->GetGovernment()->GetName() : "Unknown Government";
	font.Draw({gov, alignLeft}, uiPoint + Point(0., 13.), (commodity == SHOW_GOVERNMENT) ? medium : dim);
	if(commodity == SHOW_GOVERNMENT)
		PointerShader::Draw(uiPoint + Point(0., 20.), Point(1., 0.),
			10.f, 10.f, 0.f, medium);

	const double tradeHeight = mapInterface->GetValue("trade height");
	uiPoint = Point(Screen::Left() + startingX, Screen::Bottom() - tradeHeight);

	// Trade sprite goes after at the bottom.
	const Sprite *tradeSprite = SpriteSet::Get("ui/map trade");
	SpriteShader::Draw(tradeSprite, uiPoint);
	tradeY = uiPoint.Y() - tradeSprite->Height() / 2. + 15.;

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
		if(canView && selectedSystem->IsInhabited(player.Flagship()))
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
			price = (canView ? "n/a" : "?");

		const auto alignRight = Layout(140, Alignment::RIGHT, Truncate::BACK);
		font.Draw({price, alignRight}, uiPoint, color);

		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10.f, 10.f, 0.f, color);

		uiPoint.Y() += 20.;
	}

	if(selectedPlanet && !selectedPlanet->Description().empty()
			&& player.HasVisited(*selectedPlanet) && !selectedPlanet->IsWormhole())
	{
		static const int X_OFFSET = 190;
		static const int WIDTH = 500;
		const Sprite *panelSprite = SpriteSet::Get("ui/description panel");
		Point pos(Screen::Left() + X_OFFSET + .5f * panelSprite->Width(),
			Screen::Bottom() - .5f * panelSprite->Height());
		SpriteShader::Draw(panelSprite, pos);

		WrappedText text(font);
		text.SetAlignment(Alignment::JUSTIFIED);
		text.SetWrapWidth(WIDTH - 20);
		text.Wrap(selectedPlanet->Description());
		text.Draw(Point(Screen::Left() + X_OFFSET + 20, Screen::Bottom() - panelSprite->Height() + 20), medium);

		selectedSystemOffset = 0;
	}
}



// Draw the planet orbits in the currently selected system, on the current day.
void MapDetailPanel::DrawOrbits()
{
	planets.clear();
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits and key");
	SpriteShader::Draw(orbitSprite, Screen::TopRight() + .5 * Point(-orbitSprite->Width(), orbitSprite->Height()));
	Point orbitCenter = Screen::TopRight() + Point(-120., 160.);

	if(!player.CanView(*selectedSystem))
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
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;

		Point pos = orbitCenter + object.Position() * scale;
		// Special case: wormholes which would lead to an inaccessible location should not
		// be drawn as landable.
		bool hasPlanet = object.HasValidPlanet();
		bool inaccessible = hasPlanet && object.GetPlanet()->GetWormhole()
			&& object.GetPlanet()->GetWormhole()->WormholeDestination(*selectedSystem).Inaccessible();
		if(hasPlanet && object.GetPlanet()->IsAccessible(player.Flagship()) && !inaccessible)
			planets[object.GetPlanet()] = pos;

		// The above wormhole check prevents the wormhole from being selected, but does not change its color
		// on the orbits radar.
		const float *rgb = inaccessible ? Radar::GetColor(Radar::INACTIVE).Get()
			: Radar::GetColor(object.RadarType(player.Flagship())).Get();
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
			// TODO: Add a "batch pointershader" method that takes
			// the shape description, a count, and a reference point+orientation.
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
	if(scroll > maxScroll)
		scroll = maxScroll;
}
