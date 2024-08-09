/* ResourceProvider.h
Copyright (c) 2024 by tibetiroka

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

#include <array>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>



// Provides various editable per-thread resources. Each Lock() call acquires one of each resource.
// After a resource is released, any new items added to it get added to the shared, non per-thread resource.
template <class... Types>
class ResourceProvider : public std::tuple<std::vector<Types>...> {
public:
	// Holds resources until destruction, upon which it synchronizes them.
	class ResourceGuard {
	public:
		ResourceGuard(size_t index, ResourceProvider<Types...> &provider);
		~ResourceGuard();

		// Don't allow copying or moving
		ResourceGuard(const ResourceGuard &other) = delete;
		ResourceGuard(const ResourceGuard &&other) = delete;
		ResourceGuard &operator=(const ResourceGuard &other) = delete;
		ResourceGuard &operator=(const ResourceGuard &&other) = delete;

		template<std::size_t I>
		inline auto &get() const;

		template<std::size_t I>
		inline auto &get();

		// Gets the index of this mutex in the parent ResourceProvider.
		inline size_t Index() const;


	private:
		template<int Index = 0>
		inline std::enable_if_t<Index < sizeof...(Types)> Sync();
		template<int Index = 0>
		inline std::enable_if_t<Index >= sizeof...(Types)> Sync();

		// Adds all contents of the local container to the remote.
		template<class Item, class Alloc>
		inline void SyncSingle(std::vector<Item, Alloc> &remote, std::vector<Item, Alloc> &local);

		template<class Item, class Alloc1, class Alloc2>
		inline void SyncSingle(std::vector<std::vector<Item, Alloc2>, Alloc1> &remote,
				std::vector<std::vector<Item, Alloc2>, Alloc1> &local);

		template<class Key, template<class...> class Container, class ...NestedValue, class Compare, class Alloc>
		inline void SyncSingle(std::map<Key, Container<NestedValue...>, Compare, Alloc> &remote,
				std::map<Key, Container<NestedValue...>, Compare, Alloc> &local);

		template<class Item, class Alloc = std::allocator<Item>>
		inline void SyncSingle(std::list<Item, Alloc> &remote, std::list<Item, Alloc> &local);


	private:
		const size_t index;
		ResourceProvider<Types...> &provider;
	};


public:
	ResourceProvider() = delete;
	// Creates a provider with the specified number of mutexes (support for that many concurrent threads).
	explicit ResourceProvider(size_t size, Types & ...remoteResources);
	// Creates a provider with the default number of mutexes (support for that many concurrent threads).
	explicit ResourceProvider(Types & ...remoteResources);

	// Don't allow copying or moving
	ResourceProvider(const ResourceProvider &other) = delete;
	ResourceProvider(const ResourceProvider &&other) = delete;
	ResourceProvider &operator=(const ResourceProvider &other) = delete;
	ResourceProvider &operator=(const ResourceProvider &&other) = delete;

	// Lock a mutex, and return a guard for that lock and its guarded resources.
	const ResourceGuard Lock();

	// The number of supported concurrent threads.
	size_t Size() const;


private:
	// Default-initialize the correct number of members in each per-thread resource.
	template<size_t Index = 0>
	typename std::enable_if_t<Index < sizeof...(Types), void>
	inline initialize_vectors(size_t size);

	template<size_t Index = 0>
	typename std::enable_if_t<Index == sizeof...(Types), void>
	inline initialize_vectors(size_t size);


private:
	// The remote resources. This class provides facades that support addition of new items,
	// which get synchronized to these remote resources upon destruction.
	std::tuple<Types &...> remoteResources;
	// Locks for each remote resource.
	std::array<std::mutex, sizeof...(Types)> remoteLocks;
	// Locks for each per-thread resource.
	std::vector<std::mutex> locks;
};



template <class ...Types>
ResourceProvider<Types...>::ResourceGuard::ResourceGuard(size_t index, ResourceProvider<Types...> &provider)
	: index(index), provider(provider)
{
}



template <class ...Types>
ResourceProvider<Types...>::ResourceGuard::~ResourceGuard()
{
	Sync();
	provider.locks[Index()].unlock();
}



template <class ...Types>
template<std::size_t I>
auto &ResourceProvider<Types...>::ResourceGuard::get() const
{
	return std::get<I>(provider)[index];
}



template <class ...Types>
template<std::size_t I>
auto &ResourceProvider<Types...>::ResourceGuard::get()
{
	return std::get<I>(provider)[index];
}



// Gets the index of this mutex in the parent ResourceProvider.
template <class ...Types>
size_t ResourceProvider<Types...>::ResourceGuard::Index() const
{
	return index;
}



template<class ...Types>
template<int I>
std::enable_if_t<I < sizeof...(Types)>
ResourceProvider<Types...>::ResourceGuard::Sync()
{
	auto &local = this->get<I>();
	if(!local.empty())
	{
		const std::lock_guard<std::mutex> lock(this->provider.remoteLocks[I]);
		SyncSingle(std::get<I>(provider.remoteResources), local);
	}
	Sync<I + 1>();
}



template<class ...Types>
template<int index>
std::enable_if_t<index >= sizeof...(Types)>
ResourceProvider<Types...>::ResourceGuard::Sync()
{
	// End of recursion
}



// Adds all contents of the local container to the remote.
template<class ...Types>
template<class Item, class Alloc>
void ResourceProvider<Types...>::ResourceGuard::SyncSingle(std::vector<Item, Alloc> &remote,
		std::vector<Item, Alloc> &local)
{
	if(remote.empty())
		remote.swap(local);
	else if(remote.size() <= local.size())
	{
		remote.swap(local);
		remote.insert(remote.end(), make_move_iterator(local.begin()), make_move_iterator(local.end()));
		local.clear();
	}
	else
	{
		remote.insert(remote.end(), make_move_iterator(local.begin()), make_move_iterator(local.end()));
		local.clear();
	}
}



template<class ...Types>
template<class Item, class Alloc1, class Alloc2>
void ResourceProvider<Types...>::ResourceGuard::SyncSingle(std::vector<std::vector<Item, Alloc2>, Alloc1> &remote,
		std::vector<std::vector<Item, Alloc2>, Alloc1> &local)
{
	for(auto &item : local)
		if(!item.empty())
			remote.emplace_back().swap(item);
	local.clear();
}



template<class ...Types>
template<class Key, template<class...> class Container, class ...NestedValue, class Compare, class Alloc>
void ResourceProvider<Types...>::ResourceGuard::SyncSingle(
		std::map<Key, Container<NestedValue...>, Compare, Alloc> &remote,
		std::map<Key, Container<NestedValue...>, Compare, Alloc> &local)
{
	for(auto &it : local)
		SyncSingle(remote[it.first], it.second);
	local.clear();
}



template<class ...Types>
template<class Item, class Alloc>
void ResourceProvider<Types...>::ResourceGuard::SyncSingle(std::list<Item, Alloc> &remote,
		std::list<Item, Alloc> &local)
{
	remote.splice(remote.end(), local);
}



// Creates a provider with the specified number of mutexes.
template <class... Types>
ResourceProvider<Types...>::ResourceProvider(size_t size, Types & ...remoteResources)
		: remoteResources(remoteResources...), locks(size)
{
	initialize_vectors(size);
}



// Creates a provider with the default number of mutexes.
template <class ...Types>
ResourceProvider<Types...>::ResourceProvider(Types & ...remoteResources)
		: ResourceProvider(std::thread::hardware_concurrency() * 2, remoteResources...)
{
}



// Lock a mutex, and return a guard for that lock.
// The guard has helper functions for accessing the acquired resources.
template <class ...Types>
const typename ResourceProvider<Types...>::ResourceGuard ResourceProvider<Types...>::Lock()
{
	// Find a free lock, and get a resource guard with the locked resources.
	size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	const size_t index = id % Size();
	if(locks[index].try_lock())
		return ResourceGuard(index, *this);

	for(unsigned newIndex = 0; newIndex < Size(); newIndex++)
		if(locks[newIndex].try_lock())
			return ResourceGuard(newIndex, *this);

	// If there is no free lock, fall back to the original guess.
	locks[index].lock();
	return ResourceGuard(index, *this);
}



// The number of resources stored.
template <class ...Types>
size_t ResourceProvider<Types...>::Size() const
{
	return locks.size();
}



// Default-initialize the correct number of members in each per-thread resource.
template <class ...Types>
template<size_t Index>
typename std::enable_if_t<Index < sizeof...(Types), void>
ResourceProvider<Types...>::initialize_vectors(size_t size)
{
	std::get<Index>(*this).resize(size);
	initialize_vectors<Index + 1>(size);
}



template <class ...Types>
template<size_t Index>
typename std::enable_if_t<Index == sizeof...(Types), void>
ResourceProvider<Types...>::initialize_vectors(size_t)
{
	// End of recursion
}
