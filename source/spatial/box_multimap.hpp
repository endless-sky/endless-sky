// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   box_multimap.hpp
 *  Contains the definition of the \box_multimap containers.
 */

#ifndef SPATIAL_BOX_MULTIMAP_HPP
#define SPATIAL_BOX_MULTIMAP_HPP

#include <memory>  // std::allocator
#include <utility> // std::pair
#include "function.hpp"
#include "bits/spatial_check_concept.hpp"
#include "bits/spatial_relaxed_kdtree.hpp"

namespace spatial
{

  /**
   *  A mapped containers to store values in space that can be represented as
   *  boxes.
   */
  template<dimension_type Rank, typename Key, typename Mapped,
           typename Compare = bracket_less<Key>,
           typename BalancingPolicy = loose_balancing,
           typename Alloc = std::allocator<std::pair<const Key, Mapped> > >
  class box_multimap
    : public details::Relaxed_kdtree<details::Static_rank<Rank>, const Key,
                                     std::pair<const Key, Mapped>,
                                     Compare, BalancingPolicy, Alloc>
  {
  private:
    typedef typename
    enable_if_c<(Rank & 1u) == 0>::type check_concept_dimension_is_even;

    typedef details::Relaxed_kdtree
    <details::Static_rank<Rank>, const Key, std::pair<const Key, Mapped>,
     Compare, BalancingPolicy, Alloc>         base_type;
    typedef box_multimap<Rank, Key, Mapped, Compare,
                   BalancingPolicy, Alloc>    Self;

  public:
    typedef Mapped                            mapped_type;

    box_multimap() { }

    explicit box_multimap(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    box_multimap(const Compare& compare, const BalancingPolicy& balancing)
      : base_type(details::Static_rank<Rank>(), compare, balancing)
    { }

    box_multimap(const Compare& compare, const BalancingPolicy& balancing,
             const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, balancing, alloc)
    { }

    box_multimap(const box_multimap& other)
      : base_type(other)
    { }

    box_multimap&
    operator=(const box_multimap& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };

  /**
   *  Specialization for \box_multimap with runtime rank support. The rank of
   *  the \box_multimap can be determined at run time and does not need to be
   *  fixed at compile time.
   *
   *  The rank is then passed as a parameter to the constructor. Using:
   *
   *  \code
   *    struct box { ... };
   *    spatial::box_multimap<0, box, std::string> my_map(2);
   *  \endcode
   *
   *  If no parameter is given, the rank defaults to 2.
   */
  template<typename Key, typename Mapped,
           typename Compare,
           typename BalancingPolicy,
           typename Alloc>
  struct box_multimap<0, Key, Mapped, Compare, BalancingPolicy, Alloc>
    : details::Relaxed_kdtree<details::Dynamic_rank, const Key,
                              std::pair<const Key, Mapped>, Compare,
                              BalancingPolicy, Alloc>
  {
  private:
    typedef details::Relaxed_kdtree
    <details::Dynamic_rank, const Key, std::pair<const Key, Mapped>,
     Compare, BalancingPolicy, Alloc>         base_type;
    typedef box_multimap<0, Key, Mapped, Compare,
                   BalancingPolicy, Alloc>    Self;

  public:
    typedef Mapped                            mapped_type;

    box_multimap() : base_type(details::Dynamic_rank(2)) { }

    explicit box_multimap(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_even_rank(dim); }

    box_multimap(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_even_rank(dim); }

    box_multimap(dimension_type dim, const Compare& compare,
           const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(dim), compare, policy)
    { except::check_even_rank(dim); }

    box_multimap(dimension_type dim, const Compare& compare,
           const BalancingPolicy& policy, const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, policy, alloc)
    { except::check_even_rank(dim); }

    explicit box_multimap(const Compare& compare)
      : base_type(details::Dynamic_rank(2), compare)
    { }

    box_multimap(const Compare& compare, const BalancingPolicy& policy)
      : base_type(details::Dynamic_rank(2), compare, policy)
    { }

    box_multimap(const Compare& compare, const BalancingPolicy& policy,
           const Alloc& alloc)
      : base_type(details::Dynamic_rank(2), compare, policy, alloc)
    { }

    box_multimap(const box_multimap& other)
      : base_type(other)
    { }

    box_multimap&
    operator=(const box_multimap& other)
    { return static_cast<Self&>(base_type::operator=(other)); }
  };

}

#endif // SPATIAL_BOX_MULTIMAP_HPP
