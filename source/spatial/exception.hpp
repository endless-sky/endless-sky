// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   exception.hpp
 *  Defines exceptions that are being thrown by the library.
 */

#ifndef SPATIAL_EXCEPTION_HPP
#define SPATIAL_EXCEPTION_HPP

#include <stdexcept>
#include "spatial.hpp"

namespace spatial
{
  /**
   *  Thrown to report that an invalid rank was passed as an argument.
   *  Generally thrown because 0 was passed as an argument for the rank.
   */
  struct invalid_rank : std::logic_error
  {
    explicit invalid_rank(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an odd rank value was passed as a argument.
   */
  struct invalid_odd_rank : std::logic_error
  {
    explicit invalid_odd_rank(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an invalid dimension was passed as an
   *  argument.
   */
  struct invalid_dimension : std::logic_error
  {
    explicit invalid_dimension(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an invalid node was passed as an argument.
   */
  struct invalid_node : std::logic_error
  {
    explicit invalid_node(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an invalid iterator was passed as an argument.
   */
  struct invalid_iterator : std::logic_error
  {
    explicit invalid_iterator(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an empty container was passed as an argument,
   *  while the function does not accept an empty container.
   */
  struct invalid_empty_container : std::logic_error
  {
    explicit invalid_empty_container(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an invalid range bound has been given as
   *  argument.
   *  \see except::check_open_bounds()
   *
   *  Generally, this means that the lower bound has a value that overlaps with
   *  the upper bound over one dimension, at least.
   */
  struct invalid_bounds : std::logic_error
  {
    explicit invalid_bounds(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that a box has incorrect coordinates with regards to
   *  its layout.
   *  \see except::check_invalid_box()
   */
  struct invalid_box : std::logic_error
  {
    explicit invalid_box(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an negative distance has been passed as a parameter
   *  while distances are expected to be positive.
   *
   *  \see except::check_addition()
   *  \see except::check_multiplication()
   */
  struct invalid_distance : std::logic_error
  {
    explicit invalid_distance(const std::string& arg)
      : std::logic_error(arg) { }
  };

  /**
   *  Thrown to report that an arithmetic error has occured during a
   *  calculation. It could be an overflow, or another kind of error.
   *
   *  \see except::check_addition()
   *  \see except::check_multiplication()
   */
  struct arithmetic_error : std::logic_error
  {
    explicit arithmetic_error(const std::string& arg)
      : std::logic_error(arg) { }
  };

} // namespace spatial

#endif // SPATIAL_EXCEPTION_HPP
