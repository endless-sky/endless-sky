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
#include "AttributeStore.h"
#include "Body.h"
#include "Distribution.h"
#include "Point.h"

#include <cstddef>
#include <cstdint>
#include <map>
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
public:
	struct Submunition{
		Submunition() noexcept = default;
		explicit Submunition(const Weapon *weapon, std::size_t count) noexcept
			: weapon(weapon), count(count) {};

		const Weapon *weapon = nullptr;
		std::size_t count = 0;
		// The angular offset from the source projectile, relative to its current facing.
		Angle facing;
		// The base offset from the source projectile's position, relative to its current facing.
		Point offset;

		bool spawnOnNaturalDeath = true;
		bool spawnOnAntiMissileDeath = false;
	};


public:
	// Load from a "weapon" node, either in an outfit, a ship (explosion), or a hazard.
	void LoadWeapon(const DataNode &node);
	bool IsWeapon() const;

	// Get assets used by this weapon.
	const Body &WeaponSprite() const;
	const Body &HardpointSprite() const;
	const Sound *WeaponSound() const;
	const Outfit *Ammo() const;
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
	int Homing() const;

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

	double Get(const char *attribute) const;
	template <class T>
	double Get(const T &attribute) const;
	double Get(const AttributeAccessor attribute) const;
	const AttributeStore &Attributes() const;

protected:
	// Legacy support: allow turret outfits with no turn rate to specify a
	// default turnrate.
	void SetTurretTurn(double rate);

	// A pair representing the outfit that is consumed as ammo and the number
	// of that outfit consumed upon fire.
	std::pair<const Outfit*, int> ammo;

	mutable AttributeStore attributes;

private:
	double TotalDamage(const AttributeEffectType effect) const;


private:
	// Sprites and sounds.
	Body sprite;
	Body hardpointSprite;
	const Sound *sound = nullptr;
	const Sprite *icon = nullptr;

	// Fire, die and hit effects.
	std::map<const Effect *, int> fireEffects;
	std::map<const Effect *, int> liveEffects;
	std::map<const Effect *, int> hitEffects;
	std::map<const Effect *, int> targetEffects;
	std::map<const Effect *, int> dieEffects;
	std::vector<Submunition> submunitions;

	// This stores whether or not the weapon has been loaded.
	bool isWeapon = false;
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

	// Attributes.
	int lifetime = 0;
	int randomLifetime = 0;
	int fadeOut = 0;
	double reload = 1.;
	double burstReload = 1.;
	int burstCount = 1;
	int homing = 0;

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

	double splitRange = 0.;
	double triggerRadius = 0.;
	double blastRadius = 0.;
	double safeRange = 0.;

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



template <class T>
double Weapon::Get(const T &attribute) const
{
	return attributes.Get(attribute);
}



// Inline the accessors because they get called so frequently.
inline int Weapon::Lifetime() const { return lifetime; }
inline int Weapon::RandomLifetime() const { return randomLifetime; }
inline int Weapon::FadeOut() const { return fadeOut; }
inline double Weapon::Reload() const { return reload; }
inline double Weapon::BurstReload() const { return burstReload; }
inline int Weapon::BurstCount() const { return burstCount; }
inline int Weapon::Homing() const { return homing; }

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

inline double Weapon::FiringEnergy() const { return Get({FIRING, ENERGY}); }
inline double Weapon::FiringForce() const { return Get({FIRING, FORCE}); }
inline double Weapon::FiringFuel() const { return Get({FIRING, FUEL}); }
inline double Weapon::FiringHeat() const { return Get({FIRING, HEAT}); }
inline double Weapon::FiringHull() const { return Get({FIRING, HULL}); }
inline double Weapon::FiringShields() const { return Get({FIRING, SHIELDS}); }
inline double Weapon::FiringIon() const{ return Get({FIRING, ENERGY, Modifier::OVER_TIME}); }
inline double Weapon::FiringScramble() const { return Get({FIRING, JAM, Modifier::OVER_TIME}); }
inline double Weapon::FiringSlowing() const{ return Get({FIRING, THRUST, Modifier::OVER_TIME}); }
inline double Weapon::FiringDisruption() const{ return Get({FIRING, PIERCING, Modifier::OVER_TIME}); }
inline double Weapon::FiringDischarge() const{ return Get({FIRING, SHIELDS, Modifier::OVER_TIME}); }
inline double Weapon::FiringCorrosion() const{ return Get({FIRING, HULL, Modifier::OVER_TIME}); }
inline double Weapon::FiringLeak() const{ return Get({FIRING, FUEL, Modifier::OVER_TIME}); }
inline double Weapon::FiringBurn() const{ return Get({FIRING, HEAT, Modifier::OVER_TIME}); }

