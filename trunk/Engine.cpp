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

#include "DotShader.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "FrameTimer.h"
#include "PointerShader.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

using namespace std;



Engine::Engine(const GameData &data, PlayerInfo &playerInfo)
	: data(data), playerInfo(playerInfo),
	playerGovernment(data.Governments().Get("Escort")),
	calcTickTock(false), drawTickTock(false), terminate(false), step(0),
	asteroids(data),
	load(0.), loadCount(0), loadSum(0.)
{
	// Start the thread for doing calculations.
	calcThread = thread(&Engine::ThreadEntryPoint, this);
	
	if(!playerInfo.IsLoaded() || !playerInfo.GetSystem())
		return;
	playerInfo.GetSystem()->SetDate(playerInfo.GetDate());
	
	// Now we know the player's current position. Draw the planets.
	Point center;
	if(playerInfo.GetPlanet())
	{
		for(const StellarObject &object : playerInfo.GetSystem()->Objects())
			if(object.GetPlanet() == playerInfo.GetPlanet())
				center = object.Position();
	}
	for(const StellarObject &object : playerInfo.GetSystem()->Objects())
		if(!object.GetSprite().IsEmpty())
		{
			Point position = object.Position();
			Point unit(0., -1.);
			if(position)
				unit = position.Unit();
			position -= center;
			
			int type = object.IsStar() ? Radar::SPECIAL :
				object.GetPlanet() ? Radar::FRIENDLY : Radar::INACTIVE;
			double r = max(2., object.Radius() * .03 + .5);
			
			draw[calcTickTock].Add(object.GetSprite(), position, unit);
			radar[calcTickTock].Add(type, position, r, r - 1.);
		}
	
	// Add all neighboring systems to the radar.
	Point pos = playerInfo.GetSystem()->Position();
	for(const System *system : playerInfo.GetSystem()->Links())
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
	for(const shared_ptr<Ship> &ship : playerInfo.Ships())
	{
		ships.push_back(ship);
		Point pos;
		Angle angle = Angle::Random(360.);
		// All your ships that are in system with the player act as if they are
		// leaving the planet along with you.
		if(playerInfo.GetPlanet() && ship->GetSystem() == playerInfo.GetSystem())
		{
			ship->SetPlanet(playerInfo.GetPlanet());
			for(const StellarObject &object : ship->GetSystem()->Objects())
				if(object.GetPlanet() == playerInfo.GetPlanet())
				{
					pos = object.Position();
					int radius = static_cast<int>(object.Radius());
					if(radius)
						pos += angle.Unit() * (rand() % radius);
				}
		}
		ship->Place(pos, angle.Unit(), angle);
	}
	
	playerInfo.SetPlanet(nullptr);
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
		
		// The calculation thread is now paused, so it is safe to access things.
		const Ship *player = playerInfo.GetShip();
		if(player)
		{
			position = player->Position();
			velocity = player->Velocity();
		}
		ai.UpdateKeys(data.Keys().State(), &playerInfo);
		if(!ai.Message().empty())
			AddMessage(ai.Message());
		
		// Any of the player's ships that are in system are assumed to have
		// landed along with the player.
		if(player && player->GetPlanet() && isActive)
		{
			playerInfo.SetPlanet(player->GetPlanet());
			
			for(const shared_ptr<Ship> &ship : playerInfo.Ships())
				if(ship->GetSystem() == player->GetSystem())
					ship->Recharge();
		}
		
		const System *currentSystem = playerInfo.GetSystem();
		// Update this here, for thread safety.
		if(playerInfo.HasTravelPlan() && currentSystem == playerInfo.TravelPlan().back())
			playerInfo.PopTravel();
		
		targets.clear();
		
		// Update the player's ammo amounts.
		ammo.clear();
		if(player)
			for(const auto &it : player->Outfits())
				if(it.first->Ammo())
					ammo.emplace_back(it.first, player->OutfitCount(it.first->Ammo()));
		
		if(player && player->Hull())
			info.SetSprite("player sprite", player->GetSprite().GetSprite());
		else
			info.SetSprite("player sprite", nullptr);
		info.SetString("location", currentSystem->Name());
		info.SetString("date", playerInfo.GetDate().ToString());
		if(player)
		{
			info.SetBar("fuel", player->Fuel());
			info.SetBar("energy", player->Energy());
			info.SetBar("heat", player->Heat());
			info.SetBar("shields", player->Shields());
			info.SetBar("hull", player->Hull(), 20.);
		}
		else
		{
			info.SetBar("fuel", 0.);
			info.SetBar("energy", 0.);
			info.SetBar("heat", 0.);
			info.SetBar("shields", 0.);
			info.SetBar("hull", 0.);
		}
		info.SetString("credits", to_string(playerInfo.Accounts().Credits()) + " credits");
		if(player && player->GetTargetPlanet())
		{
			info.SetString("navigation mode", "Landing on:");
			info.SetString("destination", player->GetTargetPlanet()->Name());
			
			targets.push_back({
				player->GetTargetPlanet()->Position() - player->Position(),
				Angle(45.),
				player->GetTargetPlanet()->Radius(),
				Radar::FRIENDLY});
		}
		else if(player && player->GetTargetSystem())
		{
			info.SetString("navigation mode", "Hyperspace:");
			if(playerInfo.HasVisited(player->GetTargetSystem()))
				info.SetString("destination", player->GetTargetSystem()->Name());
			else
				info.SetString("destination", "unexplored system");
		}
		else
		{
			info.SetString("navigation mode", "Navigation:");
			info.SetString("destination", "no destination");
		}
		info.SetRadar(radar[drawTickTock]);
		shared_ptr<const Ship> target;
		if(player)
			target = player->GetTargetShip().lock();
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
			if(target->GetSystem() == player->GetSystem())
			{
				info.SetBar("target shields", target->Shields());
				info.SetBar("target hull", target->Hull(), 20.);
			
				// The target area will be a square, with sides equal to the average
				// of the width and the height of the sprite.
				const Animation &anim = target->GetSprite();
				double size = target->Zoom() * (anim.Width() + anim.Height()) * .175;
				targets.push_back({
					target->Position() - player->Position(),
					Angle(45.) + target->Facing(),
					size,
					target->IsDisabled() ? Radar::INACTIVE : Radar::FRIENDLY});
			}
			else
			{
				info.SetBar("target shields", 0.);
				info.SetBar("target hull", 0.);
			}
		}
		
		// Begin the next frame's calculations.
		if(isActive)
			calcTickTock = !calcTickTock;
	}
	if(isActive)
		condition.notify_one();
}


