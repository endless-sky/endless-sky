#ifndef AL_SPAN_H
#define AL_SPAN_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace al {

template<typename T>
constexpr auto size(const T &cont) noexcept(noexcept(cont.size())) -> decltype(cont.size())
{ return cont.size(); }

template<typename T, size_t N>
constexpr size_t size(const T (&)[N]) noexcept
{ return N; }


template<typename T>
constexpr auto data(T &cont) noexcept(noexcept(cont.data())) -> decltype(cont.data())
{ return cont.data(); }

template<typename T>
constexpr auto data(const T &cont) noexcept(noexcept(cont.data())) -> decltype(cont.data())
{ return cont.data(); }

template<typename T, size_t N>
constexpr T* data(T (&arr)[N]) noexcept
{ return arr; }

template<typename T>
constexpr const T* data(std::initializer_list<T> list) noexcept
{ return list.begin(); }


constexpr size_t dynamic_extent{static_cast<size_t>(-1)};

template<typename T, size_t E=dynamic_extent>
class span;

namespace detail_ {
    template<typename... Ts>
    struct make_void { using type = void; };
    template<typename... Ts>
    using void_t = typename make_void<Ts...>::type;

    template<typename T>
    struct is_span_ : std::false_type { };
    template<typename T, size_t E>
    struct is_span_<span<T,E>> : std::true_type { };
    template<typename T>
    using is_span = is_span_<std::remove_cv_t<T>>;

    template<typename T>
    struct is_std_array_ : std::false_type { };
    template<typename T, size_t N>
    struct is_std_array_<std::array<T,N>> : std::true_type { };
    template<typename T>
    using is_std_array = is_std_array_<std::remove_cv_t<T>>;

    template<typename T, typename = void>
    struct has_size_and_data : std::false_type { };
    template<typename T>
    struct has_size_and_data<T,
        void_t<decltype(al::size(std::declval<T>())), decltype(al::data(std::declval<T>()))>>
        : std::true_type { };
} // namespace detail_

#define REQUIRES(...) bool rt_=true, std::enable_if_t<rt_ && (__VA_ARGS__),bool> = true
#define IS_VALID_CONTAINER(C)                                                 \
    !detail_::is_span<C>::value && !detail_::is_std_array<C>::value &&        \
    !std::is_array<C>::value && detail_::has_size_and_data<C>::value &&       \
    std::is_convertible<std::remove_pointer_t<decltype(al::data(std::declval<C&>()))>(*)[],element_type(*)[]>::value

template<typename T, size_t E>
class span {
public:
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

    static constexpr size_t extent{E};

    template<REQUIRES(extent==0)>
    constexpr span() noexcept { }
    constexpr span(pointer ptr, index_type /*count*/) : mData{ptr} { }
    constexpr span(pointer first, pointer /*last*/) : mData{first} { }
    constexpr span(element_type (&arr)[E]) noexcept : span{al::data(arr), al::size(arr)} { }
    constexpr span(std::array<value_type,E> &arr) noexcept : span{al::data(arr), al::size(arr)} { }
    template<REQUIRES(std::is_const<element_type>::value)>
    constexpr span(const std::array<value_type,E> &arr) noexcept
      : span{al::data(arr), al::size(arr)}
    { }
    template<typename U, REQUIRES(IS_VALID_CONTAINER(U))>
    constexpr span(U &cont) : span{al::data(cont), al::size(cont)} { }
    template<typename U, REQUIRES(IS_VALID_CONTAINER(const U))>
    constexpr span(const U &cont) : span{al::data(cont), al::size(cont)} { }
    template<typename U, REQUIRES(!std::is_same<element_type,U>::value
        && std::is_convertible<U(*)[],element_type(*)[]>::value)>
    constexpr span(const span<U,E> &span_) noexcept : span{al::data(span_), al::size(span_)} { }
    constexpr span(const span&) noexcept = default;

    constexpr span& operator=(const span &rhs) noexcept = default;

    constexpr reference front() const { return *mData; }
    constexpr reference back() const { return *(mData+E-1); }
    constexpr reference operator[](index_type idx) const { return mData[idx]; }
    constexpr pointer data() const noexcept { return mData; }

    constexpr index_type size() const noexcept { return E; }
    constexpr index_type size_bytes() const noexcept { return E * sizeof(value_type); }
    constexpr bool empty() const noexcept { return E == 0; }

    constexpr iterator begin() const noexcept { return mData; }
    constexpr iterator end() const noexcept { return mData+E; }
    constexpr const_iterator cbegin() const noexcept { return mData; }
    constexpr const_iterator cend() const noexcept { return mData+E; }

    constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
    constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
    constexpr const_reverse_iterator crbegin() const noexcept
    { return const_reverse_iterator{cend()}; }
    constexpr const_reverse_iterator crend() const noexcept
    { return const_reverse_iterator{cbegin()}; }

    template<size_t C>
    constexpr span<element_type,C> first() const
    {
        static_assert(E >= C, "New size exceeds original capacity");
        return span<element_type,C>{mData, C};
    }

    template<size_t C>
    constexpr span<element_type,C> last() const
    {
        static_assert(E >= C, "New size exceeds original capacity");
        return span<element_type,C>{mData+(E-C), C};
    }

    template<size_t O, size_t C>
    constexpr auto subspan() const -> std::enable_if_t<C!=dynamic_extent,span<element_type,C>>
    {
        static_assert(E >= O, "Offset exceeds extent");
        static_assert(E-O >= C, "New size exceeds original capacity");
        return span<element_type,C>{mData+O, C};
    }

