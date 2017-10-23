// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   metric.hpp
 *  Contains the definition of the default metrics available to use
 *  with the neighbor iterators.
 *
 *  \see neighbor
 */

#ifndef SPATIAL_METRIC_HPP
#define SPATIAL_METRIC_HPP

#include "function.hpp"
#include "bits/spatial_math.hpp"
#include "bits/spatial_builtin.hpp"

namespace spatial
{
  /**
   *  Defines a metric working on the Euclidian space where distances are
   *  expressed in one of C++'s floating point types.
   *
   *  \concept_metric
   *
   *  \attention \c This metric works on floating types only. It will fail
   *  to compile if given non-floating types as a parameter for the distance.
   *
   *  \ref euclidian uses square root calculation in the distance. It will
   *  return proper distances therefore but will be slower than \ref
   *  quadrance. When defining SPATIAL_SAFER_ARITHMETICS however, \ref euclidian
   *  uses the hypot algorithm which has much less chance to overflow or
   *  underflow than \ref quadrance. However is it also much slower.
   */
  template<typename Container, typename DistanceType, typename Diff>
  class euclidian : Diff
  {
    // Check that DistanceType is a fundamental floating point type
    typedef typename enable_if<import::is_floating_point<DistanceType> >::type
    check_concept_distance_type_is_floating_point;

  public:
    /**
     *  The type used to compute the difference between 2 keys along the same
     *  dimension.
     */
    typedef typename details::rebind_builtin_difference
    <Diff, DistanceType>::type difference_type;

  private:
    /**
     *  The key_type of the container being used for calculations.
     */
    typedef typename Container::key_type key_type;

  public:
    /**
     *  The distance type being used for distance calculations.
     */
    typedef DistanceType distance_type;

    //! The constructors allows you to specify a custom difference type.
    explicit euclidian(const difference_type& diff = Diff()) : Diff(diff) { }

    //! Copy the metric from another metric with any DistanceType.
    template <typename AnyDistanceType>
    euclidian(const euclidian<Container, AnyDistanceType, Diff>& other)
      : Diff(other.difference()) { }

    /**
     *  Compute the distance between the point of \c origin and the \c key.
     *  \return The resulting distance.
     */
    distance_type
    distance_to_key(dimension_type rank,
                    const key_type& origin, const key_type& key) const
    {
      return math::euclid_distance_to_key
        <key_type, difference_type, DistanceType>(rank, origin, key,
                                                  difference());
    }

    /**
     *  The distance between the point of \c origin and the closest point to
     *  the plane orthogonal to the axis of dimension \c dim and crossing \c
     *  key.
     *
     *  Given any 2 points 'origin' and 'key', the result of distance_to_plane
     *  must always be less or equal to the result of distance_to_key.
     *
     *  \return The resulting distance.
     */
    distance_type
    distance_to_plane(dimension_type, dimension_type dim,
                      const key_type& origin, const key_type& key) const
    {
      return math::euclid_distance_to_plane
        <key_type, difference_type, DistanceType>(dim, origin, key,
                                                  difference());
    }

    /**
     *  Returns the difference functor used in this type.
     */
    difference_type difference() const
    { return *static_cast<const Diff*>(this); }
  };

  /**
   *  Defines a metric in the Euclidian space where only the square of
   *  the distances are being computed into a scalar value expressed with
   *  the DistanceType which is one of C++'s arithmetic types.
   *
   *  \concept_metric
   *
   *  This method of distance calculation is more flexible than
   *  \euclidian since it can support all of the C++'s arithmetic types.
   *
   *  When using this metric and reading the distance value, you must
   *  remember that you are reading the square of the distance, and not the
   *  real distance. Thus, you should use square root to recover the real
   *  distance.
   *
   *  This metric has one important drawback: if you are working with large
   *  value over the entire range permissible by DistanceType, then chances
   *  that the computation overflows is non-negligible. To receive an \ref
   *  arithmetic_error exception upon overflow, compile your application with
   *  \c \#define \c SPATIAL_SAFER_ARITHEMTICS.
   *
   */
  template<typename Container, typename DistanceType, typename Diff>
  class quadrance : Diff
  {
    // Check that DistanceType is a fundamental floating point type
    typedef typename enable_if<import::is_arithmetic<DistanceType> >::type
    check_concept_distance_type_is_arithmetic;

  public:
    /**
     *  The type used to compute the difference between 2 keys along the same
     *  dimension.
     */
    typedef typename details::rebind_builtin_difference
    <Diff, DistanceType>::type difference_type;

  private:
    /**
     *  The key_type of the container being used for calculations.
     */
    typedef typename Container::key_type key_type;