// Draw a frame.
void Engine::Draw() const
{
	data.Background().Draw(position, velocity);
	draw[drawTickTock].Draw();
	
	// Draw messages.
	const Font &font = FontSet::Get(14);
	while(!messages.empty() && messages.front().first < step - 1000)
		messages.erase(messages.begin());
	Point messagePoint(
		Screen::Width() * -.5 + 20.,
		Screen::Height() * .5 - 20. * messages.size());
	for(const auto &it : messages)
	{
		float alpha = (it.first + 1000 - step) * .001f;
		float color[4] = {alpha, alpha, alpha, 0.};
		font.Draw(it.second, messagePoint, color);
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
	
	data.Interfaces().Get("status")->Draw(info);
	data.Interfaces().Get("targets")->Draw(info);
	
	// Draw ammo status.
	Point pos(Screen::Width() / 2 - 80, Screen::Height() / 2);
	const Sprite *selectedSprite = SpriteSet::Get("ui/ammo selected");
	const Sprite *unselectedSprite = SpriteSet::Get("ui/ammo unselected");
	Color selectedColor(.8, 0.);
	Color unselectedColor(.3, 0.);
	for(const pair<const Outfit *, int> &it : ammo)
	{
		pos.Y() -= 30.;
		
		bool isSelected = it.first == playerInfo.SelectedWeapon();
		
		SpriteShader::Draw(it.first->Icon(), pos);
		SpriteShader::Draw(
			isSelected ? selectedSprite : unselectedSprite, pos + Point(35., 0.));
		
		string amount = to_string(it.second);
		Point textPos = pos + Point(55 - font.Width(amount), -(30 - font.Height()) / 2);
		font.Draw(amount, textPos, (isSelected ? selectedColor : unselectedColor).Get());
	}
	
	if(data.ShouldShowLoad())
	{
		string loadString = to_string(static_cast<int>(load * 100. + .5)) + "% CPU";
		const float color[4] = {.4f, .4f, .4f, 0.f};
		FontSet::Get(14).Draw(loadString, Point(0., Screen::Height() * -.5 + 20.), color);
	}
}



void Engine::EnterSystem()
{
	const Ship *player = playerInfo.GetShip();
	if(!player)
		return;
	
	playerInfo.SetSystem(player->GetSystem());
	
	AddMessage(playerInfo.IncrementDate());
	const Date &today = playerInfo.GetDate();
	AddMessage("Entering the " + player->GetSystem()->Name() + " system on "
		+ today.ToString() + (player->GetSystem()->IsInhabited() ?
			"." : ". No inhabited planets detected."));
	
	for(const auto &it : data.Systems())
		it.second.SetDate(today);
	
	asteroids.Clear();
	for(const System::Asteroid &a : player->GetSystem()->Asteroids())
		asteroids.Add(a.Name(), a.Count(), a.Energy());
	
	projectiles.clear();
	effects.clear();
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
			drawTickTock = calcTickTock;
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
	
	// Now, all the ships must decide what they are doing next.
	ai.Step(ships, playerInfo);
	
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
	
	// If the player has entered a new system, update the asteroids, etc.
	const Ship *player = playerInfo.GetShip();
	if(player && player->GetSystem() != playerInfo.GetSystem())
		EnterSystem();
	
	// Now we know the player's current position. Draw the planets.
	Point center;
	if(player)
		center = player->Position();
	else if(playerInfo.GetPlanet())
	{
		for(const StellarObject &object : playerInfo.GetSystem()->Objects())
			if(object.GetPlanet() == playerInfo.GetPlanet())
				center = object.Position();
	}
	for(const StellarObject &object : playerInfo.GetSystem()->Objects())
		if(!object.GetSprite().IsEmpty())
		{
			Point position = object.Position();
			Point unit(0., -1.);
			if(position)
				unit = position.Unit();
			position -= center;
			
			int type = object.IsStar() ? Radar::SPECIAL :
				object.GetPlanet() ? Radar::FRIENDLY : Radar::INACTIVE;
			double r = max(2., object.Radius() * .03 + .5);
			
			draw[calcTickTock].Add(object.GetSprite(), position, unit);
			radar[calcTickTock].Add(type, position, r, r - 1.);
		}
	
	// Add all neighboring systems to the radar.
	const System *targetSystem = player ? player->GetTargetSystem() : nullptr;
	for(const System *system : playerInfo.GetSystem()->Links())
		radar[calcTickTock].AddPointer(
			(system == targetSystem) ? Radar::SPECIAL : Radar::INACTIVE,
			system->Position() - playerInfo.GetSystem()->Position());
	
	// Now that the planets have been drawn, we can draw the asteroids on top
	// of them. This could be done later, as long as it is done before the
	// collision detection.
	asteroids.Step();
	asteroids.Draw(draw[calcTickTock], center);
	
	// Move existing projectiles. Do this before ships fire, which will create
	// new projectiles, since those should just stay where they are created for
	// this turn. This is also where projectiles get deleted, which may also
	// result in a "die" effect or a sub-munition being created. We could not
	// move the projectiles before this because some of them are homing and need
	// to know the current positions of the ships.
	for(auto it = projectiles.begin(); it != projectiles.end(); )
	{
		if(!it->Move(effects))
		{
			it->MakeSubmunitions(projectiles);
			it = projectiles.erase(it);
		}
		else
			++it;
	}
	
	// Now, ships fire new projectiles, which includes launching fighters. If an
	// anti-missile system is ready to fire, it does not actually fire unless a
	// missile is detected in range during collision detection, below.
	vector<Ship *> hasAntiMissile;
	for(shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem() == playerInfo.GetSystem())
		{
			// Note: if a ship "fires" a fighter, that fighter was already in
			// existence and under the control of the same AI as the ship, but
			// its system was null to mark that it was not active.
			ship->Launch(ships);
			if(ship->Fire(projectiles))
				hasAntiMissile.push_back(ship.get());
		
			// This is a good opportunity to draw all the ships in system.
			if(ship->GetSprite().IsEmpty())
				continue;
			
			Point position = ship->Position() - center;
			
			// EnginePoints() returns empty if there is no flare sprite, or if
			// the ship is not thrusting right now.
			for(const Point &point : ship->EnginePoints())
				draw[calcTickTock].Add(
					ship->FlareSprite(),
					ship->Facing().Rotate(point) * .5 * ship->Zoom() + position,
					ship->Unit());
			
			draw[calcTickTock].Add(
				ship->GetSprite(),
				position,
				ship->Unit());
			
			auto target = ship->GetTargetShip().lock();
			radar[calcTickTock].Add(
				ship->GetGovernment() == playerGovernment ? Radar::PLAYER :
					ship->IsDisabled() ? Radar::INACTIVE :
					(target && target->GetGovernment() == playerGovernment) ? Radar::HOSTILE :
					ship->GetGovernment()->IsEnemy(playerGovernment) ?
						Radar::UNFRIENDLY : Radar::FRIENDLY,
				position,
				(ship->GetSprite().Width() + ship->GetSprite().Height()) * .005 + .5);
		}
	
	// Collision detection:
	for(Projectile &projectile : projectiles)
	{
		// The asteroids can collide with projectiles, the same as any other
		// object. If the asteroid turns out to be closer than the ship, it
		// shields the ship (unless the projectile has  blast radius).
		double closestHit = asteroids.Collide(projectile, step);
		Ship *hit = nullptr;
		const Government *gov = projectile.GetGovernment();
		
		// Projectiles can only collide with ships that are in the current
		// system and are not landing, and that are hostile to this projectile.
		for(shared_ptr<Ship> &ship : ships)
			if(ship->GetSystem() == playerInfo.GetSystem() && !ship->IsLanding())
			{
				if(ship.get() != projectile.Target() && !gov->IsEnemy(ship->GetGovernment()))
					continue;
				
				// This returns a value of 0 if the projectile has a trigger
				// radius and the ship is within it.
				double range = projectile.CheckCollision(*ship, step);
				if(range < closestHit)
				{
					closestHit = range;
					hit = ship.get();
				}
			}
		
		if(closestHit < 1.)
		{
			// Create the explosion the given distance along the projectile's
			// motion path for this step.
			projectile.Explode(effects, closestHit);
			
			// If this projectile has a blast radius, find all ships within its
			// radius. Otherwise, only one is damaged.
			if(projectile.HasBlastRadius())
			{
				// Even friendly ships can be hit by the blast.
				for(shared_ptr<Ship> &ship : ships)
					if(ship->GetSystem() == playerInfo.GetSystem() && !ship->IsLanding())
						if(projectile.InBlastRadius(*ship, step))
							ship->TakeDamage(projectile);
			}
			else if(hit)
				hit->TakeDamage(projectile);
			
			// Whatever ship you hit directly is provoked against you.
			if(hit)
				hit->GetGovernment()->Provoke(gov);
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
		
		// Now, we can draw the projectile.
		draw[calcTickTock].Add(
			projectile.GetSprite(),
			projectile.Position() - center + .5 * projectile.Velocity(),
			projectile.Unit(),
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
	if(!(rand() % 100))
	{
		int index = rand() % playerInfo.GetSystem()->Links().size();
		const System *source = playerInfo.GetSystem()->Links()[index];
		
		int type = rand() % data.Ships().size();
		for(const auto &it : data.Ships())
			if(!type--)
				ships.push_front(shared_ptr<Ship>(new Ship(it.second)));
		
		ships.front()->Place();
		ships.front()->SetSystem(source);
		ships.front()->SetTargetSystem(playerInfo.GetSystem());
		static const std::string GOV[4] = {
			"Merchant",
			"Republic",
			"Militia",
			"Pirate"};
		ships.front()->SetGovernment(data.Governments().Get(GOV[rand() % 4]));
		ships.front()->SetName(data.ShipNames().Get("civilian")->Get());
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



void Engine::AddMessage(const string &message)
{
	for(auto it = messages.begin(); it != messages.end(); )
	{
		if(it->second == message)
			messages.erase(it);
		else
			++it;
	}
	messages.emplace_back(step, message);
}
