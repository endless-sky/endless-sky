// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_condition.hpp
 *  Defines spatial::details::condition meta programing type
 */

#ifndef SPATIAL_CONDITION_HPP
#define SPATIAL_CONDITION_HPP

namespace spatial
{
  namespace details
  {
    ///@{
    /*
     *  A meta-programing type that picks Tp1 if \e true or Tp2 otherwise.
     */
    template<bool, typename Tp1, typename Tp2>
    struct condition
    { typedef Tp1 type; };
    template<typename Tp1, typename Tp2>
    struct condition<false, Tp1, Tp2>
    { typedef Tp2 type; };
    ///@}
  }
}

#endif // SPATIAL_CONDITION_HPP
