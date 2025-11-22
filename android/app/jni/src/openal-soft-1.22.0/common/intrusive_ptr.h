#ifndef INTRUSIVE_PTR_H
#define INTRUSIVE_PTR_H

#include <utility>

#include "atomic.h"
#include "opthelpers.h"


namespace al {

template<typename T>
class intrusive_ref {
    RefCount mRef{1u};

public:
    unsigned int add_ref() noexcept { return IncrementRef(mRef); }
    unsigned int release() noexcept
    {
        auto ref = DecrementRef(mRef);
        if UNLIKELY(ref == 0)
            delete static_cast<T*>(this);
        return ref;
    }

    /**
     * Release only if doing so would not bring the object to 0 references and
     * delete it. Returns false if the object could not be released.
     *
     * NOTE: The caller is responsible for handling a failed release, as it
     * means the object has no other references and needs to be be deleted
     * somehow.
     */
    bool releaseIfNoDelete() noexcept
    {
        auto val = mRef.load(std::memory_order_acquire);
        while(val > 1 && !mRef.compare_exchange_strong(val, val-1, std::memory_order_acq_rel))
        {
            /* val was updated with the current value on failure, so just try
             * again.
             */
        }

        return val >= 2;
    }
};


template<typename T>
class intrusive_ptr {
    T *mPtr{nullptr};

public:
    intrusive_ptr() noexcept = default;
    intrusive_ptr(const intrusive_ptr &rhs) noexcept : mPtr{rhs.mPtr}
    { if(mPtr) mPtr->add_ref(); }
    intrusive_ptr(intrusive_ptr&& rhs) noexcept : mPtr{rhs.mPtr}
    { rhs.mPtr = nullptr; }
    intrusive_ptr(std::nullptr_t) noexcept { }
    explicit intrusive_ptr(T *ptr) noexcept : mPtr{ptr} { }
    ~intrusive_ptr() { if(mPtr) mPtr->release(); }

    intrusive_ptr& operator=(const intrusive_ptr &rhs) noexcept
    {
        static_assert(noexcept(std::declval<T*>()->release()), "release must be noexcept");

        if(rhs.mPtr) rhs.mPtr->add_ref();
        if(mPtr) mPtr->release();
        mPtr = rhs.mPtr;
        return *this;
    }
    intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
    {
        if(likely(&rhs != this))
        {
            if(mPtr) mPtr->release();
            mPtr = std::exchange(rhs.mPtr, nullptr);
        }
        return *this;
    }

    explicit operator bool() const noexcept { return mPtr != nullptr; }

    T& operator*() const noexcept { return *mPtr; }
    T* operator->() const noexcept { return mPtr; }
    T* get() const noexcept { return mPtr; }

    void reset(T *ptr=nullptr) noexcept
    {
        if(mPtr)
            mPtr->release();
        mPtr = ptr;
    }

    T* release() noexcept { return std::exchange(mPtr, nullptr); }

    void swap(intrusive_ptr &rhs) noexcept { std::swap(mPtr, rhs.mPtr); }
    void swap(intrusive_ptr&& rhs) noexcept { std::swap(mPtr, rhs.mPtr); }
};

#define AL_DECL_OP(op)                                                        \
template<typename T>                                                          \
inline bool operator op(const intrusive_ptr<T> &lhs, const T *rhs) noexcept   \
{ return lhs.get() op rhs; }                                                  \
template<typename T>                                                          \
inline bool operator op(const T *lhs, const intrusive_ptr<T> &rhs) noexcept   \
{ return lhs op rhs.get(); }

AL_DECL_OP(==)
AL_DECL_OP(!=)
AL_DECL_OP(<=)
AL_DECL_OP(>=)
AL_DECL_OP(<)
AL_DECL_OP(>)

#undef AL_DECL_OP

} // namespace al

#endif /* INTRUSIVE_PTR_H */
