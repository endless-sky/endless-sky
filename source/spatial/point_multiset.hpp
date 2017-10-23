// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   point_multiset.hpp
 *  Contains the definition of the \point_multiset. These
 *  containers are not mapped containers and store values in space that can
 *  be represented as points.
 *
 *  Iterating these containers always yield a constant value iterator. That is
 *  because modifying the value stored in the container may compromise the
 *  ordering in the container. One way around this issue is to use a \ref
 *  pointmap container or to \c const_cast the value dereferenced from the
 *  iterator.
 *
 *  \see point_multiset
 */

#ifndef SPATIAL_POINT_MULTISET_HPP
#define SPATIAL_POINT_MULTISET_HPP

#include <memory>  // std::allocator
#include "function.hpp"
#include "bits/spatial_relaxed_kdtree.hpp"

namespace spatial
{

  template<dimension_type Rank, typename Key,
           typename Compare = bracket_less<Key>,
           typename BalancingPolicy = loose_balancing,
           typename Alloc = std::allocator<Key> >
  struct point_multiset
    : details::Relaxed_kdtree<details::Static_rank<Rank>, const Key, const Key,
                              Compare, BalancingPolicy, Alloc>
  {
  private:
    typedef
    details::Relaxed_kdtree<details::Static_rank<Rank>, const Key, const Key,
                            Compare, BalancingPolicy, Alloc>      base_type;
    typedef point_multiset<Rank, Key, Compare, BalancingPolicy, Alloc>  Self;

  public:
    point_multiset() { }

    explicit point_multiset(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    point_multiset(const Compare& compare, const BalancingPolicy& balancing)
      : base_type(details::Static_rank<Rank>(), compare, balancing)
    { }

    point_multiset(const Compare& compare, const BalancingPolicy& balancing,
             const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, balancing, alloc)
    { }

    point_multiset(const point_multiset& other)
      : base_type(other)
    { }

    point_multiset&
    operator=(const point_multiset& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };

  /**
   *  Specialization for \point_multiset with runtime rank support. The rank of
   *  the \point_multiset can be determined at run time and does not need to be
   *  fixed at compile time. Using:
   *
   *  \code
   *    struct point { ... };
   *    point_multiset<0, point> my_set;
   *  \endcode
   */
  template<typename Key, typename Compare, typename BalancingPolicy,
           typename Alloc>
  struct point_multiset<0, Key, Compare, BalancingPolicy, Alloc>
    : details::Relaxed_kdtree<details::Dynamic_rank, const Key, const Key,
                              Compare, BalancingPolicy, Alloc>
  {
  private:
    typedef details::Relaxed_kdtree<details::Dynamic_rank, const Key, const Key,
                                    Compare, BalancingPolicy, Alloc> base_type;
    typedef point_multiset<0, Key, Compare, BalancingPolicy, Alloc>        Self;

  public:
    point_multiset() { }

    explicit point_multiset(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_rank(dim); }

    point_multiset(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_rank(dim); }

    point_multiset(dimension_type dim, const Compare& compare,
                     const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(dim), compare, policy)
    { except::check_rank(dim); }

    point_multiset(dimension_type dim, const Compare& compare,
                     const BalancingPolicy& policy, const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, policy, alloc)
    { except::check_rank(dim); }

    explicit point_multiset(const Compare& compare)
      : base_type(details::Dynamic_rank(), compare)
    { }

    point_multiset(const Compare& compare, const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(), compare, policy)
    { }

    point_multiset(const Compare& compare, const BalancingPolicy& policy,
                     const Alloc& alloc)
      : base_type(details::Dynamic_rank(), compare, policy, alloc)
    { }

    point_multiset(const point_multiset& other)
      : base_type(other)
    { }

    point_multiset&
    operator=(const point_multiset& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };

}

#endif // SPATIAL_POINT_MULTISET_HPP
