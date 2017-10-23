// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_math.hpp
 *  Gather in one file all the mathematical operations, mainly for the
 *  metric types.
 *
 *  Most of the mathematical operations written in this file are not very well
 *  optimized, and for a given compiler or architecture it would be easy to
 *  write more efficient algorithms. Therefore, if you are really looking to
 *  increase the speed of your computation, you might want to write your own
 *  metric optimized for the type you are working with.
 *
 *  \see neighbor
 */

#ifndef SPATIAL_MATH_HPP
#define SPATIAL_MATH_HPP

#include <limits>
#include <cmath>
#include <sstream>
#include "spatial_import_type_traits.hpp"
#include "../exception.hpp"
#include "spatial_check_concept.hpp"

namespace spatial
{
  namespace except
  {
    /**
     *  Check that the distance given \c x has a positive value.
     *  \throws negative_distance
     */
    template<typename Tp>
    inline typename enable_if<import::is_arithmetic<Tp> >::type
    check_positive_distance(Tp x)
    {
      if (x < Tp()) // Tp() == 0 by convention
        {
          std::stringstream out;
          out << x << " is negative";
          throw invalid_distance(out.str());
        }
    }

    /**
     *  This arithmetic check is only used when the macro
     *  SPATIAL_SAFER_ARITHMETICS is defined. Check that the absolute of an
     *  element has not led to an error such as an overflow, by forcing the
     *  error itself.
     *
     *  This check is not the best check for arithmetic errors. There are
     *  probably ways to make it faster, but it is intended to be portable and
     *  provide users with the possibility to quickly check the architmectics
     *  during computation with little efforts from their part.
     *
     *  The std::abs() function is working fine for floating point types,
     *  however, for signed integral types (char, wchar_t, short, int, long,
     *  long long) and their signed version, it returns an incorrect value when
     *  trying to compute std::abs(std::numeric_limits<signed int>::min()). To
     *  signal this issue we raise an exception in this case.
     */
    ///@{
    template <typename Tp>
    inline typename enable_if_c
    <std::numeric_limits<Tp>::is_integer
    && std::numeric_limits<Tp>::is_signed, Tp>::type
    check_abs(Tp x)
    {
      if (x == (std::numeric_limits<Tp>::min)())
        {
          std::stringstream out;
          out << "absolute of " << x << " caused overflow";
          throw arithmetic_error(out.str());
        }
      return std::abs(x);
    }

    template <typename Tp>
    inline typename enable_if_c
    <!std::numeric_limits<Tp>::is_integer
    || !std::numeric_limits<Tp>::is_signed, Tp>::type
    check_abs(Tp x)
    { return std::abs(x); }
    ///@}

    /**
     *  This arithmetic check is only used when the macro
     *  SPATIAL_SAFER_ARITHMETICS is defined. Check that the addtion of 2
     *  elements of positive value has not led to an error such as an overflow,
     *  by forcing the error itself.
     *
     *  This check is not the best check for arithmetic errors. There are ways
     *  to make it faster, but it is intended to be portable and provide users
     *  with the possibility to quickly check the architmectics during
     *  computation with little efforts from their part.
     *
     *  In particular, if \c Tp is not a base type, the author of the type must
     *  define the numeric limits \c numeric_limits<Tp>::max() for that type.
     */
    template <typename Tp>
    inline typename enable_if<import::is_arithmetic<Tp>, Tp>::type
    check_positive_add(Tp x, Tp y)
    {
      // The additional bracket is to avoid conflict with MSVC min/max macros
      if (((std::numeric_limits<Tp>::max)() - x) < y)
        {
          std::stringstream out;
          out << x << " + " << y << " caused overflow";
          throw arithmetic_error(out.str());
        }
      return x + y;
    }

    /**
     *  This arithmetic check is only used when the macro
     *  SPATIAL_SAFER_ARITHMETICS is defined. Check that the computation of the
     *  square of an element has not overflown.
     *
     *  This check is not the best check for arithmetic errors. There are ways
     *  to make it faster, but it is intended to be portable and provide users
     *  with the possibility to quickly check the architmectics during
     *  computation with little efforts from their part.
     */
    template <typename Tp>
    inline typename enable_if<import::is_arithmetic<Tp>, Tp>::type
    check_square(Tp x)
    {
      Tp zero = Tp(); // get 0
      if (x != zero)
        {
          Tp abs = check_abs(x);
          if (((std::numeric_limits<Tp>::max)() / abs) < abs)
            {
              std::stringstream out;
              out << "square(" << x << ") caused overflow";
              throw arithmetic_error(out.str());
            }
          return x * x;
        }
      return zero;
    }

    /**
     *  This arithmetic check is only used when the macro
     *  SPATIAL_SAFER_ARITHMETICS is defined. Check that the multiplication of 2
     *  positive elements has not resulted into an arithmetic error such as
     *  an overflow.
     *
     *  This check will only work for 2 positive element x and y. This check is
     *  not the best check for arithmetic errors. There are ways to make it
     *  better, but it's hard to make it more portable.
     *
     *  In particular, if \c Tp is not a base type, the author of the type must
     *  define the numeric limits \c std::numeric_limits<Tp>::max() for that
     *  type.
     */
    template <typename Tp>
    inline typename enable_if<import::is_arithmetic<Tp>, Tp>::type
    check_positive_mul(Tp x, Tp y)
    {
      Tp zero = Tp(); // get 0
      if (x != zero)
        {
          if (((std::numeric_limits<Tp>::max)() / x) < y)
            {
              std::stringstream out;
              out << x << " * " << y << " caused overflow";
              throw arithmetic_error(out.str());
            }
          return x * y;
        }
      return zero;
    }
  } // spatial except

