/* ConditionEntry.h
Copyright (c) 2024-2025 by Peter van der Meer

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

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class ConditionsStore;



/// Class that provides access to one single condition. It can:
/// - act as an int64_t proxy, to emulate int64 values
/// - provide direct access (polling type access)
/// - support continuous monitoring access for listeners that want to get an interrupt when the condition changes.
class ConditionEntry {
	friend ConditionsStore;

public:
	explicit ConditionEntry(const std::string &name);

	// Prevent copying of ConditionEntries. We cannot safely copy the references to the provider, since we depend on the
	// conditionsStore to set prefix providers.
	ConditionEntry(const ConditionEntry &) = delete;
	ConditionEntry &operator=(const ConditionEntry &) = delete;
	// Also prevent moving, the provider uses a pointer to the entry, so the entry should stay where it is.
	ConditionEntry(ConditionEntry &&) = delete;
	ConditionEntry &operator=(ConditionEntry &&) = delete;

	const std::string &Name() const;
	const std::string NameWithoutPrefix() const;

	// int64_t proxy helper functions. Those functions allow access to the conditions
	// using `operator[]` on ConditionsStores or on ConditionsProviders.
	operator int64_t() const;
	ConditionEntry &operator=(int64_t val);
	ConditionEntry &operator++();
	ConditionEntry &operator--();
	ConditionEntry &operator+=(int64_t val);
	ConditionEntry &operator-=(int64_t val);


	/// Configure entry for prefixed derived providing, with only a get function.
	void ProvidePrefixed(std::function<int64_t(const ConditionEntry &)> getFunction);
	/// Configure entry for prefixed derived providing, with a get and a set function.
	void ProvidePrefixed(std::function<int64_t(const ConditionEntry &)> getFunction,
		std::function<void(ConditionEntry &, int64_t)> setFunction);

	/// Configure entry for named derived providing, with only a get function.
	void ProvideNamed(std::function<int64_t(const ConditionEntry &)> getFunction);
	/// Configure entry for named derived providing, with a get and a set function.
	void ProvideNamed(std::function<int64_t(const ConditionEntry &)> getFunction,
		std::function<void(ConditionEntry &, int64_t)> setFunction);

	/// Notify all subscribed listeners that the value of the condition changed.
	void NotifyUpdate(uint64_t value);


private:
	std::string name; ///< Name of this entry, set during construction of the entry object.
	int64_t value = 0; ///< Value of this condition, in case of direct access.

	/// Get function to get the value of the ConditionEntry. GetFunctions are required for any derived provider.
	std::function<int64_t(const ConditionEntry &)> getFunction;
	/// Lambda function to set the value for a condition entry.
	std::function<void(ConditionEntry &, int64_t)> setFunction;

	/// conditionEntry that provides the prefixed condition, or nullptr if this is a regular or named condition.
	const ConditionEntry *providingEntry = nullptr;
};