inline double Weapon::RelativeFiringEnergy() const{ return Get(
		AttributeAccessor(FIRING, ENERGY, Modifier::RELATIVE)); }
inline double Weapon::RelativeFiringHeat() const{ return Get(
		AttributeAccessor(FIRING, HEAT, Modifier::RELATIVE)); }
inline double Weapon::RelativeFiringFuel() const{ return Get(
		AttributeAccessor(FIRING, FUEL, Modifier::RELATIVE)); }
inline double Weapon::RelativeFiringHull() const{ return Get(
		AttributeAccessor(FIRING, HULL, Modifier::RELATIVE)); }
inline double Weapon::RelativeFiringShields() const{ return Get(
		AttributeAccessor(FIRING, SHIELDS, Modifier::RELATIVE)); }

inline double Weapon::Piercing() const { return Get({DAMAGE, PIERCING}); }

inline double Weapon::Prospecting() const { return prospecting; }

inline double Weapon::SplitRange() const { return splitRange; }
inline double Weapon::TriggerRadius() const { return triggerRadius; }
inline double Weapon::BlastRadius() const { return blastRadius; }
inline double Weapon::SafeRange() const { return safeRange; }
inline double Weapon::HitForce() const { return TotalDamage(FORCE); }

inline bool Weapon::IsSafe() const { return isSafe; }
inline bool Weapon::IsPhasing() const { return isPhasing; }
inline bool Weapon::IsDamageScaled() const { return isDamageScaled; }
inline bool Weapon::IsGravitational() const { return isGravitational; }
inline bool Weapon::IsFused() const { return isFused; }
inline bool Weapon::CanCollideShips() const { return canCollideShips; }
inline bool Weapon::CanCollideAsteroids() const { return canCollideAsteroids; }
inline bool Weapon::CanCollideMinables() const { return canCollideMinables; }

inline double Weapon::ShieldDamage() const { return TotalDamage(SHIELDS); }
inline double Weapon::HullDamage() const { return TotalDamage(HULL); }
inline double Weapon::DisabledDamage() const { return TotalDamage(DISABLED); }
inline double Weapon::MinableDamage() const { return TotalDamage(MINABLE); }
inline double Weapon::FuelDamage() const { return TotalDamage(FUEL); }
inline double Weapon::HeatDamage() const { return TotalDamage(HEAT); }
inline double Weapon::EnergyDamage() const { return TotalDamage(ENERGY); }

inline double Weapon::IonDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(ENERGY, Modifier::OVER_TIME)); }
inline double Weapon::ScramblingDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(JAM, Modifier::OVER_TIME)); }
inline double Weapon::DisruptionDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(PIERCING, Modifier::OVER_TIME)); }
inline double Weapon::SlowingDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(THRUST, Modifier::OVER_TIME)); }
inline double Weapon::DischargeDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(SHIELDS, Modifier::OVER_TIME)); }
inline double Weapon::CorrosionDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(HULL, Modifier::OVER_TIME)); }
inline double Weapon::LeakDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(FUEL, Modifier::OVER_TIME)); }
inline double Weapon::BurnDamage() const { return TotalDamage(
		AttributeAccessor::WithModifier(HEAT, Modifier::OVER_TIME)); }

inline double Weapon::RelativeShieldDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, SHIELDS, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeHullDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, HULL, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeDisabledDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, DISABLED, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeMinableDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, MINABLE, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeFuelDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, FUEL, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeHeatDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, HEAT, Modifier::RELATIVE).Effect()); }
inline double Weapon::RelativeEnergyDamage() const {
		return TotalDamage(AttributeAccessor(DAMAGE, ENERGY, Modifier::RELATIVE).Effect()); }

inline bool Weapon::DoesDamage() const { if(!calculatedDamage) TotalDamage(SHIELDS); return doesDamage; }

inline bool Weapon::HasDamageDropoff() const { return hasDamageDropoff; }
