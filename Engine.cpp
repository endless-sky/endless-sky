/* Engine.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Engine.h"

#include "Audio.h"
#include "DotShader.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "LineShader.h"
#include "Messages.h"
#include "OutlineShader.h"
#include "pi.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "Random.h"
#include "RingShader.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <cmath>

using namespace std;



Engine::Engine(PlayerInfo &player)
	: player(player),
	calcTickTock(false), drawTickTock(false), terminate(false), step(0),
	flash(0.), doFlash(false), wasLeavingHyperspace(false),
	load(0.), loadCount(0), loadSum(0.)
{
	// Start the thread for doing calculations.
	calcThread = thread(&Engine::ThreadEntryPoint, this);
	
	if(!player.IsLoaded() || !player.GetSystem())
		return;
	
	// Preload any landscapes for this system.
	for(const StellarObject &object : player.GetSystem()->Objects())
		if(object.GetPlanet())
			GameData::Preload(object.GetPlanet()->Landscape());
	
	// Now we know the player's current position. Draw the planets.
	Point center;
	if(player.GetPlanet())
	{
		for(const StellarObject &object : player.GetSystem()->Objects())
			if(object.GetPlanet() == player.GetPlanet())
				center = object.Position();
	}
	for(const StellarObject &object : player.GetSystem()->Objects())
		if(!object.GetSprite().IsEmpty())
		{
			Point position = object.Position();
			Point unit = object.Unit();
			position -= center;
			
			int type = object.IsStar() ? Radar::SPECIAL :
				!object.GetPlanet() ? Radar::INACTIVE :
				object.GetPlanet()->IsWormhole() ? Radar::ANOMALOUS :
				object.GetPlanet()->CanLand() ? Radar::FRIENDLY : Radar::HOSTILE;
			double r = max(2., object.Radius() * .03 + .5);
			
			draw[calcTickTock].Add(object.GetSprite(), position, unit);
			radar[calcTickTock].Add(type, position, r, r - 1.);
		}
	
	// Add all neighboring systems to the radar.
	Point pos = player.GetSystem()->Position();
	for(const System *system : player.GetSystem()->Links())
		radar[calcTickTock].AddPointer(Radar::INACTIVE, system->Position() - pos);
}



Engine::~Engine()
{
	{
		unique_lock<mutex> lock(swapMutex);
		terminate = true;
	}
	condition.notify_all();
	calcThread.join();
}



void Engine::Place()
{
	ships.clear();
	
	EnterSystem();
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		ships.push_back(ship);
		Point pos;
		Angle angle = Angle::Random(360.);
		// All your ships that are in system with the player act as if they are
		// leaving the planet along with you.
		if(player.GetPlanet() && ship->GetSystem() == player.GetSystem())
		{
			ship->SetPlanet(player.GetPlanet());
			for(const StellarObject &object : ship->GetSystem()->Objects())
				if(object.GetPlanet() == player.GetPlanet())
				{
					pos = object.Position() + angle.Unit() * Random::Real() * object.Radius();
				}
		}
		ship->Place(pos, angle.Unit(), angle);
	}
	shared_ptr<Ship> flagship;
	if(!player.Ships().empty())
		flagship = player.Ships().front();
	for(const Mission &mission : player.Missions())
		for(const NPC &npc : mission.NPCs())
		{
			map<Ship *, int> droneCarriers;
			map<Ship *, int> fighterCarriers;
			for(const shared_ptr<Ship> &ship : npc.Ships())
			{
				// Skip ships that have been destroyed.
				if(ship->IsDestroyed())
					continue;
				
				if(ship->DroneBaysFree())
					droneCarriers[&*ship] = ship->DroneBaysFree();
				if(ship->FighterBaysFree())
					fighterCarriers[&*ship] = ship->FighterBaysFree();
				// Redo the loading up of fighters.
				ship->UnloadFighters();
			}
			
			for(const shared_ptr<Ship> &ship : npc.Ships())
			{
				// Skip ships that have been destroyed.
				if(ship->IsDestroyed())
					continue;
				
				ship->Recharge();
				
				if(ship->IsFighter())
				{
					bool docked = false;
					if(ship->Attributes().Category() == "Drone")
					{
						for(auto &it : droneCarriers)
							if(it.second)
							{
								it.first->AddFighter(ship);
								--it.second;
								docked = true;
								break;
							}
					}
					else if(ship->Attributes().Category() == "Fighter")
					{
						for(auto &it : fighterCarriers)
							if(it.second)
							{
								it.first->AddFighter(ship);
								--it.second;
								docked = true;
								break;
							}
					}
					if(docked)
						continue;
				}
				
				ships.push_back(ship);
				if(!ship->GetPersonality().IsUninterested())
					ship->SetParent(flagship);
				
				Point pos;
				Angle angle = Angle::Random(360.);
				// All your ships that are in system with the player act as if they are
				// leaving the planet along with you.
				if(player.GetPlanet() && ship->GetSystem() == player.GetSystem()
						&& (player.GetPlanet()->CanLand(*ship) || ship->GetGovernment()->IsPlayer())
						&& !(ship->GetPersonality().IsStaying() || ship->GetPersonality().IsWaiting()))
				{
					ship->SetPlanet(player.GetPlanet());
					for(const StellarObject &object : ship->GetSystem()->Objects())
						if(object.GetPlanet() == player.GetPlanet())
							pos = object.Position() + angle.Unit() * Random::Real() * object.Radius();
				}
				else
				{
					pos = Angle::Random().Unit() * ((Random::Real() + 1.) * 600.);
					for(const StellarObject &object : ship->GetSystem()->Objects())
						if(object.GetPlanet() == player.GetPlanet())
							pos += object.Position();
				}
				ship->Place(pos, angle.Unit(), angle);
			}
		}
	
	player.SetPlanet(nullptr);
}



// Begin the next step of calculations.
void Engine::Step(bool isActive)
{
	{
		unique_lock<mutex> lock(swapMutex);
		while(calcTickTock != drawTickTock)
			condition.wait(lock);
		
		if(isActive)
			++step;
		
		events.swap(eventQueue);
		eventQueue.clear();
		
		// The calculation thread is now paused, so it is safe to access things.
		const Ship *flagship = player.GetShip();
		if(flagship)
		{
			position = flagship->Position();
			velocity = flagship->Velocity();
			bool isLeavingHyperspace = flagship->IsHyperspacing();
			if(!isLeavingHyperspace && wasLeavingHyperspace)
			{
				int type = ShipEvent::JUMP;
				shared_ptr<Ship> ship = player.Ships().front();
				events.emplace_back(ship, ship, type);
			}
			wasLeavingHyperspace = isLeavingHyperspace;
		}
		ai.UpdateEvents(events);
		ai.UpdateKeys(&player, isActive && wasActive);
		wasActive = isActive;
		Audio::Update(position, velocity);
		
		// Any of the player's ships that are in system are assumed to have
		// landed along with the player.
		if(flagship && flagship->GetPlanet() && isActive)
			player.SetPlanet(flagship->GetPlanet());
		
		const System *currentSystem = player.GetSystem();
		// Update this here, for thread safety.
		if(!player.HasTravelPlan() && flagship && flagship->GetTargetSystem())
			player.AddTravel(flagship->GetTargetSystem());
		if(player.HasTravelPlan() && currentSystem == player.TravelPlan().back())
			player.PopTravel();
		if(doFlash)
		{
			flash = .4;
			doFlash = false;
		}
		else if(flash)
			flash = max(0., flash * .99 - .002);
		
		targets.clear();
		
		// Update the player's ammo amounts.
		ammo.clear();
		if(flagship)
			for(const auto &it : flagship->Outfits())
			{
				if(it.first->Ammo())
					ammo.emplace_back(it.first,
						flagship->OutfitCount(it.first->Ammo()));
				else if(it.first->FiringFuel())
				{
					double remaining = flagship->Fuel()
						* flagship->Attributes().Get("fuel capacity");
					ammo.emplace_back(it.first,
						remaining / it.first->FiringFuel());
				}
			}
		
		// Display escort information for all ships of the "Escort" government,
		// and all ships with the "escort" personality, except for fighters that
		// are not owned by the player.
		escorts.Clear();
		for(const auto &it : ships)
			if(it->GetGovernment()->IsPlayer() || it->GetPersonality().IsEscort())
				if(!it->IsFighter() && it.get() != flagship)
					escorts.Add(*it, it->GetSystem() == currentSystem);
		for(const shared_ptr<Ship> &escort : player.Ships())
			if(escort->IsFighter())
				escorts.Add(*escort, escort->GetSystem() == currentSystem);
		
		// Create the status overlays.
		statuses.clear();
		if(isActive && Preferences::Has("Show status overlays"))
			for(const auto &it : ships)
			{
				if(!it->GetGovernment() || it->GetSystem() != currentSystem || it->Cloaking() == 1.)
					continue;
			
				bool isEnemy = it->GetGovernment()->IsEnemy();
				if(isEnemy || it->GetGovernment()->IsPlayer())
				{
					double width = min(it->GetSprite().Width(), it->GetSprite().Height());
					statuses.emplace_back(it->Position() - position, it->Shields(), it->Hull(),
						it->Zoom() * max(20., width * .25), isEnemy);
				}
			}
		
		if(flagship && flagship->IsOverheated())
			Messages::Add("Your ship has overheated.");
		
		if(flagship && flagship->Hull())
			info.SetSprite("player sprite", flagship->GetSprite().GetSprite());
		else
			info.SetSprite("player sprite", nullptr);
		if(currentSystem)
			info.SetString("location", currentSystem->Name());
		info.SetString("date", player.GetDate().ToString());
		if(flagship)
		{
			info.SetBar("fuel", flagship->Fuel(),
				flagship->Attributes().Get("fuel capacity") * .01);
			info.SetBar("energy", flagship->Energy());
			info.SetBar("heat", flagship->Heat());
			info.SetBar("shields", flagship->Shields());
			info.SetBar("hull", flagship->Hull(), 20.);
		}
		else
		{
			info.SetBar("fuel", 0.);
			info.SetBar("energy", 0.);
			info.SetBar("heat", 0.);
			info.SetBar("shields", 0.);
			info.SetBar("hull", 0.);
		}
		info.SetString("credits",
			Format::Number(player.Accounts().Credits()) + " credits");
		if(flagship && flagship->GetTargetPlanet() && !flagship->Commands().Has(Command::JUMP))
		{
			const StellarObject *object = flagship->GetTargetPlanet();
			info.SetString("navigation mode", "Landing on:");
			const string &name = object->Name();
			info.SetString("destination", name.empty() ? "???" : name);
			
			targets.push_back({
				object->Position() - flagship->Position(),
				Angle(45.),
				object->Radius(),
				object->GetPlanet()->CanLand() ? Radar::FRIENDLY : Radar::HOSTILE});
		}
		else if(flagship && flagship->GetTargetSystem())
		{
			info.SetString("navigation mode", "Hyperspace:");
			if(player.HasVisited(flagship->GetTargetSystem()))
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
		info.SetRadar(radar[!drawTickTock]);
		shared_ptr<const Ship> target;
		if(flagship)
			target = flagship->GetTargetShip();
		if(!target)
		{
			info.SetSprite("target sprite", nullptr);
			info.SetString("target name", "no target");
			info.SetString("target type", "");
			info.SetString("target government", "");
			info.SetBar("target shields", 0.);
			info.SetBar("target hull", 0.);
		}
		else
		{
			info.SetSprite("target sprite", target->GetSprite().GetSprite());
			info.SetString("target name", target->Name());
			info.SetString("target type", target->ModelName());
			if(!target->GetGovernment())
				info.SetString("target government", "No Government");
			else
				info.SetString("target government", target->GetGovernment()->GetName());
			
			shared_ptr<const Ship> targetTarget = target->GetTargetShip();
			bool hostile = targetTarget && targetTarget->GetGovernment()->IsPlayer();
			int targetType = (target->IsDisabled() || target->IsOverheated()) ? Radar::INACTIVE :
				!target->GetGovernment()->IsEnemy() ? Radar::FRIENDLY :
				hostile ? Radar::HOSTILE : Radar::UNFRIENDLY;
			info.SetOutlineColor(Radar::GetColor(targetType));
			
			if(target->GetSystem() == player.GetSystem() && target->IsTargetable())
			{
				info.SetBar("target shields", target->Shields());
				info.SetBar("target hull", target->Hull(), 20.);
			
				// The target area will be a square, with sides equal to the average
				// of the width and the height of the sprite.
				const Animation &anim = target->GetSprite();
				double size = target->Zoom() * (anim.Width() + anim.Height()) * .175;
				targets.push_back({
					target->Position() - flagship->Position(),
					Angle(45.) + target->Facing(),
					size,
					targetType});
			}
			else
			{
				info.SetBar("target shields", 0.);
				info.SetBar("target hull", 0.);
			}
		}
		
		// Begin the next frame's calculations.
		if(isActive)
			drawTickTock = !drawTickTock;
	}
	if(isActive)
		condition.notify_one();
}



const list<ShipEvent> &Engine::Events() const
{
	return events;
}



// Draw a frame.
void Engine::Draw() const
{
	GameData::Background().Draw(position, velocity);
	draw[drawTickTock].Draw();
	
	for(const auto &it : statuses)
	{
		if(it.hull <= 0.)
			continue;
		
		static const Color color[4] = {
			Color(0., .5, 0., .25),
			Color(.5, .15, 0., .25),
			Color(.45, .5, 0., .25),
			Color(.5, .3, 0., .25)
		};
		RingShader::Draw(it.position, it.radius + 3., 1.5, it.shields, color[it.isEnemy]);
		RingShader::Draw(it.position, it.radius, 1.5, it.hull, color[2 + it.isEnemy], 20.);
	}
	
	if(flash)
		FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), Color(flash, flash));
	
	// Draw messages.
	const Font &font = FontSet::Get(14);
	const vector<Messages::Entry> &messages = Messages::Get(step);
	Point messagePoint(
		Screen::Left() + 120.,
		Screen::Bottom() - 20. * messages.size());
	for(const auto &it : messages)
	{
		float alpha = (it.step + 1000 - step) * .001f;
		Color color(alpha, 0.);
		font.Draw(it.message, messagePoint, color);
		messagePoint.Y() += 20.;
	}
	
	// Draw crosshairs around anything that is targeted.
	for(const Target &target : targets)
	{
		Angle a = target.angle;
		Angle da(90.);
		
		for(int i = 0; i < 4; ++i)
		{
			PointerShader::Draw(target.center, a.Unit(), 10., 10., -target.radius,
				Radar::GetColor(target.type));
			a += da;
		}
	}
	
	GameData::Interfaces().Get("status")->Draw(info);
	GameData::Interfaces().Get("targets")->Draw(info);
	
	// Draw ammo status.
	Point pos(Screen::Right() - 80, Screen::Bottom());
	const Sprite *selectedSprite = SpriteSet::Get("ui/ammo selected");
	const Sprite *unselectedSprite = SpriteSet::Get("ui/ammo unselected");
	Color selectedColor = *GameData::Colors().Get("bright");
	Color unselectedColor = *GameData::Colors().Get("dim");
	for(const pair<const Outfit *, int> &it : ammo)
	{
		pos.Y() -= 30.;
		
		bool isSelected = it.first == player.SelectedWeapon();
		
		SpriteShader::Draw(it.first->Icon(), pos);
		SpriteShader::Draw(
			isSelected ? selectedSprite : unselectedSprite, pos + Point(35., 0.));
		
		string amount = to_string(it.second);
		Point textPos = pos + Point(55 - font.Width(amount), -(30 - font.Height()) / 2);
		font.Draw(amount, textPos, isSelected ? selectedColor : unselectedColor);
	}
	
	// Draw escort status.
	escorts.Draw();
	
	if(Preferences::Has("Show CPU / GPU load"))
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% CPU";
		Color color = *GameData::Colors().Get("medium");
		FontSet::Get(14).Draw(loadString,
			Point(-10 - font.Width(loadString), Screen::Height() * -.5 + 5.), color);
	}
}



void Engine::EnterSystem()
{
	ai.Clean();
	grudge.clear();
	
	const Ship *flagship = player.GetShip();
	if(!flagship)
		return;
	
	const System *system = flagship->GetSystem();
	player.SetSystem(system);
	
	player.IncrementDate();
	const Date &today = player.GetDate();
	Messages::Add("Entering the " + system->Name() + " system on "
		+ today.ToString() + (system->IsInhabited() ?
			"." : ". No inhabited planets detected."));
	
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet())
			GameData::Preload(object.GetPlanet()->Landscape());
	
	GameData::SetDate(today);
	// SetDate() clears any bribes from yesterday, so restore any auto-clearance.
	for(const Mission &mission : player.Missions())
		if(mission.ClearanceMessage() == "auto")
			mission.Destination()->Bribe(mission.HasFullClearance());
	
	asteroids.Clear();
	for(const System::Asteroid &a : system->Asteroids())
		asteroids.Add(a.Name(), a.Count(), a.Energy());
	
	// Place five seconds worth of fleets.
	for(int i = 0; i < 5; ++i)
		for(const System::FleetProbability &fleet : system->Fleets())
			if(Random::Int(fleet.Period()) < 60)
				fleet.Get()->Place(*system, ships);
	// Find out how attractive the player's fleet is to pirates. Aside from a
	// heavy freighter, no single ship should attract extra pirate attention.
	unsigned attraction = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		const string &category = ship->Attributes().Category();
		if(category == "Light Freighter")
			attraction += 1;
		if(category == "Heavy Freighter")
			attraction += 2;
	}
	if(attraction > 2)
		for(int i = 0; i < 10; ++i)
			if(Random::Int(200) + 1 < attraction)
				GameData::Fleets().Get("pirate raid")->Place(*system, ships);
	
	projectiles.clear();
	effects.clear();
	
	// Help message for new players. Show this message for the first four days,
	// since the new player ships can make at most four jumps before landing.
	if(today <= Date(21, 11, 3013))
	{
		Messages::Add(string("Press \"")
			+ Command::MAP.KeyName()
			+ string("\" to view your map, and \"")
			+ Command::JUMP.KeyName()
			+ string("\" to make a hyperspace jump."));
		Messages::Add(string("Or, press \"")
			+ Command::LAND.KeyName()
			+ string("\" to land. For the main menu, press \"")
			+ Command::MENU.KeyName() + string("\"."));
	}
}



// Thread entry point.
void Engine::ThreadEntryPoint()
{
	while(true)
	{
		{
			unique_lock<mutex> lock(swapMutex);
			while(calcTickTock == drawTickTock && !terminate)
				condition.wait(lock);
		
			if(terminate)
				break;
		}
		
		// Do all the calculations.
		CalculateStep();
		
		{
			unique_lock<mutex> lock(swapMutex);
			calcTickTock = drawTickTock;
		}
		condition.notify_one();
	}
}



void Engine::CalculateStep()
{
	FrameTimer loadTimer;
	
	// Clear the list of objects to draw.
	draw[calcTickTock].Clear(step);
	radar[calcTickTock].Clear();
	
	if(!player.GetSystem())
		return;
	
	// Now, all the ships must decide what they are doing next.
	ai.Step(ships, player);
	const Ship *flagship = player.GetShip();
	bool wasHyperspacing = (flagship && flagship->IsEnteringHyperspace());
	
	// Now, move all the ships. We must finish moving all of them before any of
	// them fire, or their turrets will be targeting where a given ship was
	// instead of where it is now. This is also where ships get deleted, and
	// where they may create explosions if they are dying.
	for(auto it = ships.begin(); it != ships.end(); )
	{
		// Give the ship the list of effects so that if it is dying, it can
		// create explosions. Eventually ships might create other effects too.
		// Note that engine flares are handled separately, so that they will be
		// drawn immediately under the ship.
		if(!(*it)->Move(effects))
			it = ships.erase(it);
		else
			++it;
	}
	
	if(!wasHyperspacing && flagship && flagship->IsEnteringHyperspace())
		Audio::Play(Audio::Get(flagship->Attributes().Get("jump drive") ? "jump_drive" : "hyperspace"));
	
	// If the player has entered a new system, update the asteroids, etc.
	if(wasHyperspacing && !flagship->IsEnteringHyperspace())
	{
		doFlash = true;
		EnterSystem();
	}
	else if(flagship && player.GetSystem() != flagship->GetSystem())
	{
		// Wormhole travel:
		player.ClearTravel();
		doFlash = true;
		EnterSystem();
	}
	
	// Now we know the player's current position. Draw the planets.
	Point center;
	Point centerVelocity;
	if(flagship)
	{
		center = flagship->Position();
		centerVelocity = flagship->Velocity();
	}
	else if(player.GetPlanet())
	{
		for(const StellarObject &object : player.GetSystem()->Objects())
			if(object.GetPlanet() == player.GetPlanet())
				center = object.Position();
	}
	for(const StellarObject &object : player.GetSystem()->Objects())
		if(!object.GetSprite().IsEmpty())
		{
			Point position = object.Position();
			Point unit = object.Unit();
			position -= center;
			
			int type = object.IsStar() ? Radar::SPECIAL :
				!object.GetPlanet() ? Radar::INACTIVE :
				object.GetPlanet()->IsWormhole() ? Radar::ANOMALOUS :
				object.GetPlanet()->CanLand() ? Radar::FRIENDLY : Radar::HOSTILE;
			double r = max(2., object.Radius() * .03 + .5);
			
			// Don't apply motion blur to very large planets and stars.
			bool isBig = (object.GetSprite().Width() >= 280);
			draw[calcTickTock].Add(object.GetSprite(), position, unit, isBig ? Point() : -centerVelocity);
			radar[calcTickTock].Add(type, position, r, r - 1.);
		}
	
	// Add all neighboring systems to the radar.
	const System *targetSystem = flagship ? flagship->GetTargetSystem() : nullptr;
	for(const System *system : player.GetSystem()->Links())
		radar[calcTickTock].AddPointer(
			(system == targetSystem) ? Radar::SPECIAL : Radar::INACTIVE,
			system->Position() - player.GetSystem()->Position());
	
	// Now that the planets have been drawn, we can draw the asteroids on top
	// of them. This could be done later, as long as it is done before the
	// collision detection.
	asteroids.Step();
	asteroids.Draw(draw[calcTickTock], center, centerVelocity);
	
	// Move existing projectiles. Do this before ships fire, which will create
	// new projectiles, since those should just stay where they are created for
	// this turn. This is also where projectiles get deleted, which may also
	// result in a "die" effect or a sub-munition being created. We could not
	// move the projectiles before this because some of them are homing and need
	// to know the current positions of the ships.
	std::list<Projectile> newProjectiles;
	for(auto it = projectiles.begin(); it != projectiles.end(); )
	{
		if(!it->Move(effects))
		{
			it->MakeSubmunitions(newProjectiles);
			it = projectiles.erase(it);
		}
		else
			++it;
	}
	projectiles.splice(projectiles.end(), newProjectiles);
	
	// Keep track of the relative strength of each government in this sytem. Do
	// not add more ships to make a winning team even stronger. This is mostly
	// to avoid having the player get mobbed by pirates, say, if they hang out
	// in one system for too long.
	map<const Government *, int64_t> strength;
	// Now, ships fire new projectiles, which includes launching fighters. If an
	// anti-missile system is ready to fire, it does not actually fire unless a
	// missile is detected in range during collision detection, below.
	vector<Ship *> hasAntiMissile;
	for(shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == player.GetSystem())
		{
			strength[ship->GetGovernment()] += ship->Cost();
			
			// Note: if a ship "fires" a fighter, that fighter was already in
			// existence and under the control of the same AI as the ship, but
			// its system was null to mark that it was not active.
			ship->Launch(ships);
			if(ship->Fire(projectiles))
				hasAntiMissile.push_back(ship.get());
			
			// Boarding:
			bool autoPlunder = !ship->GetGovernment()->IsPlayer();
			shared_ptr<Ship> victim = ship->Board(autoPlunder);
			if(victim)
				eventQueue.emplace_back(ship, victim,
					ship->GetGovernment()->IsEnemy(victim->GetGovernment()) ?
						ShipEvent::BOARD : ShipEvent::ASSIST);
			
			int scan = ship->Scan();
			if(scan)
			{
				shared_ptr<Ship> target = ship->GetTargetShip();
				if(target && target->IsTargetable())
					eventQueue.emplace_back(ship, target, scan);
			}
			
			// This is a good opportunity to draw all the ships in system.
			if(ship->GetSprite().IsEmpty())
				continue;
			
			Point position = ship->Position() - center;
			
			// EnginePoints() returns empty if there is no flare sprite, or if
			// the ship is not thrusting right now.
			for(const Point &point : ship->EnginePoints())
			{
				Point pos = ship->Facing().Rotate(point) * .5 * ship->Zoom() + position;
				if(ship->Cloaking())
				{
					draw[calcTickTock].Add(
						ship->FlareSprite().GetSprite(),
						pos,
						ship->Unit(),
						ship->Velocity() - centerVelocity,
						ship->Cloaking());
				}
				else
				{
					draw[calcTickTock].Add(
						ship->FlareSprite(),
						pos,
						ship->Unit(),
						ship->Velocity() - centerVelocity);
				}
				if(ship.get() == flagship && ship->Attributes().FlareSound())
					Audio::Play(ship->Attributes().FlareSound(), pos, ship->Velocity());
			}
			
			bool isPlayer = ship->GetGovernment()->IsPlayer();
			if(ship->Cloaking())
			{
				if(isPlayer)
				{
					Animation animation = ship->GetSprite();
					animation.SetSwizzle(7);
					draw[calcTickTock].Add(
						animation,
						position,
						ship->Unit(),
						ship->Velocity() - centerVelocity);
				}
				draw[calcTickTock].Add(
					ship->GetSprite().GetSprite(),
					position,
					ship->Unit(),
					ship->Velocity() - centerVelocity,
					ship->Cloaking(),
					ship->GetSprite().GetSwizzle());
			}
			else
			{
				draw[calcTickTock].Add(
					ship->GetSprite(),
					position,
					ship->Unit(),
					ship->Velocity() - centerVelocity);
			}
			
			// Do not show cloaked ships on the radar, except the player's ships.
			if(ship->Cloaking() == 1. && !isPlayer)
				continue;
			
			auto target = ship->GetTargetShip();
			radar[calcTickTock].Add(
				(ship->GetGovernment()->IsPlayer() || ship->GetPersonality().IsEscort()) ? Radar::PLAYER :
					(ship->IsDisabled() || ship->IsOverheated()) ? Radar::INACTIVE :
					!ship->GetGovernment()->IsEnemy() ? Radar::FRIENDLY :
					(target && target->GetGovernment()->IsPlayer()) ?
						Radar::HOSTILE : Radar::UNFRIENDLY,
				position,
				sqrt(ship->GetSprite().Width() + ship->GetSprite().Height()) * .1 + .5);
		}
	
	// Collision detection:
	for(Projectile &projectile : projectiles)
	{
		// The asteroids can collide with projectiles, the same as any other
		// object. If the asteroid turns out to be closer than the ship, it
		// shields the ship (unless the projectile has  blast radius).
		Point hitVelocity;
		double closestHit = 0.;
		shared_ptr<Ship> hit;
		const Government *gov = projectile.GetGovernment();
		
		// If this "projectile" is a ship explosion, it always explodes.
		if(gov)
		{
			closestHit = asteroids.Collide(projectile, step, &hitVelocity);
			// Projectiles can only collide with ships that are in the current
			// system and are not landing, and that are hostile to this projectile.
			for(shared_ptr<Ship> &ship : ships)
				if(ship->GetSystem() == player.GetSystem() && !ship->IsLanding() && ship->Cloaking() < 1.)
				{
					if(ship.get() != projectile.Target() && !gov->IsEnemy(ship->GetGovernment()))
						continue;
					
					// This returns a value of 0 if the projectile has a trigger
					// radius and the ship is within it.
					double range = projectile.CheckCollision(*ship, step);
					if(range < closestHit)
					{
						closestHit = range;
						hit = ship;
						hitVelocity = ship->Velocity();
					}
				}
		}
		
		if(closestHit < 1.)
		{
			// Create the explosion the given distance along the projectile's
			// motion path for this step.
			projectile.Explode(effects, closestHit, hitVelocity);
			
			// If this projectile has a blast radius, find all ships within its
			// radius. Otherwise, only one is damaged.
			if(projectile.HasBlastRadius())
			{
				// Even friendly ships can be hit by the blast.
				for(shared_ptr<Ship> &ship : ships)
					if(ship->GetSystem() == player.GetSystem() && ship->Zoom() == 1.)
						if(projectile.InBlastRadius(*ship, step))
						{
							int eventType = ship->TakeDamage(projectile, ship != hit);
							if(eventType)
								eventQueue.emplace_back(
									projectile.GetGovernment(), ship, eventType);
						}
			}
			else if(hit)
			{
				int eventType = hit->TakeDamage(projectile);
				if(eventType)
					eventQueue.emplace_back(
						projectile.GetGovernment(), hit, eventType);
			}
			
			if(hit)
				DoGrudge(hit, projectile.GetGovernment());
		}
		else if(projectile.MissileStrength())
		{
			radar[calcTickTock].Add(
				Radar::SPECIAL, projectile.Position() - center, 1.);
			
			// If the projectile did not hit anything, give the anti-missile
			// systems a chance to shoot it down.
			for(Ship *ship : hasAntiMissile)
				if(ship == projectile.Target()
						|| gov->IsEnemy(ship->GetGovernment())
						|| ship->GetGovernment()->IsEnemy(gov))
					if(ship->FireAntiMissile(projectile, effects))
					{
						projectile.Kill();
						break;
					}
		}
		else if(projectile.HasBlastRadius())
			radar[calcTickTock].Add(
				Radar::SPECIAL, projectile.Position() - center, 1.8);
		
		// Now, we can draw the projectile. The motion blur should be reduced
		// depending on how much motion blur is in the sprite itself:
		double innateVelocity = 2. * projectile.GetWeapon().Velocity();
		Point relativeVelocity = projectile.Velocity() - centerVelocity
			- projectile.Unit() * innateVelocity;
		draw[calcTickTock].Add(
			projectile.GetSprite(),
			projectile.Position() - center + .5 * projectile.Velocity(),
			projectile.Unit(),
			relativeVelocity,
			closestHit);
	}
	
	// Finally, draw all the effects, and then move them (because their motion
	// is not dependent on anything else, and this way we do all the work on
	// them in a single place.
	for(auto it = effects.begin(); it != effects.end(); )
	{
		draw[calcTickTock].Add(
			it->GetSprite(),
			it->Position() - center,
			it->Unit());
		
		if(!it->Move())
			it = effects.erase(it);
		else
			++it;
	}
	
	// Add incoming ships.
	for(const System::FleetProbability &fleet : player.GetSystem()->Fleets())
		if(!Random::Int(fleet.Period()))
		{
			const Government *gov = fleet.Get()->GetGovernment();
			if(!gov)
				continue;
			
			int64_t enemyStrength = 0;
			for(const auto &it : strength)
				if(gov->IsEnemy(it.first))
					enemyStrength += it.second;
			if(enemyStrength && strength[gov] > 2 * enemyStrength)
				continue;
			
			fleet.Get()->Enter(*player.GetSystem(), ships);
		}
	
	// Occasionally have some ship hail you.
	if(!Random::Int(600) && !ships.empty())
	{
		shared_ptr<Ship> source;
		unsigned i = Random::Int(ships.size());
		for(const shared_ptr<Ship> &it : ships)
			if(!i--)
			{
				source = it;
				break;
			}
		if(source->GetGovernment() && !source->GetGovernment()->IsPlayer() && !source->IsDisabled())
		{
			string message = source->GetGovernment()->GetHail();
			if(!message.empty() && source->GetSystem() == player.GetSystem())
				Messages::Add(source->GetGovernment()->GetName() + " ship \""
					+ source->Name() + "\": " + message);
		}
	}
	
	// Keep track of how much of the CPU time we are using.
	loadSum += loadTimer.Time();
	if(++loadCount == 60)
	{
		load = loadSum;
		loadSum = 0.;
		loadCount = 0;
	}
}



void Engine::DoGrudge(const shared_ptr<Ship> &target, const Government *attacker)
{
	if(attacker->IsPlayer())
	{
		shared_ptr<const Ship> previous = grudge[target->GetGovernment()].lock();
		if(previous && previous->GetSystem() == player.GetSystem() && !previous->IsDisabled())
		{
			grudge[target->GetGovernment()].reset();
			Messages::Add(previous->GetGovernment()->GetName() + " ship \""
				+ previous->Name() + "\": Thank you for your assistance, Captain "
				+ player.LastName() + "!");
		}
		return;
	}
	
	// Check who currently has a grudge against this government. Also check if
	// someone has already said "thank you" today.
	if(grudge.find(attacker) != grudge.end())
	{
		shared_ptr<const Ship> previous = grudge[attacker].lock();
		if(!previous || (previous->GetSystem() == player.GetSystem() && !previous->IsDisabled()))
			return;
	}
	
	// Do not ask the player's help if they are your enemy or are not an enemy
	// of the ship that is attacking you.
	if(target->GetGovernment()->IsPlayer())
		return;
	if(!attacker->IsEnemy())
		return;
	if(target->GetGovernment()->IsEnemy())
		return;
	
	// No active ship has a grudge already against this government.
	// Check the relative strength of this ship and its attackers.
	double targetStrength = (target->Shields() + target->Hull()) * target->Cost();
	double attackerStrength = 0.;
	int attackerCount = 0;
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetGovernment() == attacker && ship->GetTargetShip() == target)
		{
			++attackerCount;
			attackerStrength += (ship->Shields() + ship->Hull()) * ship->Cost();
		}
	
	if(attackerStrength <= targetStrength)
		return;
	
	// Ask for help more frequently if the battle is very lopsided.
	double ratio = attackerStrength / targetStrength - 1.;
	if(Random::Real() * 10. > ratio)
		return;
	
	grudge[attacker] = target;
	string message = target->GetGovernment()->GetName() + " ship \"" + target->Name() + "\": ";
	if(target->GetPersonality().IsHeroic())
	{
		message += "Please assist us in destroying ";
		message += (attackerCount == 1 ? "this " : "these ");
		message += attacker->GetName();
		message += (attackerCount == 1 ? " ship." : " ships.");
	}
	else
	{
		message += "We are under attack by ";
		if(attackerCount == 1)
			message += "a ";
		message += attacker->GetName();
		message += (attackerCount == 1 ? " ship" : " ships");
		message += ". Please assist us!";
	}
	Messages::Add(message);
}



Engine::Status::Status(const Point &position, double shields, double hull, double radius, bool isEnemy)
	: position(position), shields(shields), hull(hull), radius(radius), isEnemy(isEnemy)
{
}
