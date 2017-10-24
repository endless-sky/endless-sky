// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   idle_point_multiset.hpp
 *  Contains the definition of the \idle_point_multiset containers.
 *  These containers are not mapped containers and store values in space
 *  that can be represented as points.
 *
 *  Iterating these containers always yield a constant value iterator. That is
 *  because modifying the value stored in the container may compromise the
 *  ordering in the container. One way around this issue is to use a \ref
 *  idle_pointmap container or to \c const_cast the value dereferenced from
 *  the iterator.
 *
 *  \see idle_point_multiset
 */

#ifndef SPATIAL_IDLE_POINT_MULTISET_HPP
#define SPATIAL_IDLE_POINT_MULTISET_HPP

#include <memory>  // std::allocator
#include "function.hpp"
#include "bits/spatial_kdtree.hpp"

namespace spatial
{

  template<dimension_type Rank, typename Key,
           typename Compare = bracket_less<Key>,
           typename Alloc = std::allocator<Key> >
  struct idle_point_multiset
    : details::Kdtree<details::Static_rank<Rank>,
                      const Key, const Key, Compare, Alloc>
  {
  private:
    typedef details::Kdtree<details::Static_rank<Rank>, const Key, const Key,
                            Compare, Alloc> base_type;
    typedef idle_point_multiset<Rank, Key, Compare, Alloc> Self;

  public:
    idle_point_multiset() { }

    explicit idle_point_multiset(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    idle_point_multiset(const Compare& compare, const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, alloc)
    { }

    idle_point_multiset(const idle_point_multiset& other, bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_point_multiset&
    operator=(const idle_point_multiset& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

  /**
   *  Specialization for \idle_point_multiset with runtime rank support. The
   *  rank of the \idle_point_multiset can be determined at run time and does
   *  not need to be fixed at compile time. Using:
   *  \code
   *    struct point { ... };
   *    idle_point_multiset<0, point> my_set;
   *  \endcode
   *
   *  \see runtime_idle_point_multiset for more information about how to use this
   *  container.
   */
  template<typename Key, typename Compare, typename Alloc>
  struct idle_point_multiset<0, Key, Compare, Alloc>
    : details::Kdtree<details::Dynamic_rank, const Key, const Key,
                      Compare, Alloc>
  {
  private:
    typedef details::Kdtree<details::Dynamic_rank, const Key, const Key,
                            Compare, Alloc> base_type;
    typedef idle_point_multiset<0, Key, Compare, Alloc>    Self;

  public:
    idle_point_multiset() { }

    explicit idle_point_multiset(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_rank(dim); }

    idle_point_multiset(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_rank(dim); }

    explicit idle_point_multiset(const Compare& compare)
      : base_type(compare)
    { }

    idle_point_multiset(dimension_type dim, const Compare& compare,
                    const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, alloc)
    { except::check_rank(dim); }

    idle_point_multiset(const Compare& compare, const Alloc& alloc)
      : base_type(details::Dynamic_rank(), compare, alloc)
    { }

    idle_point_multiset(const idle_point_multiset& other, bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_point_multiset&
    operator=(const idle_point_multiset& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

}

#endif // SPATIAL_IDLE_POINT_MULTISET_HPP
