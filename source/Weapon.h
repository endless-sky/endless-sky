/* Weapon.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Angle.h"
#include "Body.h"
#include "Distribution.h"
#include "Point.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

class DataNode;
class Effect;
class Outfit;
class Sound;
class Sprite;



// Class representing all the characteristics of a weapon, including sprites and
// effects, sounds, icons, ammo, submunitions, and other attributes. Storing
// these parameters in a separate class keeps each Projectile from needing a
// copy of them, and storing them as class variables instead of in a map of
// string to double significantly reduces access time.
class Weapon {
	friend Outfit;
public:
	struct Submunition {
		explicit Submunition(const std::shared_ptr<const Weapon> &weapon, std::size_t count) noexcept
			: weapon(weapon), count(count) {};

		const std::shared_ptr<const Weapon> &weapon;
		std::size_t count = 0;
		// The angular offset from the source projectile, relative to its current facing.
		Angle facing;
		// The base offset from the source projectile's position, relative to its current facing.
		Point offset;

		bool spawnOnNaturalDeath = true;
		bool spawnOnAntiMissileDeath = false;
	};


public:
	Weapon() = default;
	// Construct and load at the same time.
	explicit Weapon(const DataNode &node);
	// Load from a "weapon" node, either in an outfit, a ship (explosion), or a hazard.
	void Load(const DataNode &node);
	bool IsLoaded() const;

	// Get assets used by this weapon.
	const Body &WeaponSprite() const;
	const Body &HardpointSprite() const;
	const Sound *WeaponSound() const;
	const Sound *EmptySound() const;
	const Sprite *Icon() const;

	// Effects to be created at the start or end of the weapon's lifetime.
	const std::map<const Effect *, int> &FireEffects() const;
	const std::map<const Effect *, int> &LiveEffects() const;
	const std::map<const Effect *, int> &HitEffects() const;
	const std::map<const Effect *, int> &TargetEffects() const;
	const std::map<const Effect *, int> &DieEffects() const;
	const std::vector<Submunition> &Submunitions() const;

	// Accessor functions for various attributes.
	int Lifetime() const;
	int RandomLifetime() const;
	int FadeOut() const;
	double Reload() const;
	double BurstReload() const;
	int BurstCount() const;

	const Outfit *Ammo() const;
	int AmmoUsage() const;

	int MissileStrength() const;
	int AntiMissile() const;
	double TractorBeam() const;
	uint16_t PenetrationCount() const noexcept;
	// Weapons of the same type will alternate firing (streaming) rather than
	// firing all at once (clustering) if the weapon is not an anti-missile and
	// is not vulnerable to anti-missile, or has the "stream" attribute.
	bool IsStreamed() const;
	bool IsParallel() const;

	double Velocity() const;
	double RandomVelocity() const;
	double WeightedVelocity() const;
	double Acceleration() const;
	double Drag() const;
	const Point &HardpointOffset() const;

	double Turn() const;
	double Inaccuracy() const;
	std::pair<Distribution::Type, bool> InaccuracyDistribution() const;
	double TurretTurn() const;
	double Arc() const;

	double Tracking() const;
	double OpticalTracking() const;
	double InfraredTracking() const;
	double RadarTracking() const;

	// Normal damage sustained on firing ship when weapon fired.
	double FiringEnergy() const;
	double FiringForce() const;
	double FiringFuel() const;
	double FiringHeat() const;
	double FiringHull() const;
	double FiringShields() const;
	double FiringIon() const;
	double FiringScramble() const;
	double FiringSlowing() const;
	double FiringDisruption() const;
	double FiringDischarge() const;
	double FiringCorrosion() const;
	double FiringLeak() const;
	double FiringBurn() const;

	// Relative damage sustained on firing ship when weapon fired.
	double RelativeFiringEnergy() const;
	double RelativeFiringHeat() const;
	double RelativeFiringFuel() const;
	double RelativeFiringHull() const;
	double RelativeFiringShields() const;

	double SplitRange() const;
	double TriggerRadius() const;
	double BlastRadius() const;
	double SafeRange() const;
	double HitForce() const;

	// A "safe" weapon hits only hostile ships (even if it has a blast radius).
	// A "phasing" weapon hits only its intended target; it passes through
	// everything else, including asteroids.
	bool IsSafe() const;
	bool IsPhasing() const;
	// Blast radius weapons will scale damage and hit force based on distance,
	// unless the "no damage scaling" keyphrase is used in the weapon definition.
	bool IsDamageScaled() const;
	// Gravitational weapons deal the same amount of hit force to a ship regardless
	// of its mass.
	bool IsGravitational() const;
	// True if this projectile should create an explosion at the end of its lifetime
	// instead of simply disappearing or only creating a die effect. Blast radius
	// weapons will cause a blast at the end of their lifetime.
	bool IsFused() const;
	// Whether projectiles from this weapon can directly collide with objects.
	bool CanCollideShips() const;
	bool CanCollideAsteroids() const;
	bool CanCollideMinables() const;
	// Attributes that determine how projectiles from this weapon home onto targets.
	bool Homing() const;
	bool HasBlindspot() const;
	bool ThrottleControl() const;
	bool Leading() const;

	// These values include all submunitions:
	// Normal damage types:
	double ShieldDamage() const;
	double HullDamage() const;
	double DisabledDamage() const;
	double MinableDamage() const;
	double FuelDamage() const;
	double HeatDamage() const;
	double EnergyDamage() const;
	// Status effects:
	double IonDamage() const;
	double ScramblingDamage() const;
	double DisruptionDamage() const;
	double SlowingDamage() const;
	double DischargeDamage() const;
	double CorrosionDamage() const;
	double LeakDamage() const;
	double BurnDamage() const;
	// Relative damage types:
	double RelativeShieldDamage() const;
	double RelativeHullDamage() const;
	double RelativeDisabledDamage() const;
	double RelativeMinableDamage() const;
	double RelativeFuelDamage() const;
	double RelativeHeatDamage() const;
	double RelativeEnergyDamage() const;
	// Check if this weapon does damage. If not, attacking a ship with this
	// weapon is not a provocation (even if you push or pull it).
	bool DoesDamage() const;

	bool ConsumesHull() const;
	bool ConsumesFuel() const;
	bool ConsumesHeat() const;
	bool ConsumesEnergy() const;
	bool ConsumesIonization() const;
	bool ConsumesDisruption() const;
	bool ConsumesSlowing() const;

	double Piercing() const;

	double Prospecting() const;

	double TotalLifetime() const;
	double Range() const;

	// Check if this weapon has a damage dropoff range.
	bool HasDamageDropoff() const;
	// Calculate the percent damage that this weapon deals given the distance
	// that the projectile traveled if it has a damage dropoff range.
	double DamageDropoff(double distance) const;
	// Return the weapon's damage dropoff at maximum range.
	double MaxDropoff() const;
	// Return the ranges at which the weapon's damage dropoff begins and ends.
	const std::pair<double, double> &DropoffRanges() const;


private:
	double TotalDamage(int index) const;


private:
	bool isLoaded = false;

	// Sprites and sounds.
	Body sprite;
	Body hardpointSprite;
	const Sound *sound = nullptr;
	const Sound *emptySound = nullptr;
	const Sprite *icon = nullptr;

	// Fire, die and hit effects.
	std::map<const Effect *, int> fireEffects;
	std::map<const Effect *, int> liveEffects;
	std::map<const Effect *, int> hitEffects;
	std::map<const Effect *, int> targetEffects;
	std::map<const Effect *, int> dieEffects;
	std::vector<Submunition> submunitions;

	bool isStreamed = false;
	bool isSafe = false;
	bool isPhasing = false;
	bool isDamageScaled = true;
	bool isGravitational = false;
	bool isFused = false;
	bool canCollideShips = true;
	bool canCollideAsteroids = true;
	bool canCollideMinables = true;
	// Guns and missiles are by default aimed a converged point at the
	// maximum weapons range in front of the ship. When either the installed
	// weapon or the gun-port (or both) have the isParallel attribute set
	// to true, then this convergence will not be used and the weapon will
	// be aimed directly in the gunport angle/direction.
	bool isParallel = false;
	// Attributes for homing.
	bool homing = false;
	bool blindspot = false;
	bool throttleControl = false;
	bool leading = false;

	// Attributes.
	int lifetime = 0;
	int randomLifetime = 0;
	int fadeOut = 0;
	double reload = 1.;
	double burstReload = 1.;
	int burstCount = 1;

	// A pair representing the outfit that is consumed as ammo and the number
	// of that outfit consumed upon fire.
	std::pair<const Outfit *, int> ammo;

	int missileStrength = 0;
	int antiMissile = 0;
	double tractorBeam = 0.;
	// Use of an unsigned integer allows explicit penetration count values of 0
	// to result in a projectile that will hit 65k targets before being destroyed
	// (which is effectively infinite under any reasonable balance).
	uint16_t penetrationCount = 1U;

	double velocity = 0.;
	double randomVelocity = 0.;
	double acceleration = 0.;
	double drag = 0.;
	Point hardpointOffset = {0., 0.};

	double turn = 0.;
	double inaccuracy = 0.;
	// A pair representing the distribution type of this weapon's inaccuracy
	// and whether it is inverted
	std::pair<Distribution::Type, bool> inaccuracyDistribution = {Distribution::Type::Triangular, false};
	double turretTurn = 0.;
	double maxAngle = 360.;

	double tracking = 0.;
	double opticalTracking = 0.;
	double infraredTracking = 0.;
	double radarTracking = 0.;

	double firingEnergy = 0.;
	double firingForce = 0.;
	double firingFuel = 0.;
	double firingHeat = 0.;
	double firingHull = 0.;
	double firingShields = 0.;
	double firingIon = 0.;
	double firingScramble = 0.;
	double firingSlowing = 0.;
	double firingDisruption = 0.;
	double firingDischarge = 0.;
	double firingCorrosion = 0.;
	double firingLeak = 0.;
	double firingBurn = 0.;

	double relativeFiringEnergy = 0.;
	double relativeFiringHeat = 0.;
	double relativeFiringFuel = 0.;
	double relativeFiringHull = 0.;
	double relativeFiringShields = 0.;

	double splitRange = 0.;
	double triggerRadius = 0.;
	double blastRadius = 0.;
	double safeRange = 0.;

	static const int DAMAGE_TYPES = 23;
	static const int HIT_FORCE = 0;
	// Normal damage types:
	static const int SHIELD_DAMAGE = 1;
	static const int HULL_DAMAGE = 2;
	static const int DISABLED_DAMAGE = 3;
	static const int MINABLE_DAMAGE = 4;
	static const int FUEL_DAMAGE = 5;
	static const int HEAT_DAMAGE = 6;
	static const int ENERGY_DAMAGE = 7;
	// Status effects:
	static const int ION_DAMAGE = 8;
	static const int WEAPON_JAMMING_DAMAGE = 9;
	static const int DISRUPTION_DAMAGE = 10;
	static const int SLOWING_DAMAGE = 11;
	static const int DISCHARGE_DAMAGE = 12;
	static const int CORROSION_DAMAGE = 13;
	static const int LEAK_DAMAGE = 14;
	static const int BURN_DAMAGE = 15;
	// Relative damage types:
	static const int RELATIVE_SHIELD_DAMAGE = 16;
	static const int RELATIVE_HULL_DAMAGE = 17;
	static const int RELATIVE_DISABLED_DAMAGE = 18;
	static const int RELATIVE_MINABLE_DAMAGE = 19;
	static const int RELATIVE_FUEL_DAMAGE = 20;
	static const int RELATIVE_HEAT_DAMAGE = 21;
	static const int RELATIVE_ENERGY_DAMAGE = 22;
	mutable double damage[DAMAGE_TYPES] = {};

	double piercing = 0.;

	double prospecting = 0.;

	double rangeOverride = 0.;
	double velocityOverride = 0.;

	bool hasDamageDropoff = false;
	std::pair<double, double> damageDropoffRange;
	double damageDropoffModifier = 1.;

	// Cache the calculation of these values, for faster access.
	mutable bool calculatedDamage = true;
	mutable bool doesDamage = false;
	mutable double totalLifetime = -1.;
};



// Inline the accessors because they get called so frequently.
inline int Weapon::Lifetime() const { return lifetime; }
inline int Weapon::RandomLifetime() const { return randomLifetime; }
inline int Weapon::FadeOut() const { return fadeOut; }
inline double Weapon::Reload() const { return reload; }
inline double Weapon::BurstReload() const { return burstReload; }
inline int Weapon::BurstCount() const { return burstCount; }

inline int Weapon::MissileStrength() const { return missileStrength; }
inline int Weapon::AntiMissile() const { return antiMissile; }
inline double Weapon::TractorBeam() const { return tractorBeam; }
inline uint16_t Weapon::PenetrationCount() const noexcept { return penetrationCount; }
inline bool Weapon::IsStreamed() const { return isStreamed; }

inline double Weapon::Velocity() const { return velocity; }
inline double Weapon::RandomVelocity() const { return randomVelocity; }
inline double Weapon::WeightedVelocity() const { return (velocityOverride > 0.) ? velocityOverride : velocity; }
inline double Weapon::Acceleration() const { return acceleration; }
inline double Weapon::Drag() const { return drag; }
inline const Point &Weapon::HardpointOffset() const { return hardpointOffset; }

inline double Weapon::Turn() const { return turn; }
inline double Weapon::TurretTurn() const { return turretTurn; }
inline double Weapon::Arc() const { return maxAngle; }

inline double Weapon::Tracking() const { return tracking; }
inline double Weapon::OpticalTracking() const { return opticalTracking; }
inline double Weapon::InfraredTracking() const { return infraredTracking; }
inline double Weapon::RadarTracking() const { return radarTracking; }

inline double Weapon::FiringEnergy() const { return firingEnergy; }
inline double Weapon::FiringForce() const { return firingForce; }
inline double Weapon::FiringFuel() const { return firingFuel; }
inline double Weapon::FiringHeat() const { return firingHeat; }
inline double Weapon::FiringHull() const { return firingHull; }
inline double Weapon::FiringShields() const { return firingShields; }
inline double Weapon::FiringIon() const{ return firingIon; }
inline double Weapon::FiringScramble() const { return firingScramble; }
inline double Weapon::FiringSlowing() const{ return firingSlowing; }
inline double Weapon::FiringDisruption() const{ return firingDisruption; }
inline double Weapon::FiringDischarge() const{ return firingDischarge; }
inline double Weapon::FiringCorrosion() const{ return firingCorrosion; }
inline double Weapon::FiringLeak() const{ return firingLeak; }
inline double Weapon::FiringBurn() const{ return firingBurn; }

inline double Weapon::RelativeFiringEnergy() const{ return relativeFiringEnergy; }
inline double Weapon::RelativeFiringHeat() const{ return relativeFiringHeat; }
inline double Weapon::RelativeFiringFuel() const{ return relativeFiringFuel; }
inline double Weapon::RelativeFiringHull() const{ return relativeFiringHull; }
inline double Weapon::RelativeFiringShields() const{ return relativeFiringShields; }

inline double Weapon::Piercing() const { return piercing; }

inline double Weapon::Prospecting() const { return prospecting; }

inline double Weapon::SplitRange() const { return splitRange; }
inline double Weapon::TriggerRadius() const { return triggerRadius; }
inline double Weapon::BlastRadius() const { return blastRadius; }
inline double Weapon::SafeRange() const { return safeRange; }
inline double Weapon::HitForce() const { return TotalDamage(HIT_FORCE); }

inline bool Weapon::IsSafe() const { return isSafe; }
inline bool Weapon::IsPhasing() const { return isPhasing; }
inline bool Weapon::IsDamageScaled() const { return isDamageScaled; }
inline bool Weapon::IsGravitational() const { return isGravitational; }
inline bool Weapon::IsFused() const { return isFused; }
inline bool Weapon::CanCollideShips() const { return canCollideShips; }
inline bool Weapon::CanCollideAsteroids() const { return canCollideAsteroids; }
inline bool Weapon::CanCollideMinables() const { return canCollideMinables; }
inline bool Weapon::Homing() const { return homing; }
inline bool Weapon::HasBlindspot() const { return blindspot; }
inline bool Weapon::ThrottleControl() const { return throttleControl; }
inline bool Weapon::Leading() const { return leading; }

inline double Weapon::ShieldDamage() const { return TotalDamage(SHIELD_DAMAGE); }
inline double Weapon::HullDamage() const { return TotalDamage(HULL_DAMAGE); }
inline double Weapon::DisabledDamage() const { return TotalDamage(DISABLED_DAMAGE); }
inline double Weapon::MinableDamage() const { return TotalDamage(MINABLE_DAMAGE); }
inline double Weapon::FuelDamage() const { return TotalDamage(FUEL_DAMAGE); }
inline double Weapon::HeatDamage() const { return TotalDamage(HEAT_DAMAGE); }
inline double Weapon::EnergyDamage() const { return TotalDamage(ENERGY_DAMAGE); }

inline double Weapon::IonDamage() const { return TotalDamage(ION_DAMAGE); }
inline double Weapon::ScramblingDamage() const { return TotalDamage(WEAPON_JAMMING_DAMAGE); }
inline double Weapon::DisruptionDamage() const { return TotalDamage(DISRUPTION_DAMAGE); }
inline double Weapon::SlowingDamage() const { return TotalDamage(SLOWING_DAMAGE); }
inline double Weapon::DischargeDamage() const { return TotalDamage(DISCHARGE_DAMAGE); }
inline double Weapon::CorrosionDamage() const { return TotalDamage(CORROSION_DAMAGE); }
inline double Weapon::LeakDamage() const { return TotalDamage(LEAK_DAMAGE); }
inline double Weapon::BurnDamage() const { return TotalDamage(BURN_DAMAGE); }

inline double Weapon::RelativeShieldDamage() const { return TotalDamage(RELATIVE_SHIELD_DAMAGE); }
inline double Weapon::RelativeHullDamage() const { return TotalDamage(RELATIVE_HULL_DAMAGE); }
inline double Weapon::RelativeDisabledDamage() const { return TotalDamage(RELATIVE_DISABLED_DAMAGE); }
inline double Weapon::RelativeMinableDamage() const { return TotalDamage(RELATIVE_MINABLE_DAMAGE); }
inline double Weapon::RelativeFuelDamage() const { return TotalDamage(RELATIVE_FUEL_DAMAGE); }
inline double Weapon::RelativeHeatDamage() const { return TotalDamage(RELATIVE_HEAT_DAMAGE); }
inline double Weapon::RelativeEnergyDamage() const { return TotalDamage(RELATIVE_ENERGY_DAMAGE); }

inline bool Weapon::DoesDamage() const { if(!calculatedDamage) TotalDamage(0); return doesDamage; }

inline bool Weapon::ConsumesHull() const { return FiringHull() > 0. || RelativeFiringHull() > 0.; }
inline bool Weapon::ConsumesFuel() const { return FiringFuel() > 0. || RelativeFiringFuel() > 0.; }
inline bool Weapon::ConsumesHeat() const { return FiringHeat() < 0. || RelativeFiringHeat() > 0.; }
inline bool Weapon::ConsumesEnergy() const { return FiringEnergy() > 0. || RelativeFiringEnergy() > 0.; }
inline bool Weapon::ConsumesIonization() const { return FiringIon() < 0.; }
inline bool Weapon::ConsumesDisruption() const { return FiringDisruption() < 0.; }
inline bool Weapon::ConsumesSlowing() const { return FiringSlowing() < 0.; }

inline bool Weapon::HasDamageDropoff() const { return hasDamageDropoff; }
