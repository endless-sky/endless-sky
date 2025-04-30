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
	ConditionEntry(const std::string &name);
	~ConditionEntry();

	// Prevent copying of ConditionEntries. We cannot safely copy the references to the provider, since we depend on the
	// conditionsStore to set prefix providers.
	ConditionEntry(ConditionEntry &) = delete;
	ConditionEntry &operator=(const ConditionEntry &) = delete;

	void Clear();

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
	// Helper class for DerivedProviders, the (lambda) functions that provide access to the derived conditions are
	// registered in this class.
	class DerivedProvider {
		friend ConditionEntry;

	public:
		/// Sets up a derived provider
		///
		/// @param mainEntry is nullptr for named providers, and the prefixed entry for prefix providers.
		DerivedProvider(ConditionEntry *mainEntry);


	public:
		/// Get function to get the value of the ConditionEntry. GetFunctions are required for any derived provider.
		std::function<int64_t(const ConditionEntry &)> getFunction;

		/// Lambda function to set the value for a condition entry.
		std::function<void(ConditionEntry &, int64_t)> setFunction;

		/// Toplevel entry for prefixed providers, nullptr for named (non-prefixed) providers.
		ConditionEntry *mainEntry;
	};


private:
	std::string name; ///< Name of this entry, set during construction of the entry object.
	int64_t value = 0; ///< Value of this condition, in case of direct access.
	DerivedProvider *provider = nullptr; ///< Provider, if this is a named or prefixed derived condition.

};
