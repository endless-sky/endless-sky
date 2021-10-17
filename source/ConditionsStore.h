/* ConditionsStore.h
Copyright (c) 2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONDITIONS_STORE_H_
#define CONDITIONS_STORE_H_

#include <functional>
#include <initializer_list>
#include <map>
#include <string>



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
	// Storage types for conditions;
	// - VALUE means that the condition-value is stored directly in this store.
	// - PREFIX_PROVIDER means that conditions starting with the given prefix
	//         are all handled by the given provider.
	// - EXACT_PROVIDER means that the condition with the exact name is handled
	//         by the given provider.
	enum StorageType {VALUE, PREFIX_PROVIDER, EXACT_PROVIDER};

	// Class for DerivedProviders, the (lambda) functions that provide access
	// to the derived conditions are registered in this class.
	class DerivedProvider
	{
	friend ConditionsStore;
	public:
		// Functions to set the lambda functions for accessing the conditions.
		void SetGetFun(std::function<int64_t(const std::string&)> newGetFun);
		void SetHasFun(std::function<bool(const std::string&)> newHasFun);
		void SetSetFun(std::function<bool(const std::string&, int64_t)> newSetFun);
		void SetEraseFun(std::function<bool(const std::string&)> newEraseFun);

	public:
		// Private constructor, only to be called from within ConditionsStore.
		DerivedProvider(const std::string &name, bool isPrefixProvider);
		
	private:
		std::string name;
		bool isPrefixProvider;
	
		// Lambda functions for accessing the derived conditions, with some sensible
		// default implementations;
		std::function<int64_t(const std::string&)> getFun = [](const std::string &name) { return 0; };
		std::function<bool(const std::string&)> hasFun = [](const std::string &name) { return true; };
		std::function<bool(const std::string&, int64_t)> setFun = [](const std::string &name, int64_t value) { return false; };
		std::function<bool(const std::string&)> eraseFun = [](const std::string &name) { return false; };
	};

	
private:
	// Private class that describes the internal storage format of this conditions store.
	class ConditionEntry
	{
	public:
		int64_t Get(const std::string& name) const;
		bool Has(const std::string& name) const;
		bool Set(const std::string& name, int64_t newValue);
		bool Erase(const std::string& name);
		
		// If we want to allow access to the conditions using `operator[]`
		// on ConditionsStore, then we could implement the following functions:
		//   operator int64_t() const;
		//   ConditionEntry &operator=(int64_t val);
		//   ConditionEntry &operator+=(int64_t val);
		//   ConditionEntry &operator-=(int64_t val);
		// Implementing those functions should already be possible for Value
		// and for Exact conditions. To also support this for Prefix conditions
		// requires some additional changes to allow prefix-conditions to be in
		// the list multiple times (where each entry uses a different string key
		// to access the prefixed condition provider).
	
	public:
		StorageType type = VALUE;
		int64_t value;
		DerivedProvider *provider;
	};
	
	
public:
	// Input_iterator helper class to iterate over primary conditions.
	// This can be used when saving primary conditions to savegames and/or
	// for displaying some data based on primary conditions.
	class PrimariesIterator : public std::iterator<
		std::input_iterator_tag,                      // iterator_category
		std::pair<const std::string, int64_t>,        // iterator: value_type
		std::ptrdiff_t,                               // iterator: difference_type
		const std::pair<const std::string, int64_t>*, // iterator: pointer
		std::pair<const std::string, int64_t>>        // iterator: reference
	{
		using CondMapItType = std::map<std::string, ConditionEntry>::const_iterator;
	
	public:
		PrimariesIterator(CondMapItType it, CondMapItType endIt);
		
		// Default input_iterator operations.
		std::pair<std::string, int64_t> operator*() const;
		const std::pair<std::string, int64_t>* operator->();
		PrimariesIterator& operator++();
		PrimariesIterator operator++(int);
		bool operator== (const PrimariesIterator& rhs) const;
		bool operator!= (const PrimariesIterator& rhs) const;
	
	
	public:
		// Helper function to ensure that the primary-conditions iterator points
		// to a primary (value) condition or to the end-iterator value.
		void MoveToValueCondition();
	
	
	private:
		// The operator->() requires a return value that is a pointer, but in this
		// case there is no original pair-object to point to, so we generate a
		// virtual object on the fly while iterating.
		std::pair<std::string, int64_t> itVal;
		CondMapItType condMapIt;
		CondMapItType condMapEnd;
	};



public:
	// Constructors to initialize this class.
	ConditionsStore() = default;
	ConditionsStore(std::initializer_list<std::pair<std::string, int64_t>> initialConditions);
	ConditionsStore(const std::map<std::string, int64_t> &initialConditions);

	// Retrieve a "condition" flag from this store (directly or from the
	// connected provider).
	int64_t Get(const std::string &name) const;
	bool Has(const std::string &name) const;
	// Add a value to a condition, set a value for a condition or erase a
	// condition completely. Returns true on success, false on failure.
	bool Add(const std::string &name, int64_t value);
	bool Set(const std::string &name, int64_t value);
	bool Erase(const std::string &name);
	
	// Direct (read-only) access to the stored primary conditions.
	PrimariesIterator PrimariesBegin() const;
	PrimariesIterator PrimariesEnd() const;
	PrimariesIterator PrimariesLowerBound(const std::string &key) const;
	
	// Builds providers for derived conditions based on prefix and name.
	DerivedProvider &GetProviderPrefixed(const std::string &prefix);
	DerivedProvider &GetProviderNamed(const std::string &name);
	
	// Helper to completely remove all data and linked condition-providers from the store.
	void Clear();


private:
	// Retrieve a condition entry based on a condition name, the entry doesn't
	// get created if it doesn't exist yet (the Set function will handle
	// creation if required).
	ConditionEntry *GetEntry(const std::string &name);
	const ConditionEntry *GetEntry(const std::string &name) const;



private:
	// Storage for both the primary conditions as well as the providers.
	std::map<std::string, ConditionEntry> storage;
	std::map<std::string, DerivedProvider> providers;
};



#endif