  namespace math
  {
    /**
     *  Compute the distance between the \p origin and the closest point
     *  to the plane orthogonal to the axis of dimension \c dim and passing by
     *  \c key.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_arithmetic<Unit>, Unit>::type
    square_euclid_distance_to_plane
    (dimension_type dim, const Key& origin, const Key& key, Difference diff)
    {
      Unit d = diff(dim, origin, key);
#ifdef SPATIAL_SAFER_ARITHMETICS
      return except::check_square(d);
#else
      return d * d;
#endif
    }

    /**
     *  Compute the square value of the distance between \p origin and
     *  \p key.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_arithmetic<Unit>, Unit>::type
    square_euclid_distance_to_key
    (dimension_type rank, const Key& origin, const Key& key, Difference diff)
    {
      Unit sum = square_euclid_distance_to_plane<Key, Difference, Unit>
        (0, origin, key, diff);
      for (dimension_type i = 1; i < rank; ++i)
        {
#ifdef SPATIAL_SAFER_ARITHMETICS
          sum = except::check_positive_add
            (square_euclid_distance_to_plane<Key, Difference, Unit>
             (i, origin, key, diff), sum);
#else
          sum += square_euclid_distance_to_plane
            <Key, Difference, Unit>(i, origin, key, diff);
#endif
        }
      return sum;
    }

    /**
     *  Compute the distance between the \p origin and the closest point to the
     *  plane orthogonal to the axis of dimension \c dim and passing by \c key.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_floating_point<Unit>, Unit>::type
    euclid_distance_to_plane
    (dimension_type dim, const Key& origin, const Key& key, Difference diff)
    {
      return std::abs(diff(dim, origin, key)); // floating types abs is always okay!
    }

    /**
     *  Computes the euclidian distance between 2 points.
     *
     *  When compiled without SPATIAL_SAFER_ARITHMETICS, it uses the naive
     *  approach, which may overflow or underflow, but is much faster.
     *
     *  When compiled without SPATIAL_SAFER_ARITHMETICS the calculation uses
     *  the hypot() algorithm in order to compute the distance: minimize
     *  possibilities of overflow or underflow at the expense of speed.
     *
     *  The principle of hypot() is to find the maximum value among all the
     *  component of the distance and then divide all other components with this
     *  one.
     *
     *  The algorithm comes from this equality:
     *  sqrt( x^2 + y^2 + z^2 + ... ) = abs(x) * sqrt( 1 + (y/x)^2 + (z/x)^2 + ...)
     *
     *  Where the second form (on the right of the equation) is less likely to
     *  overflow or underflow than the first form during computation.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_floating_point<Unit>, Unit>::type
    euclid_distance_to_key
    (dimension_type rank, const Key& origin, const Key& key, Difference diff)
    {
#ifdef SPATIAL_SAFER_ARITHMETICS
      // Find a non zero maximum or return 0
      Unit max = euclid_distance_to_plane<Key, Difference, Unit>
        (0, origin, key, diff);
      dimension_type max_dim = 0;
      for (dimension_type i = 1; i < rank; ++i)
        {
          Unit d = euclid_distance_to_plane<Key, Difference, Unit>
            (i, origin, key, diff);
          if (d > max) { max = d; max_dim = i; }
        }
      const Unit zero = Unit();
      if (max == zero) return zero; // they're all zero!
      // Compute the distance
      Unit sum = zero;
      for (dimension_type i = 0; i < max_dim; ++i)
        {
          Unit div = diff(i, origin, key) / max;
          sum += div * div;
        }
      for (dimension_type i = max_dim + 1; i < rank; ++i)
        {
          Unit div = diff(i, origin, key) / max;
          sum += div * div;
        }
      return except::check_positive_mul(max, std::sqrt(((Unit) 1.0) + sum));
#else
      return std::sqrt(square_euclid_distance_to_key<Key, Difference, Unit>
                       (rank, origin, key, diff));
#endif
    }

    /**
     *  Compute the distance between the \p origin and the closest point
     *  to the plane orthogonal to the axis of dimension \c dim and passing by
     *  \c key.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_arithmetic<Unit>, Unit>::type
    manhattan_distance_to_plane
    (dimension_type dim, const Key& origin, const Key& key, Difference diff)
    {
#ifdef SPATIAL_SAFER_ARITHMETICS
      return except::check_abs(diff(dim, origin, key));
#else
      return std::abs(diff(dim, origin, key));
#endif
    }

    /**
     *  Compute the manhattan distance between \p origin and \p key.
     */
    template <typename Key, typename Difference, typename Unit>
    inline typename enable_if<import::is_arithmetic<Unit>, Unit>::type
    manhattan_distance_to_key
    (dimension_type rank, const Key& origin, const Key& key, Difference diff)
    {
      Unit sum = manhattan_distance_to_plane<Key, Difference, Unit>
        (0, origin, key, diff);
      for (dimension_type i = 1; i < rank; ++i)
        {
#ifdef SPATIAL_SAFER_ARITHMETICS
          sum = except::check_positive_add
            (manhattan_distance_to_plane<Key, Difference, Unit>
             (i, origin, key, diff), sum);
#else
          sum += manhattan_distance_to_plane
            <Key, Difference, Unit>(i, origin, key, diff);
#endif
        }
      return sum;
    }

    /*
      // For a future implementation where we take earth-like spheroid as an
      // example for non-euclidian spaces, or manifolds.
      math::great_circle_distance_to_key
      math::great_circle_distance_to_plane
      math::vincenty_distance_to_key
      math::vincenty_distance_to_plane
    */
  } // namespace math
}

#endif // SPATIAL_MATH_HPP
