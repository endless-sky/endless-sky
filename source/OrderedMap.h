/* OrderedMap.h
Copyright (c) 2026 by Amazinite

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

#include <algorithm>
#include <utility>
#include <vector>


// A map that maintains insertion order. The underlying data structure is just a vector of pairs.
template<class K, class V>
class OrderedMap {
	using Pair = std::pair<K, V>;
	using iterator = typename std::vector<Pair>::iterator;
	using const_iterator = typename std::vector<Pair>::const_iterator;
public:
	iterator find(const K &key);
	const_iterator find(const K &key) const;

	bool contains(const K &key) const;

	V &operator[](const K &key);
	template<class ...Args>
	V &emplace_back(const K &key, Args &&...args);

	V &at(const K &key);
	const V &at(const K &key) const;

	Pair &index(std::size_t i) { return map[i]; }
	const Pair &index(std::size_t i) const { return map[i]; }

	std::pair<iterator, bool> insert(Pair &element);
	std::pair<iterator, bool> insert(Pair &&element);

	bool empty() const { return map.empty(); }
	std::size_t size() const { return map.size(); }
	void clear() { map.clear(); }

	iterator erase(iterator it) { return map.erase(it); }
	iterator erase(const_iterator it) { return map.erase(it); }
	iterator erase(iterator first, iterator last) { return map.erase(first, last); }
	iterator erase(const_iterator first, const_iterator last) { return map.erase(first, last); }
	std::size_t erase(const K &key);

	iterator begin() { return map.begin(); }
	const_iterator begin() const { return map.begin(); }
	iterator end() { return map.end(); }
	const_iterator end() const { return map.end(); }

	Pair &front() { return map.front(); }
	const Pair &front() const { return map.front(); }
	Pair &back() { return map.back(); }
	const Pair &back() const { return map.back(); }


private:
	std::vector<std::pair<K, V>> map;
};



template<class K, class V>
typename std::vector<std::pair<K, V>>::iterator OrderedMap<K, V>::find(const K &key)
{
	return std::find_if(map.begin(), map.end(),
		[&key](const std::pair<K, V> &element) -> bool { return element.first == key; });
}



template<class K, class V>
typename std::vector<std::pair<K, V>>::const_iterator OrderedMap<K, V>::find(const K &key) const
{
	return std::find_if(map.begin(), map.end(),
		[&key](const std::pair<K, V> &element) -> bool { return element.first == key; });
}



template<class K, class V>
bool OrderedMap<K, V>::contains(const K &key) const
{
	return find(key) != map.end();
}



template<class K, class V>
V &OrderedMap<K, V>::operator[](const K &key)
{
	return this->emplace_back(key);
}



template<class K, class V>
template<class ...Args>
V &OrderedMap<K, V>::emplace_back(const K &key, Args &&...args)
{
	auto it = find(key);
	if(it == map.end())
	{
		map.emplace_back(key, V(std::forward<Args>(args)...));
		return map.back().second;
	}
	return it->second;
}



template<class K, class V>
V &OrderedMap<K, V>::at(const K &key)
{
	auto it = find(key);
	if(it == map.end())
		throw std::out_of_range("OrderedMap::at");
	return it->second;
}



template<class K, class V>
const V &OrderedMap<K, V>::at(const K &key) const
{
	auto it = find(key);
	if(it == map.end())
		throw std::out_of_range("OrderedMap::at");
	return it->second;
}



template<class K, class V>
std::pair<typename std::vector<std::pair<K, V>>::iterator, bool> OrderedMap<K, V>::insert(std::pair<K, V> &element)
{
	auto it = find(element.first);
	if(it != map.end())
		return {it, false};
	map.push_back(element);
	return {std::prev(map.end()), true};
}



template<class K, class V>
std::pair<typename std::vector<std::pair<K, V>>::iterator, bool> OrderedMap<K, V>::insert(std::pair<K, V> &&element)
{
	auto it = find(element.first);
	if(it != map.end())
		return {it, false};
	map.push_back(std::move(element));
	return {std::prev(map.end()), true};
}



template<class K, class V>
std::size_t OrderedMap<K, V>::erase(const K &key)
{
	auto it = find(key);
	if(it == map.end())
		return 0;
	map.erase(it);
	return 1;
}
