/* Government.cpp
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

#include "Government.h"

#include "Conversation.h"
#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"
#include "Outfit.h"
#include "Phrase.h"
#include "Politics.h"
#include "Ship.h"
#include "ShipEvent.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Load ShipEvent strings and corresponding numerical values into a map.
	void PenaltyHelper(const DataNode &node, map<int, Government::PenaltyEffect> &penalties)
	{
		auto loadPenalty = [&penalties](const DataNode &child, int eventType) -> void
		{
			double amount = child.Value(1);
			Government::SpecialPenalty specialPenalty = Government::SpecialPenalty::NONE;
			// Reverse compatibility: transfer the provoke and atrocity events to their respective
			// special penalties if one is not explicitly given to them.
			if(eventType == ShipEvent::PROVOKE)
				specialPenalty = Government::SpecialPenalty::PROVOKE;
			else if(eventType == ShipEvent::ATROCITY)
				specialPenalty = Government::SpecialPenalty::ATROCITY;
			if(child.Size() >= 3)
			{
				const string &effect = child.Token(2);
				if(effect == "none")
					specialPenalty = Government::SpecialPenalty::NONE;
				else if(effect == "provoke")
				{
					specialPenalty = Government::SpecialPenalty::PROVOKE;
					if(amount <= 0.)
					{
						child.PrintTrace("The \"provoke\" effect will not work"
							" without a positive, non-zero penalty to reputation. Defaulting to 0:");
						amount = 0.;
					}
				}
				else if(effect == "atrocity")
				{
					specialPenalty = Government::SpecialPenalty::ATROCITY;
					if(amount <= .05)
					{
						child.PrintTrace("The \"atrocity\" effect will not work"
							" without a penalty to reputation higher or equal to 0.05. Defaulting to 0.05:");
						amount = .05;
					}
				}
				else
					child.PrintTrace("Skipping unrecognized reputation effect:");
			}
			penalties[eventType] = Government::PenaltyEffect(amount, specialPenalty);
		};
		for(const DataNode &child : node)
			if(child.Size() >= 2)
			{
				const string &key = child.Token(0);
				if(key == "assist")
					loadPenalty(child, ShipEvent::ASSIST);
				else if(key == "disable")
					loadPenalty(child, ShipEvent::DISABLE);
				else if(key == "board")
					loadPenalty(child, ShipEvent::BOARD);
				else if(key == "capture")
					loadPenalty(child, ShipEvent::CAPTURE);
				else if(key == "destroy")
					loadPenalty(child, ShipEvent::DESTROY);
				else if(key == "scan")
				{
					loadPenalty(child, ShipEvent::SCAN_OUTFITS);
					loadPenalty(child, ShipEvent::SCAN_CARGO);
				}
				else if(key == "provoke")
					loadPenalty(child, ShipEvent::PROVOKE);
				else if(key == "atrocity")
					loadPenalty(child, ShipEvent::ATROCITY);
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
			else
				child.PrintTrace("Skipping unrecognized attribute:");
	}

	// Determine the penalty for the given ShipEvent based on the values in the given map.
	Government::PenaltyEffect PenaltyHelper(int eventType, const map<int, Government::PenaltyEffect> &penalties)
	{
		Government::PenaltyEffect penalty;
		for(const auto &it : penalties)
			if(eventType & it.first)
			{
				penalty.reputationChange += it.second.reputationChange;
				if(it.second.specialPenalty > penalty.specialPenalty)
					penalty.specialPenalty = it.second.specialPenalty;
			}
		return penalty;
	}

	unsigned nextID = 0;
}



// Default constructor.
Government::Government()
{
	// Default penalties:
	penaltyFor[ShipEvent::ASSIST] = PenaltyEffect(-.1);
	penaltyFor[ShipEvent::DISABLE] = PenaltyEffect(.5);
	penaltyFor[ShipEvent::BOARD] = PenaltyEffect(.3);
	penaltyFor[ShipEvent::CAPTURE] = PenaltyEffect(1.);
	penaltyFor[ShipEvent::DESTROY] = PenaltyEffect(1.);
	penaltyFor[ShipEvent::SCAN_OUTFITS] = PenaltyEffect(0.);
	penaltyFor[ShipEvent::SCAN_CARGO] = PenaltyEffect(0.);
	penaltyFor[ShipEvent::PROVOKE] = PenaltyEffect(0., SpecialPenalty::PROVOKE);
	penaltyFor[ShipEvent::ATROCITY] = PenaltyEffect(10., SpecialPenalty::ATROCITY);

	id = nextID++;
}



// Load a government's definition from a file.
void Government::Load(const DataNode &node, const set<const System *> *visitedSystems,
	const set<const Planet *> *visitedPlanets)
{
	if(node.Size() >= 2)
	{
		trueName = node.Token(1);
		if(displayName.empty())
			displayName = trueName;
	}

	// For the following keys, if this data node defines a new value for that
	// key, the old values should be cleared (unless using the "add" keyword).
	set<string> shouldOverwrite = {"raid"};

	for(const DataNode &child : node)
	{
		bool remove = child.Token(0) == "remove";
		bool add = child.Token(0) == "add";
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = child.Size() > valueIndex;

		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		bool removeAll = (remove && !hasValue);
		// If this is the first entry for the given key, and we are not in "add"
		// or "remove" mode, its previous value should be cleared.
		bool overwriteAll = (!add && !remove && shouldOverwrite.contains(key));

		if(removeAll || overwriteAll)
		{
			if(key == "provoked on scan")
				provokedOnScan = false;
			else if(key == "travel restrictions")
				travelRestrictions = LocationFilter{};
			else if(key == "reputation")
			{
				for(const DataNode &grand : child)
				{
					const string &grandKey = grand.Token(0);
					if(grandKey == "max")
						reputationMax = numeric_limits<double>::max();
					else if(grandKey == "min")
						reputationMin = numeric_limits<double>::lowest();
				}
			}
			else if(key == "raid")
				raidFleets.clear();
			else if(key == "display name")
				displayName = trueName;
			else if(key == "death sentence")
				deathSentence = nullptr;
			else if(key == "friendly hail")
				friendlyHail = nullptr;
			else if(key == "friendly disabled hail")
				friendlyDisabledHail = nullptr;
			else if(key == "hostile hail")
				hostileHail = nullptr;
			else if(key == "hostile disabled hail")
				hostileDisabledHail = nullptr;
			else if(key == "ship bribe acceptance hail")
				shipBribeAcceptanceHail = nullptr;
			else if(key == "ship bribe rejection hail")
				shipBribeRejectionHail = nullptr;
			else if(key == "planet bribe acceptance hail")
				planetBribeAcceptanceHail = nullptr;
			else if(key == "planet bribe rejection hail")
				planetBribeRejectionHail = nullptr;
			else if(key == "language")
				language.clear();
			else if(key == "send untranslated hails")
				sendUntranslatedHails = false;
			else if(key == "trusted")
				trusted.clear();
			else if(key == "enforces")
				enforcementZones.clear();
			else if(key == "custom penalties for")
				customPenalties.clear();
			else if(key == "foreign penalties for")
				useForeignPenaltiesFor.clear();
			else if(key == "illegals")
			{
				illegalOutfits.clear();
				illegalShips.clear();
			}
			else if(key == "atrocities")
			{
				atrocityOutfits.clear();
				atrocityShips.clear();
			}
			else if(key == "bribe threshold")
				bribeThreshold = 0.;
			else
				child.PrintTrace("Cannot \"remove\" the given key:");

			// If not in "overwrite" mode, move on to the next node.
			if(overwriteAll)
				shouldOverwrite.erase(key);
			else
				continue;
		}

		if(key == "raid")
			RaidFleet::Load(raidFleets, child, remove, valueIndex);
		// Handle the attributes which cannot have a value removed.
		else if(remove)
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
		else if(key == "attitude toward")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Size() >= 2)
				{
					const Government *gov = GameData::Governments().Get(grand.Token(0));
					attitudeToward.resize(nextID, numeric_limits<double>::quiet_NaN());
					attitudeToward[gov->id] = grand.Value(1);
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(key == "reputation")
		{
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				bool hasGrandValue = grand.Size() >= 2;
				if(grandKey == "player reputation" && hasGrandValue)
					initialPlayerReputation = add ? initialPlayerReputation + grand.Value(valueIndex) : grand.Value(valueIndex);
				else if(grandKey == "max" && hasGrandValue)
					reputationMax = add ? reputationMax + grand.Value(valueIndex) : grand.Value(valueIndex);
				else if(grandKey == "min" && hasGrandValue)
					reputationMin = add ? reputationMin + grand.Value(valueIndex) : grand.Value(valueIndex);
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(key == "trusted")
		{
			bool clearTrusted = !trusted.empty();
			for(const DataNode &grand : child)
			{
				bool remove = grand.Token(0) == "remove";
				bool add = grand.Token(0) == "add";
				if((add || remove) && grand.Size() < 2)
				{
					grand.PrintTrace("Skipping invalid \"" + child.Token(0) + "\" tag:");
					continue;
				}
				if(clearTrusted && !add && !remove)
				{
					trusted.clear();
					clearTrusted = false;
				}
				const Government *gov = GameData::Governments().Get(grand.Token(remove || add));
				if(gov)
				{
					if(remove)
						trusted.erase(gov);
					else
						trusted.insert(gov);
				}
				else
					grand.PrintTrace("Skipping unrecognized government:");
			}
		}
		else if(key == "penalty for")
			PenaltyHelper(child, penaltyFor);
		else if(key == "custom penalties for")
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				if(grandKey == "remove" && grand.Size() >= 2)
					customPenalties[GameData::Governments().Get(grand.Token(1))->id].clear();
				else
				{
					auto &pens = customPenalties[GameData::Governments().Get(grandKey)->id];
					PenaltyHelper(grand, pens);
				}
			}
		else if(key == "illegals")
		{
			if(!add)
			{
				illegalOutfits.clear();
				illegalShips.clear();
			}
			for(const DataNode &grand : child)
			{
				if(grand.Size() < 2)
				{
					grand.PrintTrace("Skipping unrecognized attribute:");
					continue;
				}
				const string &grandKey = grand.Token(0);
				if(grandKey == "remove")
				{
					if(grand.Token(1) == "ignore universal")
						ignoreUniversalIllegals = false;
					else if(grand.Token(1) == "ship" && grand.Size() >= 3)
					{
						if(!illegalShips.erase(grand.Token(2)))
							grand.PrintTrace("Invalid remove, ship not found in existing illegals:");
					}
					else if(!illegalOutfits.erase(GameData::Outfits().Get(grand.Token(1))))
						grand.PrintTrace("Invalid remove, outfit not found in existing illegals:");
				}
				else if(grandKey == "ignore universal")
					ignoreUniversalIllegals = true;
				else if(grandKey == "ignore")
				{
					if(grand.Token(1) == "ship" && grand.Size() >= 3)
						illegalShips[grand.Token(2)] = 0;
					else
						illegalOutfits[GameData::Outfits().Get(grand.Token(1))] = 0;
				}
				else if(grandKey == "ship" && grand.Size() >= 3)
					illegalShips[grand.Token(1)] = grand.Value(2);
				else
					illegalOutfits[GameData::Outfits().Get(grandKey)] = grand.Value(1);
			}
		}
		else if(key == "atrocities")
		{
			if(!add)
			{
				atrocityOutfits.clear();
				atrocityShips.clear();
			}
			const Conversation *deathSentenceForBlock = nullptr;
			if(child.Size() >= valueIndex + 2 && child.Token(valueIndex) == "death sentence")
				deathSentenceForBlock = GameData::Conversations().Get(child.Token(valueIndex + 1));
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				if(grand.Size() == 1)
					atrocityOutfits[GameData::Outfits().Get(grandKey)] = {true, deathSentenceForBlock};
				else if(grandKey == "remove")
				{
					if(grand.Token(1) == "ignore universal")
						ignoreUniversalAtrocities = false;
					if(grand.Token(1) == "ship" && grand.Size() >= 3)
					{
						if(!atrocityShips.erase(grand.Token(2)))
							grand.PrintTrace("Invalid remove, ship not found in existing atrocities:");
					}
					else if(!atrocityOutfits.erase(GameData::Outfits().Get(grand.Token(1))))
						grand.PrintTrace("Invalid remove, outfit not found in existing atrocities:");
				}
				else if(grandKey == "ignore universal")
						ignoreUniversalAtrocities = true;
				else if(grandKey == "ignore")
				{
					if(grand.Token(1) == "ship" && grand.Size() >= 3)
						atrocityShips[grand.Token(2)].isAtrocity = false;
					else
						atrocityOutfits[GameData::Outfits().Get(grand.Token(1))].isAtrocity = false;
				}
				else if(grandKey == "ship")
					atrocityShips[grand.Token(1)] = {true, deathSentenceForBlock};
			}
		}
		else if(key == "enforces" && child.HasChildren())
			enforcementZones.emplace_back(child, visitedSystems, visitedPlanets);
		else if(key == "provoked on scan")
			provokedOnScan = true;
		else if(key == "travel restrictions" && child.HasChildren())
		{
			if(add)
				travelRestrictions.Load(child, visitedSystems, visitedPlanets);
			else
				travelRestrictions = LocationFilter(child, visitedSystems, visitedPlanets);
		}
		else if(key == "foreign penalties for")
			for(const DataNode &grand : child)
				useForeignPenaltiesFor.insert(GameData::Governments().Get(grand.Token(0))->id);
		else if(key == "send untranslated hails")
			sendUntranslatedHails = true;
		else if(!hasValue)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "default attitude")
			defaultAttitude = child.Value(valueIndex);
		else if(key == "player reputation")
			initialPlayerReputation = add ? initialPlayerReputation + child.Value(valueIndex) : child.Value(valueIndex);
		else if(key == "crew attack")
			crewAttack = max(0., add ? child.Value(valueIndex) + crewAttack : child.Value(valueIndex));
		else if(key == "crew defense")
			crewDefense = max(0., add ? child.Value(valueIndex) + crewDefense : child.Value(valueIndex));
		else if(key == "bribe")
			bribe = child.Value(valueIndex) + (add ? bribe : 0.);
		else if(key == "bribe threshold")
			bribeThreshold = child.Value(valueIndex) + (add ? bribeThreshold : 0.);
		else if(key == "fine")
			fine = child.Value(valueIndex) + (add ? fine : 0.);
		else if(add)
			child.PrintTrace("Unsupported use of add:");
		else if(key == "display name")
			displayName = child.Token(valueIndex);
		else if(key == "swizzle")
			swizzle = GameData::Swizzles().Get(child.Token(valueIndex));
		else if(key == "color")
		{
			if(child.Size() >= 3 + valueIndex)
				color = ExclusiveItem<Color>(Color(child.Value(valueIndex),
						child.Value(valueIndex + 1), child.Value(valueIndex + 2)));
			else if(child.Size() >= 1 + valueIndex)
				color = ExclusiveItem<Color>(GameData::Colors().Get(child.Token(valueIndex)));
		}
		else if(key == "death sentence")
			deathSentence = GameData::Conversations().Get(child.Token(valueIndex));
		else if(key == "friendly hail")
			friendlyHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "friendly disabled hail")
			friendlyDisabledHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "hostile hail")
			hostileHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "hostile disabled hail")
			hostileDisabledHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "ship bribe acceptance hail")
			shipBribeAcceptanceHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "ship bribe rejection hail")
			shipBribeRejectionHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "planet bribe acceptance hail")
			planetBribeAcceptanceHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "planet bribe rejection hail")
			planetBribeRejectionHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "language")
			language = child.Token(valueIndex);
		else if(key == "enforces" && child.Token(valueIndex) == "all")
		{
			enforcementZones.clear();
			child.PrintTrace("Deprecated use of \"enforces all\". Use \"remove enforces\" instead:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// Ensure reputation minimum is not above the
	// maximum, and set reputation again to enforce limits.
	if(reputationMin > reputationMax)
		reputationMin = reputationMax;
	SetReputation(Reputation());
}



// Get the display name of this government.
const string &Government::DisplayName() const
{
	return displayName;
}



// Set / Get the true name used for this government in the data files.
void Government::SetTrueName(const string &trueName)
{
	this->trueName = trueName;
}



const string &Government::TrueName() const
{
	return trueName;
}



// Get the color swizzle to use for ships of this government.
const Swizzle *Government::GetSwizzle() const
{
	return swizzle;
}



// Get the color to use for displaying this government on the map.
const Color &Government::GetColor() const
{
	return *color;
}



// Get the government's initial disposition toward other governments or
// toward the player.
double Government::AttitudeToward(const Government *other) const
{
	if(!other)
		return 0.;
	if(other == this)
		return 1.;

	if(attitudeToward.size() <= other->id)
		return defaultAttitude;

	double attitude = attitudeToward[other->id];
	return std::isnan(attitude) ? defaultAttitude : attitude;
}



double Government::InitialPlayerReputation() const
{
	return initialPlayerReputation;
}



// Get the amount that your reputation changes for the given offense against the given government.
// The given value should be a combination of one or more ShipEvent values.
// Returns 0 if the Government is null.
Government::PenaltyEffect Government::PenaltyFor(int eventType, const Government *other) const
{
	if(!other)
		return 0.;

	if(other == this)
		return PenaltyHelper(eventType, penaltyFor);

	const int id = other->id;
	const auto &penalties = useForeignPenaltiesFor.contains(id) ? other->penaltyFor : penaltyFor;

	const auto it = customPenalties.find(id);
	if(it == customPenalties.end())
		return PenaltyHelper(eventType, penalties);

	map<int, PenaltyEffect> tempPenalties = penalties;
	for(const auto &penalty : it->second)
		tempPenalties[penalty.first] = penalty.second;
	return PenaltyHelper(eventType, tempPenalties);
}



// In order to successfully bribe this government you must pay them this
// fraction of your fleet's value. (Zero means they cannot be bribed.)
double Government::GetBribeFraction() const
{
	return bribe;
}



// This government will never accept a bribe if the player's reputation
// with them is below this value, if it is negative. If the value is 0,
// bribes are accepted regardless of reputation.
double Government::GetBribeThreshold() const
{
	return bribeThreshold;
}



double Government::GetFineFraction() const
{
	return fine;
}



bool Government::Trusts(const Government *government) const
{
	return government == this || trusted.contains(government);
}



// Returns true if this government has no enforcement restrictions, or if the
// indicated system matches at least one enforcement zone.
bool Government::CanEnforce(const System *system) const
{
	for(const LocationFilter &filter : enforcementZones)
		if(filter.Matches(system))
			return true;
	return enforcementZones.empty();
}



// Returns true if this government has no enforcement restrictions, or if the
// indicated planet matches at least one enforcement zone.
bool Government::CanEnforce(const Planet *planet) const
{
	for(const LocationFilter &filter : enforcementZones)
		if(filter.Matches(planet))
			return true;
	return enforcementZones.empty();
}



const Conversation *Government::DeathSentence() const
{
	return deathSentence;
}



// Get a hail message (which depends on whether this is an enemy government
// and if the ship is disabled).
string Government::GetHail(bool isDisabled) const
{
	const Phrase *phrase = nullptr;

	if(IsEnemy())
		phrase = isDisabled ? hostileDisabledHail : hostileHail;
	else
		phrase = isDisabled ? friendlyDisabledHail : friendlyHail;

	return phrase ? phrase->Get() : "";
}



string Government::GetShipBribeAcceptanceHail() const
{
	return shipBribeAcceptanceHail ? shipBribeAcceptanceHail->Get() : "It's a pleasure doing business with you.";
}



string Government::GetShipBribeRejectionHail() const
{
	return shipBribeRejectionHail ? shipBribeRejectionHail->Get() : "I do not want your money.";
}



string Government::GetPlanetBribeAcceptanceHail() const
{
	return planetBribeAcceptanceHail ? planetBribeAcceptanceHail->Get() : "It's a pleasure doing business with you.";
}



string Government::GetPlanetBribeRejectionHail() const
{
	return planetBribeRejectionHail ? planetBribeRejectionHail->Get() : "I do not want your money.";
}



// Find out if this government speaks a different language.
const string &Government::Language() const
{
	return language;
}



// Find out if this government should send custom hails even if the player does not know its language.
bool Government::SendUntranslatedHails() const
{
	return sendUntranslatedHails;
}



// Pirate raids in this government's systems use these fleet definitions. If
// it is empty, there are no pirate raids.
// The second attribute denotes the minimal and maximal attraction required for the fleet to appear.
const vector<RaidFleet> &Government::RaidFleets() const
{
	return raidFleets;
}



// Check if, according to the politics stored by GameData, this government is
// an enemy of the given government right now.
bool Government::IsEnemy(const Government *other) const
{
	return GameData::GetPolitics().IsEnemy(this, other);
}



// Check if this government is an enemy of the player.
bool Government::IsEnemy() const
{
	return GameData::GetPolitics().IsEnemy(this, GameData::PlayerGovernment());
}



// Check if this is the player government.
bool Government::IsPlayer() const
{
	return (this == GameData::PlayerGovernment());
}



// Commit the given "offense" against this government (which may not
// actually consider it to be an offense). This may result in temporary
// hostilities (if the even type is PROVOKE), or a permanent change to your
// reputation.
void Government::Offend(int eventType, int count) const
{
	return GameData::GetPolitics().Offend(this, eventType, count);
}



// Bribe this government to be friendly to you for one day.
void Government::Bribe() const
{
	GameData::GetPolitics().Bribe(this);
}



// Check to see if the player has done anything they should be fined for.
// Each government can only fine you once per day.
pair<const Conversation *, string> Government::Fine(PlayerInfo &player, int scan,
	const Ship *target, double security) const
{
	return GameData::GetPolitics().Fine(player, this, scan, target, security);
}



Government::Atrocity Government::Condemns(const Outfit *outfit) const
{
	const auto it = atrocityOutfits.find(outfit);
	return it != atrocityOutfits.cend() ? it->second
		: Atrocity{!IgnoresUniversalAtrocities() && outfit->Get("atrocity") > 0., nullptr};
}



Government::Atrocity Government::Condemns(const Ship *ship) const
{
	const auto it = atrocityShips.find(ship->TrueModelName());
	return it != atrocityShips.cend() ? it->second
		: Atrocity{!IgnoresUniversalAtrocities() && ship->BaseAttributes().Get("atrocity") > 0., nullptr};
}



bool Government::IgnoresUniversalAtrocities() const
{
	return ignoreUniversalAtrocities;
}



int Government::Fines(const Outfit *outfit) const
{
	// If this government doesn't fine anything, it won't fine this outfit.
	if(!fine)
		return 0;

	for(const auto &it : illegalOutfits)
		if(it.first == outfit)
			return it.second;
	return IgnoresUniversalIllegals() ? 0 : outfit->Get("illegal");
}



int Government::Fines(const Ship *ship) const
{
	// If this government doesn't fine anything, it won't fine this ship.
	if(!fine)
		return 0;

	for(const auto &it : illegalShips)
		if(it.first == ship->TrueModelName())
			return it.second;
	return IgnoresUniversalIllegals() ? 0 : ship->BaseAttributes().Get("illegal");
}



bool Government::IgnoresUniversalIllegals() const
{
	return ignoreUniversalIllegals;
}



bool Government::FinesContents(const Ship *ship) const
{
	for(auto &it : ship->Outfits())
		if(this->Fines(it.first) || this->Condemns(it.first).isAtrocity)
			return true;

	return ship->Cargo().IllegalCargoFine(this).first;
}



// Get or set the player's reputation with this government.
double Government::Reputation() const
{
	return GameData::GetPolitics().Reputation(this);
}



double Government::ReputationMax() const
{
	return reputationMax;
}



double Government::ReputationMin() const
{
	return reputationMin;
}



void Government::AddReputation(double value) const
{
	GameData::GetPolitics().AddReputation(this, value);
}



void Government::SetReputation(double value) const
{
	GameData::GetPolitics().SetReputation(this, value);
}



double Government::CrewAttack() const
{
	return crewAttack;
}



double Government::CrewDefense() const
{
	return crewDefense;
}



bool Government::IsProvokedOnScan() const
{
	return provokedOnScan;
}



bool Government::IsRestrictedFrom(const System &system) const
{
	return !travelRestrictions.IsEmpty() && travelRestrictions.Matches(&system);
}



bool Government::IsRestrictedFrom(const Planet &planet) const
{
	return !travelRestrictions.IsEmpty() && travelRestrictions.Matches(&planet);
}
