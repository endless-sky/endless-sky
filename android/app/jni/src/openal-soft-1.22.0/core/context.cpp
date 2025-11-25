
#include "config.h"

#include <memory>

#include "async_event.h"
#include "context.h"
#include "device.h"
#include "effectslot.h"
#include "logging.h"
#include "ringbuffer.h"
#include "voice.h"
#include "voice_change.h"


ContextBase::ContextBase(DeviceBase *device) : mDevice{device}
{ }

ContextBase::~ContextBase()
{
    size_t count{0};
    ContextProps *cprops{mParams.ContextUpdate.exchange(nullptr, std::memory_order_relaxed)};
    if(cprops)
    {
        ++count;
        delete cprops;
    }
    cprops = mFreeContextProps.exchange(nullptr, std::memory_order_acquire);
    while(cprops)
    {
        std::unique_ptr<ContextProps> old{cprops};
        cprops = old->next.load(std::memory_order_relaxed);
        ++count;
    }
    TRACE("Freed %zu context property object%s\n", count, (count==1)?"":"s");

    count = 0;
    EffectSlotProps *eprops{mFreeEffectslotProps.exchange(nullptr, std::memory_order_acquire)};
    while(eprops)
    {
        std::unique_ptr<EffectSlotProps> old{eprops};
        eprops = old->next.load(std::memory_order_relaxed);
        ++count;
    }
    TRACE("Freed %zu AuxiliaryEffectSlot property object%s\n", count, (count==1)?"":"s");

    if(EffectSlotArray *curarray{mActiveAuxSlots.exchange(nullptr, std::memory_order_relaxed)})
    {
        al::destroy_n(curarray->end(), curarray->size());
        delete curarray;
    }

    delete mVoices.exchange(nullptr, std::memory_order_relaxed);

    if(mAsyncEvents)
    {
        count = 0;
        auto evt_vec = mAsyncEvents->getReadVector();
        if(evt_vec.first.len > 0)
        {
            al::destroy_n(reinterpret_cast<AsyncEvent*>(evt_vec.first.buf), evt_vec.first.len);
            count += evt_vec.first.len;
        }
        if(evt_vec.second.len > 0)
        {
            al::destroy_n(reinterpret_cast<AsyncEvent*>(evt_vec.second.buf), evt_vec.second.len);
            count += evt_vec.second.len;
        }
        if(count > 0)
            TRACE("Destructed %zu orphaned event%s\n", count, (count==1)?"":"s");
        mAsyncEvents->readAdvance(count);
    }
}


void ContextBase::allocVoiceChanges()
{
    constexpr size_t clustersize{128};

    VoiceChangeCluster cluster{std::make_unique<VoiceChange[]>(clustersize)};
    for(size_t i{1};i < clustersize;++i)
        cluster[i-1].mNext.store(std::addressof(cluster[i]), std::memory_order_relaxed);
    cluster[clustersize-1].mNext.store(mVoiceChangeTail, std::memory_order_relaxed);

    mVoiceChangeClusters.emplace_back(std::move(cluster));
    mVoiceChangeTail = mVoiceChangeClusters.back().get();
}

void ContextBase::allocVoiceProps()
{
    constexpr size_t clustersize{32};

    TRACE("Increasing allocated voice properties to %zu\n",
        (mVoicePropClusters.size()+1) * clustersize);

    VoicePropsCluster cluster{std::make_unique<VoicePropsItem[]>(clustersize)};
    for(size_t i{1};i < clustersize;++i)
        cluster[i-1].next.store(std::addressof(cluster[i]), std::memory_order_relaxed);
    mVoicePropClusters.emplace_back(std::move(cluster));

    VoicePropsItem *oldhead{mFreeVoiceProps.load(std::memory_order_acquire)};
    do {
        mVoicePropClusters.back()[clustersize-1].next.store(oldhead, std::memory_order_relaxed);
    } while(mFreeVoiceProps.compare_exchange_weak(oldhead, mVoicePropClusters.back().get(),
        std::memory_order_acq_rel, std::memory_order_acquire) == false);
}

void ContextBase::allocVoices(size_t addcount)
{
    constexpr size_t clustersize{32};
    /* Convert element count to cluster count. */
    addcount = (addcount+(clustersize-1)) / clustersize;

    if(addcount >= std::numeric_limits<int>::max()/clustersize - mVoiceClusters.size())
        throw std::runtime_error{"Allocating too many voices"};
    const size_t totalcount{(mVoiceClusters.size()+addcount) * clustersize};
    TRACE("Increasing allocated voices to %zu\n", totalcount);

    auto newarray = VoiceArray::Create(totalcount);
    while(addcount)
    {
        mVoiceClusters.emplace_back(std::make_unique<Voice[]>(clustersize));
        --addcount;
    }

    auto voice_iter = newarray->begin();
    for(VoiceCluster &cluster : mVoiceClusters)
    {
        for(size_t i{0};i < clustersize;++i)
            *(voice_iter++) = &cluster[i];
    }

    if(auto *oldvoices = mVoices.exchange(newarray.release(), std::memory_order_acq_rel))
    {
        mDevice->waitForMix();
        delete oldvoices;
    }
}
