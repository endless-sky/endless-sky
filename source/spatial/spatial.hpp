// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial.hpp
 *  Introduces the spatial namespace, defines base types used throughout the
 *  library.
 */

#ifndef SPATIAL_HPP
#define SPATIAL_HPP

/**
 *  Define version numbers for the spatial C++ library. This number can be used
 *  by libraries based on spatial, like binding to other languages, etc, that
 *  need to ensure compatibility within different version of the library.
 *
 *  The version number will loosely follow the major, minor and release number
 *  concatenated together. However this is not guaranteed, as their may be
 *  several digits in each of major, minor and release numbers.
 */
///@{
#define SPATIAL_VERSION_MAJOR 2
#define SPATIAL_VERSION_MINOR 2
#define SPATIAL_VERSION_RELEASE 0
//! In general, this number will start by the major, then 2 digits for the
//! minor, then 2 more for the revision.
#define SPATIAL_VERSION 20200
///@}

/**
 *  The main namespace used in the library. All types, functions and variables
 *  defined in the library are contained within this namespace. Declaring
 *  <tt>using spatial</tt> in the source of your program gives you access to all
 *  features of the library at once.
 *
 *  The main reason to access the \c spatial namespace is to access one of its
 *  containers, such as \point_multiset, \box_multimap, or
 *  \idle_point_multiset.
 *
 *  \see Containers
 */
namespace spatial
{
  /**
   *  Defines a positive integral type for counting objects.
   */
  typedef std::size_t size_type;

  /**
   *  Defines the type for the dimension as a positive, native integer.
   */
  typedef unsigned dimension_type;

  /**
   *  Defines the weight as a positive, native integer.
   */
  typedef unsigned weight_type;

  /**
   *  Defines values for relative ordering.
   */
  typedef
  enum { below = -1, matching = 0, above = 1 }
    relative_order;

  /**
   *  Represents a coordinate layout for the box. llhh stands for low, low,
   *  high, high. This layout means that the lower coordinates of the box are
   *  expressed first, and the higher coordinates are expressed after.
   *
   *  For a box of dimension 2, the llhh layout means that for each box,
   *  delmited by 2 points x and y, the coordinate are ordered as follow:
   *
   *      B = { x0, x1, y0, y1 }
   *
   *  With <tt>x0 <= y0</tt> and <tt>x1 <= y1</tt>.
   */
  struct llhh_layout_tag { };

  /**
   *  Represents a coordinate layout for the box. lhlh stands for low, high,
   *  low, high. This layout means that the lower coordinates of the box are
   *  expressed first, and the higher coordinates are expressed second,
   *  alternatively for each dimension.
   *
   *  For a box of dimension 2, the lhlh layout means that for each box,
   *  delmited by 2 points x and y, the coordinate are ordered as follow:
   *
   *      B = { x0, y0, x1, y1 }
   *
   *  With <tt>x0 <= y0</tt> and <tt>x1 <= y1</tt>.
   */
  struct lhlh_layout_tag { };

  /**
   *  Represents a coordinate layout for the box. hhll stands for high, high,
   *  low, low. This layout means that the upper coordinates of the box are
   *  expressed first, and the lower coordinates are expressed after.
   *
   *  For a box of dimension 2, the hhll layout means that for each box,
   *  delmited by 2 points x and y, the coordinate are ordered as follow:
   *
   *      B = { x0, x1, y0, y1 }
   *
   *  With <tt>x0 >= y0</tt> and <tt>x1 >= y1</tt>.
   */
  struct hhll_layout_tag { };

  /**
   *  Represents a coordinate layout for the box. lhlh stands for high, low
   *  high, low. This layout means that the upper coordinates of the box are
   *  expressed first, and the lower coordinates are expressed second,
   *  alternatively for each dimension.
   *
   *  For a box of dimension 2, the hlhl layout means that for each box,
   *  delmited by 2 points x and y, the coordinate are ordered as follow:
   *
   *      B = { x0, y0, x1, y1 }
   *
   *  With <tt>x0 >= y0</tt> and <tt>x1 >= y1</tt>.
   */
  struct hlhl_layout_tag { };

  /**
   *  This constant is used to declare a type layout tag, which is required by
   *  the overlap and enclosed region iterators. Creation of these iterators is
   *  facilitated by these objects.
   *
   *  \warning This constant is unique for each translation unit, and therefore
   *  its address will differ for each translation unit.
   *
   */
  ///@{
  const llhh_layout_tag llhh_layout = llhh_layout_tag();
  const lhlh_layout_tag lhlh_layout = lhlh_layout_tag();
  const hhll_layout_tag hhll_layout = hhll_layout_tag();
  const hlhl_layout_tag hlhl_layout = hlhl_layout_tag();
  ///@}
}

#endif // SPATIAL_HPP
