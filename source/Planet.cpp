/* Planet.cpp
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

#include "Planet.h"

#include "DataNode.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "image/SpriteSet.h"
#include "System.h"
#include "Wormhole.h"

#include <algorithm>

using namespace std;

namespace {
	const string WORMHOLE = "wormhole";
	const string PLANET = "planet";

	// Planet attributes in the form "requires: <attribute>" restrict the ability of ships to land
	// unless the ship has all required attributes.
	void SetRequiredAttributes(const set<string> &attributes, set<string> &required)
	{
		static const string PREFIX = "requires: ";
		static const string PREFIX_END = "requires:!";
		required.clear();
		for_each(attributes.lower_bound(PREFIX), attributes.lower_bound(PREFIX_END), [&](const string &attribute)
		{
			required.emplace_hint(required.cend(), attribute.substr(PREFIX.length()));
		});
	}
}



// Load a planet's description from a file.
void Planet::Load(const DataNode &node, Set<Wormhole> &wormholes, const ConditionsStore *playerConditions)
{
	if(node.Size() < 2)
		return;
	trueName = node.Token(1);
	// The planet's name is needed to save references to this object, so a
	// flag is used to test whether Load() was called at least once for it.
	isDefined = true;

	// If this planet has been loaded before, these sets of items should be
	// reset instead of appending to them:
	set<string> shouldOverwrite = {"attributes", "description", "spaceport", "port"};

	for(const DataNode &child : node)
	{
		// Check for the "add" or "remove" keyword.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		// Get the key and value (if any).
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);

		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		bool removeAll = (remove && !hasValue);
		// "<key> clear" is the deprecated way of writing "remove <key>."
		removeAll |= (!add && !remove && hasValue && value == "clear");
		// If this is the first entry for the given key, and we are not in "add"
		// or "remove" mode, its previous value should be cleared.
		bool overwriteAll = (!add && !remove && !removeAll && shouldOverwrite.contains(key));
		// Clear the data of the given type.
		if(removeAll || overwriteAll)
		{
			// Clear the data of the given type.
			if(key == "display name")
				displayName.clear();
			else if(key == "music")
				music.clear();
			else if(key == "attributes")
				attributes.clear();
			else if(key == "description")
				description.Clear();
			else if(key == "port" || key == "spaceport")
			{
				port = Port();
				// Overwriting either port or spaceport counts as overwriting the other.
				if(overwriteAll)
				{
					shouldOverwrite.erase("port");
					shouldOverwrite.erase("spaceport");
				}
			}
			else if(key == "shipyard")
				shipSales.clear();
			else if(key == "outfitter")
				outfitSales.clear();
			else if(key == "government")
				government = nullptr;
			else if(key == "required reputation")
				requiredReputation = 0.;
			else if(key == "bribe")
				bribe = 0.;
			else if(key == "bribe threshold")
				bribeThreshold = 0.;
			else if(key == "security")
				security = 0.;
			else if(key == "tribute")
			{
				tribute = 0;
				defenseThreshold = 4000;
				defenseFleets.clear();
				ResetDefense();
			}
			else if(key == "wormhole")
				wormhole = nullptr;
			else if(key == "to")
			{
				if(value == "know")
					toKnow = ConditionSet();
				else if(value == "land")
					toLand = ConditionSet();
				else if(value == "access" && child.Size() > valueIndex + 1)
				{
					const string &shop = child.Token(valueIndex + 1);
					if(shop == "shipyard")
						toAccessShipyard = ConditionSet();
					else if(shop == "outfitter")
						toAccessOutfitter = ConditionSet();
				}
			}

			// If not in "overwrite" mode, move on to the next node.
			if(overwriteAll)
				shouldOverwrite.erase(key);
			else
				continue;
		}

		if(key == "port")
			port.Load(child, playerConditions);
		// Handle the attributes which can be "removed."
		else if(!hasValue)
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		else if(key == "attributes")
		{
			if(remove)
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.erase(child.Token(i));
			else
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.insert(child.Token(i));
		}
		else if(key == "shipyard")
		{
			if(remove)
				shipSales.erase(GameData::Shipyards().Get(value));
			else
				shipSales.insert(GameData::Shipyards().Get(value));
		}
		else if(key == "outfitter")
		{
			if(remove)
				outfitSales.erase(GameData::Outfitters().Get(value));
			else
				outfitSales.insert(GameData::Outfitters().Get(value));
		}
		// Handle the attributes which cannot be "removed."
		else if(remove)
		{
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
			continue;
		}
		else if(key == "display name")
			displayName = value;
		else if(key == "landscape")
			landscape = SpriteSet::Get(value);
		else if(key == "music")
			music = value;
		else if(key == "description")
			description.Load(child, playerConditions);
		else if(key == "spaceport")
		{
			port.LoadDefaultSpaceport();
			port.LoadDescription(child, playerConditions);
		}
		else if(key == "government")
			government = GameData::Governments().Get(value);
		else if(key == "required reputation")
			requiredReputation = child.Value(valueIndex);
		else if(key == "bribe")
			bribe = child.Value(valueIndex);
		else if(key == "bribe threshold")
			bribeThreshold = child.Value(valueIndex);
		else if(key == "security")
		{
			customSecurity = true;
			security = child.Value(valueIndex);
		}
		else if(key == "tribute")
		{
			tribute = child.Value(valueIndex);
			bool resetFleets = !defenseFleets.empty();
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				bool grandHasValue = grand.Size() >= 2;
				if(grandKey == "threshold" && grandHasValue)
					defenseThreshold = grand.Value(1);
				else if(grandKey == "fleet")
				{
					if(grandHasValue && !grand.HasChildren())
					{
						// Allow only one "tribute" node to define the tribute fleets.
						if(resetFleets)
						{
							defenseFleets.clear();
							resetFleets = false;
						}
						defenseFleets.insert(defenseFleets.end(),
								grand.Size() >= 3 ? grand.Value(2) : 1,
								GameData::Fleets().Get(grand.Token(1))
						);
					}
					else
						grand.PrintTrace("Skipping unsupported tribute fleet definition:");
				}
				else if(grandKey == "daily reputation penalty" && grandHasValue)
					dailyTributePenalty = grand.Value(1);
				else
					grand.PrintTrace("Skipping unrecognized tribute attribute:");
			}
		}
		else if(key == "wormhole")
		{
			wormhole = wormholes.Get(value);
			wormhole->SetPlanet(*this);
		}
		else if(key == "to")
		{
			if(value == "know")
				toKnow.Load(child, playerConditions);
			else if(value == "land")
				toLand.Load(child, playerConditions);
			else if(value == "access" && child.Size() > valueIndex + 1)
			{
				const string &shop = child.Token(valueIndex + 1);
				if(shop == "shipyard")
					toAccessShipyard.Load(child, playerConditions);
				else if(shop == "outfitter")
					toAccessOutfitter.Load(child, playerConditions);
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// For reverse compatibility, if this planet has a spaceport but it was not custom loaded,
	// and the planet has the "uninhabited" attribute, replace the spaceport with a special-case
	// uninhabited spaceport.
	if(HasNamedPort() && !port.CustomLoaded())
	{
		if(attributes.contains("uninhabited"))
			port.LoadUninhabitedSpaceport();
		else
			port.LoadDefaultSpaceport();
	}

	// Apply any auto-attributes to this planet depending on what it has.
	static const vector<string> AUTO_ATTRIBUTES = {
		"spaceport",
		"port",
		"shipyard",
		"outfitter",
		"service: trading",
		"service: jobs",
		"service: bank",
		"service: crew",
		"service: missions",
		"recharges: shields",
		"recharges: hull",
		"recharges: energy",
		"recharges: fuel",
		"spaceport news",
	};
	bool autoValues[14] = {
		port.HasService(Port::ServicesType::All, false) && port.CanRecharge(Port::RechargeType::All, false)
				&& port.HasNews() && HasNamedPort(),
		HasNamedPort(),
		!shipSales.empty(),
		!outfitSales.empty(),
		port.HasService(Port::ServicesType::Trading, false),
		port.HasService(Port::ServicesType::JobBoard, false),
		port.HasService(Port::ServicesType::Bank, false),
		port.HasService(Port::ServicesType::HireCrew, false),
		port.HasService(Port::ServicesType::OffersMissions, false),
		port.CanRecharge(Port::RechargeType::Shields, false),
		port.CanRecharge(Port::RechargeType::Hull, false),
		port.CanRecharge(Port::RechargeType::Energy, false),
		port.CanRecharge(Port::RechargeType::Fuel, false),
		port.HasNews(),
	};
	for(unsigned i = 0; i < AUTO_ATTRIBUTES.size(); ++i)
	{
		if(autoValues[i])
			attributes.insert(AUTO_ATTRIBUTES[i]);
		else
			attributes.erase(AUTO_ATTRIBUTES[i]);
	}

	// Precalculate commonly used values that can only change due to Load().
	inhabited = (HasServices(false) || requiredReputation || !defenseFleets.empty())
			&& !attributes.contains("uninhabited");
	SetRequiredAttributes(Attributes(), requiredAttributes);
}



// Legacy wormhole do not have an associated Wormhole object so
// we must auto generate one if we detect such legacy wormhole.
void Planet::FinishLoading(Set<Wormhole> &wormholes)
{
	// If this planet is in multiple systems, then it is a wormhole.
	if(!wormhole && systems.size() > 1)
	{
		wormhole = wormholes.Get(TrueName());
		wormhole->LoadFromPlanet(*this);
		Logger::Log("Deprecated automatic generation of wormhole \"" + trueName + "\" from a multi-system planet.",
			Logger::Level::WARNING);
	}
	// If the wormhole was autogenerated we need to update it to
	// match the planet's state.
	else if(wormhole && wormhole->IsAutogenerated())
		wormhole->LoadFromPlanet(*this);
	// If this planet is not a wormhole, make sure it has a display name.
	else if(!wormhole && displayName.empty())
		displayName = trueName;
}



// Test if this planet has been loaded (vs. just referred to). It must also be located in
// at least one system, and all systems that claim it must themselves be valid.
bool Planet::IsValid() const
{
	return isDefined && !systems.empty() && all_of(systems.begin(), systems.end(),
		[](const System *s) noexcept -> bool { return s->IsValid(); });
}



// Get the name used for this planet in the data files.
const string &Planet::TrueName() const
{
	return trueName;
}



void Planet::SetTrueName(const string &name)
{
	trueName = name;
	if(displayName.empty())
		displayName = trueName;
}



// Get the display name of the planet.
const string &Planet::DisplayName() const
{
	return IsWormhole() ? wormhole->DisplayName() : displayName;
}



// Get the planet's descriptive text.
const Paragraphs &Planet::Description() const
{
	return description;
}



// Get the landscape sprite.
const Sprite *Planet::Landscape() const
{
	return landscape;
}



// Get the name of the ambient audio to play on this planet.
const string &Planet::MusicName() const
{
	return music;
}



// Get the list of "attributes" of the planet.
const set<string> &Planet::Attributes() const
{
	return attributes;
}



// Get the list of "attributes" of the planet.
const set<string> &Planet::RequiredAttributes() const
{
	return requiredAttributes;
}



// Get planet's noun descriptor from attributes
const string &Planet::Noun() const
{
	if(IsWormhole())
		return WORMHOLE;

	for(const string &attribute : attributes)
		if(attribute == "moon" || attribute == "station")
			return attribute;

	return PLANET;
}



// Check whether this planet's port is named.
bool Planet::HasNamedPort() const
{
	return !port.DisplayName().empty();
}



// Get this planet's port.
const Port &Planet::GetPort() const
{
	return port;
}



// Check whether there are port services (such as trading, jobs, banking, and hiring)
// available on this planet.
bool Planet::HasServices(bool isPlayer) const
{
	return port.HasServices(isPlayer);
}



// Check if this planet is inhabited (i.e. it has a spaceport, and does not
// have the "uninhabited" attribute).
bool Planet::IsInhabited() const
{
	return inhabited;
}



// Check if this planet has a permanent shipyard.
bool Planet::HasShipyard() const
{
	return !shipSales.empty() && toAccessShipyard.Test();
}



// Get the list of ships in the permanent shipyard.
const Sale<Ship> &Planet::ShipyardStock() const
{
	shipyard.clear();
	for(const Shop<Ship> *sale : shipSales)
		shipyard.Add(sale->Stock());

	return shipyard;
}



// Get the list of shipyards currently available on this planet.
// This will include conditionally available shops.
set<const Shop<Ship> *> Planet::Shipyards() const
{
	set<const Shop<Ship> *> shops = shipSales;
	for(const auto &shop : GameData::Shipyards())
		if(shop.second.CanStock(this))
			shops.insert(&shop.second);
	return shops;
}



// Check if this planet has a permanent outfitter.
bool Planet::HasOutfitter() const
{
	return !outfitSales.empty() && toAccessOutfitter.Test();
}



// Get the list of outfits available from the permanent outfitter.
const Sale<Outfit> &Planet::OutfitterStock() const
{
	outfitter.clear();
	for(const Shop<Outfit> *sale : outfitSales)
		outfitter.Add(sale->Stock());

	return outfitter;
}



// Get the list of outitters available on this planet.
// This will include conditionally available shops.
set<const Shop<Outfit> *> Planet::Outfitters() const
{
	set<const Shop<Outfit> *> shops = outfitSales;
	for(const auto &shop : GameData::Outfitters())
		if(shop.second.CanStock(this))
			shops.insert(&shop.second);
	return shops;
}



// Get this planet's government. Most planets follow the government of the system they are in.
const Government *Planet::GetGovernment() const
{
	return government ? government : systems.empty() ? nullptr : GetSystem()->GetGovernment();
}



// You need this good a reputation with this system's government to land here.
double Planet::RequiredReputation() const
{
	return requiredReputation;
}



// This is what fraction of your fleet's value you must pay as a bribe in
// order to land on this planet. (If zero, you cannot bribe it.)
double Planet::GetBribeFraction() const
{
	return bribe;
}



// This is how likely the planet's authorities are to notice if you are
// doing something illegal.
double Planet::Security() const
{
	return security;
}



bool Planet::HasCustomSecurity() const
{
	return customSecurity;
}



const System *Planet::GetSystem() const
{
	return (systems.empty() ? nullptr : systems.front());
}



// Check if this planet is in the given system. Note that wormholes may be
// in more than one system.
bool Planet::IsInSystem(const System *system) const
{
	return (find(systems.begin(), systems.end(), system) != systems.end());
}



void Planet::SetSystem(const System *system)
{
	if(find(systems.begin(), systems.end(), system) == systems.end())
		systems.push_back(system);
}



// Remove the given system from the list of systems this planet is in. This
// must be done when game events rearrange the planets in a system.
void Planet::RemoveSystem(const System *system)
{
	auto it = find(systems.begin(), systems.end(), system);
	if(it != systems.end())
		systems.erase(it);
}



const vector<const System *> &Planet::Systems() const
{
	return systems;
}



// Check if this is a wormhole (that is, it appears in multiple systems).
bool Planet::IsWormhole() const
{
	return wormhole;
}



const Wormhole *Planet::GetWormhole() const
{
	return wormhole;
}



// Check if the given ship has all the attributes necessary to allow it to
// land on this planet.
bool Planet::IsAccessible(const Ship *ship) const
{
	// If this is a wormhole that leads to an inaccessible system, no ship can land here.
	if(wormhole && ship && ship->GetSystem() && wormhole->WormholeDestination(*ship->GetSystem()).Inaccessible())
		return false;
	// If this ship is yours, but you lack the conditions to know that it's a landable object,
	// then the ship can't land.
	if(ship && ship->IsYours() && !toKnow.Test())
		return false;
	// If there are no required attributes, then any ship may land here.
	if(IsUnrestricted())
		return true;
	if(!ship)
		return false;

	const auto &shipAttributes = ship->Attributes();
	return all_of(requiredAttributes.cbegin(), requiredAttributes.cend(),
			[&](const string &attr) -> bool { return shipAttributes.Get(attr); });
}



// Check if this planet has any required attributes that restrict landability.
bool Planet::IsUnrestricted() const
{
	return requiredAttributes.empty();
}



// Below are convenience functions which access the game state in Politics,
// but do so with a less convoluted syntax:
bool Planet::HasFuelFor(const Ship &ship) const
{
	return !IsWormhole() && port.CanRecharge(Port::RechargeType::Fuel, ship.IsYours()) && CanLand(ship);
}



bool Planet::CanBribe() const
{
	// If this planet has a minimum reputation for accepting bribes and your
	// reputation is below this value, you can't bribe.
	if(bribeThreshold && GameData::GetPolitics().Reputation(government) < bribeThreshold)
		return false;
	// If you can't land then you can't bribe.
	return toLand.Test();
}



bool Planet::CanLand(const Ship &ship) const
{
	if(!ship.IsYours())
		return IsAccessible(&ship) && GameData::GetPolitics().CanLand(ship, this);

	return IsAccessible(&ship) && GameData::GetPolitics().CanLand(ship, this)
		&& (!port.RequiresBribe() || GameData::GetPolitics().HasClearance(this))
		&& toLand.Test();
}



bool Planet::CanLand() const
{
	return GameData::GetPolitics().CanLand(this)
		&& (!port.RequiresBribe() || GameData::GetPolitics().HasClearance(this))
		&& toLand.Test();
}



Planet::Friendliness Planet::GetFriendliness() const
{
	if(GameData::GetPolitics().HasDominated(this))
		return Friendliness::DOMINATED;
	else if(GetGovernment()->IsEnemy())
		return Friendliness::HOSTILE;
	else if(CanLand())
		return Friendliness::FRIENDLY;
	else
		return Friendliness::RESTRICTED;
}



bool Planet::CanUseServices() const
{
	return GameData::GetPolitics().CanUseServices(this) && port.CanAccess();
}



void Planet::Bribe(bool fullAccess) const
{
	GameData::GetPolitics().BribePlanet(this, fullAccess);
}



// Demand tribute, and get the planet's response.
string Planet::DemandTribute(PlayerInfo &player) const
{
	const auto &playerTribute = player.GetTribute();
	if(playerTribute.find(this) != playerTribute.end())
		return "We are already paying you as much as we can afford.";
	if(!tribute || defenseFleets.empty())
		return "Please don't joke about that sort of thing.";
	if(player.Conditions().Get("combat rating") < defenseThreshold)
		return "You're not worthy of our time.";

	// The player is scary enough for this planet to take notice. Check whether
	// this is the first demand for tribute, or not.
	if(!isDefending)
	{
		isDefending = true;
		set<const Government *> toProvoke;
		for(const auto &fleet : defenseFleets)
			toProvoke.insert(fleet->GetGovernment());
		for(const auto &gov : toProvoke)
			if(gov)
				gov->Offend(ShipEvent::PROVOKE);
		// Terrorizing a planet is not taken lightly by it or its allies.
		// TODO: Use a distinct event type for the domination system and
		// expose syntax for controlling its impact on the targeted government
		// and those that know it.
		GetGovernment()->Offend(ShipEvent::ATROCITY);
		return "Our defense fleet will make short work of you.";
	}

	// The player has already demanded tribute. Have they defeated the entire defense fleet?
	bool isDefeated = (defenseDeployed == defenseFleets.size());
	for(const shared_ptr<Ship> &ship : defenders)
		if(!ship->IsDisabled() && !ship->IsYours())
		{
			isDefeated = false;
			break;
		}

	if(!isDefeated)
		return "We're not ready to surrender yet.";

	player.SetTribute(this, tribute);
	return "We surrender. We will pay you " + Format::CreditString(tribute) + " per day to leave us alone.";
}



// While being tributed, attempt to spawn the next specified defense fleet.
void Planet::DeployDefense(list<shared_ptr<Ship>> &ships) const
{
	if(!isDefending || Random::Int(60) || defenseDeployed == defenseFleets.size())
		return;

	auto end = defenders.begin();
	if(defenseFleets[defenseDeployed]->IsValid())
		defenseFleets[defenseDeployed]->Enter(*GetSystem(), defenders, this);
	else
		Logger::Log("Skipped an incomplete defense fleet of planet \"" + trueName + "\".", Logger::Level::WARNING);
	ships.insert(ships.begin(), defenders.begin(), end);

	// All defenders use a special personality.
	Personality defenderPersonality = Personality::Defender();
	Personality fighterPersonality = Personality::DefenderFighter();
	for(auto it = defenders.begin(); it != end; ++it)
	{
		(**it).SetPersonality(defenderPersonality);
		if((**it).HasBays())
			for(auto bay = (**it).Bays().begin(); bay != (**it).Bays().end(); ++bay)
				if(bay->ship)
					bay->ship->SetPersonality(fighterPersonality);
	}

	++defenseDeployed;
}



void Planet::ResetDefense() const
{
	isDefending = false;
	defenseDeployed = 0;
	defenders.clear();
}



bool Planet::IsDefending() const
{
	return isDefending;
}



double Planet::DailyTributePenalty() const
{
	return dailyTributePenalty;
}
