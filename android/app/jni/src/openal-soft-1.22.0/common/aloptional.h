#ifndef AL_OPTIONAL_H
#define AL_OPTIONAL_H

#include <initializer_list>
#include <type_traits>
#include <utility>

#include "almalloc.h"

namespace al {

struct nullopt_t { };
struct in_place_t { };

constexpr nullopt_t nullopt{};
constexpr in_place_t in_place{};

#define NOEXCEPT_AS(...)  noexcept(noexcept(__VA_ARGS__))

namespace detail_ {
/* Base storage struct for an optional. Defines a trivial destructor, for types
 * that can be trivially destructed.
 */
template<typename T, bool = std::is_trivially_destructible<T>::value>
struct optstore_base {
    bool mHasValue{false};
    union {
        char mDummy{};
        T mValue;
    };

    constexpr optstore_base() noexcept { }
    template<typename ...Args>
    constexpr explicit optstore_base(in_place_t, Args&& ...args)
        noexcept(std::is_nothrow_constructible<T, Args...>::value)
        : mHasValue{true}, mValue{std::forward<Args>(args)...}
    { }
    ~optstore_base() = default;
};

/* Specialization needing a non-trivial destructor. */
template<typename T>
struct optstore_base<T, false> {
    bool mHasValue{false};
    union {
        char mDummy{};
        T mValue;
    };

    constexpr optstore_base() noexcept { }
    template<typename ...Args>
    constexpr explicit optstore_base(in_place_t, Args&& ...args)
        noexcept(std::is_nothrow_constructible<T, Args...>::value)
        : mHasValue{true}, mValue{std::forward<Args>(args)...}
    { }
    ~optstore_base() { if(mHasValue) al::destroy_at(std::addressof(mValue)); }
};

/* Next level of storage, which defines helpers to construct and destruct the
 * stored object.
 */
template<typename T>
struct optstore_helper : public optstore_base<T> {
    using optstore_base<T>::optstore_base;

    template<typename... Args>
    constexpr void construct(Args&& ...args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
    {
        al::construct_at(std::addressof(this->mValue), std::forward<Args>(args)...);
        this->mHasValue = true;
    }

    constexpr void reset() noexcept
    {
        if(this->mHasValue)
            al::destroy_at(std::addressof(this->mValue));
        this->mHasValue = false;
    }

    constexpr void assign(const optstore_helper &rhs)
        noexcept(std::is_nothrow_copy_constructible<T>::value
            && std::is_nothrow_copy_assignable<T>::value)
    {
        if(!rhs.mHasValue)
            this->reset();
        else if(this->mHasValue)
            this->mValue = rhs.mValue;
        else
            this->construct(rhs.mValue);
    }

    constexpr void assign(optstore_helper&& rhs)
        noexcept(std::is_nothrow_move_constructible<T>::value
            && std::is_nothrow_move_assignable<T>::value)
    {
        if(!rhs.mHasValue)
            this->reset();
        else if(this->mHasValue)
            this->mValue = std::move(rhs.mValue);
        else
            this->construct(std::move(rhs.mValue));
    }
};

/* Define copy and move constructors and assignment operators, which may or may
 * not be trivial.
 */
template<typename T, bool trivial_copy = std::is_trivially_copy_constructible<T>::value,
    bool trivial_move = std::is_trivially_move_constructible<T>::value,
    /* Trivial assignment is dependent on trivial construction+destruction. */
    bool = trivial_copy && std::is_trivially_copy_assignable<T>::value
        && std::is_trivially_destructible<T>::value,
    bool = trivial_move && std::is_trivially_move_assignable<T>::value
        && std::is_trivially_destructible<T>::value>
struct optional_storage;

/* Some versions of GCC have issues with 'this' in the following noexcept(...)
 * statements, so this macro is a workaround.
 */
#define _this std::declval<optional_storage*>()

/* Completely trivial. */
template<typename T>
struct optional_storage<T, true, true, true, true> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage&) = default;
    constexpr optional_storage& operator=(optional_storage&&) = default;
};

/* Non-trivial move assignment. */
template<typename T>
struct optional_storage<T, true, true, true, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage&) = default;
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

/* Non-trivial move construction. */
template<typename T>
struct optional_storage<T, true, false, true, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&& rhs) NOEXCEPT_AS(_this->construct(std::move(rhs.mValue)))
    { if(rhs.mHasValue) this->construct(std::move(rhs.mValue)); }
    constexpr optional_storage& operator=(const optional_storage&) = default;
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

/* Non-trivial copy assignment. */
template<typename T>
struct optional_storage<T, true, true, false, true> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&&) = default;
};

/* Non-trivial copy construction. */
template<typename T>
struct optional_storage<T, false, true, false, true> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage &rhs) NOEXCEPT_AS(_this->construct(rhs.mValue))
    { if(rhs.mHasValue) this->construct(rhs.mValue); }
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&&) = default;
};

/* Non-trivial assignment. */
template<typename T>
struct optional_storage<T, true, true, false, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

/* Non-trivial assignment, non-trivial move construction. */
template<typename T>
struct optional_storage<T, true, false, false, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage&) = default;
    constexpr optional_storage(optional_storage&& rhs) NOEXCEPT_AS(_this->construct(std::move(rhs.mValue)))
    { if(rhs.mHasValue) this->construct(std::move(rhs.mValue)); }
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

/* Non-trivial assignment, non-trivial copy construction. */
template<typename T>
struct optional_storage<T, false, true, false, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage &rhs) NOEXCEPT_AS(_this->construct(rhs.mValue))
    { if(rhs.mHasValue) this->construct(rhs.mValue); }
    constexpr optional_storage(optional_storage&&) = default;
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

