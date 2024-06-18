/* ConditionsStore.h
Copyright (c) 2020-2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CONDITIONS_STORE_H_
#define CONDITIONS_STORE_H_

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <string>

class DataNode;
class DataWriter;



// Class that contains storage for conditions. Those conditions can be
// set directly in internal storage of this class (primary conditions)
// and those conditions can also be provided from other locations in the
// code (derived conditions).
//
// The derived conditions are typically provided "on demand" from outside
// this storage class. Some of those derived conditions could be read-only
// and in a number of cases the conditions might be converted from other
// data types than int64_t (for example double, float or even complex
// formulae).
class ConditionsStore {
public:
	// Forward declaration, needed to make ConditionEntry a friend of the
	// DerivedProvider.
	class ConditionEntry;

	// Class for DerivedProviders, the (lambda) functions that provide access
	// to the derived conditions are registered in this class.
	class DerivedProvider {
		friend ConditionsStore;
		friend ConditionEntry;

	public:
		// Functions to set the lambda functions for accessing the conditions.
		void SetGetFunction(std::function<int64_t(const std::string &)> newGetFun);
		void SetHasFunction(std::function<bool(const std::string &)> newHasFun);
		void SetSetFunction(std::function<bool(const std::string &, int64_t)> newSetFun);
		void SetEraseFunction(std::function<bool(const std::string &)> newEraseFun);

	public:
		// This is intended as a private constructor, only to be called from within
		// ConditionsStore. But we need to keep it public because of how the
		// DerivedProviders are emplaced in the providers-map-variable.
		DerivedProvider(const std::string &name, bool isPrefixProvider);

	private:
		std::string name;
		bool isPrefixProvider;

		// Lambda functions for accessing the derived conditions, with some sensible
		// default implementations;
		std::function<int64_t(const std::string &)> getFunction = [](const std::string &name) { return 0; };
		std::function<bool(const std::string &)> hasFunction = [](const std::string &name) { return true; };
		std::function<bool(const std::string &, int64_t)> setFunction = [](const std::string &name, int64_t value) {
			return false; };
		std::function<bool(const std::string &)> eraseFunction = [](const std::string &name) { return false; };
	};


	// Storage entry for a condition. Can act as an int64_t proxy when operator[] is used for access
	// to conditions in the ConditionsStore.
	class ConditionEntry {
		friend ConditionsStore;

	public:
		// int64_t proxy helper functions. Those functions allow access to the conditions
		// using `operator[]` on ConditionsStore.
		operator int64_t() const;
		ConditionEntry &operator=(int64_t val);
		ConditionEntry &operator++();
		ConditionEntry &operator--();
		ConditionEntry &operator+=(int64_t val);
		ConditionEntry &operator-=(int64_t val);

	private:
		int64_t value = 0;
		DerivedProvider *provider = nullptr;
		// The full keyname for condition we want to access. This full keyname is required
		// when accessing prefixed providers, because such providers will only know the prefix
		// part of the key.
		std::string fullKey;
	};



public:
	// Constructors to initialize this class.
	ConditionsStore() = default;
	explicit ConditionsStore(const DataNode &node);
	explicit ConditionsStore(std::initializer_list<std::pair<std::string, int64_t>> initialConditions);
	explicit ConditionsStore(const std::map<std::string, int64_t> &initialConditions);

	// Serialization support for this class.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Retrieve a "condition" flag from this store (directly or from the
	// connected provider).
	int64_t Get(const std::string &name) const;
	bool Has(const std::string &name) const;
	std::pair<bool, int64_t> HasGet(const std::string &name) const;

	// Add a value to a condition, set a value for a condition or erase a
	// condition completely. Returns true on success, false on failure.
	bool Add(const std::string &name, int64_t value);
	bool Set(const std::string &name, int64_t value);
	bool Erase(const std::string &name);

	// Direct access to a specific condition (using the ConditionEntry as proxy).
	ConditionEntry &operator[](const std::string &name);

	// Builds providers for derived conditions based on prefix and name.
	DerivedProvider &GetProviderPrefixed(const std::string &prefix);
	DerivedProvider &GetProviderNamed(const std::string &name);

	// Helper to completely remove all data and linked condition-providers from the store.
	void Clear();

	// Helper for testing; check how many primary conditions are registered.
	int64_t PrimariesSize() const;


private:
	// Retrieve a condition entry based on a condition name, the entry doesn't
	// get created if it doesn't exist yet (the Set function will handle
	// creation if required).
	ConditionEntry *GetEntry(const std::string &name);
	const ConditionEntry *GetEntry(const std::string &name) const;
	bool VerifyProviderLocation(const std::string &name, DerivedProvider *provider) const;



private:
	// Storage for both the primary conditions as well as the providers.
	std::map<std::string, ConditionEntry> storage;
	std::map<std::string, DerivedProvider> providers;
};



#endif
