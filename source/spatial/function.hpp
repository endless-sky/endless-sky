// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   function.hpp
 *  Main functors that are used in the library.
 */

#ifndef SPATIAL_FUNCTION_HPP
#define SPATIAL_FUNCTION_HPP

#include <iterator> // std::advance
#include "spatial.hpp"

namespace spatial
{
  /**
   *  This functor uses the minus operator to calculate the difference between 2
   *  elements of Tp along the dimension \c n accessed through a custom
   *  accessor. The returned value is cast into the type \c Unit.
   *
   *  \concept_difference
   */
  template <typename Accessor, typename Tp, typename Unit>
  struct accessor_minus
    : private Accessor // empty member optimization
  {
    explicit accessor_minus(Accessor accessor_ = Accessor())
      : Accessor(accessor_)
    { }

    template <typename AnyUnit>
    accessor_minus(const accessor_minus<Accessor, Tp, AnyUnit>& other)
      : Accessor(other.accessor())
    { }

    Unit
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    { return Accessor::operator()(n, x) - Accessor::operator()(n, y); }

    Accessor accessor() const { return *static_cast<const Accessor*>(this); }
  };

  /**
   *  This functor uses the minus operator to calculate the difference between 2
   *  elements of Tp along the dimension \c n accessed through the bracket
   *  operator. The returned value is cast into the type \c Unit.
   *
   *  \concept_difference
   */
  template <typename Tp, typename Unit>
  struct bracket_minus
  {
    bracket_minus() { }

    template <typename AnyUnit>
    bracket_minus(const bracket_minus<Tp, AnyUnit>&) { }

    Unit
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    { return x[n] - y[n]; }
  };

  /**
   *  \brief This functor uses the minus operator to calculate the difference
   *  between 2 elements of Tp along the dimension \c n accessed through the
   *  parenthesis operator. The returned value is cast into the type \c Unit.
   *
   *  \concept_difference
   */
  template <typename Tp, typename Unit>
  struct paren_minus
  {
    paren_minus() { }

    template <typename AnyUnit>
    paren_minus(const paren_minus<Tp, AnyUnit>&) { }

    Unit
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    { return x(n) - y(n); }
  };

  /**
   *  This functor uses the minus operator to calculate the difference between 2
   *  elements of Tp along the dimension \c n accessed through an iterator. The
   *  returned value is cast into the type \c Unit.
   *
   *  \concept_difference
   */
  template <typename Tp, typename Unit>
  struct iterator_minus
  {
    iterator_minus() { }

    template <typename AnyUnit>
    iterator_minus(const iterator_minus<Tp, AnyUnit>&) { }

    Unit
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    {
      typename Tp::const_iterator ix = x.begin();
      typename Tp::const_iterator iy = y.begin();
      {
        using namespace std;
        typedef typename std::iterator_traits<typename Tp::const_iterator>
          ::difference_type diff_t;
        advance(ix, static_cast<diff_t>(n));
        advance(iy, static_cast<diff_t>(n));
      }
      return *ix - *iy;
    }
  };

  /**
   *  A comparator that simplifies using the spatial containers with a Key type
   *  that has coordinate that are not accessible via the bracket, parenthesis
   *  operator or iterator deference.
   *
   *  \concept_generalized_compare
   *
   *  Generally, the spatial containers are used with one of \ref bracket_less,
   *  \ref paren_less, or \ref iterator_less. However, when the key used in the
   *  container cannot be compared through one of the above comparator, the
   *  following helper comparator can be used to define one through a custom
   *  accessor.
   */
  template<typename Accessor, typename Tp>
  struct accessor_less
    : private Accessor // empty member optimization
  {
    explicit accessor_less(Accessor access = Accessor())
      : Accessor(access)
    { }

    bool
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    {
      return (Accessor::operator()(n, x) < Accessor::operator()(n, y));
    }

    bool operator()
    (dimension_type a, const Tp& x, dimension_type b, const Tp& y) const
    {
      return (Accessor::operator()(a, x) < Accessor::operator()(b, y));
    }

    const Accessor& accessor() const
    { return *static_cast<const Accessor*>(this); }
  };

  /**
   *  A comparator that simplifies using the spatial containers with a Key type
   *  that has coordiates accessible via the bracket operator.
   *
   *  \concept_generalized_compare
   */
  template <typename Tp>
  struct bracket_less
  {
    bool
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    {
      return (x[n] < y[n]);
    }

    bool operator()
    (dimension_type a, const Tp& x, dimension_type b, const Tp& y) const
    {
      return (x[a] < y[b]);
    }
  };

  /**
   *  A comparator that simplifies using the spatial containers with a Key type
   *  that has coordiates accessible via the parenthesis operator.
   *
   *  \concept_generalized_compare
   */
  template <typename Tp>
  struct paren_less
  {
    bool
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    {
      return (x(n) < y(n));
    }

    bool operator()
    (dimension_type a, const Tp& x, dimension_type b, const Tp& y) const
    {
      return (x(a) < y(b));
    }
  };

  /**
   *  A comparator that simplifies using the spatial containers with a Key type
   *  that has coordiates accessible via iterator deference.
   *
   *  \concept_generalized_compare
   */
  template <typename Tp>
  struct iterator_less
  {
    bool
    operator() (dimension_type n, const Tp& x, const Tp& y) const
    {
      typename Tp::const_iterator ix = x.begin();
      typename Tp::const_iterator iy = y.begin();
      {
        using namespace std;
        typedef typename std::iterator_traits<typename Tp::const_iterator>
          ::difference_type diff_t;
        advance(ix, static_cast<diff_t>(n));
        advance(iy, static_cast<diff_t>(n));
      }
      return (*ix < *iy);
    }

    bool operator()
    (dimension_type a, const Tp& x, dimension_type b, const Tp& y) const
    {
      typename Tp::const_iterator ix = x.begin();
      typename Tp::const_iterator iy = y.begin();
      {
        using namespace std;
        typedef typename std::iterator_traits<typename Tp::const_iterator>
          ::difference_type diff_t;
        advance(ix, static_cast<diff_t>(a));
        advance(iy, static_cast<diff_t>(b));
      }
      return (*ix < *iy);
    }
  };

} // namespace spatial

#endif // SPATIAL_FUNCTION_HPP