/* Completely non-trivial. */
template<typename T>
struct optional_storage<T, false, false, false, false> : public optstore_helper<T> {
    using optstore_helper<T>::optstore_helper;
    constexpr optional_storage() noexcept = default;
    constexpr optional_storage(const optional_storage &rhs) NOEXCEPT_AS(_this->construct(rhs.mValue))
    { if(rhs.mHasValue) this->construct(rhs.mValue); }
    constexpr optional_storage(optional_storage&& rhs) NOEXCEPT_AS(_this->construct(std::move(rhs.mValue)))
    { if(rhs.mHasValue) this->construct(std::move(rhs.mValue)); }
    constexpr optional_storage& operator=(const optional_storage &rhs) NOEXCEPT_AS(_this->assign(rhs))
    { this->assign(rhs); return *this; }
    constexpr optional_storage& operator=(optional_storage&& rhs) NOEXCEPT_AS(_this->assign(std::move(rhs)))
    { this->assign(std::move(rhs)); return *this; }
};

#undef _this

} // namespace detail_

#define REQUIRES(...) std::enable_if_t<(__VA_ARGS__),bool> = true

template<typename T>
class optional {
    using storage_t = detail_::optional_storage<T>;

    storage_t mStore{};

public:
    using value_type = T;

    constexpr optional() = default;
    constexpr optional(const optional&) = default;
    constexpr optional(optional&&) = default;
    constexpr optional(nullopt_t) noexcept { }
    template<typename ...Args>
    constexpr explicit optional(in_place_t, Args&& ...args)
        NOEXCEPT_AS(storage_t{al::in_place, std::forward<Args>(args)...})
        : mStore{al::in_place, std::forward<Args>(args)...}
    { }
    template<typename U, REQUIRES(std::is_constructible<T, U&&>::value
        && !std::is_same<std::decay_t<U>, al::in_place_t>::value
        && !std::is_same<std::decay_t<U>, optional<T>>::value
        && std::is_convertible<U&&, T>::value)>
    constexpr optional(U&& rhs) NOEXCEPT_AS(storage_t{al::in_place, std::forward<U>(rhs)})
        : mStore{al::in_place, std::forward<U>(rhs)}
    { }
    template<typename U, REQUIRES(std::is_constructible<T, U&&>::value
        && !std::is_same<std::decay_t<U>, al::in_place_t>::value
        && !std::is_same<std::decay_t<U>, optional<T>>::value
        && !std::is_convertible<U&&, T>::value)>
    constexpr explicit optional(U&& rhs) NOEXCEPT_AS(storage_t{al::in_place, std::forward<U>(rhs)})
        : mStore{al::in_place, std::forward<U>(rhs)}
    { }
    ~optional() = default;

    constexpr optional& operator=(const optional&) = default;
    constexpr optional& operator=(optional&&) = default;
    constexpr optional& operator=(nullopt_t) noexcept { mStore.reset(); return *this; }
    template<typename U=T>
    constexpr std::enable_if_t<std::is_constructible<T, U>::value
        && std::is_assignable<T&, U>::value
        && !std::is_same<std::decay_t<U>, optional<T>>::value
        && (!std::is_same<std::decay_t<U>, T>::value || !std::is_scalar<U>::value),
    optional&> operator=(U&& rhs)
    {
        if(mStore.mHasValue)
            mStore.mValue = std::forward<U>(rhs);
        else
            mStore.construct(std::forward<U>(rhs));
        return *this;
    }

    constexpr const T* operator->() const { return std::addressof(mStore.mValue); }
    constexpr T* operator->() { return std::addressof(mStore.mValue); }
    constexpr const T& operator*() const& { return mStore.mValue; }
    constexpr T& operator*() & { return mStore.mValue; }
    constexpr const T&& operator*() const&& { return std::move(mStore.mValue); }
    constexpr T&& operator*() && { return std::move(mStore.mValue); }

    constexpr explicit operator bool() const noexcept { return mStore.mHasValue; }
    constexpr bool has_value() const noexcept { return mStore.mHasValue; }

    constexpr T& value() & { return mStore.mValue; }
    constexpr const T& value() const& { return mStore.mValue; }
    constexpr T&& value() && { return std::move(mStore.mValue); }
    constexpr const T&& value() const&& { return std::move(mStore.mValue); }

    template<typename U>
    constexpr T value_or(U&& defval) const&
    { return bool{*this} ? **this : static_cast<T>(std::forward<U>(defval)); }
    template<typename U>
    constexpr T value_or(U&& defval) &&
    { return bool{*this} ? std::move(**this) : static_cast<T>(std::forward<U>(defval)); }

    template<typename ...Args>
    constexpr T& emplace(Args&& ...args)
    {
        mStore.reset();
        mStore.construct(std::forward<Args>(args)...);
        return mStore.mValue;
    }
    template<typename U, typename ...Args>
    constexpr std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value,
    T&> emplace(std::initializer_list<U> il, Args&& ...args)
    {
        mStore.reset();
        mStore.construct(il, std::forward<Args>(args)...);
        return mStore.mValue;
    }

    constexpr void reset() noexcept { mStore.reset(); }
};

template<typename T>
constexpr optional<std::decay_t<T>> make_optional(T&& arg)
{ return optional<std::decay_t<T>>{in_place, std::forward<T>(arg)}; }

template<typename T, typename... Args>
constexpr optional<T> make_optional(Args&& ...args)
{ return optional<T>{in_place, std::forward<Args>(args)...}; }

template<typename T, typename U, typename... Args>
constexpr optional<T> make_optional(std::initializer_list<U> il, Args&& ...args)
{ return optional<T>{in_place, il, std::forward<Args>(args)...}; }

#undef REQUIRES
#undef NOEXCEPT_AS
} // namespace al

#endif /* AL_OPTIONAL_H */
