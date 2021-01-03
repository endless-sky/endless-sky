/* Cache.cpp
Copyright (c) 2018 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Cache.h"

#include <set>

using namespace std;

namespace {
	// This is ensure that the return value has been initialized when it uses
	// for a initialization of any static variables.
	set<CacheBase*> &GetCacheObjectSet()
	{
		// Don't destruct this object because it must be alive until all static variables are destructed.
		static set<CacheBase*> *CacheObjectSet(new set<CacheBase*>);
		return *CacheObjectSet;
	}
}



void CacheBase::SetUpdateInterval(size_t newInterval)
{
	updateInterval = newInterval;
	if(stepCount >= updateInterval)
	{
		NextGeneration();
		stepCount = 0;
	}
}



void CacheBase::Step()
{
	set<CacheBase*> &objects = GetCacheObjectSet();
	for(auto &it : objects)
		it->StepThis();
}



void CacheBase::StepThis()
{
	++stepCount;
	if(stepCount >= updateInterval)
	{
		NextGeneration();
		stepCount = 0;
	}
}


void CacheBase::RegisterCacheObject(CacheBase *cacheObject)
{
	GetCacheObjectSet().insert(cacheObject);
}



void CacheBase::UnregisterCacheObject(CacheBase *cacheObject)
{
	GetCacheObjectSet().erase(cacheObject);
}
