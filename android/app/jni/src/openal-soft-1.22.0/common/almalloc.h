#ifndef AL_MALLOC_H
#define AL_MALLOC_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "pragmadefs.h"


void al_free(void *ptr) noexcept;
[[gnu::alloc_align(1), gnu::alloc_size(2), gnu::malloc]]
void *al_malloc(size_t alignment, size_t size);
[[gnu::alloc_align(1), gnu::alloc_size(2), gnu::malloc]]
void *al_calloc(size_t alignment, size_t size);


#define DISABLE_ALLOC()                                                       \
    void *operator new(size_t) = delete;                                      \
    void *operator new[](size_t) = delete;                                    \
    void operator delete(void*) noexcept = delete;                            \
    void operator delete[](void*) noexcept = delete;

#define DEF_NEWDEL(T)                                                         \
    void *operator new(size_t size)                                           \
    {                                                                         \
        void *ret = al_malloc(alignof(T), size);                              \
        if(!ret) throw std::bad_alloc();                                      \
        return ret;                                                           \
    }                                                                         \
    void *operator new[](size_t size) { return operator new(size); }          \
    void operator delete(void *block) noexcept { al_free(block); }            \
    void operator delete[](void *block) noexcept { operator delete(block); }

#define DEF_PLACE_NEWDEL()                                                    \
    void *operator new(size_t /*size*/, void *ptr) noexcept { return ptr; }   \
    void *operator new[](size_t /*size*/, void *ptr) noexcept { return ptr; } \
    void operator delete(void *block, void*) noexcept { al_free(block); }     \
    void operator delete(void *block) noexcept { al_free(block); }            \
    void operator delete[](void *block, void*) noexcept { al_free(block); }   \
    void operator delete[](void *block) noexcept { al_free(block); }

enum FamCount : size_t { };

#define DEF_FAM_NEWDEL(T, FamMem)                                             \
    static constexpr size_t Sizeof(size_t count) noexcept                     \
    {                                                                         \
        return std::max(decltype(FamMem)::Sizeof(count, offsetof(T, FamMem)), \
            sizeof(T));                                                       \
    }                                                                         \
                                                                              \
    void *operator new(size_t /*size*/, FamCount count)                       \
    {                                                                         \
        if(void *ret{al_malloc(alignof(T), T::Sizeof(count))})                \
            return ret;                                                       \
        throw std::bad_alloc();                                               \
    }                                                                         \
    void *operator new[](size_t /*size*/) = delete;                           \
    void operator delete(void *block, FamCount) { al_free(block); }           \
    void operator delete(void *block) noexcept { al_free(block); }            \
    void operator delete[](void* /*block*/) = delete;


namespace al {

template<typename T, std::size_t alignment=alignof(T)>
struct allocator {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type;

    template<typename U>
    struct rebind {
        using other = allocator<U, (alignment<alignof(U))?alignof(U):alignment>;
    };

    constexpr explicit allocator() noexcept = default;
    template<typename U, std::size_t N>
    constexpr explicit allocator(const allocator<U,N>&) noexcept { }

    T *allocate(std::size_t n)
    {
        if(n > std::numeric_limits<std::size_t>::max()/sizeof(T)) throw std::bad_alloc();
        if(auto p = al_malloc(alignment, n*sizeof(T))) return static_cast<T*>(p);
        throw std::bad_alloc();
    }
    void deallocate(T *p, std::size_t) noexcept { al_free(p); }
};
template<typename T, std::size_t N, typename U, std::size_t M>
constexpr bool operator==(const allocator<T,N>&, const allocator<U,M>&) noexcept { return true; }
template<typename T, std::size_t N, typename U, std::size_t M>
constexpr bool operator!=(const allocator<T,N>&, const allocator<U,M>&) noexcept { return false; }


template<typename T, typename ...Args>
constexpr T* construct_at(T *ptr, Args&& ...args)
    noexcept(std::is_nothrow_constructible<T, Args...>::value)
{ return ::new(static_cast<void*>(ptr)) T{std::forward<Args>(args)...}; }

/* At least VS 2015 complains that 'ptr' is unused when the given type's
 * destructor is trivial (a no-op). So disable that warning for this call.
 */
DIAGNOSTIC_PUSH
msc_pragma(warning(disable : 4100))
template<typename T>
constexpr std::enable_if_t<!std::is_array<T>::value>
destroy_at(T *ptr) noexcept(std::is_nothrow_destructible<T>::value)
{ ptr->~T(); }
DIAGNOSTIC_POP
template<typename T>
constexpr std::enable_if_t<std::is_array<T>::value>
destroy_at(T *ptr) noexcept(std::is_nothrow_destructible<std::remove_all_extents_t<T>>::value)
{
    for(auto &elem : *ptr)
        al::destroy_at(std::addressof(elem));
}

template<typename T>
constexpr void destroy(T first, T end) noexcept(noexcept(al::destroy_at(std::addressof(*first))))
{
    while(first != end)
    {
        al::destroy_at(std::addressof(*first));
        ++first;
    }
}

template<typename T, typename N>
constexpr std::enable_if_t<std::is_integral<N>::value,T>
destroy_n(T first, N count) noexcept(noexcept(al::destroy_at(std::addressof(*first))))
{
    if(count != 0)
    {
        do {
            al::destroy_at(std::addressof(*first));
            ++first;
        } while(--count);
    }
    return first;
}


template<typename T, typename N>
inline std::enable_if_t<std::is_integral<N>::value,
T> uninitialized_default_construct_n(T first, N count)
{
    using ValueT = typename std::iterator_traits<T>::value_type;
    T current{first};
    if(count != 0)
    {
        try {
            do {
                ::new(static_cast<void*>(std::addressof(*current))) ValueT;
                ++current;
            } while(--count);
        }
        catch(...) {
            al::destroy(first, current);
            throw;
        }
    }
    return current;
}


/* Storage for flexible array data. This is trivially destructible if type T is
 * trivially destructible.
 */
template<typename T, size_t alignment, bool = std::is_trivially_destructible<T>::value>
struct FlexArrayStorage {
    const size_t mSize;
    union {
        char mDummy;
        alignas(alignment) T mArray[1];
    };

