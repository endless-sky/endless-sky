/* Engine.cpp
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

#include "Engine.h"

#include "AlertLabel.h"
#include "Audio.h"
#include "CategoryList.h"
#include "CategoryTypes.h"
#include "Collision.h"
#include "CollisionType.h"
#include "CoreStartData.h"
#include "DamageDealt.h"
#include "DamageProfile.h"
#include "Effect.h"
#include "FighterHitHelper.h"
#include "FillShader.h"
#include "Fleet.h"
#include "Flotsam.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Government.h"
#include "Hazard.h"
#include "Interface.h"
#include "Logger.h"
#include "MapPanel.h"
#include "Mask.h"
#include "Messages.h"
#include "Minable.h"
#include "Mission.h"
#include "NPC.h"
#include "OutlineShader.h"
#include "Person.h"
#include "Planet.h"
#include "PlanetLabel.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "Projectile.h"
#include "Random.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "ShipJumpNavigation.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "StellarObject.h"
#include "System.h"
#include "SystemEntry.h"
#include "Test.h"
#include "Visual.h"
#include "Weather.h"
#include "Wormhole.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace std;

namespace {
	int RadarType(const Ship &ship, int step)
	{
		if(ship.GetPersonality().IsTarget() && !ship.IsDestroyed())
		{
			// If a ship is a "target," double-blink it a few times per second.
			int count = (step / 6) % 7;
			if(count == 0 || count == 2)
				return Radar::BLINK;
		}
		if(ship.IsDisabled() || (ship.IsOverheated() && ((step / 20) % 2)))
			return Radar::INACTIVE;
		if(ship.IsYours() || (ship.GetPersonality().IsEscort() && !ship.GetGovernment()->IsEnemy()))
			return Radar::PLAYER;
		if(!ship.GetGovernment()->IsEnemy())
			return Radar::FRIENDLY;
		const auto &target = ship.GetTargetShip();
		if(target && target->IsYours())
			return Radar::HOSTILE;
		return Radar::UNFRIENDLY;
	}

	template <class Type>
	void Prune(vector<Type> &objects)
	{
		// First, erase any of the old objects that should be removed.
		typename vector<Type>::iterator in = objects.begin();
		while(in != objects.end() && !in->ShouldBeRemoved())
			++in;

		typename vector<Type>::iterator out = in;
		while(in != objects.end())
		{
			if(!in->ShouldBeRemoved())
				*out++ = std::move(*in);
			++in;
		}
		if(out != objects.end())
			objects.erase(out, objects.end());
	}

	template <class Type>
	void Prune(list<shared_ptr<Type>> &objects)
	{
		for(auto it = objects.begin(); it != objects.end(); )
		{
			if((*it)->ShouldBeRemoved())
				it = objects.erase(it);
			else
				++it;
		}
	}

	template <class Type>
	void Append(vector<Type> &objects, vector<Type> &added)
	{
		objects.insert(objects.end(), make_move_iterator(added.begin()), make_move_iterator(added.end()));
		added.clear();
	}

	// Author the given message from the given ship.
	void SendMessage(const shared_ptr<const Ship> &ship, const string &message)
	{
		if(message.empty())
			return;

		// If this ship has no name, show its model name instead.
		string tag;
		const string &gov = ship->GetGovernment()->GetName();
		if(!ship->Name().empty())
			tag = gov + " " + ship->Noun() + " \"" + ship->Name() + "\": ";
		else
			tag = ship->DisplayModelName() + " (" + gov + "): ";

		Messages::Add(tag + message, Messages::Importance::High);
	}

	void DrawFlareSprites(const Ship &ship, DrawList &draw, const vector<Ship::EnginePoint> &enginePoints,
		const vector<pair<Body, int>> &flareSprites, uint8_t side)
	{
		double gimbalDirection = (ship.Commands().Has(Command::FORWARD) || ship.Commands().Has(Command::BACK))
			* -ship.Commands().Turn();

		for(const Ship::EnginePoint &point : enginePoints)
		{
			Angle gimbal = Angle(gimbalDirection * point.gimbal.Degrees());
			Angle flareAngle = ship.Facing() + point.facing + gimbal;
			Point pos = ship.Facing().Rotate(point) * ship.Zoom() + ship.Position();
			// If multiple engines with the same flare are installed, draw up to
			// three copies of the flare sprite.
			for(const auto &it : flareSprites)
				if(point.side == side && (point.steering == Ship::EnginePoint::NONE
					|| (point.steering == Ship::EnginePoint::LEFT && ship.SteeringDirection() < 0.)
					|| (point.steering == Ship::EnginePoint::RIGHT && ship.SteeringDirection() > 0.)))
					for(int i = 0; i < it.second && i < 3; ++i)
					{
						Body sprite(it.first, pos, ship.Velocity(), flareAngle, point.zoom);
						draw.Add(sprite, ship.Cloaking());
					}
		}
	}

	const Color &GetTargetOutlineColor(int type)
	{
		if(type == Radar::PLAYER)
			return *GameData::Colors().Get("ship target outline player");
		else if(type == Radar::FRIENDLY)
			return *GameData::Colors().Get("ship target outline friendly");
		else if(type == Radar::UNFRIENDLY)
			return *GameData::Colors().Get("ship target outline unfriendly");
		else if(type == Radar::HOSTILE)
			return *GameData::Colors().Get("ship target outline hostile");
		else if(type == Radar::SPECIAL)
			return *GameData::Colors().Get("ship target outline special");
		else if(type == Radar::BLINK)
			return *GameData::Colors().Get("ship target outline blink");
		else
			return *GameData::Colors().Get("ship target outline inactive");
	}

	const Color &GetPlanetTargetPointerColor(const Planet &planet)
	{
		switch(planet.GetFriendliness())
		{
			case Planet::Friendliness::FRIENDLY:
				return *GameData::Colors().Get("planet target pointer friendly");
			case Planet::Friendliness::RESTRICTED:
				return *GameData::Colors().Get("planet target pointer restricted");
			case Planet::Friendliness::HOSTILE:
				return *GameData::Colors().Get("planet target pointer hostile");
			case Planet::Friendliness::DOMINATED:
				return *GameData::Colors().Get("planet target pointer dominated");
		}
		return *GameData::Colors().Get("planet target pointer unfriendly");
	}

	const Color &GetShipTargetPointerColor(int type)
	{
		if(type == Radar::PLAYER)
			return *GameData::Colors().Get("ship target pointer player");
		else if(type == Radar::FRIENDLY)
			return *GameData::Colors().Get("ship target pointer friendly");
		else if(type == Radar::UNFRIENDLY)
			return *GameData::Colors().Get("ship target pointer unfriendly");
		else if(type == Radar::HOSTILE)
			return *GameData::Colors().Get("ship target pointer hostile");
		else if(type == Radar::SPECIAL)
			return *GameData::Colors().Get("ship target pointer special");
		else if(type == Radar::BLINK)
			return *GameData::Colors().Get("ship target pointer blink");
		else
			return *GameData::Colors().Get("ship target pointer inactive");
	}

	const Color &GetMinablePointerColor(bool selected)
	{
		if(selected)
			return *GameData::Colors().Get("minable target pointer selected");
		return *GameData::Colors().Get("minable target pointer unselected");
	}

	const double RADAR_SCALE = .025;
	const double MAX_FUEL_DISPLAY = 5000.;

	const double CAMERA_VELOCITY_TRACKING = 0.1;
	const double CAMERA_POSITION_CENTERING = 0.01;

	pair<Point, Point> NewCenter(const Point &oldCenter, const Point &oldCenterVelocity,
		const Point &baseCenter, const Point &baseVelocity, const double influence, const bool killVelocity)
	{
		if(Preferences::CameraAcceleration() == Preferences::CameraAccel::OFF)
			return make_pair(baseCenter, baseVelocity);

		double cameraAccelMultiplier = Preferences::CameraAcceleration() == Preferences::CameraAccel::REVERSED ? -1. : 1.;

		// Flip the velocity offset if cameraAccelMultiplier is negative to simplify logic.
		const Point absoluteOldCenterVelocity = baseVelocity.Lerp(oldCenterVelocity, cameraAccelMultiplier);

		const Point newAbsVelocity = absoluteOldCenterVelocity.Lerp(baseVelocity, CAMERA_VELOCITY_TRACKING);

		Point newCenter = (oldCenter + newAbsVelocity).Lerp(baseCenter, CAMERA_POSITION_CENTERING);

		// Flip the velocity back over the baseVelocity
		Point newVelocity = baseVelocity.Lerp(newAbsVelocity, cameraAccelMultiplier);

		newCenter = newCenter.Lerp(baseCenter, pow(influence, .5));
		newVelocity = killVelocity ? baseVelocity : newVelocity.Lerp(baseVelocity, pow(influence, .5));

		return make_pair(newCenter, newVelocity);
	}
}



Engine::Engine(PlayerInfo &player)
	: player(player), ai(player, ships, asteroids.Minables(), flotsam),
	ammoDisplay(player), shipCollisions(256u, 32u, CollisionType::SHIP)
{
	zoom.base = Preferences::ViewZoom();
	zoom.modifier = Preferences::Has("Landing zoom") ? 2. : 1.;

	if(!player.IsLoaded() || !player.GetSystem())
		return;

	// Preload any landscapes for this system.
	for(const StellarObject &object : player.GetSystem()->Objects())
		if(object.HasSprite() && object.HasValidPlanet())
			GameData::Preload(queue, object.GetPlanet()->Landscape());
	queue.Wait();

	// Figure out what planet the player is landed on, if any.
	const StellarObject *object = player.GetStellarObject();
	if(object)
		center = object->Position();

	// Now we know the player's current position. Draw the planets.
	draw[currentCalcBuffer].Clear(step, zoom);
	draw[currentCalcBuffer].SetCenter(center);
	radar[currentCalcBuffer].SetCenter(center);
	const Ship *flagship = player.Flagship();
	for(const StellarObject &object : player.GetSystem()->Objects())
		if(object.HasSprite())
		{
			draw[currentCalcBuffer].Add(object);

			double r = max(2., object.Radius() * .03 + .5);
			radar[currentCalcBuffer].Add(object.RadarType(flagship), object.Position(), r, r - 1.);
		}

	// Add all neighboring systems that the player has seen to the radar.
	const System *targetSystem = flagship ? flagship->GetTargetSystem() : nullptr;
	const set<const System *> &links = (flagship && flagship->JumpNavigation().HasJumpDrive()) ?
		player.GetSystem()->JumpNeighbors(flagship->JumpNavigation().JumpRange()) : player.GetSystem()->Links();
	for(const System *system : links)
		if(player.HasSeen(*system))
			radar[currentCalcBuffer].AddPointer(
				(system == targetSystem) ? Radar::SPECIAL : Radar::INACTIVE,
				system->Position() - player.GetSystem()->Position());

	GameData::SetHaze(player.GetSystem()->Haze(), true);
}



Engine::~Engine()
{
	// Wait for any outstanding task to finish to avoid race conditions when
	// destroying the engine.
	queue.Wait();
}



void Engine::Place()
{
	ships.clear();
	ai.ClearOrders();

	player.SetSystemEntry(SystemEntry::TAKE_OFF);
	EnterSystem();

	// Add the player's flagship and escorts to the list of ships. The TakeOff()
	// code already took care of loading up fighters and assigning parents.
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(!ship->IsParked() && ship->GetSystem())
			ships.push_back(ship);

	// Add NPCs to the list of ships. Fighters have to be assigned to carriers,
	// and all but "uninterested" ships should follow the player.
	shared_ptr<Ship> flagship = player.FlagshipPtr();

	// Update the active NPCs for missions based on the player's conditions.
	player.UpdateMissionNPCs();
	for(const Mission &mission : player.Missions())
		Place(mission.NPCs(), flagship);

	// Get the coordinates of the planet the player is leaving.
	const System *system = player.GetSystem();
	const Planet *planet = player.GetPlanet();
	Point planetPos;
	double planetRadius = 0.;
	const StellarObject *object = player.GetStellarObject();
	if(object)
	{
		planetPos = object->Position();
		planetRadius = object->Radius();
	}

	// Give each non-carried, special ship we just added a random heading and position.
	// (While carried by a parent, ships will not be present in `Engine::ships`.)
	for(const shared_ptr<Ship> &ship : ships)
	{
		Point pos;
		Angle angle = Angle::Random();
		// Any ships in the same system as the player should be either
		// taking off from a specific planet or nearby.
		if(ship->GetSystem() == system && !ship->IsDisabled())
		{
			const Personality &person = ship->GetPersonality();
			bool hasOwnPlanet = ship->GetPlanet();
			bool launchesWithPlayer = (planet && planet->CanLand(*ship))
					&& !person.IsStaying() && !person.IsWaiting()
					&& (!hasOwnPlanet || (ship->IsYours() && ship->GetPlanet() == planet));
			const StellarObject *object = hasOwnPlanet ?
					ship->GetSystem()->FindStellar(ship->GetPlanet()) : nullptr;
			// Default to the player's planet in the case of data definition errors.
			if(person.IsLaunching() || launchesWithPlayer || (hasOwnPlanet && !object))
			{
				if(planet)
					ship->SetPlanet(planet);
				pos = planetPos + angle.Unit() * Random::Real() * planetRadius;
			}
			else if(hasOwnPlanet)
				pos = object->Position() + angle.Unit() * Random::Real() * object->Radius();
		}
		// If a special ship somehow was saved without a system reference, place it into the
		// player's system to avoid a nullptr deference.
		else if(!ship->GetSystem())
		{
			// Log this error.
			Logger::LogError("Engine::Place: Set fallback system for the NPC \"" + ship->Name() + "\" as it had no system");
			ship->SetSystem(system);
		}

		// If the position is still (0, 0), the special ship is in a different
		// system, disabled, or otherwise unable to land on viable planets in
		// the player's system: place it "in flight".
		if(!pos)
		{
			ship->SetPlanet(nullptr);
			Fleet::Place(*ship->GetSystem(), *ship);
		}
		// This ship is taking off from a planet.
		else
			ship->Place(pos, angle.Unit(), angle);
	}
	// Move any ships that were randomly spawned into the main list, now
	// that all special ships have been repositioned.
	ships.splice(ships.end(), newShips);

	center = flagship->Center();
	centerVelocity = flagship->Velocity();

	player.SetPlanet(nullptr);
}



// Add NPC ships to the known ships. These may have been freshly instantiated
// from an accepted assisting/boarding mission, or from existing missions when
// the player departs a planet.
void Engine::Place(const list<NPC> &npcs, shared_ptr<Ship> flagship)
{
	for(const NPC &npc : npcs)
	{
		if(!npc.ShouldSpawn())
			continue;

		map<string, map<Ship *, int>> carriers;
		for(const shared_ptr<Ship> &ship : npc.Ships())
		{
			// Skip ships that have been destroyed.
			if(ship->IsDestroyed() || ship->IsDisabled())
				continue;

			// Redo the loading up of fighters.
			if(ship->HasBays())
			{
				ship->UnloadBays();
				for(const auto &cat : GameData::GetCategory(CategoryType::BAY))
				{
					const string &bayType = cat.Name();
					int baysTotal = ship->BaysTotal(bayType);
					if(baysTotal)
						carriers[bayType][&*ship] = baysTotal;
				}
			}
		}

		shared_ptr<Ship> npcFlagship;
		for(const shared_ptr<Ship> &ship : npc.Ships())
		{
			// Skip ships that have been destroyed.
			if(ship->IsDestroyed())
				continue;

			// Avoid the exploit where the player can wear down an NPC's
			// crew by attrition over the course of many days.
			ship->AddCrew(max(0, ship->RequiredCrew() - ship->Crew()));
			if(!ship->IsDisabled())
				ship->Recharge();

			if(ship->CanBeCarried())
			{
				bool docked = false;
				const string &bayType = ship->Attributes().Category();
				for(auto &it : carriers[bayType])
					if(it.second && it.first->Carry(ship))
					{
						--it.second;
						docked = true;
						break;
					}
				if(docked)
					continue;
			}

			ships.push_back(ship);
			// The first (alive) ship in an NPC block
			// serves as the flagship of the group.
			if(!npcFlagship)
				npcFlagship = ship;

			// Only the flagship of an NPC considers the
			// player: the rest of the NPC track it.
			if(npcFlagship && ship != npcFlagship)
				ship->SetParent(npcFlagship);
			else if(!ship->GetPersonality().IsUninterested())
				ship->SetParent(flagship);
			else
				ship->SetParent(nullptr);
		}
	}
}



// Wait for the previous calculations (if any) to be done.
void Engine::Wait()
{
	queue.Wait();
	currentDrawBuffer = currentCalcBuffer;
}



// Begin the next step of calculations.
void Engine::Step(bool isActive)
{
	events.swap(eventQueue);
	eventQueue.clear();

	// Process any outstanding sprites that need to be uploaded to the GPU.
	queue.ProcessSyncTasks();

	// The calculation thread was paused by MainPanel before calling this function, so it is safe to access things.
	const shared_ptr<Ship> flagship = player.FlagshipPtr();
	const StellarObject *object = player.GetStellarObject();
	if(object)
	{
		center = object->Position();
		centerVelocity = Point();
	}
	else if(flagship)
	{
		if(isActive)
		{
			const auto [newCenter, newCenterVelocity] = NewCenter(center, centerVelocity,
				flagship->Center(), flagship->Velocity(), flagship->GetHyperspacePercentage() / 100.,
				flagship->IsHyperspacing());

			center = newCenter;
			centerVelocity = newCenterVelocity;
		}

		if(doEnterLabels)
		{
			doEnterLabels = false;
			// Create the planet labels as soon as we entered a new system.
			labels.clear();
			for(const StellarObject &object : player.GetSystem()->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsAccessible(flagship.get()))
					labels.emplace_back(labels, *player.GetSystem(), object);
		}
		if(doEnter && flagship->Zoom() == 1. && !flagship->IsHyperspacing())
		{
			doEnter = false;
			events.emplace_back(flagship, flagship, ShipEvent::JUMP);
		}
		if(flagship->IsEnteringHyperspace() || flagship->Commands().Has(Command::JUMP))
		{
			if(jumpCount < 100)
				++jumpCount;
			const System *from = flagship->GetSystem();
			const System *to = flagship->GetTargetSystem();
			if(from && to && from != to)
			{
				jumpInProgress[0] = from;
				jumpInProgress[1] = to;
			}
		}
		else if(jumpCount > 0)
			--jumpCount;
	}
	ai.UpdateEvents(events);
	if(isActive)
	{
		HandleKeyboardInputs();
		// Ignore any inputs given when first becoming active, since those inputs
		// were issued when some other panel (e.g. planet, hail) was displayed.
		if(!wasActive)
			activeCommands.Clear();
		else
			ai.UpdateKeys(player, activeCommands);
	}

	wasActive = isActive;
	Audio::Update(center);

	// Update the zoom value now that the calculation thread is paused.
	if(nextZoom)
		zoom = std::exchange(nextZoom, {});
	// Smoothly zoom in and out.
	if(isActive)
	{
		double zoomTarget = Preferences::ViewZoom();
		if(zoom.base != zoomTarget)
		{
			static const double ZOOM_SPEED = .05;

			// Define zoom speed bounds to prevent asymptotic behavior.
			static const double MAX_SPEED = .05;
			static const double MIN_SPEED = .002;

			double zoomRatio = max(MIN_SPEED, min(MAX_SPEED, abs(log2(zoom.base) - log2(zoomTarget)) * ZOOM_SPEED));
			if(zoom.base < zoomTarget)
				nextZoom.base = min(zoomTarget, zoom.base * (1. + zoomRatio));
			else if(zoom.base > zoomTarget)
				nextZoom.base = max(zoomTarget, zoom.base * (1. / (1. + zoomRatio)));
		}
		if(flagship && flagship->Zoom() < 1.)
		{
			if(!nextZoom.base)
				nextZoom.base = zoom.base;
			// Update the current zoom modifier if the flagship is landing or taking off.
			nextZoom.modifier = Preferences::Has("Landing zoom") ? 1. + pow(1. - flagship->Zoom(), 2) : 1.;
		}
	}

	outlines.clear();
	const Color &cloakColor = *GameData::Colors().Get("cloak highlight");
	if(Preferences::Has("Cloaked ship outlines"))
		for(const auto &ship : player.Ships())
		{
			if(ship->IsParked() || ship->GetSystem() != player.GetSystem() || ship->Cloaking() == 0.)
				continue;

			outlines.emplace_back(ship->GetSprite(), (ship->Position() - center) * zoom, ship->Unit() * zoom,
				ship->GetFrame(), Color::Multiply(ship->Cloaking(), cloakColor));
		}

	// Add the flagship outline last to distinguish the flagship from other ships.
	if(flagship && !flagship->IsDestroyed() && Preferences::Has("Highlight player's flagship"))
	{
		outlines.emplace_back(flagship->GetSprite(), (flagship->Center() - center) * zoom, flagship->Unit() * zoom,
			flagship->GetFrame(), *GameData::Colors().Get("flagship highlight"));
	}

	// Any of the player's ships that are in system are assumed to have
	// landed along with the player.
	if(flagship && flagship->GetPlanet() && isActive)
		player.SetPlanet(flagship->GetPlanet());

	const System *currentSystem = player.GetSystem();
	// Update this here, for thread safety.
	if(player.HasTravelPlan() && currentSystem == player.TravelPlan().back())
		player.PopTravel();
	// Check if the player's travel plan is still valid.
	if(flagship && player.HasTravelPlan())
	{
		bool travelPlanIsValid = false;
		// If the player is traveling through a wormhole to the next system, then the plan is valid.
		const System *system = player.TravelPlan().back();
		for(const StellarObject &object : flagship->GetSystem()->Objects())
			if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsWormhole()
				&& object.GetPlanet()->IsAccessible(flagship.get()) && player.HasVisited(*object.GetPlanet())
				&& player.CanView(*system))
			{
				const auto *wormhole = object.GetPlanet()->GetWormhole();
				if(&wormhole->WormholeDestination(*flagship->GetSystem()) != system)
					continue;

				travelPlanIsValid = true;
				break;
			}
		// Otherwise, the player must still be within jump range of the next system.
		travelPlanIsValid |= flagship->JumpNavigation().CanJump(flagship->GetSystem(), system);
		// Other steps of the travel plan may have been invalidated as a result of the system no longer being visible.
		travelPlanIsValid &= all_of(player.TravelPlan().begin(), player.TravelPlan().end(),
				[this](const System *system) -> bool { return player.HasSeen(*system); });
		if(!travelPlanIsValid)
		{
			if(flagship->GetTargetSystem() == player.TravelPlan().back())
				flagship->SetTargetSystem(nullptr);
			player.TravelPlan().clear();
		}
	}
	if(doFlash)
	{
		flash = .4;
		doFlash = false;
	}
	else if(flash)
		flash = max(0., flash * .99 - .002);

	targets.clear();

	// Update the player's ammo amounts.
	if(flagship)
		ammoDisplay.Update(*flagship);
	else
		ammoDisplay.Reset();

	// Display escort information for all ships of the "Escort" government,
	// and all ships with the "escort" personality, except for fighters that
	// are not owned by the player.
	escorts.Clear();
	bool fleetIsJumping = (flagship && flagship->Commands().Has(Command::JUMP));
	for(const auto &it : ships)
		if(it->GetGovernment()->IsPlayer() || it->GetPersonality().IsEscort())
			if(!it->IsYours() && !it->CanBeCarried())
			{
				bool isSelected = (flagship && flagship->GetTargetShip() == it);
				const System *system = it->GetSystem();
				escorts.Add(*it, system == currentSystem, player.KnowsName(*system), fleetIsJumping, isSelected);
			}
	for(const shared_ptr<Ship> &escort : player.Ships())
		if(!escort->IsParked() && escort != flagship && !escort->IsDestroyed())
		{
			// Check if this escort is selected.
			bool isSelected = false;
			for(const weak_ptr<Ship> &ptr : player.SelectedShips())
				if(ptr.lock() == escort)
				{
					isSelected = true;
					break;
				}
			const System *system = escort->GetSystem();
			escorts.Add(*escort, system == currentSystem, player.KnowsName(*system), fleetIsJumping, isSelected);
		}

	statuses.clear();
	missileLabels.clear();
	if(isActive)
	{
		// Create the status overlays.
		CreateStatusOverlays();
		// Create missile overlays.
		if(Preferences::Has("Show missile overlays"))
			for(const Projectile &projectile : projectiles)
			{
				Point pos = projectile.Position() - center;
				if(projectile.MissileStrength() && projectile.GetGovernment()->IsEnemy()
						&& (pos.Length() < max(Screen::Width(), Screen::Height()) * .5 / zoom))
					missileLabels.emplace_back(AlertLabel(pos, projectile, flagship, zoom));
			}
		// Update the planet label positions.
		for(PlanetLabel &label : labels)
			label.Update(center, zoom);
	}

	if(flagship && flagship->IsOverheated())
		Messages::Add("Your ship has overheated.", Messages::Importance::Highest);

	// Clear the HUD information from the previous frame.
	info = Information();
	if(flagship && flagship->Hull())
	{
		Point shipFacingUnit(0., -1.);
		if(Preferences::Has("Rotate flagship in HUD"))
			shipFacingUnit = flagship->Facing().Unit();

		info.SetSprite("player sprite", flagship->GetSprite(), shipFacingUnit, flagship->GetFrame(step));
	}
	if(currentSystem)
		info.SetString("location", currentSystem->Name());
	info.SetString("date", player.GetDate().ToString());
	if(flagship)
	{
		// Have an alarm label flash up when enemy ships are in the system
		if(alarmTime && step / 20 % 2 && Preferences::DisplayVisualAlert())
			info.SetCondition("red alert");
		double fuelCap = flagship->Attributes().Get("fuel capacity");
		// If the flagship has a large amount of fuel, display a solid bar.
		// Otherwise, display a segment for every 100 units of fuel.
		if(fuelCap <= MAX_FUEL_DISPLAY)
			info.SetBar("fuel", flagship->Fuel(), fuelCap * .01);
		else
			info.SetBar("fuel", flagship->Fuel());
		info.SetBar("energy", flagship->Energy());
		double heat = flagship->Heat();
		info.SetBar("heat", min(1., heat));
		// If heat is above 100%, draw a second overlaid bar to indicate the
		// total heat level.
		if(heat > 1.)
			info.SetBar("overheat", min(1., heat - 1.));
		if(flagship->IsOverheated() && (step / 20) % 2)
			info.SetBar("overheat blink", min(1., heat));
		info.SetBar("shields", flagship->Shields());
		info.SetBar("hull", flagship->Hull(), 20.);
		info.SetBar("disabled hull", min(flagship->Hull(), flagship->DisabledHull()), 20.);
	}
	info.SetString("credits",
		Format::CreditString(player.Accounts().Credits()));
	bool isJumping = flagship && (flagship->Commands().Has(Command::JUMP) || flagship->IsEnteringHyperspace());
	if(flagship && flagship->GetTargetStellar() && !isJumping)
	{
		const StellarObject *object = flagship->GetTargetStellar();
		string navigationMode = flagship->Commands().Has(Command::LAND) ? "Landing on:" :
			object->GetPlanet() && object->GetPlanet()->CanLand(*flagship) ? "Can land on:" :
			"Cannot land on:";
		info.SetString("navigation mode", navigationMode);
		const string &name = object->Name();
		info.SetString("destination", name);

		targets.push_back({
			object->Position() - center,
			object->Facing(),
			object->Radius(),
			GetPlanetTargetPointerColor(*object->GetPlanet()),
			5});
	}
	else if(flagship && flagship->GetTargetSystem())
	{
		info.SetString("navigation mode", "Hyperspace:");
		if(player.CanView(*flagship->GetTargetSystem()))
			info.SetString("destination", flagship->GetTargetSystem()->Name());
		else
			info.SetString("destination", "unexplored system");
	}
	else
	{
		info.SetString("navigation mode", "Navigation:");
		info.SetString("destination", "no destination");
	}
	// Use the radar that was just populated. (The draw tick-tock has not
	// yet been toggled, but it will be at the end of this function.)
	shared_ptr<const Ship> target;
	shared_ptr<const Minable> targetAsteroid;
	targetVector = Point();
	if(flagship)
	{
		target = flagship->GetTargetShip();
		targetAsteroid = flagship->GetTargetAsteroid();
		// Record that the player knows this type of asteroid is available here.
		if(targetAsteroid)
			for(const auto &payload : targetAsteroid->GetPayload())
				player.Harvest(payload.outfit);
	}
	if(!target)
		targetSwizzle = -1;
	if(!target && !targetAsteroid)
		info.SetString("target name", "no target");
	else if(!target)
	{
		info.SetSprite("target sprite",
			targetAsteroid->GetSprite(),
			targetAsteroid->Facing().Unit(),
			targetAsteroid->GetFrame(step));
		info.SetString("target name", targetAsteroid->DisplayName() + " " + targetAsteroid->Noun());

		targetVector = targetAsteroid->Position() - center;

		if(flagship->Attributes().Get("tactical scan power"))
		{
			info.SetCondition("range display");
			info.SetBar("target hull", targetAsteroid->Hull(), 20.);
			int targetRange = round(targetAsteroid->Position().Distance(flagship->Position()));
			info.SetString("target range", to_string(targetRange));
		}
	}
	else
	{
		if(target->GetSystem() == player.GetSystem() && !target->IsCloaked())
			targetUnit = target->Facing().Unit();
		info.SetSprite("target sprite", target->GetSprite(), targetUnit, target->GetFrame(step));
		info.SetString("target name", target->Name());
		info.SetString("target type", target->DisplayModelName());
		if(!target->GetGovernment())
			info.SetString("target government", "No Government");
		else
			info.SetString("target government", target->GetGovernment()->GetName());
		targetSwizzle = target->GetSwizzle();
		info.SetString("mission target", target->GetPersonality().IsTarget() ? "(mission target)" : "");

		int targetType = RadarType(*target, step);
		info.SetOutlineColor(GetTargetOutlineColor(targetType));
		if(target->GetSystem() == player.GetSystem() && target->IsTargetable())
		{
			info.SetBar("target shields", target->Shields());
			info.SetBar("target hull", target->Hull(), 20.);
			info.SetBar("target disabled hull", min(target->Hull(), target->DisabledHull()), 20.);

			// The target area will be a square, with sides proportional to the average
			// of the width and the height of the sprite.
			double size = (target->Width() + target->Height()) * .35;
			targets.push_back({
				target->Position() - center,
				Angle(45.) + target->Facing(),
				size,
				GetShipTargetPointerColor(targetType),
				4});

			targetVector = target->Position() - center;

			// Check if the target is close enough to show tactical information.
			double tacticalRange = 100. * sqrt(flagship->Attributes().Get("tactical scan power"));
			double targetRange = target->Position().Distance(flagship->Position());
			if(tacticalRange)
			{
				info.SetCondition("range display");
				info.SetString("target range", to_string(static_cast<int>(round(targetRange))));
			}
			// Actual tactical information requires a scrutable
			// target that is within the tactical scanner range.
			if((targetRange <= tacticalRange && !target->Attributes().Get("inscrutable"))
					|| (tacticalRange && target->IsYours()))
			{
				info.SetCondition("tactical display");
				info.SetString("target crew", to_string(target->Crew()));
				int fuel = round(target->Fuel() * target->Attributes().Get("fuel capacity"));
				info.SetString("target fuel", to_string(fuel));
				int energy = round(target->Energy() * target->Attributes().Get("energy capacity"));
				info.SetString("target energy", to_string(energy));
				int heat = round(100. * target->Heat());
				info.SetString("target heat", to_string(heat) + "%");
			}
		}
	}
	if(!Preferences::Has("Ship outlines in HUD"))
		info.SetCondition("fast hud sprites");
	if(target && target->IsTargetable() && target->GetSystem() == currentSystem
		&& (flagship->CargoScanFraction() || flagship->OutfitScanFraction()))
	{
		double width = max(target->Width(), target->Height());
		Point pos = target->Position() - center;
		statuses.emplace_back(pos, flagship->OutfitScanFraction(), flagship->CargoScanFraction(),
			0., 10. + max(20., width * .5), 4, 1.f, Angle(pos).Degrees() + 180.);
	}
	// Handle any events that change the selected ships.
	if(groupSelect >= 0)
	{
		// This has to be done in Step() to avoid race conditions.
		if(hasControl)
			player.SetGroup(groupSelect);
		else
			player.SelectGroup(groupSelect, hasShift);
		groupSelect = -1;
	}
	if(doClickNextStep)
	{
		// If a click command is issued, always wait until the next step to act
		// on it, to avoid race conditions.
		doClick = true;
		doClickNextStep = false;
	}
	else
		doClick = false;

	if(doClick && !isRightClick)
	{
		if(uiClickBox.Dimensions())
			doClick = !ammoDisplay.Click(uiClickBox);
		else
			doClick = !ammoDisplay.Click(clickPoint, hasControl);
		doClick = doClick && !player.SelectShips(clickBox, hasShift);
		if(doClick)
		{
			const vector<const Ship *> &stack = escorts.Click(clickPoint);
			if(!stack.empty())
				doClick = !player.SelectShips(stack, hasShift);
			else
				clickPoint /= isRadarClick ? RADAR_SCALE : zoom;
		}
	}

	// Draw crosshairs on all the selected ships.
	for(const weak_ptr<Ship> &selected : player.SelectedShips())
	{
		shared_ptr<Ship> ship = selected.lock();
		if(ship && ship != target && !ship->IsParked() && ship->GetSystem() == player.GetSystem()
				&& !ship->IsDestroyed() && ship->Zoom() > 0.)
		{
			double size = (ship->Width() + ship->Height()) * .35;
			targets.push_back({
				ship->Position() - center,
				Angle(45.) + ship->Facing(),
				size,
				*GameData::Colors().Get("ship target pointer player"),
				4});
		}
	}

	// Draw crosshairs on any minables in range of the flagship's scanners.
	bool shouldShowAsteroidOverlay = Preferences::Has("Show asteroid scanner overlay");
	// Decide before looping whether or not to catalog asteroids. This
	// results in cataloging in-range asteroids roughly 3 times a second.
	bool shouldCatalogAsteroids = (!isAsteroidCatalogComplete && !Random::Int(20));
	if(shouldShowAsteroidOverlay || shouldCatalogAsteroids)
	{
		double scanRangeMetric = flagship ? 10000. * flagship->Attributes().Get("asteroid scan power") : 0.;
		if(flagship && scanRangeMetric && !flagship->IsHyperspacing())
		{
			bool scanComplete = true;
			for(const shared_ptr<Minable> &minable : asteroids.Minables())
			{
				Point offset = minable->Position() - center;
				// Use the squared length, as we used the squared scan range.
				bool inRange = offset.LengthSquared() <= scanRangeMetric;

				// Autocatalog asteroid: Record that the player knows this type of asteroid is available here.
				if(shouldCatalogAsteroids && !asteroidsScanned.count(minable->DisplayName()))
				{
					scanComplete = false;
					if(!Random::Int(10) && inRange)
					{
						asteroidsScanned.insert(minable->DisplayName());
						for(const auto &payload : minable->GetPayload())
							player.Harvest(payload.outfit);
					}
				}

				if(!shouldShowAsteroidOverlay || !inRange || flagship->GetTargetAsteroid() == minable)
					continue;

				targets.push_back({
					offset,
					minable->Facing(),
					.8 * minable->Radius(),
					GetMinablePointerColor(false),
					3
				});
			}
			if(shouldCatalogAsteroids && scanComplete)
				isAsteroidCatalogComplete = true;
		}
	}
	const auto targetAsteroidPtr = flagship ? flagship->GetTargetAsteroid() : nullptr;
	if(targetAsteroidPtr && !flagship->IsHyperspacing())
		targets.push_back({
			targetAsteroidPtr->Position() - center,
			targetAsteroidPtr->Facing(),
			.8 * targetAsteroidPtr->Radius(),
			GetMinablePointerColor(true),
			3
		});
}



// Begin the next step of calculations.
void Engine::Go()
{
	++step;
	currentCalcBuffer = currentCalcBuffer ? 0 : 1;
	queue.Run([this] { CalculateStep(); });
}



// Give a command on behalf of the player, used for integration tests.
void Engine::GiveCommand(const Command &command)
{
	activeCommands.Set(command);
}



// Pass the list of game events to MainPanel for handling by the player, and any
// UI element generation.
list<ShipEvent> &Engine::Events()
{
	return events;
}



// Draw a frame.
void Engine::Draw() const
{
	Point motionBlur = Preferences::Has("Render motion blur") ? centerVelocity : Point();

	Preferences::ExtendedJumpEffects jumpEffectState = Preferences::GetExtendedJumpEffects();
	if(jumpEffectState != Preferences::ExtendedJumpEffects::OFF)
		motionBlur *= 1. + pow(hyperspacePercentage *
			(jumpEffectState == Preferences::ExtendedJumpEffects::MEDIUM ? 2.5 : 5.), 2);

	GameData::Background().Draw(center, motionBlur, zoom,
		(player.Flagship() ? player.Flagship()->GetSystem() : player.GetSystem()));
	static const Set<Color> &colors = GameData::Colors();
	const Interface *hud = GameData::Interfaces().Get("hud");

	// Draw any active planet labels.
	if(Preferences::Has("Show planet labels"))
		for(const PlanetLabel &label : labels)
			label.Draw();

	draw[currentDrawBuffer].Draw();
	batchDraw[currentDrawBuffer].Draw();

	for(const auto &it : statuses)
	{
		static const Color color[14] = {
			*colors.Get("overlay flagship shields"),
			*colors.Get("overlay friendly shields"),
			*colors.Get("overlay hostile shields"),
			*colors.Get("overlay neutral shields"),
			*colors.Get("overlay outfit scan"),
			*colors.Get("overlay flagship hull"),
			*colors.Get("overlay friendly hull"),
			*colors.Get("overlay hostile hull"),
			*colors.Get("overlay neutral hull"),
			*colors.Get("overlay cargo scan"),
			*colors.Get("overlay flagship disabled"),
			*colors.Get("overlay friendly disabled"),
			*colors.Get("overlay hostile disabled"),
			*colors.Get("overlay neutral disabled")
		};
		Point pos = it.position * zoom;
		double radius = it.radius * zoom;
		if(it.outer > 0.)
			RingShader::Draw(pos, radius + 3., 1.5f, it.outer,
				Color::Multiply(it.alpha, color[it.type]), 0.f, it.angle);
		double dashes = (it.type >= 5) ? 0. : 20. * min<double>(1., zoom);
		if(it.inner > 0.)
			RingShader::Draw(pos, radius, 1.5f, it.inner,
				Color::Multiply(it.alpha, color[5 + it.type]), dashes, it.angle);
		if(it.disabled > 0.)
			RingShader::Draw(pos, radius, 1.5f, it.disabled,
				Color::Multiply(it.alpha, color[10 + it.type]), dashes, it.angle);
	}

	// Draw labels on missiles
	for(const AlertLabel &label : missileLabels)
		label.Draw();

	for(const auto &outline : outlines)
	{
		if(!outline.sprite)
			continue;
		Point size(outline.sprite->Width(), outline.sprite->Height());
		OutlineShader::Draw(outline.sprite, outline.position, size, outline.color, outline.unit, outline.frame);
	}

	if(flash)
		FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), Color(flash, flash));

	// Draw messages. Draw the most recent messages first, as some messages
	// may be wrapped onto multiple lines.
	const Font &font = FontSet::Get(14);
	const vector<Messages::Entry> &messages = Messages::Get(step);
	Rectangle messageBox = hud->GetBox("messages");
	WrappedText messageLine(font);
	messageLine.SetWrapWidth(messageBox.Width());
	messageLine.SetParagraphBreak(0.);
	Point messagePoint = Point(messageBox.Left(), messageBox.Bottom());
	for(auto it = messages.rbegin(); it != messages.rend(); ++it)
	{
		messageLine.Wrap(it->message);
		messagePoint.Y() -= messageLine.Height();
		if(messagePoint.Y() < messageBox.Top())
			break;
		float alpha = (it->step + 1000 - step) * .001f;
		messageLine.Draw(messagePoint, Messages::GetColor(it->importance, false)->Additive(alpha));
	}

	// Draw crosshairs around anything that is targeted.
	for(const Target &target : targets)
	{
		Angle a = target.angle;
		Angle da(360. / target.count);

		PointerShader::Bind();
		for(int i = 0; i < target.count; ++i)
		{
			PointerShader::Add(target.center * zoom, a.Unit(), 12.f, 14.f, -target.radius * zoom, target.color);
			a += da;
		}
		PointerShader::Unbind();
	}

	// Draw the heads-up display.
	hud->Draw(info);
	if(hud->HasPoint("radar"))
	{
		radar[currentDrawBuffer].Draw(
			hud->GetPoint("radar"),
			RADAR_SCALE,
			hud->GetValue("radar radius"),
			hud->GetValue("radar pointer radius"));
	}
	if(hud->HasPoint("target") && targetVector.Length() > 20.)
	{
		Point center = hud->GetPoint("target");
		double radius = hud->GetValue("target radius");
		PointerShader::Draw(center, targetVector.Unit(), 10.f, 10.f, radius, Color(1.f));
	}

	// Draw the faction markers.
	if(targetSwizzle >= 0 && hud->HasPoint("faction markers"))
	{
		int width = font.Width(info.GetString("target government"));
		Point center = hud->GetPoint("faction markers");

		const Sprite *mark[2] = {SpriteSet::Get("ui/faction left"), SpriteSet::Get("ui/faction right")};
		// Round the x offsets to whole numbers so the icons are sharp.
		double dx[2] = {(width + mark[0]->Width() + 1) / -2, (width + mark[1]->Width() + 1) / 2};
		for(int i = 0; i < 2; ++i)
			SpriteShader::Draw(mark[i], center + Point(dx[i], 0.), 1., targetSwizzle);
	}
	if(jumpCount && Preferences::Has("Show mini-map"))
		MapPanel::DrawMiniMap(player, .5f * min(1.f, jumpCount / 30.f), jumpInProgress, step);

	// Draw ammo status.
	double ammoIconWidth = hud->GetValue("ammo icon width");
	double ammoIconHeight = hud->GetValue("ammo icon height");
	ammoDisplay.Draw(hud->GetBox("ammo"), Point(ammoIconWidth, ammoIconHeight));

	// Draw escort status.
	escorts.Draw(hud->GetBox("escorts"));

	if(Preferences::Has("Show CPU / GPU load"))
	{
		string loadString = to_string(lround(load * 100.)) + "% CPU";
		Color color = *colors.Get("medium");
		font.Draw(loadString,
			Point(-10 - font.Width(loadString), Screen::Height() * -.5 + 5.), color);
	}
}



// Select the object the player clicked on.
void Engine::Click(const Point &from, const Point &to, bool hasShift, bool hasControl)
{
	// First, see if this is a click on an escort icon.
	doClickNextStep = true;
	this->hasShift = hasShift;
	this->hasControl = hasControl;
	isRightClick = false;

	// Determine if the left-click was within the radar display.
	const Interface *hud = GameData::Interfaces().Get("hud");
	Point radarCenter = hud->GetPoint("radar");
	double radarRadius = hud->GetValue("radar radius");
	if(Preferences::Has("Clickable radar display") && (from - radarCenter).Length() <= radarRadius)
		isRadarClick = true;
	else
		isRadarClick = false;

	clickPoint = isRadarClick ? from - radarCenter : from;
	uiClickBox = Rectangle::WithCorners(from, to);
	if(isRadarClick)
		clickBox = Rectangle::WithCorners(
			(from - radarCenter) / RADAR_SCALE + center,
			(to - radarCenter) / RADAR_SCALE + center);
	else
		clickBox = Rectangle::WithCorners(from / zoom + center, to / zoom + center);
}



void Engine::RClick(const Point &point)
{
	doClickNextStep = true;
	hasShift = false;
	isRightClick = true;

	// Determine if the right-click was within the radar display, and if so, rescale.
	const Interface *hud = GameData::Interfaces().Get("hud");
	Point radarCenter = hud->GetPoint("radar");
	double radarRadius = hud->GetValue("radar radius");
	if(Preferences::Has("Clickable radar display") && (point - radarCenter).Length() <= radarRadius)
		clickPoint = (point - radarCenter) / RADAR_SCALE;
	else
		clickPoint = point / zoom;
}



void Engine::SelectGroup(int group, bool hasShift, bool hasControl)
{
	groupSelect = group;
	this->hasShift = hasShift;
	this->hasControl = hasControl;
}



// Break targeting on all projectiles between the player and the given
// government; gov projectiles stop targeting the player and player's
// projectiles stop targeting gov.
void Engine::BreakTargeting(const Government *gov)
{
	const Government *playerGov = GameData::PlayerGovernment();
	for(Projectile &projectile : projectiles)
	{
		const Government *projectileGov = projectile.GetGovernment();
		const Government *targetGov = projectile.TargetGovernment();
		if((projectileGov == playerGov && targetGov == gov)
			|| (projectileGov == gov && targetGov == playerGov))
			projectile.BreakTarget();
	}
}



void Engine::EnterSystem()
{
	ai.Clean();

	Ship *flagship = player.Flagship();
	if(!flagship)
		return;

	doEnter = true;
	doEnterLabels = true;
	player.IncrementDate();
	const Date &today = player.GetDate();

	const System *system = flagship->GetSystem();
	Audio::PlayMusic(system->MusicName());
	GameData::SetHaze(system->Haze(), false);

	Messages::Add("Entering the " + system->Name() + " system on "
		+ today.ToString() + (system->IsInhabited(flagship) ?
			"." : ". No inhabited planets detected."), Messages::Importance::Daily);

	// Preload landscapes and determine if the player used a wormhole.
	// (It is allowed for a wormhole's exit point to have no sprite.)
	const StellarObject *usedWormhole = nullptr;
	for(const StellarObject &object : system->Objects())
		if(object.HasValidPlanet())
		{
			GameData::Preload(queue, object.GetPlanet()->Landscape());
			if(object.GetPlanet()->IsWormhole() && !usedWormhole
					&& flagship->Position().Distance(object.Position()) < 1.)
				usedWormhole = &object;
		}

	// Advance the positions of every StellarObject and update politics.
	// Remove expired bribes, clearance, and grace periods from past fines.
	GameData::SetDate(today);
	GameData::StepEconomy();
	// SetDate() clears any bribes from yesterday, so restore any auto-clearance.
	for(const Mission &mission : player.Missions())
		if(mission.ClearanceMessage() == "auto")
		{
			mission.Destination()->Bribe(mission.HasFullClearance());
			for(const Planet *planet : mission.Stopovers())
				planet->Bribe(mission.HasFullClearance());
		}

	if(usedWormhole)
	{
		// If ships use a wormhole, they are emitted from its center in
		// its destination system. Player travel causes a date change,
		// thus the wormhole's new position should be used.
		flagship->SetPosition(usedWormhole->Position());
		if(player.HasTravelPlan())
		{
			// Wormhole travel generally invalidates travel plans
			// unless it was planned. For valid travel plans, the
			// next system will be this system, or accessible.
			const System *to = player.TravelPlan().back();
			if(system != to && !flagship->JumpNavigation().JumpFuel(to))
				player.TravelPlan().clear();
		}
	}

	asteroids.Clear();
	for(const System::Asteroid &a : system->Asteroids())
	{
		// Check whether this is a minable or an ordinary asteroid.
		if(a.Type())
			asteroids.Add(a.Type(), a.Count(), a.Energy(), system->AsteroidBelts());
		else
			asteroids.Add(a.Name(), a.Count(), a.Energy());
	}
	asteroidsScanned.clear();
	isAsteroidCatalogComplete = false;

	// Clear any active weather events
	activeWeather.clear();
	// Place five seconds worth of fleets and weather events. Check for
	// undefined fleets by not trying to create anything with no
	// government set.
	for(int i = 0; i < 5; ++i)
	{
		for(const auto &fleet : system->Fleets())
			if(fleet.Get()->GetGovernment() && Random::Int(fleet.Period()) < 60)
				fleet.Get()->Place(*system, newShips);

		auto CreateWeather = [this](const RandomEvent<Hazard> &hazard, Point origin)
		{
			if(hazard.Get()->IsValid() && Random::Int(hazard.Period()) < 60)
			{
				const Hazard *weather = hazard.Get();
				int hazardLifetime = weather->RandomDuration();
				// Elapse this weather event by a random amount of time.
				int elapsedLifetime = hazardLifetime - Random::Int(hazardLifetime + 1);
				activeWeather.emplace_back(weather, hazardLifetime, elapsedLifetime, weather->RandomStrength(), origin);
			}
		};
		for(const auto &hazard : system->Hazards())
			CreateWeather(hazard, Point());
		for(const auto &stellar : system->Objects())
			for(const auto &hazard : stellar.Hazards())
				CreateWeather(hazard, stellar.Position());
	}

	for(const auto &raidFleet : system->RaidFleets())
	{
		double attraction = player.RaidFleetAttraction(raidFleet, system);
		if(attraction > 0.)
			for(int i = 0; i < 10; ++i)
				if(Random::Real() < attraction)
				{
					raidFleet.GetFleet()->Place(*system, newShips);
					Messages::Add("Your fleet has attracted the interest of a "
							+ raidFleet.GetFleet()->GetGovernment()->GetName() + " raiding party.",
							Messages::Importance::Highest);
				}
	}

	grudge.clear();

	projectiles.clear();
	visuals.clear();
	flotsam.clear();
	// Cancel any projectiles, visuals, or flotsam created by ships this step.
	newProjectiles.clear();
	newVisuals.clear();
	newFlotsam.clear();


	center = flagship->Center();

	// Help message for new players. Show this message for the first four days,
	// since the new player ships can make at most four jumps before landing.
	if(today <= player.StartData().GetDate() + 4)
	{
		Messages::Add(GameData::HelpMessage("basics 1"), Messages::Importance::High);
		Messages::Add(GameData::HelpMessage("basics 2"), Messages::Importance::High);
	}
}



void Engine::CalculateStep()
{
	FrameTimer loadTimer;

	// If there is a pending zoom update then use it
	// because the zoom will get updated in the main thread
	// as soon as the calculation thread is finished.
	const double zoom = nextZoom ? nextZoom : this->zoom;

	// Clear the list of objects to draw.
	draw[currentCalcBuffer].Clear(step, zoom);
	batchDraw[currentCalcBuffer].Clear(step, zoom);
	radar[currentCalcBuffer].Clear();

	if(!player.GetSystem())
		return;

	// Handle the mouse input of the mouse navigation
	HandleMouseInput(activeCommands);
	// Now, all the ships must decide what they are doing next.
	ai.Step(activeCommands);

	// Clear the active players commands, they are all processed at this point.
	activeCommands.Clear();

	// Perform actions for all the game objects. In general this is ordered from
	// bottom to top of the draw stack, but in some cases one object type must
	// "act" before another does.

	// The only action stellar objects perform is to launch defense fleets.
	const System *playerSystem = player.GetSystem();
	for(const StellarObject &object : playerSystem->Objects())
		if(object.HasValidPlanet())
			object.GetPlanet()->DeployDefense(newShips);

	// Keep track of the flagship to see if it jumps or enters a wormhole this turn.
	const Ship *flagship = player.Flagship();
	bool flagshipWasUntargetable = (flagship && !flagship->IsTargetable());
	bool wasHyperspacing = (flagship && flagship->IsEnteringHyperspace());
	// First, move the player's flagship.
	if(flagship)
		MoveShip(player.FlagshipPtr());
	const System *flagshipSystem = (flagship ? flagship->GetSystem() : nullptr);
	bool flagshipIsTargetable = (flagship && flagship->IsTargetable());
	bool flagshipBecameTargetable = flagshipWasUntargetable && flagshipIsTargetable;
	// Then, move the other ships.
	for(const shared_ptr<Ship> &it : ships)
	{
		if(it == player.FlagshipPtr())
			continue;
		bool wasUntargetable = !it->IsTargetable();
		MoveShip(it);
		bool isTargetable = it->IsTargetable();
		if(flagshipSystem == it->GetSystem()
			&& ((wasUntargetable && isTargetable) || flagshipBecameTargetable)
			&& isTargetable && flagshipIsTargetable)
				eventQueue.emplace_back(player.FlagshipPtr(), it, ShipEvent::ENCOUNTER);
	}
	// If the flagship just began jumping, play the appropriate sound.
	if(!wasHyperspacing && flagship && flagship->IsEnteringHyperspace())
	{
		bool isJumping = flagship->IsUsingJumpDrive();
		const map<const Sound *, int> &jumpSounds = isJumping
			? flagship->Attributes().JumpSounds() : flagship->Attributes().HyperSounds();
		if(jumpSounds.empty())
			Audio::Play(Audio::Get(isJumping ? "jump drive" : "hyperdrive"));
		else
			for(const auto &sound : jumpSounds)
				Audio::Play(sound.first);
	}
	// Check if the flagship just entered a new system.
	if(flagship && playerSystem != flagship->GetSystem())
	{
		bool wormholeEntry = false;
		// Wormhole travel: mark the wormhole "planet" as visited.
		if(!wasHyperspacing)
			for(const auto &it : playerSystem->Objects())
				if(it.HasValidPlanet() && it.GetPlanet()->IsWormhole() &&
						&it.GetPlanet()->GetWormhole()->WormholeDestination(*playerSystem) == flagship->GetSystem())
				{
					wormholeEntry = true;
					player.Visit(*it.GetPlanet());
				}

		player.SetSystemEntry(wormholeEntry ? SystemEntry::WORMHOLE :
			flagship->IsUsingJumpDrive() ? SystemEntry::JUMP :
			SystemEntry::HYPERDRIVE);
		doFlash = Preferences::Has("Show hyperspace flash");
		playerSystem = flagship->GetSystem();
		player.SetSystem(*playerSystem);
		EnterSystem();
	}
	Prune(ships);

	// Move the asteroids. This must be done before collision detection. Minables
	// may create visuals or flotsam.
	asteroids.Step(newVisuals, newFlotsam, step);

	// Move the flotsam. This must happen after the ships move, because flotsam
	// checks if any ship has picked it up.
	for(const shared_ptr<Flotsam> &it : flotsam)
		it->Move(newVisuals);
	Prune(flotsam);

	// Move the projectiles.
	for(Projectile &projectile : projectiles)
		projectile.Move(newVisuals, newProjectiles);
	Prune(projectiles);

	// Step the weather.
	for(Weather &weather : activeWeather)
		weather.Step(newVisuals, flagship ? flagship->Position() : center);
	Prune(activeWeather);

	// Move the visuals.
	for(Visual &visual : visuals)
		visual.Move();
	Prune(visuals);

	// Perform various minor actions.
	SpawnFleets();
	SpawnPersons();
	GenerateWeather();
	SendHails();
	HandleMouseClicks();

	// Now, take the new objects that were generated this step and splice them
	// on to the ends of the respective lists of objects. These new objects will
	// be drawn this step (and the projectiles will participate in collision
	// detection) but they should not be moved, which is why we put off adding
	// them to the lists until now.
	ships.splice(ships.end(), newShips);
	Append(projectiles, newProjectiles);
	flotsam.splice(flotsam.end(), newFlotsam);
	Append(visuals, newVisuals);

	// Decrement the count of how long it's been since a ship last asked for help.
	if(grudgeTime)
		--grudgeTime;

	// Populate the collision detection lookup sets.
	FillCollisionSets();

	// Perform collision detection.
	for(Projectile &projectile : projectiles)
		DoCollisions(projectile);
	// Now that collision detection is done, clear the cache of ships with anti-
	// missile systems ready to fire.
	hasAntiMissile.clear();

	// Damage ships from any active weather events.
	for(Weather &weather : activeWeather)
		DoWeather(weather);

	// Check for flotsam collection (collisions with ships).
	for(const shared_ptr<Flotsam> &it : flotsam)
		DoCollection(*it);

	// Now that flotsam collection is done, clear the cache of ships with
	// tractor beam systems ready to fire.
	hasTractorBeam.clear();

	// Check for ship scanning.
	for(const shared_ptr<Ship> &it : ships)
		DoScanning(it);

	// Draw the objects. Start by figuring out where the view should be centered:
	Point newCenter = center;
	Point newCenterVelocity;
	if(flagship)
	{
		const auto [newCameraCenter, newCameraVelocity] = NewCenter(center, centerVelocity,
			flagship->Center(), flagship->Velocity(), flagship->GetHyperspacePercentage() / 100.,
			flagship->IsHyperspacing());
		newCenter = newCameraCenter;
		newCenterVelocity = newCameraVelocity;
	}
	draw[currentCalcBuffer].SetCenter(newCenter, newCenterVelocity);
	batchDraw[currentCalcBuffer].SetCenter(newCenter);
	radar[currentCalcBuffer].SetCenter(newCenter);

	// Populate the radar.
	FillRadar();

	// Draw the planets.
	for(const StellarObject &object : playerSystem->Objects())
		if(object.HasSprite())
		{
			// Don't apply motion blur to very large planets and stars.
			if(object.Width() >= 280.)
				draw[currentCalcBuffer].AddUnblurred(object);
			else
				draw[currentCalcBuffer].Add(object);
		}
	// Draw the asteroids and minables.
	asteroids.Draw(draw[currentCalcBuffer], newCenter, zoom);
	// Draw the flotsam.
	for(const shared_ptr<Flotsam> &it : flotsam)
		draw[currentCalcBuffer].Add(*it);
	// Draw the ships. Skip the flagship, then draw it on top of all the others.
	bool showFlagship = false;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == playerSystem && ship->HasSprite())
		{
			if(ship.get() != flagship)
			{
				DrawShipSprites(*ship);
				if(ship->IsThrusting() && !ship->EnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().FlareSounds())
						Audio::Play(it.first, ship->Position());
				}
				else if(ship->IsReversing() && !ship->ReverseEnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().ReverseFlareSounds())
						Audio::Play(it.first, ship->Position());
				}
				if(ship->IsSteering() && !ship->SteeringEnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().SteeringFlareSounds())
						Audio::Play(it.first, ship->Position());
				}
			}
			else
				showFlagship = true;
		}

	if(flagship && showFlagship)
	{
		DrawShipSprites(*flagship);
		if(flagship->IsThrusting() && !flagship->EnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().FlareSounds())
				Audio::Play(it.first);
		}
		else if(flagship->IsReversing() && !flagship->ReverseEnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().ReverseFlareSounds())
				Audio::Play(it.first);
		}
		if(flagship->IsSteering() && !flagship->SteeringEnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().SteeringFlareSounds())
				Audio::Play(it.first);
		}
	}
	// Draw the projectiles.
	for(const Projectile &projectile : projectiles)
		batchDraw[currentCalcBuffer].Add(projectile, projectile.Clip());
	// Draw the visuals.
	for(const Visual &visual : visuals)
		batchDraw[currentCalcBuffer].AddVisual(visual);

	// Keep track of how much of the CPU time we are using.
	loadSum += loadTimer.Time();
	if(++loadCount == 60)
	{
		load = loadSum;
		loadSum = 0.;
		loadCount = 0;
	}
}



// Move a ship. Also determine if the ship should generate hyperspace sounds or
// boarding events, fire weapons, and launch fighters.
void Engine::MoveShip(const shared_ptr<Ship> &ship)
{
	// Various actions a ship could have taken last frame may have impacted the accuracy of cached values.
	// Therefore, determine with any information needs recalculated and cache it.
	ship->UpdateCaches();

	const Ship *flagship = player.Flagship();

	bool isJump = ship->IsUsingJumpDrive();
	bool wasHere = (flagship && ship->GetSystem() == flagship->GetSystem());
	bool wasHyperspacing = ship->IsHyperspacing();
	bool wasDisabled = ship->IsDisabled();
	// Give the ship the list of visuals so that it can draw explosions,
	// ion sparks, jump drive flashes, etc.
	ship->Move(newVisuals, newFlotsam);
	if(ship->IsDisabled() && !wasDisabled)
		eventQueue.emplace_back(nullptr, ship, ShipEvent::DISABLE);
	// Bail out if the ship just died.
	if(ship->ShouldBeRemoved())
	{
		// Make sure this ship's destruction was recorded, even if it died from
		// self-destruct.
		if(ship->IsDestroyed())
		{
			eventQueue.emplace_back(nullptr, ship, ShipEvent::DESTROY);
			// Any still-docked ships' destruction must be recorded as well.
			for(const auto &bay : ship->Bays())
				if(bay.ship)
					eventQueue.emplace_back(nullptr, bay.ship, ShipEvent::DESTROY);
		}
		return;
	}

	// Check if we need to play sounds for a ship jumping in or out of
	// the system. Make no sound if it entered via wormhole.
	if(ship.get() != flagship && ship->Zoom() == 1.)
	{
		// The position from where sounds will be played.
		Point position = ship->Position();
		// Did this ship just begin hyperspacing?
		if(wasHere && !wasHyperspacing && ship->IsHyperspacing())
		{
			const map<const Sound *, int> &jumpSounds = isJump
				? ship->Attributes().JumpOutSounds() : ship->Attributes().HyperOutSounds();
			if(jumpSounds.empty())
				Audio::Play(Audio::Get(isJump ? "jump out" : "hyperdrive out"), position);
			else
				for(const auto &sound : jumpSounds)
					Audio::Play(sound.first, position);
		}

		// Did this ship just jump into the player's system?
		if(!wasHere && flagship && ship->GetSystem() == flagship->GetSystem())
		{
			const map<const Sound *, int> &jumpSounds = isJump
				? ship->Attributes().JumpInSounds() : ship->Attributes().HyperInSounds();
			if(jumpSounds.empty())
				Audio::Play(Audio::Get(isJump ? "jump in" : "hyperdrive in"), position);
			else
				for(const auto &sound : jumpSounds)
					Audio::Play(sound.first, position);
		}
	}

	// Boarding:
	bool autoPlunder = !ship->IsYours();
	// The player should not become a docked passenger on some other ship, but AI ships may.
	bool nonDocker = ship.get() == flagship;
	shared_ptr<Ship> victim = ship->Board(autoPlunder, nonDocker);
	if(victim)
		eventQueue.emplace_back(ship, victim,
			ship->GetGovernment()->IsEnemy(victim->GetGovernment()) ?
				ShipEvent::BOARD : ShipEvent::ASSIST);

	// The remaining actions can only be performed by ships in the current system.
	if(ship->GetSystem() != player.GetSystem())
		return;

	// Launch fighters.
	ship->Launch(newShips, newVisuals);

	// Fire weapons.
	ship->Fire(newProjectiles, newVisuals);

	// Anti-missile and tractor beam systems are fired separately from normal weaponry.
	// Track which ships have at least one such system ready to fire.
	if(ship->HasAntiMissile())
		hasAntiMissile.push_back(ship.get());
	if(ship->HasTractorBeam())
		hasTractorBeam.push_back(ship.get());
}



// Populate the ship collision detection set for projectile & flotsam computations.
void Engine::FillCollisionSets()
{
	shipCollisions.Clear(step);
	for(const shared_ptr<Ship> &it : ships)
		if(it->GetSystem() == player.GetSystem() && it->Zoom() == 1.)
			shipCollisions.Add(*it);

	// Get the ship collision set ready to query.
	shipCollisions.Finish();
}



// Spawn NPC (both mission and "regular") ships into the player's universe. Non-
// mission NPCs are only spawned in or adjacent to the player's system.
void Engine::SpawnFleets()
{
	// If the player has a pending boarding mission, spawn its NPCs.
	if(player.ActiveBoardingMission())
	{
		Place(player.ActiveBoardingMission()->NPCs(), player.FlagshipPtr());
		player.ClearActiveBoardingMission();
	}

	// Non-mission NPCs spawn at random intervals in neighboring systems,
	// or coming from planets in the current one.
	for(const auto &fleet : player.GetSystem()->Fleets())
		if(!Random::Int(fleet.Period()))
		{
			const Government *gov = fleet.Get()->GetGovernment();
			if(!gov)
				continue;

			// Don't spawn a fleet if its allies in-system already far outnumber
			// its enemies. This is to avoid having a system get mobbed with
			// massive numbers of "reinforcements" during a battle.
			int64_t enemyStrength = ai.EnemyStrength(gov);
			if(enemyStrength && ai.AllyStrength(gov) > 2 * enemyStrength)
				continue;

			fleet.Get()->Enter(*player.GetSystem(), newShips);
		}
}



// At random intervals, create new special "persons" who enter the current system.
void Engine::SpawnPersons()
{
	if(Random::Int(GameData::GetGamerules().PersonSpawnPeriod()) || player.GetSystem()->Links().empty())
		return;

	// Loop through all persons once to see if there are any who can enter
	// this system.
	int sum = 0;
	for(const auto &it : GameData::Persons())
		sum += it.second.Frequency(player.GetSystem());
	// Bail out if there are no eligible persons.
	if(!sum)
		return;

	// Although an attempt to spawn a person is made every 10 minutes on average,
	// that attempt can still fail due to an added weight for no person to spawn.
	sum = Random::Int(sum + GameData::GetGamerules().NoPersonSpawnWeight());
	for(const auto &it : GameData::Persons())
	{
		const Person &person = it.second;
		sum -= person.Frequency(player.GetSystem());
		if(sum < 0)
		{
			const System *source = nullptr;
			shared_ptr<Ship> parent;
			for(const shared_ptr<Ship> &ship : person.Ships())
			{
				ship->Recharge();
				if(ship->Name().empty())
					ship->SetName(it.first);
				ship->SetGovernment(person.GetGovernment());
				ship->SetPersonality(person.GetPersonality());
				ship->SetHailPhrase(person.GetHail());
				if(!parent)
					parent = ship;
				else
					ship->SetParent(parent);
				// Make sure all ships in a "person" definition enter from the
				// same source system.
				source = Fleet::Enter(*player.GetSystem(), *ship, source);
				newShips.push_back(ship);
			}

			break;
		}
	}
}



// Generate weather from the current system's hazards.
void Engine::GenerateWeather()
{
	auto CreateWeather = [this](const RandomEvent<Hazard> &hazard, Point origin)
	{
		if(hazard.Get()->IsValid() && !Random::Int(hazard.Period()))
		{
			const Hazard *weather = hazard.Get();
			// If a hazard has activated, generate a duration and strength of the
			// resulting weather and place it in the list of active weather.
			int duration = weather->RandomDuration();
			activeWeather.emplace_back(weather, duration, duration, weather->RandomStrength(), origin);
		}
	};
	// If this system has any hazards, see if any have activated this frame.
	for(const auto &hazard : player.GetSystem()->Hazards())
		CreateWeather(hazard, Point());
	for(const auto &stellar : player.GetSystem()->Objects())
		for(const auto &hazard : stellar.Hazards())
			CreateWeather(hazard, stellar.Position());
}



// At random intervals, have one of the ships in the game send you a hail.
void Engine::SendHails()
{
	if(Random::Int(600) || player.IsDead() || ships.empty())
		return;

	vector<shared_ptr<const Ship>> canSend;
	canSend.reserve(ships.size());

	// When deciding who will send a hail, only consider ships that can send hails.
	for(auto &it : ships)
		if(it && it->CanSendHail(player, true))
			canSend.push_back(it);

	if(canSend.empty())
		// No ships can send hails.
		return;

	// Randomly choose a ship to send the hail.
	unsigned i = Random::Int(canSend.size());
	shared_ptr<const Ship> source = canSend[i];

	// Generate a random hail message.
	SendMessage(source, source->GetHail(player.GetSubstitutions()));
}



// Handle any keyboard inputs for the engine. This is done in the main thread
// after all calculation threads are paused to avoid race conditions.
void Engine::HandleKeyboardInputs()
{
	Ship *flagship = player.Flagship();

	// Commands can't be issued if your flagship is dead.
	if(!flagship || flagship->IsDestroyed())
		return;

	// Determine which new keys were pressed by the player.
	Command oldHeld = keyHeld;
	keyHeld.ReadKeyboard();
	Command keyDown = keyHeld.AndNot(oldHeld);

	// Certain commands are always sent when the corresponding key is depressed.
	static const Command maneuveringCommands = Command::AFTERBURNER | Command::BACK |
		Command::FORWARD | Command::LEFT | Command::RIGHT;

	// Transfer all commands that need to be active as long as the corresponding key is pressed.
	activeCommands |= keyHeld.And(Command::PRIMARY | Command::SECONDARY | Command::SCAN |
		maneuveringCommands | Command::SHIFT | Command::MOUSE_TURNING_HOLD);

	// Certain commands (e.g. LAND, BOARD) are debounced, allowing the player to toggle between
	// navigable destinations in the system.
	static const Command debouncedCommands = Command::LAND | Command::BOARD;
	constexpr int keyCooldown = 60;
	++keyInterval;
	if(oldHeld.Has(debouncedCommands))
		keyInterval = 0;

	// If all previously-held maneuvering keys have been released,
	// restore any autopilot commands still being requested.
	if(!keyHeld.Has(maneuveringCommands) && oldHeld.Has(maneuveringCommands))
	{
		activeCommands |= keyHeld.And(Command::JUMP | Command::FLEET_JUMP | debouncedCommands);

		// Do not switch debounced targets when restoring autopilot.
		keyInterval = keyCooldown;
	}

	// If holding JUMP or toggling a debounced command, also send WAIT. This prevents the jump from
	// starting (e.g. while escorts are aligning), or switches the associated debounced target.
	if(keyHeld.Has(Command::JUMP) || (keyInterval < keyCooldown && keyHeld.Has(debouncedCommands)))
		activeCommands |= Command::WAIT;

	// Transfer all newly pressed, unhandled keys to active commands.
	activeCommands |= keyDown;

	// Some commands are activated by combining SHIFT with a different key.
	if(keyHeld.Has(Command::SHIFT))
	{
		// Translate shift+BACK to a command to a STOP command to stop all movement of the flagship.
		// Translation is done here to allow the autopilot (which will execute the STOP-command) to
		// act on a single STOP command instead of the shift+BACK modifier.
		if(keyHeld.Has(Command::BACK))
		{
			activeCommands |= Command::STOP;
			activeCommands.Clear(Command::BACK);
		}
		else if(keyHeld.Has(Command::JUMP))
			activeCommands |= Command::FLEET_JUMP;
	}

	if(keyHeld.Has(Command::AUTOSTEER) && !activeCommands.Turn()
			&& !activeCommands.Has(Command::LAND | Command::JUMP | Command::BOARD | Command::STOP))
		activeCommands |= Command::AUTOSTEER;
}



// Handle any mouse clicks. This is done in the calculation thread rather than
// in the main UI thread to avoid race conditions.
void Engine::HandleMouseClicks()
{
	// Mouse clicks can't be issued if your flagship is dead.
	Ship *flagship = player.Flagship();
	if(!flagship)
		return;

	// Handle escort travel orders sent via the Map.
	if(player.HasEscortDestination())
	{
		auto moveTarget = player.GetEscortDestination();
		ai.IssueMoveTarget(moveTarget.second, moveTarget.first);
		player.SetEscortDestination();
	}

	// If there is no click event sent while the engine was active, bail out.
	if(!doClick)
		return;

	// Check for clicks on stellar objects. Only left clicks apply, and the
	// flagship must not be in the process of landing or taking off.
	bool clickedPlanet = false;
	const System *playerSystem = player.GetSystem();
	if(!isRightClick && flagship->Zoom() == 1.)
		for(const StellarObject &object : playerSystem->Objects())
			if(object.HasSprite() && object.HasValidPlanet())
			{
				// If the player clicked to land on a planet,
				// do so unless already landing elsewhere.
				Point position = object.Position() - center;
				const Planet *planet = object.GetPlanet();
				if(planet->IsAccessible(flagship) && (clickPoint - position).Length() < object.Radius())
				{
					if(&object == flagship->GetTargetStellar())
					{
						if(!planet->CanLand(*flagship))
							Messages::Add("The authorities on " + planet->Name()
									+ " refuse to let you land.", Messages::Importance::Highest);
						else
						{
							activeCommands |= Command::LAND;
							Messages::Add("Landing on " + planet->Name() + ".", Messages::Importance::High);
						}
					}
					else
						flagship->SetTargetStellar(&object);

					clickedPlanet = true;
				}
			}

	// Check for clicks on ships in this system.
	double clickRange = 50.;
	shared_ptr<Ship> clickTarget;
	for(shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == playerSystem && &*ship != flagship && ship->IsTargetable())
		{
			Point position = ship->Position() - flagship->Position();
			const Mask &mask = ship->GetMask(step);
			double range = mask.Range(clickPoint - position, ship->Facing());
			if(range <= clickRange)
			{
				clickRange = range;
				clickTarget = ship;
				// If we've found an enemy within the click zone, favor
				// targeting it rather than any other ship. Otherwise, keep
				// checking for hits because another ship might be an enemy.
				if(!range && ship->GetGovernment()->IsEnemy())
					break;
			}
		}

	bool clickedAsteroid = false;
	if(clickTarget)
	{
		if(isRightClick)
			ai.IssueShipTarget(clickTarget);
		else
		{
			// Left click: has your flagship select or board the target.
			if(clickTarget == flagship->GetTargetShip())
				activeCommands |= Command::BOARD;
			else
			{
				flagship->SetTargetShip(clickTarget);
				if(clickTarget->IsYours())
					player.SelectShip(clickTarget.get(), hasShift);
			}
		}
	}
	else if(flagship->Attributes().Get("asteroid scan power"))
	{
		// If the click was not on any ship, check if it was on a minable.
		double scanRange = 100. * sqrt(flagship->Attributes().Get("asteroid scan power"));
		for(const shared_ptr<Minable> &minable : asteroids.Minables())
		{
			Point position = minable->Position() - flagship->Position();
			if(position.Length() > scanRange)
				continue;

			double range = clickPoint.Distance(position) - minable->Radius();
			if(range <= clickRange)
			{
				clickedAsteroid = true;
				clickRange = range;
				flagship->SetTargetAsteroid(minable);
				if(isRightClick)
					ai.IssueAsteroidTarget(minable);
			}
		}
	}
	if(isRightClick && !clickTarget && !clickedAsteroid && !isMouseTurningEnabled)
		ai.IssueMoveTarget(clickPoint + center, playerSystem);

	// Treat an "empty" click as a request to clear targets.
	if(!clickTarget && !isRightClick && !clickedAsteroid && !clickedPlanet)
		flagship->SetTargetShip(nullptr);
}



// Determines alternate mouse turning, setting player mouse angle, and right-click firing weapons.
void Engine::HandleMouseInput(Command &activeCommands)
{
	isMouseHoldEnabled = activeCommands.Has(Command::MOUSE_TURNING_HOLD);
	bool isMouseToggleEnabled = Preferences::Has("Control ship with mouse");

	// XOR mouse hold and mouse toggle. If mouse toggle is OFF, then mouse hold
	// will temporarily turn ON mouse control. If mouse toggle is ON, then mouse
	// hold will temporarily turn OFF mouse control.
	isMouseTurningEnabled = (isMouseHoldEnabled ^ isMouseToggleEnabled);
	if(!isMouseTurningEnabled)
		return;
	activeCommands.Set(Command::MOUSE_TURNING_HOLD);
	bool rightMouseButtonHeld = false;
	int mousePosX;
	int mousePosY;
	if((SDL_GetMouseState(&mousePosX, &mousePosY) & SDL_BUTTON_RMASK) != 0)
		rightMouseButtonHeld = true;
	double relX = mousePosX - Screen::RawWidth() / 2.0;
	double relY = mousePosY - Screen::RawHeight() / 2.0;
	ai.SetMousePosition(Point(relX, relY));

	// Activate firing command.
	if(isMouseTurningEnabled && rightMouseButtonHeld)
		activeCommands.Set(Command::PRIMARY);
}



// Perform collision detection. Note that unlike the preceding functions, this
// one adds any visuals that are created directly to the main visuals list. If
// this is multi-threaded in the future, that will need to change.
void Engine::DoCollisions(Projectile &projectile)
{
	// The asteroids can collide with projectiles, the same as any other
	// object. If the asteroid turns out to be closer than the ship, it
	// shields the ship (unless the projectile has a blast radius).
	vector<Collision> collisions;
	const Government *gov = projectile.GetGovernment();
	const Weapon &weapon = projectile.GetWeapon();

	if(projectile.ShouldExplode())
		collisions.emplace_back(nullptr, CollisionType::NONE, 0.);
	else if(weapon.IsPhasing() && projectile.Target())
	{
		// "Phasing" projectiles that have a target will never hit any other ship.
		// They also don't care whether the weapon has "no ship collisions" on, as
		// otherwise a phasing projectile would never hit anything.
		shared_ptr<Ship> target = projectile.TargetPtr();
		if(target)
		{
			Point offset = projectile.Position() - target->Position();
			double range = target->GetMask(step).Collide(offset, projectile.Velocity(), target->Facing());
			if(range < 1.)
				collisions.emplace_back(target.get(), CollisionType::SHIP, range);
		}
	}
	else
	{
		// For weapons with a trigger radius, check if any detectable object will set it off.
		double triggerRadius = weapon.TriggerRadius();
		if(triggerRadius)
			for(const Body *body : shipCollisions.Circle(projectile.Position(), triggerRadius))
			{
				const Ship *ship = reinterpret_cast<const Ship *>(body);
				if(body == projectile.Target() || (gov->IsEnemy(body->GetGovernment())
						&& !ship->IsCloaked()))
				{
					collisions.emplace_back(nullptr, CollisionType::NONE, 0.);
					break;
				}
			}

		// If nothing triggered the projectile, check for collisions with ships and asteroids.
		if(collisions.empty())
		{
			if(weapon.CanCollideShips())
			{
				const vector<Collision> &newShipHits = shipCollisions.Line(projectile);
				collisions.insert(collisions.end(), newShipHits.begin(), newShipHits.end());
			}
			if(weapon.CanCollideAsteroids())
			{
				const vector<Collision> &newAsteroidHits = asteroids.CollideAsteroids(projectile);
				collisions.insert(collisions.end(), newAsteroidHits.begin(), newAsteroidHits.end());
			}
			if(weapon.CanCollideMinables())
			{
				const vector<Collision> &newMinableHits = asteroids.CollideMinables(projectile);
				collisions.insert(collisions.end(), newMinableHits.begin(), newMinableHits.end());
			}
		}
	}

	// Sort the Collisions by increasing range so that the closer collisions are evaluated first.
	sort(collisions.begin(), collisions.end());

	// Run all collisions until either the projectile dies or there are no more collisions left.
	for(Collision &collision : collisions)
	{
		Body *hit = collision.HitBody();
		CollisionType collisionType = collision.GetCollisionType();
		double range = collision.IntersectionRange();

		shared_ptr<Ship> shipHit;
		if(hit && collisionType == CollisionType::SHIP)
			shipHit = reinterpret_cast<Ship *>(hit)->shared_from_this();

		// Don't collide with carried ships that are disabled and not directly targeted.
		if(shipHit && hit != projectile.Target()
				&& !FighterHitHelper::IsValidTarget(shipHit.get()))
			continue;

		// If the ship is cloaked, and phasing, then skip this ship (during this step).
		if(shipHit && shipHit->Phases(projectile))
			continue;

		// Create the explosion the given distance along the projectile's
		// motion path for this step.
		projectile.Explode(visuals, range, hit ? hit->Velocity() : Point());

		const DamageProfile damage(projectile.GetInfo(range));

		// If this projectile has a blast radius, find all ships within its
		// radius. Otherwise, only one is damaged.
		// TODO: Also deal blast damage to minables?
		double blastRadius = weapon.BlastRadius();
		if(blastRadius)
		{
			// Even friendly ships can be hit by the blast, unless it is a
			// "safe" weapon.
			Point hitPos = projectile.Position() + range * projectile.Velocity();
			bool isSafe = weapon.IsSafe();
			for(Body *body : shipCollisions.Circle(hitPos, blastRadius))
			{
				Ship *ship = reinterpret_cast<Ship *>(body);
				bool targeted = (projectile.Target() == ship);
				// Phasing cloaked ship will have a chance to ignore the effects of the explosion.
				if((isSafe && !targeted && !gov->IsEnemy(ship->GetGovernment())) || ship->Phases(projectile))
					continue;

				// Only directly targeted ships get provoked by blast weapons.
				int eventType = ship->TakeDamage(visuals, damage.CalculateDamage(*ship, ship == hit),
					targeted ? gov : nullptr);
				if(eventType)
					eventQueue.emplace_back(gov, ship->shared_from_this(), eventType);
			}
		}
		else if(hit)
		{
			if(collisionType == CollisionType::SHIP)
			{
				int eventType = shipHit->TakeDamage(visuals, damage.CalculateDamage(*shipHit), gov);
				if(eventType)
					eventQueue.emplace_back(gov, shipHit, eventType);
			}
			else if(collisionType == CollisionType::MINABLE)
				reinterpret_cast<Minable *>(hit)->TakeDamage(projectile);
		}

		if(shipHit)
			DoGrudge(shipHit, gov);
		if(projectile.IsDead())
			break;
	}

	// If the projectile is still alive, give the anti-missile systems a chance to shoot it down.
	if(!projectile.IsDead() && projectile.MissileStrength())
	{
		for(Ship *ship : hasAntiMissile)
			if(ship == projectile.Target() || gov->IsEnemy(ship->GetGovernment()))
				if(ship->FireAntiMissile(projectile, visuals))
				{
					projectile.Kill();
					break;
				}
	}
}



// Determine whether any active weather events have impacted the ships within
// the system. As with DoCollisions, this function adds visuals directly to
// the main visuals list.
void Engine::DoWeather(Weather &weather)
{
	weather.CalculateStrength();
	if(weather.HasWeapon() && !Random::Int(weather.Period()))
	{
		const Hazard *hazard = weather.GetHazard();
		const DamageProfile damage(weather.GetInfo());

		// Get all ship bodies that are touching a ring defined by the hazard's min
		// and max ranges at the hazard's origin. Any ship touching this ring takes
		// hazard damage.
		for(Body *body : (hazard->SystemWide() ? shipCollisions.All()
			: shipCollisions.Ring(weather.Origin(), hazard->MinRange(), hazard->MaxRange())))
		{
			Ship *hit = reinterpret_cast<Ship *>(body);
			hit->TakeDamage(visuals, damage.CalculateDamage(*hit), nullptr);
		}
	}
}



// Check if any ship collected the given flotsam.
void Engine::DoCollection(Flotsam &flotsam)
{
	// Check if any ship can pick up this flotsam. Cloaked ships without "cloaked pickup" cannot act.
	Ship *collector = nullptr;
	for(Body *body : shipCollisions.Circle(flotsam.Position(), 5.))
	{
		Ship *ship = reinterpret_cast<Ship *>(body);
		if(!ship->CannotAct(Ship::ActionType::PICKUP) && ship->CanPickUp(flotsam))
		{
			collector = ship;
			break;
		}
	}
	// If the flotsam was not collected, give tractor beam systems a chance to
	// pull it.
	if(!collector)
	{
		// Keep track of the net effect of all the tractor beams pulling on
		// this flotsam.
		Point pullVector;
		// Also determine the average velocity of the ships pulling on this flotsam.
		Point avgShipVelocity;
		int count = 0;
		for(Ship *ship : hasTractorBeam)
		{
			Point shipPull = ship->FireTractorBeam(flotsam, visuals);
			if(shipPull)
			{
				pullVector += shipPull;
				avgShipVelocity += ship->Velocity();
				++count;
			}
		}

		if(pullVector)
		{
			// If any tractor beams successfully fired on this flotsam, also drag the flotsam with
			// the average velocity of each ship.
			// When dealing with individual ships, this makes tractor beams feel more capable of
			// dragging flotsam to the ship. Otherwise, a ship could be drifting away from a flotsam
			// at the same speed that the tractor beam is pulling the flotsam toward the ship,
			// which looks awkward and makes the tractor beam feel pointless; the whole point of
			// a tractor beam should be that it collects flotsam for you.
			// This does mean that if you fly toward a flotsam that is in your tractor beam then
			// you'll be pushing the flotsam away from your ship, but the pull of the tractor beam
			// will still slowly close the distance between the ship and the flotsam.
			// When dealing with multiple ships, this causes a better appearance of a struggle between
			// the ships all trying to get a hold of the flotsam should the ships all have similar velocities.
			// If the ships have differing velocities, then it can make it look like the quicker ship is
			// yanking the flotsam away from the slower ship.
			pullVector += avgShipVelocity / count;
			flotsam.SetVelocity(pullVector);
		}
		return;
	}

	// Checks for player FlotsamCollection setting
	if(collector->IsYours())
	{
		const auto flotsamSetting = Preferences::GetFlotsamCollection();
		if(flotsamSetting == Preferences::FlotsamCollection::OFF)
			return;
		if(collector == player.Flagship() && flotsamSetting == Preferences::FlotsamCollection::ESCORT)
			return;
		if(collector != player.Flagship() && flotsamSetting == Preferences::FlotsamCollection::FLAGSHIP)
			return;
	}

	// Transfer cargo from the flotsam to the collector ship.
	int amount = flotsam.TransferTo(collector);
	// If the collector is not one of the player's ships, we can bail out now.
	if(!collector->IsYours())
		return;

	if(collector->GetParent() && !Preferences::Has("Extra fleet status messages"))
		return;

	// One of your ships picked up this flotsam. Describe who it was.
	string name = (!collector->GetParent() ? "You" :
			"Your " + collector->Noun() + " \"" + collector->Name() + "\"") + " picked up ";
	// Describe what they collected from this flotsam.
	string commodity;
	string message;
	if(flotsam.OutfitType())
	{
		const Outfit *outfit = flotsam.OutfitType();
		if(outfit->Get("minable") > 0.)
		{
			commodity = outfit->DisplayName();
			player.Harvest(outfit);
		}
		else
			message = name + to_string(amount) + " "
				+ (amount == 1 ? outfit->DisplayName() : outfit->PluralName()) + ".";
	}
	else
		commodity = flotsam.CommodityType();

	// If an ordinary commodity or harvestable was collected, describe it in
	// terms of tons, not in terms of units.
	if(!commodity.empty())
	{
		double amountInTons = amount * flotsam.UnitSize();
		message = name + Format::CargoString(amountInTons, Format::LowerCase(commodity)) + ".";
	}

	// Unless something went wrong while forming the message, display it.
	if(!message.empty())
	{
		int free = collector->Cargo().Free();
		message += " (" + Format::CargoString(free, "free space") + " remaining.)";
		Messages::Add(message, Messages::Importance::High);
	}
}



// Scanning can't happen in the same loop as ship movement because it relies on
// all the ships already being in their final position for this step.
void Engine::DoScanning(const shared_ptr<Ship> &ship)
{
	int scan = ship->Scan(player);
	if(scan)
	{
		shared_ptr<Ship> target = ship->GetTargetShip();
		if(target && target->IsTargetable())
			eventQueue.emplace_back(ship, target, scan);
	}
}



// Fill in all the objects in the radar display.
void Engine::FillRadar()
{
	const Ship *flagship = player.Flagship();
	const System *playerSystem = player.GetSystem();

	// Add stellar objects.
	for(const StellarObject &object : playerSystem->Objects())
		if(object.HasSprite())
		{
			double r = max(2., object.Radius() * .03 + .5);
			radar[currentCalcBuffer].Add(object.RadarType(flagship), object.Position(), r, r - 1.);
		}

	// Add pointers for neighboring systems.
	if(flagship)
	{
		const System *targetSystem = flagship->GetTargetSystem();
		const set<const System *> &links = (flagship->JumpNavigation().HasJumpDrive()) ?
			playerSystem->JumpNeighbors(flagship->JumpNavigation().JumpRange()) : playerSystem->Links();
		for(const System *system : links)
			if(player.HasSeen(*system))
				radar[currentCalcBuffer].AddPointer(
					(system == targetSystem) ? Radar::SPECIAL : Radar::INACTIVE,
					system->Position() - playerSystem->Position());
	}

	// Add viewport brackets.
	if(!Preferences::Has("Disable viewport on radar"))
	{
		radar[currentCalcBuffer].AddViewportBoundary(Screen::TopLeft() / zoom);
		radar[currentCalcBuffer].AddViewportBoundary(Screen::TopRight() / zoom);
		radar[currentCalcBuffer].AddViewportBoundary(Screen::BottomLeft() / zoom);
		radar[currentCalcBuffer].AddViewportBoundary(Screen::BottomRight() / zoom);
	}

	// Add ships. Also check if hostile ships have newly appeared.
	bool hasHostiles = false;
	for(shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == playerSystem)
		{
			// Do not show cloaked ships on the radar, except the player's ships, and those who should show on radar.
			bool isYours = ship->IsYours();
			if(ship->IsCloaked() && !isYours)
				continue;

			// Figure out what radar color should be used for this ship.
			bool isYourTarget = (flagship && ship == flagship->GetTargetShip());
			int type = isYourTarget ? Radar::SPECIAL : RadarType(*ship, step);
			// Calculate how big the radar dot should be.
			double size = sqrt(ship->Width() + ship->Height()) * .14 + .5;

			radar[currentCalcBuffer].Add(type, ship->Position(), size);

			// Check if this is a hostile ship.
			hasHostiles |= (!ship->IsDisabled() && ship->GetGovernment()->IsEnemy()
				&& ship->GetTargetShip() && ship->GetTargetShip()->IsYours());
		}
	// If hostile ships have appeared, play the siren.
	if(alarmTime)
		--alarmTime;
	else if(hasHostiles && !hadHostiles)
	{
		if(Preferences::PlayAudioAlert())
			Audio::Play(Audio::Get("alarm"));
		alarmTime = 300;
		hadHostiles = true;
	}
	else if(!hasHostiles)
		hadHostiles = false;

	// Add projectiles that have a missile strength or homing.
	for(Projectile &projectile : projectiles)
	{
		if(projectile.MissileStrength())
		{
			bool isEnemy = projectile.GetGovernment() && projectile.GetGovernment()->IsEnemy();
			radar[currentCalcBuffer].Add(
				isEnemy ? Radar::SPECIAL : Radar::INACTIVE, projectile.Position(), 1.);
		}
		else if(projectile.GetWeapon().BlastRadius())
			radar[currentCalcBuffer].Add(Radar::SPECIAL, projectile.Position(), 1.8);
	}
}



// Each ship is drawn as an entire stack of sprites, including hardpoint sprites
// and engine flares and any fighters it is carrying externally.
void Engine::DrawShipSprites(const Ship &ship)
{
	bool hasFighters = ship.PositionFighters();
	double cloak = ship.Cloaking();
	bool drawCloaked = (cloak && ship.IsYours());
	bool fancyCloak = Preferences::Has("Cloaked ship outlines");
	auto &itemsToDraw = draw[currentCalcBuffer];
	auto drawObject = [&itemsToDraw, cloak, drawCloaked, fancyCloak](const Body &body) -> void
	{
		// Draw cloaked/cloaking sprites swizzled red or transparent (depending on whether we are using fancy
		// cloaking effects), and overlay this solid sprite with an increasingly transparent "regular" sprite.
		if(drawCloaked)
			itemsToDraw.AddSwizzled(body, fancyCloak ? 9 : 27, fancyCloak ? 0.5 : 0.25);
		itemsToDraw.Add(body, cloak);
	};

	if(hasFighters)
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.side == Ship::Bay::UNDER && bay.ship)
				drawObject(*bay.ship);

	if(ship.IsThrusting() && !ship.EnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.EnginePoints(),
			ship.Attributes().FlareSprites(), Ship::EnginePoint::UNDER);
	else if(ship.IsReversing() && !ship.ReverseEnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.ReverseEnginePoints(),
			ship.Attributes().ReverseFlareSprites(), Ship::EnginePoint::UNDER);
	if(ship.IsSteering() && !ship.SteeringEnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.SteeringEnginePoints(),
			ship.Attributes().SteeringFlareSprites(), Ship::EnginePoint::UNDER);

	auto drawHardpoint = [&drawObject, &ship](const Hardpoint &hardpoint) -> void
	{
		if(hardpoint.GetOutfit() && hardpoint.GetOutfit()->HardpointSprite().HasSprite())
		{
			Body body(
				hardpoint.GetOutfit()->HardpointSprite(),
				ship.Position() + ship.Zoom() * ship.Facing().Rotate(hardpoint.GetPoint()),
				ship.Velocity(),
				ship.Facing() + hardpoint.GetAngle(),
				ship.Zoom());
			drawObject(body);
		}
	};

	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.IsUnder())
			drawHardpoint(hardpoint);
	drawObject(ship);
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(!hardpoint.IsUnder())
			drawHardpoint(hardpoint);

	if(ship.IsThrusting() && !ship.EnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.EnginePoints(),
			ship.Attributes().FlareSprites(), Ship::EnginePoint::OVER);
	else if(ship.IsReversing() && !ship.ReverseEnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.ReverseEnginePoints(),
			ship.Attributes().ReverseFlareSprites(), Ship::EnginePoint::OVER);
	if(ship.IsSteering() && !ship.SteeringEnginePoints().empty())
		DrawFlareSprites(ship, draw[currentCalcBuffer], ship.SteeringEnginePoints(),
			ship.Attributes().SteeringFlareSprites(), Ship::EnginePoint::OVER);

	if(hasFighters)
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.side == Ship::Bay::OVER && bay.ship)
				drawObject(*bay.ship);
}



// If a ship just damaged another ship, update information on who has asked the
// player for assistance (and ask for assistance if appropriate).
void Engine::DoGrudge(const shared_ptr<Ship> &target, const Government *attacker)
{
	if(attacker->IsPlayer())
	{
		shared_ptr<const Ship> previous = grudge[target->GetGovernment()].lock();
		if(previous && previous->CanSendHail(player))
		{
			grudge[target->GetGovernment()].reset();
			SendMessage(previous, "Thank you for your assistance, Captain "
				+ player.LastName() + "!");
		}
		return;
	}
	if(grudgeTime)
		return;

	// Check who currently has a grudge against this government. Also check if
	// someone has already said "thank you" today.
	if(grudge.count(attacker))
	{
		shared_ptr<const Ship> previous = grudge[attacker].lock();
		// If the previous ship is destroyed, or was able to send a
		// "thank you" already, skip sending a new thanks.
		if(!previous || previous->CanSendHail(player))
			return;
	}

	// If an enemy of the player, or being attacked by those that are
	// not enemies of the player, do not request help.
	if(target->GetGovernment()->IsEnemy() || !attacker->IsEnemy())
		return;
	// Ensure that this attacked ship is able to send hails (e.g. not mute,
	// a player ship, automaton, shares a language with the player, etc.)
	if(!target || !target->CanSendHail(player))
		return;

	// No active ship has a grudge already against this government.
	// Check the relative strength of this ship and its attackers.
	double attackerStrength = 0.;
	int attackerCount = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetGovernment() == attacker && ship->GetTargetShip() == target)
		{
			++attackerCount;
			attackerStrength += (ship->Shields() + ship->Hull()) * ship->Strength();
		}

	// Only ask for help if outmatched.
	double targetStrength = (target->Shields() + target->Hull()) * target->Strength();
	if(attackerStrength <= targetStrength)
		return;

	// Ask for help more frequently if the battle is very lopsided.
	double ratio = attackerStrength / targetStrength - 1.;
	if(Random::Real() * 10. > ratio)
		return;

	grudge[attacker] = target;
	grudgeTime = 120;
	string message;
	if(target->GetPersonality().IsDaring())
	{
		message = "Please assist us in ";
		message += (target->GetPersonality().Disables() ? "disabling " : "destroying ");
		message += (attackerCount == 1 ? "this " : "these ");
		message += attacker->GetName();
		message += (attackerCount == 1 ? " ship." : " ships.");
	}
	else
	{
		message = "We are under attack by ";
		if(attackerCount == 1)
			message += "a ";
		message += attacker->GetName();
		message += (attackerCount == 1 ? " ship" : " ships");
		message += ". Please assist us!";
	}
	SendMessage(target, message);
}



void Engine::CreateStatusOverlays()
{
	const auto overlayAllSetting = Preferences::StatusOverlaysState(Preferences::OverlayType::ALL);

	if(overlayAllSetting == Preferences::OverlayState::OFF)
		return;

	const System *currentSystem = player.GetSystem();
	const auto flagship = player.FlagshipPtr();

	static const set<Preferences::OverlayType> overlayTypes = {
		Preferences::OverlayType::FLAGSHIP,
		Preferences::OverlayType::ESCORT,
		Preferences::OverlayType::ENEMY,
		Preferences::OverlayType::NEUTRAL
	};

	map<Preferences::OverlayType, Preferences::OverlayState> overlaySettings;

	for(const auto &it : overlayTypes)
		overlaySettings[it] = Preferences::StatusOverlaysState(it);

	for(const auto &it : ships)
	{
		if(!it->GetGovernment() || it->GetSystem() != currentSystem || (!it->IsYours() && it->Cloaking() == 1.))
			continue;
		// Don't show status for dead ships.
		if(it->IsDestroyed())
			continue;

		if(it == flagship)
			EmplaceStatusOverlay(it, overlaySettings[Preferences::OverlayType::FLAGSHIP], 0, it->Cloaking());
		else if(it->GetGovernment()->IsEnemy())
			EmplaceStatusOverlay(it, overlaySettings[Preferences::OverlayType::ENEMY], 2, it->Cloaking());
		else if(it->IsYours() || it->GetPersonality().IsEscort())
			EmplaceStatusOverlay(it, overlaySettings[Preferences::OverlayType::ESCORT], 1, it->Cloaking());
		else
			EmplaceStatusOverlay(it, overlaySettings[Preferences::OverlayType::NEUTRAL], 3, it->Cloaking());
	}
}



void Engine::EmplaceStatusOverlay(const shared_ptr<Ship> &it, Preferences::OverlayState overlaySetting,
	int type, double cloak)
{
	if(overlaySetting == Preferences::OverlayState::OFF)
		return;

	if(overlaySetting == Preferences::OverlayState::DAMAGED && !it->IsDamaged())
		return;

	double width = min(it->Width(), it->Height());
	float alpha = 1.f;
	if(overlaySetting == Preferences::OverlayState::ON_HIT)
	{
		// The number of frames left where we start fading the overlay.
		static constexpr int FADE_STEPS = 10;

		const int t = it->DamageOverlayTimer();
		if(t >= FADE_STEPS)
			alpha = 1.f;
		else if(t > 0)
			alpha = static_cast<float>(t) / FADE_STEPS;
		else
			alpha = 0.f;
	}

	if(it->IsYours())
		cloak *= 0.6;

	statuses.emplace_back(it->Position() - center, it->Shields(), it->Hull(),
		min(it->Hull(), it->DisabledHull()), max(20., width * .5), type, alpha * (1. - cloak));
}
