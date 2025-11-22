
#include "config.h"

#include "effectslot.h"

#include <stddef.h>

#include "almalloc.h"
#include "context.h"


EffectSlotArray *EffectSlot::CreatePtrArray(size_t count) noexcept
{
    /* Allocate space for twice as many pointers, so the mixer has scratch
     * space to store a sorted list during mixing.
     */
    void *ptr{al_calloc(alignof(EffectSlotArray), EffectSlotArray::Sizeof(count*2))};
    return al::construct_at(static_cast<EffectSlotArray*>(ptr), count);
}

EffectSlot::~EffectSlot()
{
    if(mWetBuffer)
        mWetBuffer->mInUse = false;
}