    template<size_t O, size_t C=dynamic_extent>
    constexpr auto subspan() const -> std::enable_if_t<C==dynamic_extent,span<element_type,E-O>>
    {
        static_assert(E >= O, "Offset exceeds extent");
        return span<element_type,E-O>{mData+O, E-O};
    }

    /* NOTE: Can't declare objects of a specialized template class prior to
     * defining the specialization. As a result, these methods need to be
     * defined later.
     */
    constexpr span<element_type,dynamic_extent> first(size_t count) const;
    constexpr span<element_type,dynamic_extent> last(size_t count) const;
    constexpr span<element_type,dynamic_extent> subspan(size_t offset,
        size_t count=dynamic_extent) const;

private:
    pointer mData{nullptr};
};

template<typename T>
class span<T,dynamic_extent> {
public:
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

    static constexpr size_t extent{dynamic_extent};

    constexpr span() noexcept = default;
    constexpr span(pointer ptr, index_type count) : mData{ptr}, mDataEnd{ptr+count} { }
    constexpr span(pointer first, pointer last) : mData{first}, mDataEnd{last} { }
    template<size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : span{al::data(arr), al::size(arr)} { }
    template<size_t N>
    constexpr span(std::array<value_type,N> &arr) noexcept : span{al::data(arr), al::size(arr)} { }
    template<size_t N, REQUIRES(std::is_const<element_type>::value)>
    constexpr span(const std::array<value_type,N> &arr) noexcept
      : span{al::data(arr), al::size(arr)}
    { }
    template<typename U, REQUIRES(IS_VALID_CONTAINER(U))>
    constexpr span(U &cont) : span{al::data(cont), al::size(cont)} { }
    template<typename U, REQUIRES(IS_VALID_CONTAINER(const U))>
    constexpr span(const U &cont) : span{al::data(cont), al::size(cont)} { }
    template<typename U, size_t N, REQUIRES((!std::is_same<element_type,U>::value || extent != N)
        && std::is_convertible<U(*)[],element_type(*)[]>::value)>
    constexpr span(const span<U,N> &span_) noexcept : span{al::data(span_), al::size(span_)} { }
    constexpr span(const span&) noexcept = default;

    constexpr span& operator=(const span &rhs) noexcept = default;

    constexpr reference front() const { return *mData; }
    constexpr reference back() const { return *(mDataEnd-1); }
    constexpr reference operator[](index_type idx) const { return mData[idx]; }
    constexpr pointer data() const noexcept { return mData; }

    constexpr index_type size() const noexcept { return static_cast<index_type>(mDataEnd-mData); }
    constexpr index_type size_bytes() const noexcept
    { return static_cast<index_type>(mDataEnd-mData) * sizeof(value_type); }
    constexpr bool empty() const noexcept { return mData == mDataEnd; }

    constexpr iterator begin() const noexcept { return mData; }
    constexpr iterator end() const noexcept { return mDataEnd; }
    constexpr const_iterator cbegin() const noexcept { return mData; }
    constexpr const_iterator cend() const noexcept { return mDataEnd; }

    constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
    constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
    constexpr const_reverse_iterator crbegin() const noexcept
    { return const_reverse_iterator{cend()}; }
    constexpr const_reverse_iterator crend() const noexcept
    { return const_reverse_iterator{cbegin()}; }

    template<size_t C>
    constexpr span<element_type,C> first() const
    { return span<element_type,C>{mData, C}; }

    constexpr span first(size_t count) const
    { return (count >= size()) ? *this : span{mData, mData+count}; }

    template<size_t C>
    constexpr span<element_type,C> last() const
    { return span<element_type,C>{mDataEnd-C, C}; }

    constexpr span last(size_t count) const
    { return (count >= size()) ? *this : span{mDataEnd-count, mDataEnd}; }

    template<size_t O, size_t C>
    constexpr auto subspan() const -> std::enable_if_t<C!=dynamic_extent,span<element_type,C>>
    { return span<element_type,C>{mData+O, C}; }

    template<size_t O, size_t C=dynamic_extent>
    constexpr auto subspan() const -> std::enable_if_t<C==dynamic_extent,span<element_type,C>>
    { return span<element_type,C>{mData+O, mDataEnd}; }

    constexpr span subspan(size_t offset, size_t count=dynamic_extent) const
    {
        return (offset > size()) ? span{} :
            (count >= size()-offset) ? span{mData+offset, mDataEnd} :
            span{mData+offset, mData+offset+count};
    }

private:
    pointer mData{nullptr};
    pointer mDataEnd{nullptr};
};

template<typename T, size_t E>
constexpr inline auto span<T,E>::first(size_t count) const -> span<element_type,dynamic_extent>
{
    return (count >= size()) ? span<element_type>{mData, extent} :
        span<element_type>{mData, count};
}

template<typename T, size_t E>
constexpr inline auto span<T,E>::last(size_t count) const -> span<element_type,dynamic_extent>
{
    return (count >= size()) ? span<element_type>{mData, extent} :
        span<element_type>{mData+extent-count, count};
}

template<typename T, size_t E>
constexpr inline auto span<T,E>::subspan(size_t offset, size_t count) const
    -> span<element_type,dynamic_extent>
{
    return (offset > size()) ? span<element_type>{} :
        (count >= size()-offset) ? span<element_type>{mData+offset, mData+extent} :
        span<element_type>{mData+offset, mData+offset+count};
}

#undef IS_VALID_CONTAINER
#undef REQUIRES

} // namespace al

#endif /* AL_SPAN_H */
