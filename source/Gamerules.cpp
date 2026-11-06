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
const string Gamerules::DEPRECIATION_MIN = "depreciation min";
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
const string Gamerules::SPAWN_RAID_FLEETS = "spawn raid fleets";
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
	settings[LOCK_GAMERULES] = Setting::TrueFalse("Lock gamerules", true);
	settings[LOCK_GAMERULES].SetOnToggleFunc([this](UI *) -> void {
		storage.lockGamerules = !storage.lockGamerules;
	});
	settings[LOCK_GAMERULES].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.lockGamerules = preset.storage.lockGamerules;
		return storage.lockGamerules;
	});

	settings[UNIVERSAL_RAMSCOOP] = Setting::TrueFalse("Universal ramscoop", true);
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
			[this](int value) -> void { storage.personSpawnPeriod = max(1, value); },
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
			[this](int value) -> void { storage.noPersonSpawnWeight = max(0, value); },
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
			[this](int value) -> void { storage.npcMaxMiningTime = max(0, value); },
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
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.universalFrugalThreshold = clamp(value, 0., 1.); },
			validate, message, UniversalFrugalThreshold()));
	});
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetConditionFunc([this]() -> int {
		return UniversalFrugalThreshold() * 1000;
	});
	settings[UNIVERSAL_FRUGAL_THRESHOLD].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.universalFrugalThreshold = preset.storage.universalFrugalThreshold;
		return std::nullopt;
	});

	settings[DEPRECIATION_MIN] = Setting::Unique("Minimum value");
	settings[DEPRECIATION_MIN].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationMin(), 2), true);
	});
	settings[DEPRECIATION_MIN].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the minimum deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.depreciationMin = clamp(value, 0., 1.); },
			validate, message, DepreciationMin()));
	});
	settings[DEPRECIATION_MIN].SetConditionFunc([this]() -> int {
		return DepreciationMin() * 1000;
	});
	settings[DEPRECIATION_MIN].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationMin = preset.storage.depreciationMin;
		return std::nullopt;
	});

	settings[DEPRECIATION_DAILY] = Setting::Unique("Daily depreciation");
	settings[DEPRECIATION_DAILY].SetDisplayFunc([this]() -> pair<string, bool> {
		return make_pair(Format::Percentage(DepreciationDaily(), 2), true);
	});
	settings[DEPRECIATION_DAILY].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the daily deprecation value. (Decimal value between 0 and 1.)";
		auto validate = [](double value) -> bool { return value >= 0.0 && value <= 1.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.depreciationDaily = clamp(value, 0., 1.); },
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
		return make_pair(Format::AbbreviatedNumber(DepreciationGracePeriod()), true);
	});
	settings[DEPRECIATION_GRACE_PERIOD].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the depreciation grace period. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.depreciationGracePeriod = max(0, value); },
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
		return make_pair(Format::AbbreviatedNumber(DepreciationMaxAge()), true);
	});
	settings[DEPRECIATION_MAX_AGE].SetOnToggleFunc([this](UI *ui) -> void {
		if(!ui)
			return;
		string message = "Set the depreciation maximum age. (Integer value greater than or equal to 0.)";
		auto validate = [](int value) -> bool { return value >= 0; };
		ui->Push(DialogPanel::RequestIntegerWithValidation(
			[this](int value) -> void { storage.depreciationMaxAge = max(0, value); },
			validate, message, DepreciationMaxAge()));
	});
	settings[DEPRECIATION_MAX_AGE].SetConditionFunc([this]() -> int {
		return DepreciationMaxAge();
	});
	settings[DEPRECIATION_MAX_AGE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.depreciationMaxAge = preset.storage.depreciationMaxAge;
		return std::nullopt;
	});

	settings[FIGHTER_DODGE_POLICY] = Setting::List("Fighter crossfire immunity", 2, {"none", "player", "all"});
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
		auto validate = [](double value) -> bool { return value >= 0.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.systemDepartureMin = max(0., value); },
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

	settings[HABITABLE_BASED_ARRIVAL_DISTANCE] = Setting::TrueFalse("Habitable arrival distance", true);
	settings[HABITABLE_BASED_ARRIVAL_DISTANCE].SetOnToggleFunc([this](UI *ui) -> void {
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
		auto validate = [](double value) -> bool { return value >= 0.0; };
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
		auto validate = [](double value) -> bool { return value >= 0.0; };
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
		auto validate = [](double value) -> bool { return value >= 0.0; };
		ui->Push(DialogPanel::RequestDoubleWithValidation(
			[this](double value) -> void { storage.fleetMultiplier = max(0., value); },
			validate, message, FleetMultiplier()));
	});
	settings[FLEET_MULTIPLIER].SetConditionFunc([this]() -> int {
		return FleetMultiplier() * 1000;
	});
	settings[FLEET_MULTIPLIER].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.fleetMultiplier = preset.storage.fleetMultiplier;
		return std::nullopt;
	});

	settings[SPAWN_RAID_FLEETS] = Setting::TrueFalse("Spawn raid fleets", true);
	settings[SPAWN_RAID_FLEETS].SetOnToggleFunc([this](UI *) -> void {
		storage.spawnRaidFleets = !storage.spawnRaidFleets;
	});
	settings[SPAWN_RAID_FLEETS].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.spawnRaidFleets = preset.storage.spawnRaidFleets;
		return storage.spawnRaidFleets;
	});

	settings[FLEET_SIZE_LIMITATION] = Setting::List("Fleet size limitation", 0,
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
			[this](int value) -> void { storage.defaultMaxEscortCount = max(0, value); },
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
			[this](int value) -> void { storage.defaultMaxEscortCrew = max(0, value); },
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

	settings[PERMADEATH_MODE] = Setting::List("Permadeath", 0,
		{"off", "lock on death", "delete on death", "lock on takeoff", "delete on takeoff"});
	settings[PERMADEATH_MODE].SetOnToggleFunc([this](UI *) -> void {
		storage.permadeathMode = static_cast<PermadeathMode>(settings[PERMADEATH_MODE].Index());
	});
	settings[PERMADEATH_MODE].SetResetRuleFunc([this](const Gamerules &preset) -> optional<int> {
		storage.permadeathMode = preset.storage.permadeathMode;
		return static_cast<int>(storage.permadeathMode );
	});

	settings[SINGLE_SAVE_FILE] = Setting::TrueFalse("Single save file", false);
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

	settings[RESTRICTED_SAVE_LOADING] = Setting::TrueFalse("Restricted save loading", false);
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

	settings[UNIVERSAL_AMMO_RESTOCKING] = Setting::TrueFalse("Universal ammo stocking", true);
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
			storage.universalFrugalThreshold = min(1., max(0., child.Value(1)));
		else if(key == DEPRECIATION_MIN)
			storage.depreciationMin = min(1., max(0., child.Value(1)));
		else if(key == DEPRECIATION_DAILY)
			storage.depreciationDaily = min(1., max(0., child.Value(1)));
		else if(key == DEPRECIATION_GRACE_PERIOD)
			storage.depreciationGracePeriod = max<int>(0, child.Value(1));
		else if(key == DEPRECIATION_MAX_AGE)
			storage.depreciationMaxAge = max<int>(0, child.Value(1));
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
		else if(key == "raid fleet roll chance")
			storage.raidFleetRollChance = clamp(child.Value(1), 0., 1.);
		else if(key == "raid attraction multiplier")
			storage.raidAttractionMultiplier = max(0., child.Value(1));
		else if(key == "raid deterrence multiplier")
			storage.raidDeterrenceMultiplier = max(0., child.Value(1));
		else if(key == "raid fleet spawn attempts")
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
		if(storage.depreciationMin != preset.storage.depreciationMin)
			out.Write(DEPRECIATION_MIN, storage.depreciationMin);
		if(storage.depreciationDaily != preset.storage.depreciationDaily)
			out.Write(DEPRECIATION_DAILY, storage.depreciationDaily);
		if(storage.depreciationGracePeriod != preset.storage.depreciationGracePeriod)
			out.Write(DEPRECIATION_GRACE_PERIOD, storage.depreciationGracePeriod);
		if(storage.depreciationMaxAge != preset.storage.depreciationMaxAge)
			out.Write(DEPRECIATION_MAX_AGE, storage.depreciationMaxAge);
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
			out.Write("raid fleet roll chance", storage.raidFleetRollChance);
		if(storage.raidAttractionMultiplier != preset.storage.raidAttractionMultiplier)
			out.Write("raid attraction multiplier", storage.raidAttractionMultiplier);
		if(storage.raidDeterrenceMultiplier != preset.storage.raidDeterrenceMultiplier)
			out.Write("raid deterrence multiplier", storage.raidDeterrenceMultiplier);
		if(storage.raidFleetSpawnAttempts != preset.storage.raidFleetSpawnAttempts)
			out.Write(SPAWN_RAID_FLEETS, storage.raidFleetSpawnAttempts);
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
	// "spawn raid fleets" has been replaced by "raid fleet roll chance".
	// Keeping this in place for reverse compatibility.
	if(rule == "spawn raid fleets")
		return storage.raidFleetRollChance > 0.;

	auto it = settings.find(rule);
	if(it != settings.end())
		return it->second.GetCondition();

	auto sit = storage.miscRules.find(rule);
	if(sit == storage.miscRules.end())
		return 0;
	return sit->second;
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



double Gamerules::DepreciationMin() const
{
	return storage.depreciationMin;
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



bool Gamerules::operator==(const Gamerules &other) const
{
	return name == other.name && storage == other.storage;
}



void Gamerules::SyncSettings()
{
	settings[LOCK_GAMERULES].SetIndex(LockGamerules());
	settings[UNIVERSAL_RAMSCOOP].SetIndex(UniversalRamscoopActive());
	settings[HABITABLE_BASED_ARRIVAL_DISTANCE].SetIndex(HabitableBasedArrivalDistance());
	settings[SINGLE_SAVE_FILE].SetIndex(SingleSaveFile());
	settings[RESTRICTED_SAVE_LOADING].SetIndex(RestrictedSaveLoading());
	settings[UNIVERSAL_AMMO_RESTOCKING].SetIndex(storage.miscRules[UNIVERSAL_AMMO_RESTOCKING]);
	settings[FIGHTER_DODGE_POLICY].SetIndex(static_cast<int>(FightersHitWhenDisabled()));
	settings[FLEET_SIZE_LIMITATION].SetIndex(static_cast<int>(GetFleetSizeLimitation()));
	settings[PERMADEATH_MODE].SetIndex(static_cast<int>(GetPermadeathMode()));
}
