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
#include "audio/Audio.h"
#include "CategoryList.h"
#include "CategoryType.h"
#include "Collision.h"
#include "CollisionType.h"
#include "CoreStartData.h"
#include "DamageDealt.h"
#include "DamageProfile.h"
#include "Effect.h"
#include "FighterHitHelper.h"
#include "shader/FillShader.h"
#include "Fleet.h"
#include "Flotsam.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Government.h"
#include "Hazard.h"
#include "Interface.h"
#include "Logger.h"
#include "MapPanel.h"
#include "image/Mask.h"
#include "Messages.h"
#include "Minable.h"
#include "MinableDamageDealt.h"
#include "Mission.h"
#include "NPC.h"
#include "shader/OutlineShader.h"
#include "Person.h"
#include "Planet.h"
#include "PlanetLabel.h"
#include "PlayerInfo.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "Projectile.h"
#include "Random.h"
#include "shader/RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "ship/ShipAICache.h"
#include "ShipEvent.h"
#include "ShipJumpNavigation.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "shader/StarField.h"
#include "StellarObject.h"
#include "System.h"
#include "SystemEntry.h"
#include "test/Test.h"
#include "UI.h"
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

	constexpr auto PrunePointers = [](auto &objects) { erase_if(objects,
			[](const auto &obj) { return obj->ShouldBeRemoved(); }); };
	constexpr auto Prune = [](auto &objects) { erase_if(objects,
			[](const auto &obj) { return obj.ShouldBeRemoved(); }); };

	template<class Type>
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
		const string &gov = ship->GetGovernment()->DisplayName();
		if(!ship->GivenName().empty())
			tag = gov + " " + ship->Noun() + " \"" + ship->GivenName() + "\": ";
		else
			tag = ship->DisplayModelName() + " (" + gov + "): ";

		Messages::Add({tag + message, GameData::MessageCategories().Get("normal")});
	}

	Point FlareCurve(double x)
	{
		double x2 = x * x;
		double x3 = x2 * x;
		return Point(x2, x3);
	}

	Point ScaledFlareCurve(const Ship &ship, Ship::ThrustKind kind)
	{
		// When a ship lands, its thrusters should scale with it.
		return FlareCurve(ship.ThrustHeldFraction(kind)) * ship.Zoom();
	}

	void DrawFlareSprites(const Ship &ship, DrawList &draw, const vector<Ship::EnginePoint> &enginePoints,
		const vector<pair<Body, int>> &flareSprites, uint8_t side, bool reverse)
	{
		Point thrustScale = ScaledFlareCurve(ship, reverse ? Ship::ThrustKind::REVERSE : Ship::ThrustKind::FORWARD);
		Point leftTurnScale = ScaledFlareCurve(ship, Ship::ThrustKind::LEFT);
		Point rightTurnScale = ScaledFlareCurve(ship, Ship::ThrustKind::RIGHT);

		double gimbalDirection = (ship.Commands().Has(Command::FORWARD) || ship.Commands().Has(Command::BACK))
			* -ship.Commands().Turn();

		for(const Ship::EnginePoint &point : enginePoints)
		{
			Angle gimbal = Angle(gimbalDirection * point.gimbal.Degrees());
			Angle flareAngle = ship.Facing() + point.facing + gimbal;
			Point pos = ship.Facing().Rotate(point) * ship.Zoom() + ship.Position();
			auto DrawFlares = [&draw, &pos, &ship, &flareAngle, &point](const pair<Body, int> &it, const Point &scale)
			{
				// If multiple engines with the same flare are installed, draw up to
				// three copies of the flare sprite.
				for(int i = 0; i < it.second && i < 3; ++i)
				{
					Body sprite(it.first, pos, ship.Velocity(), flareAngle, point.zoom, scale);
					draw.Add(sprite, ship.Cloaking());
				}
			};
			for(const auto &it : flareSprites)
				if(point.side == side)
				{
					if(point.steering == Ship::EnginePoint::NONE)
						DrawFlares(it, thrustScale);
					else if(point.steering == Ship::EnginePoint::LEFT && leftTurnScale)
						DrawFlares(it, leftTurnScale);
					else if(point.steering == Ship::EnginePoint::RIGHT && rightTurnScale)
						DrawFlares(it, rightTurnScale);
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
	const double MAX_FUEL_DISPLAY = 3000.;
}



Engine::Engine(PlayerInfo &player)
	: player(player), ai(player, ships, asteroids.Minables(), flotsam),
	ammoDisplay(player), minimap(player), shipCollisions(256u, 32u, CollisionType::SHIP)
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
		camera.SnapTo(object->Position());

	// Now we know the player's current position. Draw the planets.
	draw[currentCalcBuffer].Clear(step, zoom);
	draw[currentCalcBuffer].SetCenter(camera.Center());
	radar[currentCalcBuffer].SetCenter(camera.Center());
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
	GameData::SetBackgroundPosition(camera.Center());
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
			Logger::Log("Engine::Place: Set fallback system for the NPC \"" + ship->GivenName()
				+ "\" as it had no system.", Logger::Level::WARNING);
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

	camera.SnapTo(flagship->Center());

	player.SetPlanet(nullptr);
}



// Add NPC ships to the known ships. These may have been freshly instantiated
// from an accepted assisting/boarding mission, or from existing missions when
// the player departs a planet.
void Engine::Place(const list<NPC> &npcs, const shared_ptr<Ship> &flagship)
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
		camera.SnapTo(object->Position());
	else if(flagship)
	{
		if(isActive && !timePaused)
			camera.MoveTo(flagship->Center(), hyperspacePercentage);

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

		minimap.Step(flagship);
	}
	else
		// If there is no flagship, stop the camera.
		camera.SnapTo(camera.Center());
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
	Audio::Update(camera.Center());

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

		// Step the background to account for the current velocity and zoom.
		GameData::StepBackground(timePaused ? Point() : camera.Velocity(), zoom);
	}

	outlines.clear();
	const Color &cloakColor = *GameData::Colors().Get("cloak highlight");
	if(Preferences::Has("Cloaked ship outlines"))
		for(const auto &ship : player.Ships())
		{
			if(ship->IsParked() || ship->GetSystem() != player.GetSystem() || ship->Cloaking() == 0.)
				continue;

			outlines.emplace_back(ship->GetSprite(), (ship->Position() - camera.Center()) * zoom, ship->Unit() * zoom,
				ship->GetFrame(), Color::Multiply(ship->Cloaking(), cloakColor));
		}

	// Add the flagship outline last to distinguish the flagship from other ships.
	if(flagship && !flagship->IsDestroyed() && Preferences::Has("Highlight player's flagship"))
	{
		outlines.emplace_back(flagship->GetSprite(),
			(flagship->Center() - camera.Center()) * zoom,
			flagship->Unit() * zoom * flagship->Scale(),
			flagship->GetFrame(),
			*GameData::Colors().Get("flagship highlight"));
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
				escorts.Add(it, system == currentSystem, player.KnowsName(*system), fleetIsJumping, isSelected);
			}
	for(const shared_ptr<Ship> &escort : player.Ships())
		if(!escort->IsParked() && escort != flagship && !escort->IsDestroyed())
		{
			// Check if this escort is selected.
			bool isSelected = false;
			for(const weak_ptr<Ship> &ptr : player.SelectedEscorts())
				if(ptr.lock() == escort)
				{
					isSelected = true;
					break;
				}
			const System *system = escort->GetSystem();
			escorts.Add(escort, system == currentSystem, system && player.KnowsName(*system), fleetIsJumping, isSelected);
		}

	statuses.clear();
	missileLabels.clear();
	turretOverlays.clear();
	if(isActive)
	{
		// Create the status overlays.
		CreateStatusOverlays();
		// Create missile overlays.
		if(Preferences::Has("Show missile overlays"))
			for(const Projectile &projectile : projectiles)
			{
				Point pos = projectile.Position() - camera.Center();
				if(projectile.MissileStrength() && projectile.GetGovernment()->IsEnemy()
						&& (pos.Length() < max(Screen::Width(), Screen::Height()) * .5 / zoom))
					missileLabels.emplace_back(AlertLabel(pos, projectile, flagship, zoom));
			}
		// Create overlays for flagship turrets with blindspots.
		if(flagship && Preferences::GetTurretOverlays() != Preferences::TurretOverlays::OFF)
			for(const Hardpoint &hardpoint : flagship->Weapons())
				if(!hardpoint.GetBaseAttributes().blindspots.empty())
				{
					bool isBlind = hardpoint.IsBlind();
					if(Preferences::GetTurretOverlays() == Preferences::TurretOverlays::BLINDSPOTS_ONLY && !isBlind)
						continue;
					// TODO: once Apple Clang adds support for C++20 aggregate initialization,
					// this can be removed.
#ifdef __APPLE__
					turretOverlays.push_back({
#else
					turretOverlays.emplace_back(
#endif
						(flagship->Position() - camera.Center()
							+ flagship->Zoom() * flagship->Facing().Rotate(hardpoint.GetPoint()))
							* static_cast<double>(zoom),
						(flagship->Facing() + hardpoint.GetAngle()).Unit(),
						flagship->Zoom() * static_cast<double>(zoom),
						isBlind
#ifdef __APPLE__
					}
#endif
					);
				}
		// Update the planet label positions.
		for(PlanetLabel &label : labels)
			label.Update(camera.Center(), zoom, labels, *player.GetSystem());
	}

	if(flagship && flagship->IsOverheated())
		Messages::Add(*GameData::Messages().Get("overheated"));

	// Clear the HUD information from the previous frame.
	info = Information();
	if(flagship && flagship->Hull())
	{
		Point shipFacingUnit(0., -1.);
		if(Preferences::Has("Rotate flagship in HUD"))
			shipFacingUnit = flagship->Facing().Unit();

		info.SetSprite("player sprite", flagship->GetSprite(), shipFacingUnit, flagship->GetFrame(step),
			flagship->GetSwizzle());
	}
	if(currentSystem)
		info.SetString("location", currentSystem->DisplayName());
	info.SetString("date", player.GetDate().ToString());
	if(flagship)
	{
		// Have an alarm label flash up when enemy ships are in the system
		if(alarmTime && uiStep / 20 % 2 && Preferences::DisplayVisualAlert())
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
		if(flagship->IsOverheated() && (uiStep / 20) % 2)
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
		const string &name = object->DisplayName();
		info.SetString("destination", name);

		targets.push_back({
			object->Position() - camera.Center(),
			object->Facing(),
			object->Radius(),
			GetPlanetTargetPointerColor(*object->GetPlanet()),
			5});
	}
	else if(flagship && flagship->GetTargetSystem())
	{
		info.SetString("navigation mode", "Hyperspace:");
		if(player.CanView(*flagship->GetTargetSystem()))
			info.SetString("destination", flagship->GetTargetSystem()->DisplayName());
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
		targetSwizzle = nullptr;
	if(!target && !targetAsteroid)
		info.SetString("target name", "no target");
	else if(!target)
	{
		info.SetSprite("target sprite",
			targetAsteroid->GetSprite(),
			targetAsteroid->Facing().Unit(),
			targetAsteroid->GetFrame(step),
			0);
		info.SetString("target name", targetAsteroid->DisplayName() + " " + targetAsteroid->Noun());

		targetVector = targetAsteroid->Position() - camera.Center();

		if(flagship->Attributes().Get("tactical scan power") || flagship->Attributes().Get("strategic scan power"))
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
		targetSwizzle = target->GetSwizzle();
		info.SetSprite("target sprite", target->GetSprite(), targetUnit, target->GetFrame(step), targetSwizzle);
		info.SetString("target name", target->GivenName());
		info.SetString("target type", target->DisplayModelName());
		if(!target->GetGovernment())
			info.SetString("target government", "No Government");
		else
			info.SetString("target government", target->GetGovernment()->DisplayName());
		info.SetString("mission target", target->GetPersonality().IsTarget() ? "(mission target)" : "");

		// Only update the "active" state shown for the target if it is
		// in the current system and targetable, or owned by the player.
		int targetType = RadarType(*target, uiStep);
		const bool blinking = targetType == Radar::BLINK;
		if(!blinking && ((target->GetSystem() == player.GetSystem() && target->IsTargetable()) || target->IsYours()))
			lastTargetType = targetType;
		if(blinking)
			info.SetOutlineColor(GetTargetOutlineColor(Radar::BLINK));
		else
			info.SetOutlineColor(GetTargetOutlineColor(lastTargetType));

		if(target->GetSystem() == player.GetSystem() && target->IsTargetable())
		{
			info.SetBar("target shields", target->Shields());
			info.SetBar("target hull", target->Hull(), 20.);
			info.SetBar("target disabled hull", min(target->Hull(), target->DisabledHull()), 20.);

			// The target area will be a square, with sides proportional to the average
			// of the width and the height of the sprite.
			double size = (target->Width() + target->Height()) * .35;
			targets.push_back({
				target->Position() - camera.Center(),
				Angle(45.) + target->Facing(),
				size,
				GetShipTargetPointerColor(targetType),
				4});

			targetVector = target->Position() - camera.Center();

			double targetRange = target->Position().Distance(flagship->Position());
			// Finds the range of the scan collections.
			double tacticalRange = 100. * sqrt(flagship->Attributes().Get("tactical scan power"));
			double strategicRange = 100. * sqrt(flagship->Attributes().Get("strategic scan power"));
			// Finds the range of the individual information types.
			double crewScanRange = tacticalRange + 100. * sqrt(flagship->Attributes().Get("crew scan power"));
			double fuelScanRange = tacticalRange + 100. * sqrt(flagship->Attributes().Get("fuel scan power"));
			double energyScanRange = tacticalRange + 100. * sqrt(flagship->Attributes().Get("energy scan power"));
			double thermalScanRange = tacticalRange + 100. * sqrt(flagship->Attributes().Get("thermal scan power"));
			double maneuverScanRange = strategicRange + 100. * sqrt(flagship->Attributes().Get("maneuver scan power"));
			double accelerationScanRange = strategicRange + 100. * sqrt(flagship->Attributes().Get("acceleration scan power"));
			double velocityScanRange = strategicRange + 100. * sqrt(flagship->Attributes().Get("velocity scan power"));
			double weaponScanRange = strategicRange + 100. * sqrt(flagship->Attributes().Get("weapon scan power"));
			bool rangeFinder = flagship->Attributes().Get("range finder power") > 0.;

			// Range information. If the player has any range finding,
			// then calculate the range and store it. If they do not
			// have strategic or weapon range info, use normal display.
			// If they do, then use strategic range display.
			if(tacticalRange || strategicRange || rangeFinder)
			{
				info.SetString("target range", to_string(static_cast<int>(round(targetRange))));
				if(strategicRange)
					info.SetCondition("strategic range display");
				else
					info.SetCondition("range display");
			}
			// Actual information requires a scrutable target
			// that is within the relevant scanner range, unless the target
			// is player owned, in which case information is available regardless
			// of range and scrutability.
			bool scrutable = !target->Attributes().Get("inscrutable");
			if((targetRange <= crewScanRange && scrutable) || (crewScanRange && target->IsYours()))
			{
				info.SetString("target crew", to_string(target->Crew()));
				if(accelerationScanRange || velocityScanRange)
					info.SetCondition("mobility crew display");
				else
					info.SetCondition("target crew display");
			}
			if((targetRange <= energyScanRange && scrutable) || (energyScanRange && target->IsYours()))
			{
				info.SetCondition("target energy display");
				int energy = round(target->Energy() * target->Attributes().Get("energy capacity"));
				info.SetString("target energy", to_string(energy));
			}
			if((targetRange <= fuelScanRange && scrutable) || (fuelScanRange && target->IsYours()))
			{
				info.SetCondition("target fuel display");
				int fuel = round(target->Fuel() * target->Attributes().Get("fuel capacity"));
				info.SetString("target fuel", to_string(fuel));
			}
			if((targetRange <= thermalScanRange && scrutable) || (thermalScanRange && target->IsYours()))
			{
				info.SetCondition("target thermal display");
				int heat = round(100. * target->Heat());
				info.SetString("target heat", to_string(heat) + "%");
			}
			if((targetRange <= weaponScanRange && scrutable) || (weaponScanRange && target->IsYours()))
			{
				info.SetCondition("target weapon range display");
				int turretRange = round(target->GetAICache().TurretRange());
				info.SetString("target turret", to_string(turretRange) + " ");
				int gunRange = round(target->GetAICache().GunRange());
				info.SetString("target gun", to_string(gunRange) + " ");
			}
			const bool mobilityScan = maneuverScanRange || velocityScanRange || accelerationScanRange;
			if((targetRange <= crewScanRange && targetRange <= maneuverScanRange && scrutable)
				|| (targetRange <= accelerationScanRange && scrutable)
				|| (mobilityScan && crewScanRange && target->IsYours()))
			{
				info.SetCondition("turn while combined");
				int turnRate = round(60 * target->TrueTurnRate());
				info.SetString("target turnrate", to_string(turnRate) + " ");
			}
			else if((targetRange >= crewScanRange && targetRange <= maneuverScanRange && scrutable)
				|| (maneuverScanRange && target->IsYours() && !tacticalRange && !crewScanRange))
			{
				info.SetCondition("turn while not combined");
				int turnRate = round(60 * target->TrueTurnRate());
				info.SetString("target turnrate", to_string(turnRate) + " ");
			}
			if((targetRange <= accelerationScanRange && scrutable) || (accelerationScanRange && target->IsYours()))
			{
				info.SetCondition("target velocity display");
				int presentSpeed = round(60 * target->CurrentSpeed());
				info.SetString("target velocity", to_string(presentSpeed) + " ");
			}
			if((targetRange <= velocityScanRange && scrutable) || (velocityScanRange && target->IsYours()))
			{
				info.SetCondition("target acceleration display");
				int presentAcceleration = 3600 * target->TrueAcceleration();
				info.SetString("target acceleration", to_string(presentAcceleration) + " ");
			}
		}
	}
	if(!Preferences::Has("Ship outlines in HUD"))
		info.SetCondition("fast hud sprites");
	if(target && target->IsTargetable() && target->GetSystem() == currentSystem
		&& (flagship->CargoScanFraction() || flagship->OutfitScanFraction()))
	{
		double width = max(target->Width(), target->Height());
		Point pos = target->Position() - camera.Center();
		const bool outfitInRange = pos.LengthSquared() <= (flagship->Attributes().Get("outfit scan power") * 10000);
		const Status::Type outfitOverlayType = outfitInRange ? Status::Type::SCAN : Status::Type::SCAN_OUT_OF_RANGE;
		statuses.emplace_back(pos, flagship->OutfitScanFraction(), 0.,
			0., 10. + max(20., width * .5), outfitOverlayType, 1.f, Angle(pos).Degrees() + 180.);
		const bool cargoInRange = pos.LengthSquared() <= (flagship->Attributes().Get("cargo scan power") * 10000);
		const Status::Type cargoOverlayType = cargoInRange ? Status::Type::SCAN : Status::Type::SCAN_OUT_OF_RANGE;
		statuses.emplace_back(pos, 0., flagship->CargoScanFraction(),
			0., 10. + max(20., width * .5), cargoOverlayType, 1.f, Angle(pos).Degrees() + 180.);
	}
	// Handle any events that change the selected ships.
	if(groupSelect >= 0)
	{
		// This has to be done in Step() to avoid race conditions.
		if(hasControl)
			player.SetEscortGroup(groupSelect);
		else
			player.SelectEscortGroup(groupSelect, hasShift);
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

	if(doClick && mouseButton == MouseButton::LEFT)
	{
		if(uiClickBox.Dimensions())
			doClick = !ammoDisplay.Click(uiClickBox);
		else
			doClick = !ammoDisplay.Click(clickPoint, hasControl);
		doClick = doClick && !player.SelectEscorts(clickBox, hasShift);
		if(doClick)
		{
			const vector<weak_ptr<Ship>> &stack = escorts.Click(clickPoint);
			if(!stack.empty())
			{
				player.SelectShips(stack, hasShift);
				doClick = false;
			}
			else
				clickPoint /= isRadarClick ? RADAR_SCALE : zoom;
		}
	}

	// Draw crosshairs on all the selected ships.
	for(const weak_ptr<Ship> &selected : player.SelectedEscorts())
	{
		shared_ptr<Ship> ship = selected.lock();
		if(ship && ship != target && !ship->IsParked() && ship->GetSystem() == player.GetSystem()
				&& !ship->IsDestroyed() && ship->Zoom() > 0.)
		{
			double size = (ship->Width() + ship->Height()) * .35;
			targets.push_back({
				ship->Position() - camera.Center(),
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
				Point offset = minable->Position() - camera.Center();
				// Use the squared length, as we used the squared scan range.
				bool inRange = offset.LengthSquared() <= scanRangeMetric;

				// Autocatalog asteroid: Record that the player knows this type of asteroid is available here.
				if(shouldCatalogAsteroids && !asteroidsScanned.contains(minable->DisplayName()))
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
			targetAsteroidPtr->Position() - camera.Center(),
			targetAsteroidPtr->Facing(),
			.8 * targetAsteroidPtr->Radius(),
			GetMinablePointerColor(true),
			3
		});
}



// Begin the next step of calculations.
void Engine::Go()
{
	if(!timePaused)
		++step;
	currentCalcBuffer = currentCalcBuffer ? 0 : 1;
	queue.Run([this] { CalculateStep(); });
}



// Whether the flow of time is paused.
bool Engine::IsPaused() const
{
	return timePaused;
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
	++uiStep;

	Point motionBlur = camera.Velocity();
	double baseBlur = Preferences::Has("Render motion blur") ? 1. : 0.;

	Preferences::ExtendedJumpEffects jumpEffectState = Preferences::GetExtendedJumpEffects();
	if(jumpEffectState != Preferences::ExtendedJumpEffects::OFF)
		motionBlur *= baseBlur + pow(hyperspacePercentage *
			(jumpEffectState == Preferences::ExtendedJumpEffects::MEDIUM ? 2.5 : 5.), 2);
	else
		motionBlur *= baseBlur;

	GameData::Background().Draw(motionBlur,
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
		static const Color color[16] = {
			*colors.Get("overlay flagship shields"),
			*colors.Get("overlay friendly shields"),
			*colors.Get("overlay hostile shields"),
			*colors.Get("overlay neutral shields"),
			*colors.Get("overlay outfit scan"),
			*colors.Get("overlay outfit scan out of range"),
			*colors.Get("overlay flagship hull"),
			*colors.Get("overlay friendly hull"),
			*colors.Get("overlay hostile hull"),
			*colors.Get("overlay neutral hull"),
			*colors.Get("overlay cargo scan"),
			*colors.Get("overlay cargo scan out of range"),
			*colors.Get("overlay flagship disabled"),
			*colors.Get("overlay friendly disabled"),
			*colors.Get("overlay hostile disabled"),
			*colors.Get("overlay neutral disabled")
		};
		Point pos = it.position * zoom;
		double radius = it.radius * zoom;
		int colorIndex = static_cast<int>(it.type);
		if(it.outer > 0.)
			RingShader::Draw(pos, radius + 3., 1.5f, it.outer,
				Color::Multiply(it.alpha, color[colorIndex]), 0.f, it.angle);
		double dashes = (it.type >= Status::Type::SCAN) ? 0. : 20. * min<double>(1., zoom);
		colorIndex += static_cast<int>(Status::Type::COUNT);
		if(it.inner > 0.)
			RingShader::Draw(pos, radius, 1.5f, it.inner,
				Color::Multiply(it.alpha, color[colorIndex]), dashes, it.angle);
		colorIndex += static_cast<int>(Status::Type::COUNT);
		if(it.disabled > 0.)
			RingShader::Draw(pos, radius, 1.5f, it.disabled,
				Color::Multiply(it.alpha, color[colorIndex]), dashes, it.angle);
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

	// Draw turret overlays.
	if(!turretOverlays.empty())
	{
		const Color &blindspot = *GameData::Colors().Get("overlay turret blindspot");
		const Color &normal = *GameData::Colors().Get("overlay turret");
		PointerShader::Bind();
		for(const TurretOverlay &it : turretOverlays)
			PointerShader::Add(it.position, it.angle, 8 * it.scale, 24 * it.scale, 24 * it.scale,
				it.isBlind ? blindspot : normal);
		PointerShader::Unbind();
	}

	if(flash)
		FillShader::Fill(Point(), Screen::Dimensions(), Color(flash, flash));

	// Draw messages. Draw the most recent messages first, as some messages
	// may be wrapped onto multiple lines.
	const Font &font = FontSet::Get(14);
	Rectangle messageBox = hud->GetBox("messages");
	bool messagesReversed = hud->GetValue("messages reversed");
	double animationDuration = hud->GetValue("message animation duration");
	const vector<Messages::Entry> &messages = Messages::Get(uiStep, animationDuration);
	auto messageAnimation = [animationDuration](double age) -> double
	{
		return max(0., 1. - pow((age - animationDuration) / animationDuration, 2));
	};
	auto naturalDecay = [animationDuration](int age) -> float
	{
		return (1000 + animationDuration - age) * .001f;
	};
	WrappedText messageLine(font);
	messageLine.SetWrapWidth(messageBox.Width());
	messageLine.SetParagraphBreak(0.);
	Point messagePoint{messageBox.Left(), messagesReversed ? messageBox.Top() : messageBox.Bottom()};
	for(auto it = messages.rbegin(); it != messages.rend(); ++it)
	{
		messageLine.Wrap(it->message);
		int height = messageLine.Height();
		if(messagesReversed && it == messages.rbegin())
			messagePoint.Y() -= height;
		// Dying messages are those scheduled for removal as duplicates.
		bool isDying = it->deathStep >= 0;
		int naturalAge = uiStep - it->step;
		// New messages should fade in, while dying ones should fade out.
		int age = isDying ? it->deathStep - uiStep : naturalAge;
		bool isAnimating = age < animationDuration;
		if(isAnimating)
			height *= messageAnimation(age);
		if(messagesReversed)
		{
			messagePoint.Y() += height;
			if(messagePoint.Y() > messageBox.Bottom())
				break;
		}
		else
		{
			messagePoint.Y() -= height;
			if(messagePoint.Y() < messageBox.Top())
				break;
		}
		float alpha = isAnimating ? isDying ? min<double>(messageAnimation(age), naturalDecay(naturalAge))
			: messageAnimation(age) : naturalDecay(age);
		messageLine.Draw(messagePoint, it->category->MainColor().Additive(alpha));
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
	if(targetSwizzle && hud->HasPoint("faction markers"))
	{
		int width = font.Width(info.GetString("target government"));
		Point center = hud->GetPoint("faction markers");

		const Sprite *mark[2] = {SpriteSet::Get("ui/faction left"), SpriteSet::Get("ui/faction right")};
		// Round the x offsets to whole numbers so the icons are sharp.
		double dx[2] = {(width + mark[0]->Width() + 1) / -2, (width + mark[1]->Width() + 1) / 2};
		for(int i = 0; i < 2; ++i)
			SpriteShader::Draw(mark[i], center + Point(dx[i], 0.), 1., targetSwizzle);
	}

	// Draw the systems mini-map.
	minimap.Draw(uiStep);

	// Draw ammo status.
	double ammoIconWidth = hud->GetValue("ammo icon width");
	double ammoIconHeight = hud->GetValue("ammo icon height");
	ammoDisplay.Draw(hud->GetBox("ammo"), Point(ammoIconWidth, ammoIconHeight));

	// Draw escort status.
	escorts.Draw(hud->GetBox("escorts"));
}



// Select the object the player clicked on.
void Engine::Click(const Point &from, const Point &to, bool hasShift, bool hasControl)
{
	// First, see if this is a click on an escort icon.
	doClickNextStep = true;
	this->hasShift = hasShift;
	this->hasControl = hasControl;
	mouseButton = MouseButton::LEFT;

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
			(from - radarCenter) / RADAR_SCALE + camera.Center(),
			(to - radarCenter) / RADAR_SCALE + camera.Center());
	else
		clickBox = Rectangle::WithCorners(from / zoom + camera.Center(), to / zoom + camera.Center());
}



void Engine::RightOrMiddleClick(const Point &point, MouseButton button)
{
	doClickNextStep = true;
	hasShift = false;
	mouseButton = button;

	// Determine if the right/middle-click was within the radar display, and if so, rescale.
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
	player.AdvanceDate();
	const Date &today = player.GetDate();

	const System *system = flagship->GetSystem();
	Audio::PlayMusic(system->MusicName());
	GameData::SetHaze(system->Haze(), false);

	Messages::Add({"Entering the " + system->DisplayName() + " system on "
		+ today.ToString() + (system->IsInhabited(flagship) ?
		"." : ". No inhabited planets detected."),
		GameData::MessageCategories().Get("daily")});

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
	double fleetMultiplier = GameData::GetGamerules().FleetMultiplier();
	for(int i = 0; i < 5; ++i)
	{
		for(const auto &fleet : system->Fleets())
			if(fleetMultiplier ? fleet.Get()->GetGovernment() && Random::Int(fleet.Period() / fleetMultiplier) < 60
				&& fleet.CanTrigger() : false)
				fleet.Get()->Place(*system, newShips);

		auto CreateWeather = [this](const RandomEvent<Hazard> &hazard, Point origin)
		{
			if(hazard.Get()->IsValid() && Random::Int(hazard.Period()) < 60 && hazard.CanTrigger())
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
					Messages::Add({"Your fleet has attracted the interest of a "
						+ raidFleet.GetFleet()->GetGovernment()->DisplayName() + " raiding party.",
						GameData::MessageCategories().Get("high")});
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

	emptySoundsTimer.clear();

	camera.SnapTo(flagship->Center(), true);

	// If the player entered a system by wormhole or jump drive, center the background
	// on the player's position. This is not done when entering a system by hyperdrive
	// so that there is a seamless transition between systems, and entry by taking off
	// from a planet is also excluded as to prevent the background from snapping to a
	// different position relative to what it was when the player landed.
	SystemEntry entry = player.GetSystemEntry();
	if(entry == SystemEntry::WORMHOLE || entry == SystemEntry::JUMP)
		GameData::SetBackgroundPosition(camera.Center());

	// Help message for new players. Show this message for the first four days,
	// since the new player ships can make at most four jumps before landing.
	if(today <= player.StartData().GetDate() + 4)
	{
		Messages::Add(*GameData::Messages().Get("basics 1"));
		Messages::Add(*GameData::Messages().Get("basics 2"));
		Messages::Add(*GameData::Messages().Get("basics 3"));
	}
}



void Engine::CalculateStep()
{
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

	const Ship *flagship = player.Flagship();
	const System *playerSystem = player.GetSystem();

	if(timePaused)
	{
		// Only process player commands and handle mouse clicks.
		ai.MovePlayer(*player.Flagship(), activeCommands);
		activeCommands.Clear();
		HandleMouseClicks();
	}
	else
		CalculateUnpaused(flagship, playerSystem);

	// Draw the objects. Start by figuring out where the view should be centered:
	Camera newCamera = camera;
	if(flagship && !timePaused)
	{
		if(flagship->IsHyperspacing())
			hyperspacePercentage = flagship->GetHyperspacePercentage() / 100.;
		else
			hyperspacePercentage = 0.;
		newCamera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
	draw[currentCalcBuffer].SetCenter(newCamera.Center(), newCamera.Velocity());
	batchDraw[currentCalcBuffer].SetCenter(newCamera.Center());
	radar[currentCalcBuffer].SetCenter(newCamera.Center());

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
	asteroids.Draw(draw[currentCalcBuffer], newCamera.Center(), zoom);
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
				if(timePaused)
					continue;
				if(ship->IsThrusting() && !ship->EnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().FlareSounds())
						Audio::Play(it.first, ship->Position(), SoundCategory::ENGINE);
				}
				else if(ship->IsReversing() && !ship->ReverseEnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().ReverseFlareSounds())
						Audio::Play(it.first, ship->Position(), SoundCategory::ENGINE);
				}
				if(ship->IsSteering() && !ship->SteeringEnginePoints().empty())
				{
					for(const auto &it : ship->Attributes().SteeringFlareSounds())
						Audio::Play(it.first, ship->Position(), SoundCategory::ENGINE);
				}
			}
			else
				showFlagship = true;
		}

	if(flagship && showFlagship)
		DrawShipSprites(*flagship);
	if(!timePaused && flagship && showFlagship)
	{
		if(flagship->IsThrusting() && !flagship->EnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().FlareSounds())
				Audio::Play(it.first, SoundCategory::ENGINE);
		}
		else if(flagship->IsReversing() && !flagship->ReverseEnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().ReverseFlareSounds())
				Audio::Play(it.first, SoundCategory::ENGINE);
		}
		if(flagship->IsSteering() && !flagship->SteeringEnginePoints().empty())
		{
			for(const auto &it : flagship->Attributes().SteeringFlareSounds())
				Audio::Play(it.first, SoundCategory::ENGINE);
		}
	}
	// Draw the projectiles.
	for(const Projectile &projectile : projectiles)
		batchDraw[currentCalcBuffer].Add(projectile, projectile.Clip());
	// Draw the visuals.
	for(const Visual &visual : visuals)
		batchDraw[currentCalcBuffer].AddVisual(visual);
}



// Calculate things that require the engine not to be paused.
void Engine::CalculateUnpaused(const Ship *flagship, const System *playerSystem)
{
	// Now, all the ships must decide what they are doing next.
	ai.Step(activeCommands);

	// Clear the active player's commands, because they are all processed at this point.
	activeCommands.Clear();

	// Perform actions for all the game objects. In general this is ordered from
	// bottom to top of the draw stack, but in some cases one object type must
	// "act" before another does.

	// The only action stellar objects perform is to launch defense fleets.
	for(const StellarObject &object : playerSystem->Objects())
		if(object.HasValidPlanet())
			object.GetPlanet()->DeployDefense(newShips);

	// Keep track of the flagship to see if it jumps or enters a wormhole this frame.
	bool flagshipWasUntargetable = (flagship && !flagship->IsTargetable());
	bool wasHyperspacing = (flagship && flagship->IsEnteringHyperspace());
	// First, move the player's flagship.
	if(flagship)
	{
		emptySoundsTimer.resize(flagship->Weapons().size());
		for(int &it : emptySoundsTimer)
			if(it > 0)
				--it;
		MoveShip(player.FlagshipPtr());
	}
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
		if(flagship->Attributes().Get("silent jumps"))
		{
			// No sounds.
		}
		else if(jumpSounds.empty())
			Audio::Play(Audio::Get(isJumping ? "jump drive" : "hyperdrive"), SoundCategory::JUMP);
		else
			for(const auto &sound : jumpSounds)
				Audio::Play(sound.first, SoundCategory::JUMP);
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
	PrunePointers(ships);

	// Move the asteroids. This must be done before collision detection. Minables
	// may create visuals or flotsam.
	asteroids.Step(newVisuals, newFlotsam, step);

	// Move the flotsam. This must happen after the ships move, because flotsam
	// checks if any ship has picked it up.
	for(const shared_ptr<Flotsam> &it : flotsam)
		it->Move(newVisuals);
	PrunePointers(flotsam);

	// Move the projectiles.
	for(Projectile &projectile : projectiles)
		projectile.Move(newVisuals, newProjectiles);
	Prune(projectiles);

	// Step the weather.
	for(Weather &weather : activeWeather)
		weather.Step(newVisuals, flagship ? flagship->Position() : camera.Center());
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
}



// Move a ship. Also determine if the ship should generate hyperspace sounds or
// boarding events, fire weapons, and launch fighters.
void Engine::MoveShip(const shared_ptr<Ship> &ship)
{
	// Various actions a ship could have taken last frame may have impacted the accuracy of cached values.
	// Therefore, determine with any information needs recalculated and cache it.
	ship->UpdateCaches();

	const Ship *flagship = player.Flagship();
	bool isFlagship = ship.get() == flagship;

	bool isJump = ship->IsUsingJumpDrive();
	const System *oldSystem = ship->GetSystem();
	bool wasHere = (flagship && oldSystem == flagship->GetSystem());
	bool wasHyperspacing = ship->IsHyperspacing();
	bool wasDisabled = ship->IsDisabled();
	// Give the ship the list of visuals so that it can draw explosions,
	// ion sparks, jump drive flashes, etc.
	ship->Move(newVisuals, newFlotsam);
	if(ship->IsDisabled() && !wasDisabled)
		eventQueue.emplace_back(nullptr, ship, ShipEvent::DISABLE);
	// Track the movements of mission NPCs.
	if(ship->IsSpecial() && !ship->IsYours() && ship->GetSystem() != oldSystem)
		eventQueue.emplace_back(ship, ship, ShipEvent::JUMP);
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
			// If this is a player ship, make sure it's no longer selected.
			if(ship->IsYours())
				player.DeselectEscort(ship.get());
		}
		return;
	}

	// Check if we need to play sounds for a ship jumping in or out of
	// the system. Make no sound if it entered via wormhole.
	if(!isFlagship && ship->Zoom() == 1.)
	{
		// The position from where sounds will be played.
		Point position = ship->Position();
		// Did this ship just begin hyperspacing?
		if(wasHere && !wasHyperspacing && ship->IsHyperspacing())
		{
			const map<const Sound *, int> &jumpSounds = isJump
				? ship->Attributes().JumpOutSounds() : ship->Attributes().HyperOutSounds();
			if(ship->Attributes().Get("silent jumps"))
			{
				// No sounds.
			}
			else if(jumpSounds.empty())
				Audio::Play(Audio::Get(isJump ? "jump out" : "hyperdrive out"), position, SoundCategory::JUMP);
			else
				for(const auto &sound : jumpSounds)
					Audio::Play(sound.first, position, SoundCategory::JUMP);
		}

		// Did this ship just jump into the player's system?
		if(!wasHere && flagship && ship->GetSystem() == flagship->GetSystem())
		{
			const map<const Sound *, int> &jumpSounds = isJump
				? ship->Attributes().JumpInSounds() : ship->Attributes().HyperInSounds();
			if(ship->Attributes().Get("silent jumps"))
			{
				// No sounds.
			}
			else if(jumpSounds.empty())
				Audio::Play(Audio::Get(isJump ? "jump in" : "hyperdrive in"), position, SoundCategory::JUMP);
			else
				for(const auto &sound : jumpSounds)
					Audio::Play(sound.first, position, SoundCategory::JUMP);
		}
	}

	// Boarding:
	bool autoPlunder = !ship->IsYours();
	// The player should not become a docked passenger on some other ship, but AI ships may.
	shared_ptr<Ship> victim = ship->Board(autoPlunder, isFlagship);
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
	ship->Fire(newProjectiles, newVisuals, ship.get() == flagship ? &emptySoundsTimer : nullptr);

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
	if(player.ActiveInFlightMission())
	{
		Place(player.ActiveInFlightMission()->NPCs(), player.FlagshipPtr());
		player.ClearActiveInFlightMission();
	}

	// Non-mission NPCs spawn at random intervals in neighboring systems,
	// or coming from planets in the current one.
	double fleetMultiplier = GameData::GetGamerules().FleetMultiplier();
	for(const auto &fleet : player.GetSystem()->Fleets())
		if(fleetMultiplier ? !Random::Int(fleet.Period() / fleetMultiplier) && fleet.CanTrigger() : false)
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

	// Although an attempt to spawn a person is specified by a gamerule,
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
				if(ship->GivenName().empty())
					ship->SetGivenName(it.first);
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
		if(hazard.Get()->IsValid() && !Random::Int(hazard.Period()) && hazard.CanTrigger())
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
		maneuveringCommands | Command::SHIFT | Command::MOUSE_TURNING_HOLD | Command::AIM_TURRET_HOLD);

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

	if(keyDown.Has(Command::PAUSE))
	{
		timePaused = !timePaused;
		if(timePaused)
			Audio::Pause();
		else
			Audio::Resume();
	}
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
	if(mouseButton == MouseButton::LEFT && flagship->Zoom() == 1.)
		for(const StellarObject &object : playerSystem->Objects())
			if(object.HasSprite() && object.HasValidPlanet())
			{
				// If the player clicked to land on a planet,
				// do so unless already landing elsewhere.
				Point position = object.Position() - camera.Center();
				const Planet *planet = object.GetPlanet();
				if(planet->IsAccessible(flagship) && (clickPoint - position).Length() < object.Radius())
				{
					if(&object == flagship->GetTargetStellar())
					{
						if(!planet->CanLand(*flagship))
							Messages::Add({"The authorities on " + planet->DisplayName()
								+ " refuse to let you land.", GameData::MessageCategories().Get("high")});
						else if(!flagship->IsDestroyed() && !flagship->Commands().Has(Command::LAND))
							activeCommands |= Command::LAND;
					}
					else
					{
						UI::PlaySound(UI::UISound::TARGET);
						flagship->SetTargetStellar(&object);
						if(flagship->Commands().Has(Command::LAND))
							ai.DisengageAutopilot();
					}

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
		UI::PlaySound(UI::UISound::TARGET);
		if(mouseButton == MouseButton::RIGHT)
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
					player.SelectEscort(clickTarget.get(), hasShift);
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
				if(mouseButton == MouseButton::RIGHT)
					ai.IssueAsteroidTarget(minable);
			}
		}
	}
	if(!clickTarget && !clickedAsteroid
		&& mouseButton == (isMouseTurningEnabled ? MouseButton::MIDDLE : MouseButton::RIGHT))
	{
		UI::PlaySound(UI::UISound::TARGET);
		ai.IssueMoveTarget(clickPoint + camera.Center(), playerSystem);
	}

	// Treat an "empty" click as a request to clear targets.
	if(!clickTarget && mouseButton == MouseButton::LEFT && !clickedAsteroid && !clickedPlanet)
		flagship->SetTargetShip(nullptr);
}



