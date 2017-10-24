// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_template_member_swap.hpp
 *  Defines the template member swap function that is optimized for
 *  empty base types.
 */

#ifndef SPATIAL_TEMPLATE_MEMBER_SWAP_HPP
#define SPATIAL_TEMPLATE_MEMBER_SWAP_HPP

#include <algorithm> // provides: ::std::swap
// provides: import::is_empty, import::false_type and import::true_type
#include "spatial_import_type_traits.hpp"

namespace spatial
{
  namespace details
  {
    ///@{
    /**
     *  Perform a specialized assign for empty classes.
     */
    template<bool, typename Tp>
    struct template_member_assign_provider
    {
      static void do_it(Tp& left, const Tp& right)
      { left = right; }
    };

    template<typename Tp>
    struct template_member_assign_provider<true, Tp>
    { static void do_it(Tp&, const Tp&) { } };

    template <typename Tp>
    struct template_member_assign
      : template_member_assign_provider<import::is_empty<Tp>::value, Tp>
    { };
    ///@}

    ///@{
    /**
     *  Perform a specialized swap for empty classes.
     */
    template<bool, typename Tp>
    struct template_member_swap_provider
    {
      static void do_it(Tp& left, Tp& right)
      {
        using ::std::swap;
        swap(left, right);
      }
    };

    template<typename Tp>
    struct template_member_swap_provider<true, Tp>
    { static void do_it(Tp&, Tp&) { } };

    template <typename Tp>
    struct template_member_swap
      : template_member_swap_provider<import::is_empty<Tp>::value, Tp>
    { };
    ///@}
  }
}

#endif // SPATIAL_TEMPLATE_MEMBER_SWAP_HPP
