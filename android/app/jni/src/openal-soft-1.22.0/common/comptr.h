#ifndef COMMON_COMPTR_H
#define COMMON_COMPTR_H

#include <cstddef>
#include <utility>

#include "opthelpers.h"


template<typename T>
class ComPtr {
    T *mPtr{nullptr};

public:
    ComPtr() noexcept = default;
    ComPtr(const ComPtr &rhs) : mPtr{rhs.mPtr} { if(mPtr) mPtr->AddRef(); }
    ComPtr(ComPtr&& rhs) noexcept : mPtr{rhs.mPtr} { rhs.mPtr = nullptr; }
    ComPtr(std::nullptr_t) noexcept { }
    explicit ComPtr(T *ptr) noexcept : mPtr{ptr} { }
    ~ComPtr() { if(mPtr) mPtr->Release(); }

    ComPtr& operator=(const ComPtr &rhs)
    {
        if(!rhs.mPtr)
        {
            if(mPtr)
                mPtr->Release();
            mPtr = nullptr;
        }
        else
        {
            rhs.mPtr->AddRef();
            try {
                if(mPtr)
                    mPtr->Release();
                mPtr = rhs.mPtr;
            }
            catch(...) {
                rhs.mPtr->Release();
                throw;
            }
        }
        return *this;
    }
    ComPtr& operator=(ComPtr&& rhs)
    {
        if(likely(&rhs != this))
        {
            if(mPtr) mPtr->Release();
            mPtr = std::exchange(rhs.mPtr, nullptr);
        }
        return *this;
    }

    explicit operator bool() const noexcept { return mPtr != nullptr; }

    T& operator*() const noexcept { return *mPtr; }
    T* operator->() const noexcept { return mPtr; }
    T* get() const noexcept { return mPtr; }
    T** getPtr() noexcept { return &mPtr; }

    T* release() noexcept { return std::exchange(mPtr, nullptr); }

    void swap(ComPtr &rhs) noexcept { std::swap(mPtr, rhs.mPtr); }
    void swap(ComPtr&& rhs) noexcept { std::swap(mPtr, rhs.mPtr); }
};

#endif
