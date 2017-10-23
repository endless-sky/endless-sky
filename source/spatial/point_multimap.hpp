// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   point_multimap.hpp
 *  Contains the definition of the \point_multimap.
 */

#ifndef SPATIAL_POINT_MULTIMAP_HPP
#define SPATIAL_POINT_MULTIMAP_HPP

#include <memory>  // std::allocator
#include <utility> // std::pair
#include "function.hpp"
#include "bits/spatial_relaxed_kdtree.hpp"

namespace spatial
{
  /**
   *  These containers are mapped containers and store values in space that can
   *  be represented as points.
   */
  template<dimension_type Rank, typename Key, typename Mapped,
           typename Compare = bracket_less<Key>,
           typename BalancingPolicy = loose_balancing,
           typename Alloc = std::allocator<std::pair<const Key, Mapped> > >
  struct point_multimap
    : details::Relaxed_kdtree<details::Static_rank<Rank>, const Key,
                              std::pair<const Key, Mapped>, Compare,
                              BalancingPolicy, Alloc>
  {
  private:
    typedef details::Relaxed_kdtree
    <details::Static_rank<Rank>, const Key, std::pair<const Key, Mapped>,
     Compare, BalancingPolicy, Alloc>         base_type;
    typedef point_multimap<Rank, Key, Mapped, Compare,
                     BalancingPolicy, Alloc>  Self;

  public:
    typedef Mapped                            mapped_type;

    point_multimap() { }

    explicit point_multimap(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    point_multimap(const Compare& compare, const BalancingPolicy& balancing)
      : base_type(details::Static_rank<Rank>(), compare, balancing)
    { }

    point_multimap(const Compare& compare, const BalancingPolicy& balancing,
             const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, balancing, alloc)
    { }

    point_multimap(const point_multimap& other)
      : base_type(other)
    { }

    point_multimap&
    operator=(const point_multimap& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };

  /**
   *  When specified with a null dimension, the rank of the point_multimap can
   *  be determined at run time and does not need to be fixed at compile time.
   */
  template<typename Key, typename Mapped, typename Compare,
           typename BalancingPolicy, typename Alloc>
  struct point_multimap<0, Key, Mapped, Compare, BalancingPolicy, Alloc>
    : details::Relaxed_kdtree<details::Dynamic_rank, const Key,
                              std::pair<const Key, Mapped>, Compare,
                              BalancingPolicy, Alloc>
  {
  private:
    typedef details::Relaxed_kdtree
    <details::Dynamic_rank, const Key, std::pair<const Key, Mapped>,
     Compare, BalancingPolicy, Alloc>       base_type;
    typedef point_multimap<0, Key, Mapped, Compare, BalancingPolicy, Alloc>
    Self;

  public:
    typedef Mapped mapped_type;

    point_multimap() { }

    explicit point_multimap(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_rank(dim); }

    point_multimap(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_rank(dim); }

    point_multimap(dimension_type dim, const Compare& compare,
                     const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(dim), compare, policy)
    { except::check_rank(dim); }

    point_multimap(dimension_type dim, const Compare& compare,
                     const BalancingPolicy& policy, const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, policy, alloc)
    { except::check_rank(dim); }

    explicit point_multimap(const Compare& compare)
      : base_type(details::Dynamic_rank(), compare)
    { }

    point_multimap(const Compare& compare, const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(), compare, policy)
    { }

    point_multimap(const Compare& compare, const BalancingPolicy& policy,
                     const Alloc& alloc)
      : base_type(details::Dynamic_rank(), compare, policy, alloc)
    { }

    point_multimap(const point_multimap& other)
      : base_type(other)
    { }

    point_multimap&
    operator=(const point_multimap& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };
}

#endif // SPATIAL_POINT_MULTIMAP_HPP
