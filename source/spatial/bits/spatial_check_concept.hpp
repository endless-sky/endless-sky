// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_check_concept.hpp
 *  This file contains the definition of the enable_if trick for
 *  spatial.
 *
 *  Obliged to duplicate the trick since the library intends to support C99/TR1
 *  conformant compilers for the moment.
 */

#ifndef SPATIAL_CHECK_CONCEPT_HPP
#define SPATIAL_CHECK_CONCEPT_HPP

namespace spatial
{
  /**
   *  If B is true, \c spatial::enable_if has a public member typedef type,
   *  equal to \c Tp; otherwise, there is no member typedef.
   *
   *  This metafunction is used to conditionally remove functions and classes
   *  from overload resolution based on type traits and to provide separate
   *  function overloads and specializations for different type traits. \c
   *  spatial::enable_if can be used as an additional function argument (not
   *  applicable to operator overloads), as a return type (not applicable to
   *  constructors and destructors), or as a class template or function template
   *  parameter.
   *
   *  The brief description and the explanation given above are paraphrased from
   *  http://en.cppreference.com/w/cpp/types/enable_if
   */
  ///@{
  template <bool B, typename Tp = void> struct enable_if_c { };

  template <typename Tp> struct enable_if_c<true, Tp> { typedef Tp type; };

  template <typename Cond, typename Tp = void>
  struct enable_if : public enable_if_c<Cond::value, Tp> { };
  ///@}

} // namespace spatial

#endif // SPATIAL_CHECK_CONCEPT_HPP