    static constexpr size_t Sizeof(size_t count, size_t base=0u) noexcept
    {
        const size_t len{sizeof(T)*count};
        return std::max(offsetof(FlexArrayStorage,mArray)+len, sizeof(FlexArrayStorage)) + base;
    }

    FlexArrayStorage(size_t size) : mSize{size}
    { al::uninitialized_default_construct_n(mArray, mSize); }
    ~FlexArrayStorage() = default;

    FlexArrayStorage(const FlexArrayStorage&) = delete;
    FlexArrayStorage& operator=(const FlexArrayStorage&) = delete;
};

template<typename T, size_t alignment>
struct FlexArrayStorage<T,alignment,false> {
    const size_t mSize;
    union {
        char mDummy;
        alignas(alignment) T mArray[1];
    };

    static constexpr size_t Sizeof(size_t count, size_t base) noexcept
    {
        const size_t len{sizeof(T)*count};
        return std::max(offsetof(FlexArrayStorage,mArray)+len, sizeof(FlexArrayStorage)) + base;
    }

    FlexArrayStorage(size_t size) : mSize{size}
    { al::uninitialized_default_construct_n(mArray, mSize); }
    ~FlexArrayStorage() { al::destroy_n(mArray, mSize); }

    FlexArrayStorage(const FlexArrayStorage&) = delete;
    FlexArrayStorage& operator=(const FlexArrayStorage&) = delete;
};

/* A flexible array type. Used either standalone or at the end of a parent
 * struct, with placement new, to have a run-time-sized array that's embedded
 * with its size.
 */
template<typename T, size_t alignment=alignof(T)>
struct FlexArray {
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using index_type = size_t;
    using difference_type = ptrdiff_t;

    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using Storage_t_ = FlexArrayStorage<element_type,alignment>;

    Storage_t_ mStore;

    static constexpr index_type Sizeof(index_type count, index_type base=0u) noexcept
    { return Storage_t_::Sizeof(count, base); }
    static std::unique_ptr<FlexArray> Create(index_type count)
    {
        void *ptr{al_calloc(alignof(FlexArray), Sizeof(count))};
        return std::unique_ptr<FlexArray>{al::construct_at(static_cast<FlexArray*>(ptr), count)};
    }

    FlexArray(index_type size) : mStore{size} { }
    ~FlexArray() = default;

    index_type size() const noexcept { return mStore.mSize; }
    bool empty() const noexcept { return mStore.mSize == 0; }

    pointer data() noexcept { return mStore.mArray; }
    const_pointer data() const noexcept { return mStore.mArray; }

    reference operator[](index_type i) noexcept { return mStore.mArray[i]; }
    const_reference operator[](index_type i) const noexcept { return mStore.mArray[i]; }

    reference front() noexcept { return mStore.mArray[0]; }
    const_reference front() const noexcept { return mStore.mArray[0]; }

    reference back() noexcept { return mStore.mArray[mStore.mSize-1]; }
    const_reference back() const noexcept { return mStore.mArray[mStore.mSize-1]; }

    iterator begin() noexcept { return mStore.mArray; }
    const_iterator begin() const noexcept { return mStore.mArray; }
    const_iterator cbegin() const noexcept { return mStore.mArray; }
    iterator end() noexcept { return mStore.mArray + mStore.mSize; }
    const_iterator end() const noexcept { return mStore.mArray + mStore.mSize; }
    const_iterator cend() const noexcept { return mStore.mArray + mStore.mSize; }

    reverse_iterator rbegin() noexcept { return end(); }
    const_reverse_iterator rbegin() const noexcept { return end(); }
    const_reverse_iterator crbegin() const noexcept { return cend(); }
    reverse_iterator rend() noexcept { return begin(); }
    const_reverse_iterator rend() const noexcept { return begin(); }
    const_reverse_iterator crend() const noexcept { return cbegin(); }

    DEF_PLACE_NEWDEL()
};

} // namespace al

#endif /* AL_MALLOC_H */