  public:
    typedef DistanceType distance_type;

    //! The constructor allows you to specify a custom difference type.
    explicit quadrance(const difference_type& diff = Diff()) : Diff(diff) { }

    //! Copy the metric from another metric with any DistanceType.
    template <typename AnyDistanceType>
    quadrance(const quadrance<Container, AnyDistanceType, Diff>& other)
      : Diff(other.difference()) { }

    /**
     *  Compute the distance between the point of \c origin and the \c key.
     *  \return The resulting square distance.
     */
    distance_type
    distance_to_key(dimension_type rank,
                    const key_type& origin, const key_type& key) const
    {
      return math::square_euclid_distance_to_key
        <key_type, difference_type, DistanceType>(rank, origin, key,
                                                  difference());
    }

    /**
     *  The distance between the point of \c origin and the closest point to
     *  the plane orthogonal to the axis of dimension \c dim and crossing \c
     *  key.
     *  \return The resulting square distance.
     */
    distance_type
    distance_to_plane(dimension_type, dimension_type dim,
                      const key_type& origin, const key_type& key) const
    {
      return math::square_euclid_distance_to_plane
        <key_type, difference_type, DistanceType>(dim, origin, key,
                                                  difference());
    }

    /**
     *  Returns the difference functor used in this type.
     */
    difference_type difference() const
    { return *static_cast<const Diff*>(this); }
  };

  /**
   *  Defines a metric for the a space where distances are the sum
   *  of all the elements of the vector. Also known as the taxicab metric.
   *
   *  \concept_metric.
   *
   *  \tparam Ct The container used with this metric.
   *  \tparam DistanceType The type used to compute distance values.
   *  \tparam Diff A difference functor that compute the difference between 2
   *  elements of a the Container's key type, along the same dimension.
   *
   *  This method of distance calculation is more flexible than \euclidian
   *  since it can support all of the C++'s arithmetic types.
   *
   *  This metric is the fastest of all built-in metric, and generally offers
   *  an acceptable approaximation to the euclidian metric. However the
   *  distance calculated by this metric cannot be converted into Euclidian
   *  distances. If you are looking for a fast metric that is convertible in
   *  Euclidian distances, check out \ref quadrance.
   *
   *  This metric has one important drawback: if you are working with large
   *  value over the entire range permissible by DistanceType, then chances
   *  that the computation overflows is non-negligible. To receive an \ref
   *  arithmetic_error exception upon overflow, compile your application with
   *  \c \#define \c SPATIAL_SAFER_ARITHEMTICS.
   */
  template<typename Container, typename DistanceType, typename Diff>
  class manhattan : Diff
  {
    // Check that DistanceType is a fundamental floating point type
    typedef typename enable_if<import::is_arithmetic<DistanceType> >::type
    check_concept_distance_type_is_arithmetic;

  public:
    /**
     *  The type used to compute the difference between 2 keys along the same
     *  dimension.
     */
    typedef typename details::rebind_builtin_difference
    <Diff, DistanceType>::type difference_type;

  private:
    /**
     *  The key_type of the container being used for calculations.
     */
    typedef typename Container::key_type key_type;

  public:
    typedef DistanceType distance_type;

    //! A constructor that allows you to specify the Difference type.
    explicit manhattan(const difference_type& diff = Diff()) : Diff(diff) { }

    //! Copy the metric from another metric with any DistanceType.
    template <typename AnyDistanceType>
    manhattan(const manhattan<Container, AnyDistanceType, Diff>& other)
      : Diff(other.difference()) { }

    /**
     *  Compute the distance between the point of \c origin and the \c key.
     *  \return The resulting manahattan distance.
     */
    distance_type
    distance_to_key(dimension_type rank,
                    const key_type& origin, const key_type& key) const
    {
      return math::manhattan_distance_to_key
        <key_type, difference_type, DistanceType>(rank, origin, key,
                                                  difference());
    }

    /**
     *  The distance between the point of \c origin and the closest point to
     *  the plane orthogonal to the axis of dimension \c dim and crossing \c
     *  key.
     *
     *  \return The resulting manhattan distance.
     */
    distance_type
    distance_to_plane(dimension_type, dimension_type dim,
                      const key_type& origin, const key_type& key) const
    {
      return math::manhattan_distance_to_plane
        <key_type, difference_type, DistanceType>(dim, origin, key,
                                                  difference());
    }

    /**
     *  Returns the difference functor used in this type.
     */
    difference_type difference() const
    { return *static_cast<const Diff*>(this); }
  };

} // namespace spatial

#endif // SPATIAL_METRIC_HPP
