// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   idle_box_multimap.hpp
 *  Contains the definition of the \idle_box_multimap containers.
 *  These containers are mapped containers and store values in space that can
 *  be represented as boxes.
 *
 *  \see idle_box_multimap
 *  \see runtime_idle_box_multimap
 */

#ifndef SPATIAL_IDLE_BOX_MULTIMAP_HPP
#define SPATIAL_IDLE_BOX_MULTIMAP_HPP

#include <memory>  // std::allocator
#include <utility> // std::pair
#include "function.hpp"
#include "bits/spatial_check_concept.hpp"
#include "bits/spatial_kdtree.hpp"

namespace spatial
{

  template<dimension_type Rank, typename Key, typename Mapped,
           typename Compare = bracket_less<Key>,
           typename Alloc = std::allocator<std::pair<const Key, Mapped> > >
  class idle_box_multimap
    : public details::Kdtree<details::Static_rank<Rank>, const Key,
                             std::pair<const Key, Mapped>, Compare, Alloc>
  {
  private:
    typedef typename
    enable_if_c<(Rank & 1u) == 0>::type check_concept_dimension_is_even;

    typedef details::Kdtree
    <details::Static_rank<Rank>, const Key, std::pair<const Key, Mapped>,
     Compare, Alloc>                                       base_type;
    typedef idle_box_multimap<Rank, Key, Mapped, Compare, Alloc> Self;

  public:
    typedef Mapped                            mapped_type;

    idle_box_multimap() { }

    explicit idle_box_multimap(const Compare& compare)
      : base_type(details::Static_rank<Rank>(), compare)
    { }

    idle_box_multimap(const Compare& compare, const Alloc& alloc)
      : base_type(details::Static_rank<Rank>(), compare, alloc)
    { }

    idle_box_multimap(const idle_box_multimap& other, bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_box_multimap&
    operator=(const idle_box_multimap& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

  /**
   *  Specialization for \idle_box_multimap with runtime rank support. The
   *  rank of the \idle_box_multimap can be determined at run time and does not
   *  need to be fixed at compile time. Using:
   *  \code
   *    struct box { ... };
   *    struct mapped { ... };
   *    idle_box_multimap<0, box, mapped> my_set;
   *  \endcode
   *
   *  \see runtime_idle_box_multimap for more information about how to use this
   *  container.
   */
  template<typename Key, typename Mapped, typename Compare, typename Alloc>
  class idle_box_multimap<0, Key, Mapped, Compare, Alloc>
    : public details::Kdtree<details::Dynamic_rank, const Key,
                             std::pair<const Key, Mapped>, Compare, Alloc>
  {
  private:
    typedef details::Kdtree<details::Dynamic_rank, const Key,
                            std::pair<const Key, Mapped>,
                            Compare, Alloc>              base_type;
    typedef idle_box_multimap<0, Key, Mapped, Compare, Alloc>  Self;

  public:
    typedef Mapped                            mapped_type;

    idle_box_multimap() : base_type(details::Dynamic_rank(2)) { }

    explicit idle_box_multimap(dimension_type dim)
      : base_type(details::Dynamic_rank(dim))
    { except::check_even_rank(dim); }

    idle_box_multimap(dimension_type dim, const Compare& compare)
      : base_type(details::Dynamic_rank(dim), compare)
    { except::check_even_rank(dim); }

    explicit idle_box_multimap(const Compare& compare)
      : base_type(details::Dynamic_rank(2), compare)
    { }

    idle_box_multimap(dimension_type dim, const Compare& compare,
                  const Alloc& alloc)
      : base_type(details::Dynamic_rank(dim), compare, alloc)
    { except::check_even_rank(dim); }

    idle_box_multimap(const Compare& compare, const Alloc& alloc)
      : base_type(details::Dynamic_rank(2), compare, alloc)
    { }

    idle_box_multimap(const idle_box_multimap& other, bool balancing = false)
      : base_type(other, balancing)
    { }

    idle_box_multimap&
    operator=(const idle_box_multimap& other)
    {
      return static_cast<Self&>(base_type::operator=(other));
    }
  };

} // namespace spatial

#endif // SPATIAL_IDLE_BOX_MULTIMAP_HPP
