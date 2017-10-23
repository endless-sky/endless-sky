// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   idle_point_multimap.hpp
 *  Contains the definition of the \idle_point_multimap.
 */

#ifndef SPATIAL_IDLE_POINT_MULTIMAP_HPP
#define SPATIAL_IDLE_POINT_MULTIMAP_HPP

#include <memory>  // std::allocator
#include <utility> // std::pair
#include "function.hpp"
#include "bits/spatial_kdtree.hpp"

namespace spatial
{

  /**
   *  These containers are mapped containers and store values in space that can
   *  be represented as points.
   */
  template<dimension_type Rank, typename Key, typename Mapped,
           typename Compare = bracket_less<Key>,
           typename Alloc = std::allocator<std::pair<const Key, Mapped> > >
  struct idle_point_multimap
    : details::Kdtree<details::Static_rank<Rank>, const Key,
                      std::pair<const Key, Mapped>, Compare, Alloc>
  {
  private:
    typedef details::Kdtree<details::Static_rank<Rank>, const Key,
                            std::pair<const Key, Mapped>, Compare,
                            Alloc>            base_type;
    typedef idle_point_multimap<Rank, Key, Mapped,
                          Compare, Alloc>     Self;

  public:
    typedef Mapped                            mapped_type;

    idle_point_multimap() { }

    explicit idle_point_multimap(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    idle_point_multimap(const Compare& compare, const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, alloc)
    { }

    idle_point_multimap(const idle_point_multimap& other, bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_point_multimap&
    operator=(const idle_point_multimap& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

  /**
   *  When specified with a null dimension, the rank of the point_multimap can
   *  be determined at run time and is not fixed at compile time.
   */
  template<typename Key, typename Mapped, typename Compare, typename Alloc>
  struct idle_point_multimap<0, Key, Mapped, Compare, Alloc>
    : details::Kdtree<details::Dynamic_rank, const Key,
                      std::pair<const Key, Mapped>, Compare, Alloc>
  {
  private:
    typedef details::Kdtree<details::Dynamic_rank, const Key,
                            std::pair<const Key, Mapped>,
                            Compare, Alloc> base_type;
    typedef idle_point_multimap<0, Key, Mapped, Compare, Alloc> Self;

  public:
    typedef Mapped mapped_type;

    idle_point_multimap() { }

    explicit idle_point_multimap(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_rank(dim); }

    idle_point_multimap(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_rank(dim); }

    explicit idle_point_multimap(const Compare& compare)
      : base_type(compare)
    { }

    idle_point_multimap(dimension_type dim, const Compare& compare,
                            const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, alloc)
    { except::check_rank(dim); }

    idle_point_multimap(const Compare& compare, const Alloc& alloc)
      : base_type(details::Dynamic_rank(), compare, alloc)
    { }

    idle_point_multimap(const idle_point_multimap& other,
                            bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_point_multimap&
    operator=(const idle_point_multimap& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

}

#endif // SPATIAL_IDLE_POINT_MULTIMAP_HPP