// Determines alternate mouse turning, setting player mouse angle, and right-click firing weapons.
void Engine::HandleMouseInput(Command &activeCommands)
{
	bool rightMouseButtonHeld = false;
	int mousePosX, mousePosY;
	if((SDL_GetMouseState(&mousePosX, &mousePosY) & SDL_BUTTON_RMASK) != 0)
		rightMouseButtonHeld = true;

	Point relPos = Point(mousePosX, mousePosY) - Point(Screen::RawWidth(), Screen::RawHeight()) / 2;
	ai.SetMousePosition(relPos / zoom);

	isMouseHoldEnabled = activeCommands.Has(Command::MOUSE_TURNING_HOLD);
	activeCommands.Clear(Command::MOUSE_TURNING_HOLD);

	// XOR mouse hold and mouse toggle. If mouse toggle is OFF, then mouse hold
	// will temporarily turn ON mouse control. If mouse toggle is ON, then mouse
	// hold will temporarily turn OFF mouse control.
	isMouseTurningEnabled = isMouseHoldEnabled ^ Preferences::Has("Control ship with mouse");
	if(!isMouseTurningEnabled)
		return;
	activeCommands.Set(Command::MOUSE_TURNING_HOLD);

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
		{
			vector<Body *> inRadius;
			inRadius.reserve(min(static_cast<vector<Body *>::size_type>(triggerRadius), ships.size()));
			shipCollisions.Circle(projectile.Position(), triggerRadius, inRadius);
			for(const Body *body : inRadius)
			{
				const Ship *ship = reinterpret_cast<const Ship *>(body);
				if(body == projectile.Target() || (gov->IsEnemy(body->GetGovernment())
						&& !ship->IsCloaked()))
				{
					collisions.emplace_back(nullptr, CollisionType::NONE, 0.);
					break;
				}
			}
		}

		// If nothing triggered the projectile, check for collisions with ships and asteroids.
		if(collisions.empty())
		{
			if(weapon.CanCollideShips())
				shipCollisions.Line(projectile, collisions);
			if(weapon.CanCollideAsteroids())
				asteroids.CollideAsteroids(projectile, collisions);
			if(weapon.CanCollideMinables())
				asteroids.CollideMinables(projectile, collisions);
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

		// If this projectile has a blast radius, find all ships and minables within its
		// radius. Otherwise, only one is damaged.
		double blastRadius = weapon.BlastRadius();
		if(blastRadius)
		{
			// Even friendly ships can be hit by the blast, unless it is a
			// "safe" weapon.
			Point hitPos = projectile.Position() + range * projectile.Velocity();
			bool isSafe = weapon.IsSafe();
			vector<Body *> blastCollisions;
			blastCollisions.reserve(32);
			shipCollisions.Circle(hitPos, blastRadius, blastCollisions);
			for(Body *body : blastCollisions)
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
			blastCollisions.clear();
			asteroids.MinablesCollisionsCircle(hitPos, blastRadius, blastCollisions);
			for(Body *body : blastCollisions)
			{
				auto minable = reinterpret_cast<Minable *>(body);
				minable->TakeDamage(damage.CalculateDamage(*minable));
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
			{
				auto minable = reinterpret_cast<Minable *>(hit);
				minable->TakeDamage(damage.CalculateDamage(*minable));
			}
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
		vector<Body *> affectedShips;
		if(hazard->SystemWide())
			affectedShips = shipCollisions.All();
		else
		{
			affectedShips.reserve(ships.size());
			shipCollisions.Ring(weather.Origin(), hazard->MinRange(), hazard->MaxRange(), affectedShips);
		}
		for(Body *body : affectedShips)
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
	vector<Body *> pickupShips;
	pickupShips.reserve(16);
	shipCollisions.Circle(flotsam.Position(), 5., pickupShips);
	for(Body *body : pickupShips)
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
	bool collectorIsFlagship = collector == player.Flagship();
	if(collector->IsYours())
	{
		const auto flotsamSetting = Preferences::GetFlotsamCollection();
		if(flotsamSetting == Preferences::FlotsamCollection::OFF)
			return;
		if(collectorIsFlagship && flotsamSetting == Preferences::FlotsamCollection::ESCORT)
			return;
		if(!collectorIsFlagship && flotsamSetting == Preferences::FlotsamCollection::FLAGSHIP)
			return;
	}

	// Transfer cargo from the flotsam to the collector ship.
	int amount = flotsam.TransferTo(collector);

	// If the collector is not one of the player's ships, we can bail out now.
	if(!collector->IsYours())
		return;

	if(!collectorIsFlagship && !Preferences::Has("Extra fleet status messages"))
		return;

	// One of your ships picked up this flotsam. Describe who it was.
	string name = (collectorIsFlagship ? "You" :
			"Your " + collector->Noun() + " \"" + collector->GivenName() + "\"") + " picked up ";
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
	if(message.empty())
		return;

	int free = collector->Cargo().Free();
	int total = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(!ship->IsDestroyed() && !ship->IsParked() && ship->GetSystem() == player.GetSystem())
			total += ship->Cargo().Free();

	message += " (" + Format::CargoString(free, "free space") + " remaining";
	if(free == total)
		message += ".)";
	else
		message += ", " + Format::MassString(total) + " in fleet.)";
	Messages::Add({message, GameData::MessageCategories().Get("normal")});
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
			int type = isYourTarget ? Radar::SPECIAL : RadarType(*ship, uiStep);
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
			Audio::Play(Audio::Get("alarm"), SoundCategory::ALERT);
		alarmTime = 300;
		hadHostiles = true;
	}
	else if(!hasHostiles)
		hadHostiles = false;

	// Add projectiles that have a missile strength or blast radius.
	for(const Projectile &projectile : projectiles)
	{
		if(!projectile.HasSprite())
			continue;

		bool isBlast = projectile.GetWeapon().BlastRadius();
		if(!projectile.MissileStrength() && !isBlast)
			continue;

		bool isEnemy = projectile.GetGovernment() && projectile.GetGovernment()->IsEnemy();
		bool isSafe = projectile.GetWeapon().IsSafe();
		radar[currentCalcBuffer].Add(isEnemy || (isBlast && !isSafe) ? Radar::SPECIAL : Radar::INACTIVE,
			projectile.Position(), isBlast ? 1.8 : 1.);
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
	const Swizzle *cloakSwizzle = GameData::Swizzles().Get(fancyCloak ? "cloak fancy base" : "cloak fast");
	auto &itemsToDraw = draw[currentCalcBuffer];
	auto drawObject = [&itemsToDraw, cloak, drawCloaked, fancyCloak, cloakSwizzle](const Body &body) -> void
	{
		// Draw cloaked/cloaking sprites swizzled red or transparent (depending on whether we are using fancy
		// cloaking effects), and overlay this solid sprite with an increasingly transparent "regular" sprite.
		if(drawCloaked)
			itemsToDraw.AddSwizzled(body, cloakSwizzle, fancyCloak ? 0.5 : 0.25);
		itemsToDraw.Add(body, cloak);
	};

	if(hasFighters)
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.side == Ship::Bay::UNDER && bay.ship)
				drawObject(*bay.ship);

	auto DrawEngineFlares = [&](uint8_t where)
	{
		if(ship.ThrustHeldFrames(Ship::ThrustKind::FORWARD) && !ship.EnginePoints().empty())
			DrawFlareSprites(ship, draw[currentCalcBuffer], ship.EnginePoints(),
				ship.Attributes().FlareSprites(), where, false);
		else if(ship.ThrustHeldFrames(Ship::ThrustKind::REVERSE) && !ship.ReverseEnginePoints().empty())
			DrawFlareSprites(ship, draw[currentCalcBuffer], ship.ReverseEnginePoints(),
				ship.Attributes().ReverseFlareSprites(), where, true);
		if((ship.ThrustHeldFrames(Ship::ThrustKind::LEFT) || ship.ThrustHeldFrames(Ship::ThrustKind::RIGHT))
			&& !ship.SteeringEnginePoints().empty())
			DrawFlareSprites(ship, draw[currentCalcBuffer], ship.SteeringEnginePoints(),
				ship.Attributes().SteeringFlareSprites(), where, false);
	};
	DrawEngineFlares(Ship::EnginePoint::UNDER);

	auto drawHardpoint = [&drawObject, &ship](const Hardpoint &hardpoint) -> void
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(!weapon)
			return;
		const Body &sprite = weapon->HardpointSprite();
		if(!sprite.HasSprite())
			return;

		Body body(
			sprite,
			ship.Position() + ship.Zoom() * ship.Facing().Rotate(hardpoint.GetPoint()),
			ship.Velocity(),
			ship.Facing() + hardpoint.GetAngle(),
			ship.Zoom());
		if(body.InheritsParentSwizzle())
			body.SetSwizzle(ship.GetSwizzle());
		drawObject(body);
	};

	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.GetSide() == Hardpoint::Side::UNDER)
			drawHardpoint(hardpoint);
	drawObject(ship);
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.GetSide() == Hardpoint::Side::OVER)
			drawHardpoint(hardpoint);

	DrawEngineFlares(Ship::EnginePoint::OVER);

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
	if(grudge.contains(attacker))
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
		message += attacker->DisplayName();
		message += (attackerCount == 1 ? " ship." : " ships.");
	}
	else
	{
		message = "We are under attack by ";
		if(attackerCount == 1)
			message += "a ";
		message += attacker->DisplayName();
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

		static auto FLAGSHIP = Preferences::OverlayType::FLAGSHIP;
		static auto FRIENDLY = Preferences::OverlayType::ESCORT;
		static auto HOSTILE = Preferences::OverlayType::ENEMY;
		static auto NEUTRAL = Preferences::OverlayType::NEUTRAL;

		if(it == flagship)
			EmplaceStatusOverlay(it, overlaySettings[FLAGSHIP], Status::Type::FLAGSHIP, it->Cloaking());
		else if(it->GetGovernment()->IsEnemy())
			EmplaceStatusOverlay(it, overlaySettings[HOSTILE], Status::Type::HOSTILE, it->Cloaking());
		else if(it->IsYours() || it->GetPersonality().IsEscort())
			EmplaceStatusOverlay(it, overlaySettings[FRIENDLY], Status::Type::FRIENDLY, it->Cloaking());
		else
			EmplaceStatusOverlay(it, overlaySettings[NEUTRAL], Status::Type::NEUTRAL, it->Cloaking());
	}
}



void Engine::EmplaceStatusOverlay(const shared_ptr<Ship> &it, Preferences::OverlayState overlaySetting,
	Status::Type type, double cloak)
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

	statuses.emplace_back(it->Position() - camera.Center(), it->Shields(), it->Hull(),
		min(it->Hull(), it->DisabledHull()), max(20., width * .5), type, alpha * (1. - cloak));
}
