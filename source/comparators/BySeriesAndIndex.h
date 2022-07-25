/* comparators/BySeriesAndIndex.h
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef BY_SERIES_AND_INDEX_H_
#define BY_SERIES_AND_INDEX_H_

#include "../CategoryList.h"
#include "../CategoryTypes.h"
#include "../GameData.h"
#include "../Outfit.h"
#include "../Ship.h"

#include <type_traits>

template<class T>
class BySeriesAndIndex {
public:
	bool operator()(const std::string &nameA, const std::string &nameB) const
	{
		if(std::is_same<T, Ship>::value)
		{
			const Outfit shipA = GameData::Ships().Get(nameA)->Attributes();
			const Outfit shipB = GameData::Ships().Get(nameB)->Attributes();
			return Helper(shipA, shipB);
		}
		else if(std::is_same<T, Outfit>::value)
		{
			const Outfit *outfitA = GameData::Outfits().Get(nameA);
			const Outfit *outfitB = GameData::Outfits().Get(nameB);
			return Helper(*outfitA, *outfitB);
		}
	}


private:
	bool Helper(const Outfit a, const Outfit b) const
	{
		CategoryList series = GameData::GetCategory(CategoryType::SERIES);
		if(a.Series() == b.Series())
		{
			if(a.Index() == b.Index())
				return a.Name() < b.Name();
			return a.Index() < b.Index();
		}
		const CategoryList::Category catA = series.GetCategory(a.Series());
		const CategoryList::Category catB = series.GetCategory(b.Series());
		return catA < catB;
	}
};

#endif
