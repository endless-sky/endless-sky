#ifndef AL_ATOMIC_H
#define AL_ATOMIC_H

#include <atomic>


using RefCount = std::atomic<unsigned int>;

inline void InitRef(RefCount &ref, unsigned int value)
{ ref.store(value, std::memory_order_relaxed); }
inline unsigned int ReadRef(RefCount &ref)
{ return ref.load(std::memory_order_acquire); }
inline unsigned int IncrementRef(RefCount &ref)
{ return ref.fetch_add(1u, std::memory_order_acq_rel)+1u; }
inline unsigned int DecrementRef(RefCount &ref)
{ return ref.fetch_sub(1u, std::memory_order_acq_rel)-1u; }


/* WARNING: A livelock is theoretically possible if another thread keeps
 * changing the head without giving this a chance to actually swap in the new
 * one (practically impossible with this little code, but...).
 */
template<typename T>
inline void AtomicReplaceHead(std::atomic<T> &head, T newhead)
{
    T first_ = head.load(std::memory_order_acquire);
    do {
        newhead->next.store(first_, std::memory_order_relaxed);
    } while(!head.compare_exchange_weak(first_, newhead,
            std::memory_order_acq_rel, std::memory_order_acquire));
}

#endif /* AL_ATOMIC_H */
