// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_mutate.hpp
 *  Defines spatial::details::mutate meta-programing type
 */

#ifndef SPATIAL_MUTATE_HPP
#define SPATIAL_MUTATE_HPP

namespace spatial
{
  namespace details
  {
    ///@{
    /**
     *  Changes a const type into a mutable type.
     */
    template<typename Tp>
    struct mutate { typedef Tp type; };
    template <typename Tp>
    struct mutate<const Tp> { typedef Tp type; };
    ///@}

    ///@{
    /**
     *  A helper functions that mutates pointers. Required to unallocate
     *  key values, which are always constant.
     */
    template<typename Tp>
    inline Tp*
    mutate_pointer(const Tp* p) { return const_cast<Tp*>(p); }

    template<typename Tp>
    inline Tp*
    mutate_pointer(Tp* p) { return p; } // hoping this one gets optimized away
    ///@}
  }
}

#endif // SPATIAL_MUTATE_HPP
