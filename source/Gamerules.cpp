/* Gamerules.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Gamerules.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Depreciation.h"
#include "DialogPanel.h"
#include "text/Format.h"
#include "OptionalInputDialogPanel.h"
#include "image/SpriteSet.h"
#include "UI.h"

#include <algorithm>

using namespace std;



const string Gamerules::LOCK_GAMERULES = "lock gamerules";
const string Gamerules::UNIVERSAL_RAMSCOOP = "universal ramscoop";
const string Gamerules::PERSON_SPAWN_PERIOD = "person spawn period";
const string Gamerules::NO_PERSON_SPAWN_WEIGHT = "no person spawn weight";
const string Gamerules::NPC_MAX_MINING_TIME = "npc max mining time";
const string Gamerules::UNIVERSAL_FRUGAL_THRESHOLD = "universal frugal threshold";
const string Gamerules::DEPRECIATIVE_AGING = "depreciation aging active";
const string Gamerules::DEPRECIATION_MIN = "depreciation min";
const string Gamerules::DEPRECIATION_CAPTURED = "depreciation captured";
const string Gamerules::DEPRECIATION_PLUNDERED = "depreciation plundered";
const string Gamerules::DEPRECIATION_CARGO_IS_PLUNDER = "depreciation cargo is plunder";
const string Gamerules::DEPRECIATION_DAILY = "depreciation daily";
const string Gamerules::DEPRECIATION_GRACE_PERIOD = "depreciation grace period";
const string Gamerules::DEPRECIATION_MAX_AGE = "depreciation max age";
const string Gamerules::FIGHTER_DODGE_POLICY = "disabled fighters avoid projectiles";
const string Gamerules::SYSTEM_DEPARTURE_MIN = "system departure min";
const string Gamerules::SYSTEM_ARRIVAL_MIN = "system arrival min";
const string Gamerules::HABITABLE_BASED_ARRIVAL_DISTANCE = "habitable based arrival distance";
const string Gamerules::HABITABLE_ARRIVAL_MIN = "habitable arrival min";
const string Gamerules::HABITABLE_ARRIVAL_MAX = "habitable arrival max";
const string Gamerules::FLEET_MULTIPLIER = "fleet multiplier";
const string Gamerules::RAID_FLEET_ROLL_CHANCE = "raid fleet roll chance";
const string Gamerules::RAID_ATTRACTION_MULTIPLIER = "raid attraction multiplier";
const string Gamerules::RAID_DETERRENCE_MULTIPLIER = "raid deterrence multiplier";
const string Gamerules::RAID_FLEET_SPAWN_ATTEMPTS = "raid fleet spawn attempts";
const string Gamerules::FLEET_SIZE_LIMITATION = "fleet size limitation";
const string Gamerules::DEFAULT_MAX_ESCORT_COUNT = "default max escort count";
const string Gamerules::DEFAULT_MAX_ESCORT_CREW = "default max escort crew";
const string Gamerules::DEFAULT_ADMIN_CAP = "default admin cap";
const string Gamerules::PERMADEATH_MODE = "permadeath mode";
const string Gamerules::SINGLE_SAVE_FILE = "single save file";
const string Gamerules::RESTRICTED_SAVE_LOADING = "restricted save loading";
const string Gamerules::UNIVERSAL_AMMO_RESTOCKING = "universal ammo restocking";



Gamerules::Gamerules()
{
	settings[LOCK_GAMERULES] = Setting::TrueFalse("Lock gamerules");
	settings[LOCK_GAMERULES].SetOnToggleFunc([this](UI *) -> void {
		storage.lockGamerules = !storage.lockGamerules;
	});
	settings[LOCK_GAMERULES].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.lockGamerules = preset.storage.lockGamerules;
		return storage.lockGamerules;
	});

	settings[UNIVERSAL_RAMSCOOP] = Setting::TrueFalse("Universal ramscoop");
	settings[UNIVERSAL_RAMSCOOP].SetOnToggleFunc([this](UI *) -> void {
		storage.universalRamscoop = !storage.universalRamscoop;
	});
	settings[UNIVERSAL_RAMSCOOP].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.universalRamscoop = preset.storage.universalRamscoop;
		return storage.universalRamscoop;
	});

	settings[PERSON_SPAWN_PERIOD] = Setting::Unique("Spawn attempt period");
	settings[PERSON_SPAWN_PERIOD].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(PersonSpawnPeriod()), true);
	});
	settings[PERSON_SPAWN_PERIOD].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the person ship spawn attempt period. (Integer value greater than or equal to 1.)";
		auto validate = [](int value) -> bool { return value >= 1; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.personSpawnPeriod = value; },
			validate, message, PersonSpawnPeriod()));
	});
	settings[PERSON_SPAWN_PERIOD].SetConditionFunc([this]() -> int {
		return PersonSpawnPeriod();
	});
	settings[PERSON_SPAWN_PERIOD].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.personSpawnPeriod = preset.storage.personSpawnPeriod;
		return std::nullopt;
	});

	settings[NO_PERSON_SPAWN_WEIGHT] = Setting::Unique("No spawn weight");
	settings[NO_PERSON_SPAWN_WEIGHT].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(NoPersonSpawnWeight()), true);
	});
	settings[NO_PERSON_SPAWN_WEIGHT].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the no person ship spawn weight. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.noPersonSpawnWeight = value; },
			validate, message, NoPersonSpawnWeight()));
	});
	settings[NO_PERSON_SPAWN_WEIGHT].SetConditionFunc([this]() -> int {
		return NoPersonSpawnWeight();
	});
	settings[NO_PERSON_SPAWN_WEIGHT].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.noPersonSpawnWeight = preset.storage.noPersonSpawnWeight;
		return std::nullopt;
	});

	settings[NPC_MAX_MINING_TIME] = Setting::Unique("NPC max mining time");
	settings[NPC_MAX_MINING_TIME].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(NPCMaxMiningTime()), true);
	});
	settings[NPC_MAX_MINING_TIME].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the NPC max mining time. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.npcMaxMiningTime = value; },
			validate, message, NPCMaxMiningTime()));
	});
	settings[NPC_MAX_MINING_TIME].SetConditionFunc([this]() -> int {
		return NPCMaxMiningTime();
	});
	settings[NPC_MAX_MINING_TIME].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.npcMaxMiningTime = preset.storage.npcMaxMiningTime;
		return std::nullopt;
	});

	settings[UNIVERSAL_FRUGAL_THRESHOLD] = Setting::Unique("Universal frugal threshold");
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(UniversalFrugalThreshold(), 2), true);
	});
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the universal frugal threshold. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0. && value <= 1.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.universalFrugalThreshold = value; },
			validate, message, UniversalFrugalThreshold()));
	});
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetConditionFunc([this]() -> int {
		return UniversalFrugalThreshold() * 1000;
	});
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.universalFrugalThreshold = preset.storage.universalFrugalThreshold;
		return std::nullopt;
	});

	settings[DEPRECIATIVE_AGING] = Setting::TrueFalse("Depreciative aging");
	settings[DEPRECIATIVE_AGING].SetOnToggleFunc([this](UI *) -> void {
		storage.depreciationAgingActive = !storage.depreciationAgingActive;
	});
	settings[DEPRECIATIVE_AGING].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationAgingActive = preset.storage.depreciationAgingActive;
		return storage.depreciationAgingActive;
	});

	settings[DEPRECIATION_MIN] = Setting::Unique("Minimum value");
	settings[DEPRECIATION_MIN].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationMin(), 2), DepreciationAgingActive());
	});
	settings[DEPRECIATION_MIN].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the minimum deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0. && value <= 1.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void {
				storage.depreciationMin = value;
				Depreciation::InvalidateCache();
			},
			validate, message, DepreciationMin()));
	});
	settings[DEPRECIATION_MIN].SetConditionFunc([this]() -> int {
		return DepreciationMin() * 1000;
	});
	settings[DEPRECIATION_MIN].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationMin = preset.storage.depreciationMin;
		return std::nullopt;
	});

	settings[DEPRECIATION_CAPTURED] = Setting::Unique("Captured value");
	settings[DEPRECIATION_CAPTURED].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationCaptured(), 2), true);
	});
	settings[DEPRECIATION_CAPTURED].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the captured deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.depreciationCaptured = value; },
			validate, message, DepreciationCaptured()));
	});
	settings[DEPRECIATION_CAPTURED].SetConditionFunc([this]() -> int {
		return DepreciationCaptured() * 1000;
	});
	settings[DEPRECIATION_CAPTURED].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationCaptured = preset.storage.depreciationCaptured;
		return std::nullopt;
	});

	settings[DEPRECIATION_PLUNDERED] = Setting::Unique("Plundered value");
	settings[DEPRECIATION_PLUNDERED].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationPlundered(), 2), true);
	});
	settings[DEPRECIATION_PLUNDERED].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the plundered deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.depreciationPlundered = value; },
			validate, message, DepreciationPlundered()));
	});
	settings[DEPRECIATION_PLUNDERED].SetConditionFunc([this]() -> int {
		return DepreciationPlundered() * 1000;
	});
	settings[DEPRECIATION_PLUNDERED].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationPlundered = preset.storage.depreciationPlundered;
		return std::nullopt;
	});

	settings[DEPRECIATION_CARGO_IS_PLUNDER] = Setting::TrueFalse("Cargo is plunder");
	settings[DEPRECIATION_CARGO_IS_PLUNDER].SetOnToggleFunc([this](UI *) -> void {
		storage.depreciationCargoIsPlunder = !storage.depreciationCargoIsPlunder;
	});
	settings[DEPRECIATION_CARGO_IS_PLUNDER].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationCargoIsPlunder = preset.storage.depreciationCargoIsPlunder;
		return storage.depreciationCargoIsPlunder;
	});

	settings[DEPRECIATION_DAILY] = Setting::Unique("Daily depreciation");
	settings[DEPRECIATION_DAILY].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationDaily(), 2), DepreciationAgingActive());
	});
	settings[DEPRECIATION_DAILY].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the daily deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0. && value <= 1.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void {
				storage.depreciationDaily = value;
				Depreciation::InvalidateCache();
			},
			validate, message, DepreciationDaily()));
	});
	settings[DEPRECIATION_DAILY].SetConditionFunc([this]() -> int {
		return DepreciationDaily() * 1000;
	});
	settings[DEPRECIATION_DAILY].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationDaily = preset.storage.depreciationDaily;
		return std::nullopt;
	});

	settings[DEPRECIATION_GRACE_PERIOD] = Setting::Unique("Grace period");
	settings[DEPRECIATION_GRACE_PERIOD].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(DepreciationGracePeriod()), DepreciationAgingActive());
	});
	settings[DEPRECIATION_GRACE_PERIOD].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the depreciation grace period. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void {
				storage.depreciationGracePeriod = value;
				Depreciation::InvalidateCache();
			},
			validate, message, DepreciationGracePeriod()));
	});
	settings[DEPRECIATION_GRACE_PERIOD].SetConditionFunc([this]() -> int {
		return DepreciationGracePeriod();
	});
	settings[DEPRECIATION_GRACE_PERIOD].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationGracePeriod = preset.storage.depreciationGracePeriod;
		return std::nullopt;
	});

	settings[DEPRECIATION_MAX_AGE] = Setting::Unique("Maximum age");
	settings[DEPRECIATION_MAX_AGE].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(DepreciationMaxAge()), DepreciationAgingActive());
	});
	settings[DEPRECIATION_MAX_AGE].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the depreciation maximum age. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void {
				storage.depreciationMaxAge = value;
				Depreciation::InvalidateCache();
			},
			validate, message, DepreciationMaxAge()));
	});
	settings[DEPRECIATION_MAX_AGE].SetConditionFunc([this]() -> int {
		return DepreciationMaxAge();
	});
	settings[DEPRECIATION_MAX_AGE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationMaxAge = preset.storage.depreciationMaxAge;
		return std::nullopt;
	});

	settings[FIGHTER_DODGE_POLICY] = Setting::List("Fighter crossfire immunity", {"none", "player", "all"});
	settings[FIGHTER_DODGE_POLICY].SetOnToggleFunc([this](UI *) -> void {
		storage.fighterHitPolicy = static_cast<FighterDodgePolicy>(settings[FIGHTER_DODGE_POLICY].Index());
	});
	settings[FIGHTER_DODGE_POLICY].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.fighterHitPolicy = preset.storage.fighterHitPolicy;
		return static_cast<int>(storage.fighterHitPolicy);
	});

	settings[SYSTEM_DEPARTURE_MIN] = Setting::Unique("Minimum departure distance");
	settings[SYSTEM_DEPARTURE_MIN].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(SystemDepartureMin(), std::nullopt), true);
	});
	settings[SYSTEM_DEPARTURE_MIN].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the minimum system departure distance. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.systemDepartureMin = value; },
			validate, message, SystemDepartureMin()));
	});
	settings[SYSTEM_DEPARTURE_MIN].SetConditionFunc([this]() -> int {
		return SystemDepartureMin() * 1000;
	});
	settings[SYSTEM_DEPARTURE_MIN].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.systemDepartureMin = preset.storage.systemDepartureMin;
		return std::nullopt;
	});

	settings[SYSTEM_ARRIVAL_MIN] = Setting::Unique("Minimum arrival distance");
	settings[SYSTEM_ARRIVAL_MIN].SetDisplayFunc([this]() -> pair<string, bool> {
		string text;
		if(SystemArrivalMin().has_value())
			text = Format::AbbreviatedNumber(*SystemArrivalMin(), std::nullopt);
		else
			text = "(unset)";
		return make_pair(text, true);
	});
	settings[SYSTEM_ARRIVAL_MIN].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the minimum system arrival distance. (Any decimal value.)";
		ui->Push(OptionalInputDialogPanel::RequestDouble(
			[this](optional<double> value) -> void { storage.systemArrivalMin = value; },
			message, SystemArrivalMin()));
	});
	settings[SYSTEM_ARRIVAL_MIN].SetConditionFunc([this]() -> int {
		return SystemArrivalMin().value_or(0.) * 1000;
	});
	settings[SYSTEM_ARRIVAL_MIN].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.systemArrivalMin = preset.storage.systemArrivalMin;
		return std::nullopt;
	});

	settings[HABITABLE_BASED_ARRIVAL_DISTANCE] = Setting::TrueFalse("Habitable arrival distance");
	settings[HABITABLE_BASED_ARRIVAL_DISTANCE].SetOnToggleFunc([this](UI *) -> void {
		storage.habitableBasedArrivalDistance = !storage.habitableBasedArrivalDistance;
	});
	settings[HABITABLE_BASED_ARRIVAL_DISTANCE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.habitableBasedArrivalDistance = preset.storage.habitableBasedArrivalDistance;
		return storage.habitableBasedArrivalDistance;
	});

	settings[HABITABLE_ARRIVAL_MIN] = Setting::Unique("   Minimum distance");
	settings[HABITABLE_ARRIVAL_MIN].SetDisplayFunc([this]() -> pair<string, bool> {
		string text;
		if(HabitableArrivalMin().has_value())
			text = Format::AbbreviatedNumber(*HabitableArrivalMin(), std::nullopt);
		else
			text = "(unset)";
		return make_pair(text, HabitableBasedArrivalDistance());
	});
	settings[HABITABLE_ARRIVAL_MIN].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the minimum system arrival distance when the arrival distance is based on the "
			"habitable distance of the system. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(OptionalInputDialogPanel::RequestDoubleWithValidation(
			[this](optional<double> value) -> void { storage.habitableArrivalMin = value; },
			validate, message, HabitableArrivalMin()));
	});
	settings[HABITABLE_ARRIVAL_MIN].SetConditionFunc([this]() -> int {
		return HabitableArrivalMin().value_or(0.) * 1000;
	});
	settings[HABITABLE_ARRIVAL_MIN].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.habitableArrivalMin = preset.storage.habitableArrivalMin;
		return std::nullopt;
	});

	settings[HABITABLE_ARRIVAL_MAX] = Setting::Unique("   Maximum distance");
	settings[HABITABLE_ARRIVAL_MAX].SetDisplayFunc([this]() -> pair<string, bool> {
		string text;
			if(HabitableArrivalMax().has_value())
				text = Format::AbbreviatedNumber(*HabitableArrivalMax(), std::nullopt);
			else
				text = "(unset)";
		return make_pair(text, HabitableBasedArrivalDistance());
	});
	settings[HABITABLE_ARRIVAL_MAX].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the maximum system arrival distance when the arrival distance is based on the "
			"habitable distance of the system. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(OptionalInputDialogPanel::RequestDoubleWithValidation(
			[this](optional<double> value) -> void { storage.habitableArrivalMax = value; },
			validate, message, HabitableArrivalMax()));
	});
	settings[HABITABLE_ARRIVAL_MAX].SetConditionFunc([this]() -> int {
		return HabitableArrivalMax().value_or(0.) * 1000;
	});
	settings[HABITABLE_ARRIVAL_MAX].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.habitableArrivalMax = preset.storage.habitableArrivalMax;
		return std::nullopt;
	});

	settings[FLEET_MULTIPLIER] = Setting::Unique("Fleet multiplier");
	settings[FLEET_MULTIPLIER].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(FleetMultiplier(), 2), true);
	});
	settings[FLEET_MULTIPLIER].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the fleet spawn multiplier. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.fleetMultiplier = value; },
			validate, message, FleetMultiplier()));
	});
	settings[FLEET_MULTIPLIER].SetConditionFunc([this]() -> int {
		return FleetMultiplier() * 1000;
	});
	settings[FLEET_MULTIPLIER].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.fleetMultiplier = preset.storage.fleetMultiplier;
		return std::nullopt;
	});

	settings[RAID_FLEET_ROLL_CHANCE] = Setting::Unique("Roll chance");
	settings[RAID_FLEET_ROLL_CHANCE].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(RaidFleetRollChance(), 2), RaidFleetRollChance());
	});
	settings[RAID_FLEET_ROLL_CHANCE].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the raid fleet roll chance. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0. && value <= 1.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.raidFleetRollChance = value; },
			validate, message, RaidFleetRollChance()));
	});
	settings[RAID_FLEET_ROLL_CHANCE].SetConditionFunc([this]() -> int {
		return RaidFleetRollChance() * 1000;
	});
	settings[RAID_FLEET_ROLL_CHANCE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.raidFleetRollChance = preset.storage.raidFleetRollChance;
		return std::nullopt;
	});

	settings[RAID_ATTRACTION_MULTIPLIER] = Setting::Unique("Attraction multiplier");
	settings[RAID_ATTRACTION_MULTIPLIER].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(RaidAttractionMultiplier(), 2), RaidFleetRollChance());
	});
	settings[RAID_ATTRACTION_MULTIPLIER].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the raid fleet attraction multiplier. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.raidAttractionMultiplier = value; },
			validate, message, RaidAttractionMultiplier()));
	});
	settings[RAID_ATTRACTION_MULTIPLIER].SetConditionFunc([this]() -> int {
		return RaidAttractionMultiplier() * 1000;
	});
	settings[RAID_ATTRACTION_MULTIPLIER].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.raidAttractionMultiplier = preset.storage.raidAttractionMultiplier;
		return std::nullopt;
	});

	settings[RAID_DETERRENCE_MULTIPLIER] = Setting::Unique("Deterrence multiplier");
	settings[RAID_DETERRENCE_MULTIPLIER].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(RaidDeterrenceMultiplier(), 2), RaidFleetRollChance());
	});
	settings[RAID_DETERRENCE_MULTIPLIER].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the raid fleet deterrence multiplier. (Decimal value greater than or equal to 0.)";
		auto validate = [](double value) -> bool { return value >= 0.; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.raidDeterrenceMultiplier = value; },
			validate, message, RaidDeterrenceMultiplier()));
	});
	settings[RAID_DETERRENCE_MULTIPLIER].SetConditionFunc([this]() -> int {
		return RaidDeterrenceMultiplier() * 1000;
	});
	settings[RAID_DETERRENCE_MULTIPLIER].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.raidDeterrenceMultiplier = preset.storage.raidDeterrenceMultiplier;
		return std::nullopt;
	});

	settings[RAID_FLEET_SPAWN_ATTEMPTS] = Setting::Unique("Fleet spawn attempts");
	settings[RAID_FLEET_SPAWN_ATTEMPTS].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::AbbreviatedNumber(RaidFleetSpawnAttempts()), RaidFleetRollChance());
	});
	settings[RAID_FLEET_SPAWN_ATTEMPTS].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the number of raid fleet spawn attempts. (Integer value greater than or equal to 1.)";
		auto validate = [](int value) -> bool { return value >= 1; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.raidFleetSpawnAttempts = value; },
			validate, message, RaidFleetSpawnAttempts()));
	});
	settings[RAID_FLEET_SPAWN_ATTEMPTS].SetConditionFunc([this]() -> int {
		return RaidFleetSpawnAttempts();
	});
	settings[RAID_FLEET_SPAWN_ATTEMPTS].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.raidFleetSpawnAttempts = preset.storage.raidFleetSpawnAttempts;
		return std::nullopt;
	});

	settings[FLEET_SIZE_LIMITATION] = Setting::List("Fleet size limitation",
		{"none", "ship cap", "crew cap", "admin cap"});
	settings[FLEET_SIZE_LIMITATION].SetOnToggleFunc([this](UI *) -> void {
		storage.fleetSizeLimitation = static_cast<FleetSizeLimitation>(settings[FLEET_SIZE_LIMITATION].Index());
	});
	settings[FLEET_SIZE_LIMITATION].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.fleetSizeLimitation = preset.storage.fleetSizeLimitation;
		return static_cast<int>(storage.fleetSizeLimitation);
	});

	settings[DEFAULT_MAX_ESCORT_COUNT] = Setting::Unique("Default max escort count");
	settings[DEFAULT_MAX_ESCORT_COUNT].SetDisplayFunc([this]() -> pair<string, bool> {
		string text = Format::AbbreviatedNumber(GetDefaultMaxEscortCount());
		bool isOn = GetFleetSizeLimitation() == FleetSizeLimitation::SHIP_CAP;
		return make_pair(text, isOn);
	});
	settings[DEFAULT_MAX_ESCORT_COUNT].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the default max escort count. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.defaultMaxEscortCount = value; },
			validate, message, GetDefaultMaxEscortCount()));
	});
	settings[DEFAULT_MAX_ESCORT_COUNT].SetConditionFunc([this]() -> int {
		return GetDefaultMaxEscortCount();
	});
	settings[DEFAULT_MAX_ESCORT_COUNT].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.defaultMaxEscortCount = preset.storage.defaultMaxEscortCount;
		return std::nullopt;
	});

	settings[DEFAULT_MAX_ESCORT_CREW] = Setting::Unique("Default max escort crew");
	settings[DEFAULT_MAX_ESCORT_CREW].SetDisplayFunc([this]() -> pair<string, bool> {
		string text = Format::AbbreviatedNumber(GetDefaultMaxEscortCrew());
		bool isOn = GetFleetSizeLimitation() == FleetSizeLimitation::CREW_CAP;
		return make_pair(text, isOn);
	});
	settings[DEFAULT_MAX_ESCORT_CREW].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the default max escort crew. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.defaultMaxEscortCrew = value; },
			validate, message, GetDefaultMaxEscortCrew()));
	});
	settings[DEFAULT_MAX_ESCORT_CREW].SetConditionFunc([this]() -> int {
		return GetDefaultMaxEscortCrew();
	});
	settings[DEFAULT_MAX_ESCORT_CREW].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.defaultMaxEscortCrew = preset.storage.defaultMaxEscortCrew;
		return std::nullopt;
	});

	settings[DEFAULT_ADMIN_CAP] = Setting::Unique("Default admin cap");
	settings[DEFAULT_ADMIN_CAP].SetDisplayFunc([this]() -> pair<string, bool> {
		string text = Format::AbbreviatedNumber(GetDefaultAdminCap());
		bool isOn = GetFleetSizeLimitation() == FleetSizeLimitation::ADMIN_CAP;
		return make_pair(text, isOn);
	});
	settings[DEFAULT_ADMIN_CAP].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the default administrative capacity. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.defaultAdminCap = value; },
			validate, message, GetDefaultAdminCap()));
	});
	settings[DEFAULT_ADMIN_CAP].SetConditionFunc([this]() -> int {
		return GetDefaultAdminCap();
	});
	settings[DEFAULT_ADMIN_CAP].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.defaultAdminCap = preset.storage.defaultAdminCap;
		return std::nullopt;
	});

	settings[PERMADEATH_MODE] = Setting::List("Permadeath",
		{"off", "lock on death", "delete on death", "lock on takeoff", "delete on takeoff"});
	settings[PERMADEATH_MODE].SetOnToggleFunc([this](UI *) -> void {
		storage.permadeathMode = static_cast<PermadeathMode>(settings[PERMADEATH_MODE].Index());
	});
	settings[PERMADEATH_MODE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.permadeathMode = preset.storage.permadeathMode;
		return static_cast<int>(storage.permadeathMode );
	});

	settings[SINGLE_SAVE_FILE] = Setting::TrueFalse("Single save file");
	settings[SINGLE_SAVE_FILE].SetToggleFunc([this](int, UI *ui) -> int {
		if(!ui)
			return storage.singleSaveFile;
		if(GetPermadeathMode() == PermadeathMode::DELETE_ON_TAKEOFF)
			ui->Push(DialogPanel::Info("This gamerule is locked to true because \"Permadeath\" is set "
				"to \"delete on takeoff\". Change the permadeath rule to a different value to toggle this rule."));
		else
			storage.singleSaveFile = !storage.singleSaveFile;
		return storage.singleSaveFile;
	});
	settings[SINGLE_SAVE_FILE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.singleSaveFile = preset.storage.singleSaveFile;
		return storage.singleSaveFile;
	});

	settings[RESTRICTED_SAVE_LOADING] = Setting::TrueFalse("Restricted save loading");
	settings[RESTRICTED_SAVE_LOADING].SetToggleFunc([this](int, UI *ui) -> int {
		if(!ui)
			return storage.restrictedSaveLoading;
		PermadeathMode mode = GetPermadeathMode();
		if(mode == PermadeathMode::LOCK_ON_TAKEOFF || mode == PermadeathMode::DELETE_ON_TAKEOFF)
			ui->Push(DialogPanel::Info("This gamerule is locked to true because \"Permadeath\" is set "
				"to \"lock on takeoff\" or \"delete on takeoff\". Change the permadeath rule to a different "
				"value to toggle this rule."));
		else
			storage.restrictedSaveLoading = !storage.restrictedSaveLoading;
		return storage.restrictedSaveLoading;
	});
	settings[RESTRICTED_SAVE_LOADING].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.restrictedSaveLoading = preset.storage.restrictedSaveLoading;
		return storage.restrictedSaveLoading;
	});

	settings[UNIVERSAL_AMMO_RESTOCKING] = Setting::TrueFalse("Universal ammo stocking");
	settings[UNIVERSAL_AMMO_RESTOCKING].SetOnToggleFunc([this](UI *) -> void {
		SetMiscValue(UNIVERSAL_AMMO_RESTOCKING, !GetValue(UNIVERSAL_AMMO_RESTOCKING));
	});
	settings[UNIVERSAL_AMMO_RESTOCKING].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		auto it = preset.storage.miscRules.find(UNIVERSAL_AMMO_RESTOCKING);
		storage.miscRules[UNIVERSAL_AMMO_RESTOCKING] = it != preset.storage.miscRules.end() ? it->second : 0;
		return storage.miscRules[UNIVERSAL_AMMO_RESTOCKING];
	});
}



void Gamerules::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	else
		name = "Default";

	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping gamerule with no value:");
			continue;
		}

		const string &key = child.Token(0);

		if(key == "description")
			description = child.Token(1);
		else if(key == "thumbnail")
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(key == LOCK_GAMERULES)
			storage.lockGamerules = child.BoolValue(1);
		else if(key == UNIVERSAL_RAMSCOOP)
			storage.universalRamscoop = child.BoolValue(1);
		else if(key == PERSON_SPAWN_PERIOD)
			storage.personSpawnPeriod = max<int>(1, child.Value(1));
		else if(key == NO_PERSON_SPAWN_WEIGHT)
			storage.noPersonSpawnWeight = max<int>(0, child.Value(1));
		else if(key == NPC_MAX_MINING_TIME)
			storage.npcMaxMiningTime = max<int>(0, child.Value(1));
		else if(key == UNIVERSAL_FRUGAL_THRESHOLD)
			storage.universalFrugalThreshold = clamp(child.Value(1), 0., 1.);
		else if(key == DEPRECIATIVE_AGING)
			storage.depreciationAgingActive = child.BoolValue(1);
		else if(key == DEPRECIATION_MIN)
			storage.depreciationMin = clamp(child.Value(1), 0., 1.);
		else if(key == DEPRECIATION_PLUNDERED)
			storage.depreciationPlundered = clamp(child.Value(1), 0., 1.);
		else if(key == DEPRECIATION_CAPTURED)
			storage.depreciationCaptured = clamp(child.Value(1), 0., 1.);
		else if(key == DEPRECIATION_DAILY)
			storage.depreciationDaily = clamp(child.Value(1), 0., 1.);
		else if(key == DEPRECIATION_GRACE_PERIOD)
			storage.depreciationGracePeriod = max<int>(0, child.Value(1));
		else if(key == DEPRECIATION_MAX_AGE)
			storage.depreciationMaxAge = max<int>(0, child.Value(1));
		else if(key == DEPRECIATION_CARGO_IS_PLUNDER)
			storage.depreciationCargoIsPlunder = child.BoolValue(1);
		else if(key == FIGHTER_DODGE_POLICY)
		{
			const string &value = child.Token(1);
			if(value == "all")
				storage.fighterHitPolicy = FighterDodgePolicy::ALL;
			else if(value == "none")
				storage.fighterHitPolicy = FighterDodgePolicy::NONE;
			else if(value == "only player")
				storage.fighterHitPolicy = FighterDodgePolicy::ONLY_PLAYER;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == SYSTEM_DEPARTURE_MIN)
			storage.systemDepartureMin = max(0., child.Value(1));
		else if(key == SYSTEM_ARRIVAL_MIN)
		{
			if(child.Token(1) == "unset")
				storage.systemArrivalMin.reset();
			else
				storage.systemArrivalMin = child.Value(1);
		}
		else if(key == HABITABLE_BASED_ARRIVAL_DISTANCE)
			storage.habitableBasedArrivalDistance = child.BoolValue(1);
		else if(key == HABITABLE_ARRIVAL_MIN)
		{
			if(child.Token(1) == "unset")
				storage.habitableArrivalMin.reset();
			else
				storage.habitableArrivalMin = max(0., child.Value(1));
		}
		else if(key == HABITABLE_ARRIVAL_MAX)
		{
			if(child.Token(1) == "unset")
				storage.habitableArrivalMax.reset();
			else
				storage.habitableArrivalMax = max(0., child.Value(1));
		}
		else if(key == FLEET_MULTIPLIER)
			storage.fleetMultiplier = max(0., child.Value(1));
		else if(key == "spawn raid fleets")
		{
			child.PrintTrace("The \"spawn raid fleets\" gamerule has been deprecated. "
					"Use \"raid fleet roll chance\" instead.");
			storage.raidFleetRollChance = child.BoolValue(1);
		}
		else if(key == RAID_FLEET_ROLL_CHANCE)
			storage.raidFleetRollChance = clamp(child.Value(1), 0., 1.);
		else if(key == RAID_ATTRACTION_MULTIPLIER)
			storage.raidAttractionMultiplier = max(0., child.Value(1));
		else if(key == RAID_DETERRENCE_MULTIPLIER)
			storage.raidDeterrenceMultiplier = max(0., child.Value(1));
		else if(key == RAID_FLEET_SPAWN_ATTEMPTS)
			storage.raidFleetSpawnAttempts = max<int>(1, child.Value(1));
		else if(key == FLEET_SIZE_LIMITATION)
		{
			const string &value = child.Token(1);
			if(value == "none")
				storage.fleetSizeLimitation = FleetSizeLimitation::NONE;
			else if(value == "ship capacity")
				storage.fleetSizeLimitation = FleetSizeLimitation::SHIP_CAP;
			else if(value == "crew capacity")
				storage.fleetSizeLimitation = FleetSizeLimitation::CREW_CAP;
			else if(value == "administrative capacity")
				storage.fleetSizeLimitation = FleetSizeLimitation::ADMIN_CAP;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == DEFAULT_MAX_ESCORT_COUNT)
			storage.defaultMaxEscortCount = max<int>(0, child.Value(1));
		else if(key == DEFAULT_MAX_ESCORT_CREW)
			storage.defaultMaxEscortCrew = max<int>(0, child.Value(1));
		else if(key == DEFAULT_ADMIN_CAP)
			storage.defaultAdminCap = max<int>(0, child.Value(1));
		else if(key == PERMADEATH_MODE)
		{
			const string &value = child.Token(1);
			if(value == "off")
				storage.permadeathMode = PermadeathMode::OFF;
			else if(value == "lock on death")
				storage.permadeathMode = PermadeathMode::LOCK_ON_DEATH;
			else if(value == "delete on death")
				storage.permadeathMode = PermadeathMode::DELETE_ON_DEATH;
			else if(value == "lock on takeoff")
				storage.permadeathMode = PermadeathMode::LOCK_ON_TAKEOFF;
			else if(value == "delete on takeoff")
				storage.permadeathMode = PermadeathMode::DELETE_ON_TAKEOFF;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == SINGLE_SAVE_FILE)
			storage.singleSaveFile = child.BoolValue(1);
		else if(key == RESTRICTED_SAVE_LOADING)
			storage.restrictedSaveLoading = child.BoolValue(1);
		else if(key == "capture profit sharing")
		{
			const string &value = child.Token(1);
			if(value == "off")
				storage.captureProfitSharing = CaptureProfitSharing::OFF;
			else if(value == "dead crew")
				storage.captureProfitSharing = CaptureProfitSharing::DEAD_CREW;
			else if(value == "living crew")
				storage.captureProfitSharing = CaptureProfitSharing::LIVING_CREW;
			else if(value == "all crew")
				storage.captureProfitSharing = CaptureProfitSharing::ALL_CREW;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == "capture captain shares")
			storage.captureCaptainShares = max(0., child.Value(1));
		else if(key == "capture dead crew shares")
			storage.captureDeadCrewShares = max(0., child.Value(1));
		else if(key == "capture living crew shares")
			storage.captureLivingCrewShares = max(0., child.Value(1));
		else if(key == "capture minimum death benefit")
			storage.captureMinimumDeathBenefit = max<int>(0, child.Value(1));
		else if(key == "capture value basis")
		{
			if(child.Token(1) == "unset")
				storage.captureValueBasis.reset();
			else
				storage.captureValueBasis = max(0., child.Value(1));
		}
		else
			storage.miscRules[key] = child.IsNumber(1) ? child.Value(1) : child.BoolValue(1);
	}
	SyncSettings();
}



void Gamerules::Save(DataWriter &out, const Gamerules &preset) const
{
	out.Write("gamerules", name);
	out.BeginChild();
	{
		if(storage.lockGamerules != preset.storage.lockGamerules)
			out.Write(LOCK_GAMERULES, storage.lockGamerules ? 1 : 0);
		if(storage.universalRamscoop != preset.storage.universalRamscoop)
			out.Write(UNIVERSAL_RAMSCOOP, storage.universalRamscoop ? 1 : 0);
		if(storage.personSpawnPeriod != preset.storage.personSpawnPeriod)
			out.Write(PERSON_SPAWN_PERIOD, storage.personSpawnPeriod);
		if(storage.noPersonSpawnWeight != preset.storage.noPersonSpawnWeight)
			out.Write(NO_PERSON_SPAWN_WEIGHT, storage.noPersonSpawnWeight);
		if(storage.npcMaxMiningTime != preset.storage.npcMaxMiningTime)
			out.Write(NPC_MAX_MINING_TIME, storage.npcMaxMiningTime);
		if(storage.universalFrugalThreshold != preset.storage.universalFrugalThreshold)
			out.Write(UNIVERSAL_FRUGAL_THRESHOLD, storage.universalFrugalThreshold);
		if(storage.depreciationAgingActive != preset.storage.depreciationAgingActive)
			out.Write(DEPRECIATIVE_AGING, storage.depreciationAgingActive);
		if(storage.depreciationMin != preset.storage.depreciationMin)
			out.Write(DEPRECIATION_MIN, storage.depreciationMin);
		if(storage.depreciationPlundered != preset.storage.depreciationPlundered)
			out.Write(DEPRECIATION_PLUNDERED, storage.depreciationPlundered);
		if(storage.depreciationCaptured != preset.storage.depreciationCaptured)
			out.Write(DEPRECIATION_CAPTURED, storage.depreciationCaptured);
		if(storage.depreciationDaily != preset.storage.depreciationDaily)
			out.Write(DEPRECIATION_DAILY, storage.depreciationDaily);
		if(storage.depreciationGracePeriod != preset.storage.depreciationGracePeriod)
			out.Write(DEPRECIATION_GRACE_PERIOD, storage.depreciationGracePeriod);
		if(storage.depreciationMaxAge != preset.storage.depreciationMaxAge)
			out.Write(DEPRECIATION_MAX_AGE, storage.depreciationMaxAge);
		if(storage.depreciationCargoIsPlunder != preset.storage.depreciationCargoIsPlunder)
			out.Write(DEPRECIATION_CARGO_IS_PLUNDER, storage.depreciationCargoIsPlunder ? 1 : 0);
		if(storage.fighterHitPolicy != preset.storage.fighterHitPolicy)
		{
			if(storage.fighterHitPolicy == FighterDodgePolicy::ALL)
				out.Write(FIGHTER_DODGE_POLICY, "all");
			else if(storage.fighterHitPolicy == FighterDodgePolicy::ONLY_PLAYER)
				out.Write(FIGHTER_DODGE_POLICY, "only player");
			else
				out.Write(FIGHTER_DODGE_POLICY, "none");
		}
		if(storage.systemDepartureMin != preset.storage.systemDepartureMin)
			out.Write(SYSTEM_DEPARTURE_MIN, storage.systemDepartureMin);
		if(storage.systemArrivalMin != preset.storage.systemArrivalMin)
		{
			if(storage.systemArrivalMin.has_value())
				out.Write(SYSTEM_ARRIVAL_MIN, *storage.systemArrivalMin);
			else
				out.Write(SYSTEM_ARRIVAL_MIN, "unset");
		}
		if(storage.habitableBasedArrivalDistance != preset.storage.habitableBasedArrivalDistance)
			out.Write(HABITABLE_BASED_ARRIVAL_DISTANCE, storage.habitableBasedArrivalDistance ? 1 : 0);
		if(storage.habitableArrivalMin != preset.storage.habitableArrivalMin)
		{
			if(storage.habitableArrivalMin.has_value())
				out.Write(HABITABLE_ARRIVAL_MIN, *storage.habitableArrivalMin);
			else
				out.Write(HABITABLE_ARRIVAL_MIN, "unset");
		}
		if(storage.habitableArrivalMax != preset.storage.habitableArrivalMax)
		{
			if(storage.habitableArrivalMax.has_value())
				out.Write(HABITABLE_ARRIVAL_MAX, *storage.habitableArrivalMax);
			else
				out.Write(HABITABLE_ARRIVAL_MAX, "unset");
		}
		if(storage.fleetMultiplier != preset.storage.fleetMultiplier)
			out.Write(FLEET_MULTIPLIER, storage.fleetMultiplier);
		if(storage.raidFleetRollChance != preset.storage.raidFleetRollChance)
			out.Write(RAID_FLEET_ROLL_CHANCE, storage.raidFleetRollChance);
		if(storage.raidAttractionMultiplier != preset.storage.raidAttractionMultiplier)
			out.Write(RAID_ATTRACTION_MULTIPLIER, storage.raidAttractionMultiplier);
		if(storage.raidDeterrenceMultiplier != preset.storage.raidDeterrenceMultiplier)
			out.Write(RAID_DETERRENCE_MULTIPLIER, storage.raidDeterrenceMultiplier);
		if(storage.raidFleetSpawnAttempts != preset.storage.raidFleetSpawnAttempts)
			out.Write(RAID_FLEET_SPAWN_ATTEMPTS, storage.raidFleetSpawnAttempts);
		if(storage.fleetSizeLimitation != preset.storage.fleetSizeLimitation)
		{
			if(storage.fleetSizeLimitation == FleetSizeLimitation::NONE)
				out.Write(FLEET_SIZE_LIMITATION, "none");
			else if(storage.fleetSizeLimitation == FleetSizeLimitation::SHIP_CAP)
				out.Write(FLEET_SIZE_LIMITATION, "ship capacity");
			else if(storage.fleetSizeLimitation == FleetSizeLimitation::CREW_CAP)
				out.Write(FLEET_SIZE_LIMITATION, "crew capacity");
			else
				out.Write(FLEET_SIZE_LIMITATION, "administrative capacity");
		}
		if(storage.defaultMaxEscortCount != preset.storage.defaultMaxEscortCount)
			out.Write(DEFAULT_MAX_ESCORT_COUNT, storage.defaultMaxEscortCount);
		if(storage.defaultMaxEscortCrew != preset.storage.defaultMaxEscortCrew)
			out.Write(DEFAULT_MAX_ESCORT_CREW, storage.defaultMaxEscortCrew);
		if(storage.defaultAdminCap != preset.storage.defaultAdminCap)
			out.Write(DEFAULT_ADMIN_CAP, storage.defaultAdminCap);
		if(storage.permadeathMode != preset.storage.permadeathMode)
		{
			if(storage.permadeathMode == PermadeathMode::OFF)
				out.Write(PERMADEATH_MODE, "none");
			else if(storage.permadeathMode == PermadeathMode::LOCK_ON_DEATH)
				out.Write(PERMADEATH_MODE, "lock on death");
			else if(storage.permadeathMode == PermadeathMode::DELETE_ON_DEATH)
				out.Write(PERMADEATH_MODE, "delete on death");
			else if(storage.permadeathMode == PermadeathMode::LOCK_ON_TAKEOFF)
				out.Write(PERMADEATH_MODE, "lock on takeoff");
			else
				out.Write(PERMADEATH_MODE, "delete on takeoff");
		}
		if(storage.singleSaveFile != preset.storage.singleSaveFile)
			out.Write(SINGLE_SAVE_FILE, storage.singleSaveFile ? 1 : 0);
		if(storage.restrictedSaveLoading != preset.storage.restrictedSaveLoading)
			out.Write(RESTRICTED_SAVE_LOADING, storage.restrictedSaveLoading ? 1 : 0);
		if(storage.captureProfitSharing != preset.storage.captureProfitSharing)
		{
			if(storage.captureProfitSharing == CaptureProfitSharing::OFF)
				out.Write("capture profit sharing", "off");
			else if(storage.captureProfitSharing == CaptureProfitSharing::DEAD_CREW)
				out.Write("capture profit sharing", "dead crew");
			else if(storage.captureProfitSharing == CaptureProfitSharing::LIVING_CREW)
				out.Write("capture profit sharing", "living crew");
			else
				out.Write("capture profit sharing", "all crew");
		}
		if(storage.captureCaptainShares != preset.storage.captureCaptainShares)
			out.Write("capture captain shares", storage.captureCaptainShares);
		if(storage.captureDeadCrewShares != preset.storage.captureDeadCrewShares)
			out.Write("capture dead crew shares", storage.captureDeadCrewShares);
		if(storage.captureLivingCrewShares != preset.storage.captureLivingCrewShares)
			out.Write("capture living crew shares", storage.captureLivingCrewShares);
		if(storage.captureMinimumDeathBenefit != preset.storage.captureMinimumDeathBenefit)
			out.Write("capture minimum death benefit", storage.captureMinimumDeathBenefit);
		if(storage.captureValueBasis != preset.storage.captureValueBasis)
		{
			if(storage.captureValueBasis.has_value())
				out.Write("capture value basis", *storage.captureValueBasis);
			else
				out.Write("capture value basis", "unset");
		}

		const map<string, int> &otherMiscRules = preset.storage.miscRules;
		for(const auto &[rule, value] : storage.miscRules)
		{
			auto sit = otherMiscRules.find(rule);
			if(sit == otherMiscRules.end() || sit->second != value)
				out.Write(rule, value);
		}
	}
	out.EndChild();
}



void Gamerules::Toggle(const string &rule, UI *ui)
{
	auto it = settings.find(rule);
	if(it != settings.end())
	{
		it->second.Toggle(ui);
		// The state of permadeath mode influences the display of other settings.
		if(rule == PERMADEATH_MODE)
		{
			settings[SINGLE_SAVE_FILE].SetIndex(SingleSaveFile());
			settings[RESTRICTED_SAVE_LOADING].SetIndex(RestrictedSaveLoading());
		}
	}
}



const string &Gamerules::DisplayName(const string &rule) const
{
	static const string UNKNOWN = "UNKNOWN SETTING";
	auto it = settings.find(rule);
	return it != settings.end() ? it->second.DisplayName() : UNKNOWN;
}



pair<string, bool> Gamerules::DisplayValue(const string &rule) const
{
	static const string UNKNOWN = "UNKNOWN SETTING";
	auto it = settings.find(rule);
	return it != settings.end() ? it->second.DisplayValue() : make_pair(UNKNOWN, false);
}



void Gamerules::Replace(const Gamerules &rules)
{
	name = rules.name;
	storage = rules.storage;
	SyncSettings();
}



void Gamerules::Reset(const string &rule, const Gamerules &preset)
{
	auto it = settings.find(rule);
	if(it != settings.end())
	{
		it->second.ResetRule(preset);
		// The state of permadeath mode influences the display of other settings.
		if(rule == PERMADEATH_MODE)
		{
			settings[SINGLE_SAVE_FILE].SetIndex(SingleSaveFile());
			settings[RESTRICTED_SAVE_LOADING].SetIndex(RestrictedSaveLoading());
		}
		return;
	}

	auto sit = preset.storage.miscRules.find(rule);
	if(sit != preset.storage.miscRules.end())
		storage.miscRules[rule] = sit->second;
}



const string &Gamerules::Name() const
{
	return name;
}



const string &Gamerules::Description() const
{
	return description;
}



const Sprite *Gamerules::Thumbnail() const
{
	return thumbnail;
}



void Gamerules::SetLockGamerules(bool value)
{
	storage.lockGamerules = value;
}



void Gamerules::SetMiscValue(const string &rule, int value)
{
	storage.miscRules[rule] = value;
}



int Gamerules::GetValue(const string &rule) const
{

	auto it = settings.find(rule);
	if(it != settings.end())
		return it->second.GetCondition();

	auto sit = storage.miscRules.find(rule);
	if(sit != storage.miscRules.end())
		return sit->second;

	// "spawn raid fleets" has been replaced by "raid fleet roll chance".
	// Keeping this in place for reverse compatibility.
	if(rule == "spawn raid fleets")
		return storage.raidFleetRollChance > 0.;
	return 0;
}



bool Gamerules::LockGamerules() const
{
	return storage.lockGamerules;
}



bool Gamerules::UniversalRamscoopActive() const
{
	return storage.universalRamscoop;
}



int Gamerules::PersonSpawnPeriod() const
{
	return storage.personSpawnPeriod;
}



int Gamerules::NoPersonSpawnWeight() const
{
	return storage.noPersonSpawnWeight;
}



int Gamerules::NPCMaxMiningTime() const
{
	return storage.npcMaxMiningTime;
}



double Gamerules::UniversalFrugalThreshold() const
{
	return storage.universalFrugalThreshold;
}



bool Gamerules::DepreciationAgingActive() const
{
	return storage.depreciationAgingActive;
}



double Gamerules::DepreciationMin() const
{
	return storage.depreciationMin;
}



double Gamerules::DepreciationPlundered() const
{
	return storage.depreciationPlundered;
}



double Gamerules::DepreciationCaptured() const
{
	return storage.depreciationCaptured;
}



double Gamerules::DepreciationDaily() const
{
	return storage.depreciationDaily;
}



int Gamerules::DepreciationGracePeriod() const
{
	return storage.depreciationGracePeriod;
}



int Gamerules::DepreciationMaxAge() const
{
	return storage.depreciationMaxAge;
}



bool Gamerules::DepreciationCargoIsPlunder() const
{
	return storage.depreciationCargoIsPlunder;
}



Gamerules::FighterDodgePolicy Gamerules::FightersHitWhenDisabled() const
{
	return storage.fighterHitPolicy;
}



double Gamerules::SystemDepartureMin() const
{
	return storage.systemDepartureMin;
}



optional<double> Gamerules::SystemArrivalMin() const
{
	return storage.systemArrivalMin;
}



bool Gamerules::HabitableBasedArrivalDistance() const
{
	return storage.habitableBasedArrivalDistance;
}



optional<double> Gamerules::HabitableArrivalMin() const
{
	return storage.habitableArrivalMin;
}



optional<double> Gamerules::HabitableArrivalMax() const
{
	return storage.habitableArrivalMax;
}



double Gamerules::FleetMultiplier() const
{
	return storage.fleetMultiplier;
}



double Gamerules::RaidFleetRollChance() const
{
	return storage.raidFleetRollChance;
}



double Gamerules::RaidAttractionMultiplier() const
{
	return storage.raidAttractionMultiplier;
}



double Gamerules::RaidDeterrenceMultiplier() const
{
	return storage.raidDeterrenceMultiplier;
}



int Gamerules::RaidFleetSpawnAttempts() const
{
	return storage.raidFleetSpawnAttempts;
}



Gamerules::FleetSizeLimitation Gamerules::GetFleetSizeLimitation() const
{
	return storage.fleetSizeLimitation;
}



int Gamerules::GetDefaultMaxEscortCount() const
{
	return storage.defaultMaxEscortCount;
}



int Gamerules::GetDefaultMaxEscortCrew() const
{
	return storage.defaultMaxEscortCrew;
}



int Gamerules::GetDefaultAdminCap() const
{
	return storage.defaultAdminCap;
}



Gamerules::PermadeathMode Gamerules::GetPermadeathMode() const
{
	return storage.permadeathMode;
}



bool Gamerules::SingleSaveFile() const
{
	return storage.singleSaveFile || storage.permadeathMode == PermadeathMode::DELETE_ON_TAKEOFF;
}



bool Gamerules::RestrictedSaveLoading() const
{
	return storage.restrictedSaveLoading || storage.permadeathMode == PermadeathMode::LOCK_ON_TAKEOFF
		|| storage.permadeathMode == PermadeathMode::DELETE_ON_TAKEOFF;
}



Gamerules::CaptureProfitSharing Gamerules::GetCaptureProfitSharing() const
{
	return storage.captureProfitSharing;
}



double Gamerules::CaptureCaptainShares() const
{
	return storage.captureCaptainShares;
}



double Gamerules::CaptureDeadCrewShares() const
{
	return storage.captureDeadCrewShares;
}



double Gamerules::CaptureLivingCrewShares() const
{
	return storage.captureLivingCrewShares;
}



int Gamerules::CaptureMinimumDeathBenefit() const
{
	return storage.captureMinimumDeathBenefit;
}



std::optional<double> Gamerules::CaptureValueBasis() const
{
	return storage.captureValueBasis;
}



bool Gamerules::operator==(const Gamerules &other) const
{
	return name == other.name && storage == other.storage;
}



void Gamerules::SyncSettings()
{
	settings[LOCK_GAMERULES].SetIndex(LockGamerules());
	settings[DEPRECIATIVE_AGING].SetIndex(DepreciationAgingActive());
	settings[DEPRECIATION_CARGO_IS_PLUNDER].SetIndex(DepreciationCargoIsPlunder());
	settings[UNIVERSAL_RAMSCOOP].SetIndex(UniversalRamscoopActive());
	settings[HABITABLE_BASED_ARRIVAL_DISTANCE].SetIndex(HabitableBasedArrivalDistance());
	settings[SINGLE_SAVE_FILE].SetIndex(SingleSaveFile());
	settings[RESTRICTED_SAVE_LOADING].SetIndex(RestrictedSaveLoading());
	settings[UNIVERSAL_AMMO_RESTOCKING].SetIndex(storage.miscRules[UNIVERSAL_AMMO_RESTOCKING]);
	settings[FIGHTER_DODGE_POLICY].SetIndex(static_cast<int>(FightersHitWhenDisabled()));
	settings[FLEET_SIZE_LIMITATION].SetIndex(static_cast<int>(GetFleetSizeLimitation()));
	settings[PERMADEATH_MODE].SetIndex(static_cast<int>(GetPermadeathMode()));
}
