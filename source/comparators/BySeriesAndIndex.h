/* BySeriesAndIndex.h
Copyright (c) 2022 by warp-core

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

#include "../CategoryList.h"
#include "../CategoryType.h"
#include "../GameData.h"
#include "../Outfit.h"
#include "../Ship.h"

namespace {
	bool Helper(const Outfit &a, const Outfit &b, const std::string &nameA, const std::string &nameB)
	{
		static const CategoryList &series = GameData::GetCategory(CategoryType::SERIES);
		if(a.Series() == b.Series())
		{
			if(a.Index() == b.Index())
				return nameA < nameB;
			return a.Index() < b.Index();
		}
		const CategoryList::Category catA = series.GetCategory(a.Series());
		const CategoryList::Category catB = series.GetCategory(b.Series());
		return catA < catB;
	}
}

template<class T>
class BySeriesAndIndex;

template<>
class BySeriesAndIndex<Ship> {
public:
	bool operator()(const std::string &nameA, const std::string &nameB) const
	{
		const Outfit &shipA = GameData::Ships().Get(nameA)->Attributes();
		const Outfit &shipB = GameData::Ships().Get(nameB)->Attributes();
		return Helper(shipA, shipB, nameA, nameB);
	}
};

template<>
class BySeriesAndIndex<Outfit> {
public:
	bool operator()(const std::string &nameA, const std::string &nameB) const
	{
		const Outfit *outfitA = GameData::Outfits().Get(nameA);
		const Outfit *outfitB = GameData::Outfits().Get(nameB);
		return Helper(*outfitA, *outfitB, nameA, nameB);
	}
};

template<class T>
class BySeriesAndIndexMap;

template<>
class BySeriesAndIndexMap<Ship> {
public:
	bool operator()(const Ship *shipA, const Ship *shipB) const
	{
		const Outfit &a = shipA->Attributes();
		const Outfit &b = shipB->Attributes();
		const std::string &nameA = shipA->TrueModelName();
		const std::string &nameB = shipB->TrueModelName();
		return Helper(a, b, nameA, nameB);
	}
};

template<>
class BySeriesAndIndexMap<Outfit> {
public:
	bool operator()(const Outfit *a, const Outfit *b) const
	{
		const std::string &nameA = a->TrueName();
		const std::string &nameB = b->TrueName();
		return Helper(*a, *b, nameA, nameB);
	}
};
