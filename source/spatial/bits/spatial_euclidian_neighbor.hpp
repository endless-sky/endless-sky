// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_neighbor.hpp
 *  Contains the definition of euclidian neighbor iterators. These iterators
 *  walk through all items in the container in order from the closest to the
 *  furthest away from a given key, using an euclidian metric.
 *
 *  \see neighbor_iterator
 */

#ifndef SPATIAL_EUCLIDIAN_NEIGHBOR_HPP
#define SPATIAL_EUCLIDIAN_NEIGHBOR_HPP

#include "spatial_neighbor.hpp"

namespace spatial
{
  /**
   *  Facilitate the creation of neighbor iterator that works with an euclidian
   *  metric.
   *
   *  This class has an associated group of functions designed to
   *  initialize the iterator position at the beginning, end, lower bound or
   *  upper bound of the container to iterate.
   *
   *  \tparam Container The container to iterate.
   *  \tparam DistanceType The type used to represent the distance, it must be a
   *  primitive floating point type.
   *  \tparam Diff The difference functor that will compute the difference
   *  between 2 key element in the container, along a specific dimension. See
   *  \difference for further explanation.
   */
  ///@{
  template <typename Container, typename DistanceType, typename Diff
            = typename details::with_builtin_difference<Container>::type>
  class euclidian_neighbor_iterator
    : public neighbor_iterator<Container,
                               euclidian<Container, DistanceType, Diff> >
  {
    // Check that DistanceType is a fundamental floating point type
    typedef typename enable_if<import::is_floating_point<DistanceType> >::type
    check_concept_distance_type_is_floating_point;

  public:
    euclidian_neighbor_iterator() { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator
    (const neighbor_iterator
     <Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator<Container, euclidian<Container, DistanceType, Diff> >
        (other.rank(), other.key_comp(), other.metric(),
         other.target_key(), other.node_dim, other.node,
         static_cast<AnyDistanceType>(other.distance())) { }
  };

  template <typename Container, typename DistanceType, typename Diff>
  class euclidian_neighbor_iterator<const Container, DistanceType, Diff>
    : public neighbor_iterator<const Container, euclidian<Container, DistanceType, Diff> >
  {
    // Some concept checking performed here
    typedef enable_if<import::is_floating_point<DistanceType> >
    check_concept_distance_type_is_floating_point;

  public:
    euclidian_neighbor_iterator() { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator
    (const neighbor_iterator
     <const Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator<const Container, euclidian<Container, DistanceType, Diff> >
        (other.rank(), other.key_comp(), other.metric(),
         other.target_key(), other.node_dim, other.node,
         static_cast<AnyDistanceType>(other.distance())) { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator
    (const neighbor_iterator
     <Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator<const Container, euclidian<Container, DistanceType, Diff> >
        (other.rank(), other.key_comp(), other.metric(),
         other.target_key(), other.node_dim, other.node,
         static_cast<AnyDistanceType>(other.distance())) { }
  };
  ///@}

  /**
   *  Facilitate the creation of an iterator range representing a sequence from
   *  closest to furthest from the target key position, in euclidian space.
   *
   *  \tparam Container    The container to iterator.
   *  \tparam DistanceType The type used to represent the distance, it must be a
   *                       primitive floating point type.
   *  \tparam Diff         The difference functor that will compute the
   *                       difference between 2 key element in the container,
   *                       along a specific dimension. See \difference for
   *                       further explanation.
   *
   *  This class has an associated group of functions designed to
   *  initialize the iterator position at the beginning, end, lower bound or
   *  upper bound of the container to iterate.
   */
  ///@{
  template <typename Container, typename DistanceType, typename Diff
            = typename details::with_builtin_difference<Container>::type>
  class euclidian_neighbor_iterator_pair
    : public neighbor_iterator_pair<Container,
                                    euclidian<Container, DistanceType, Diff> >
  {
    // Some concept checking performed here
    typedef enable_if<import::is_floating_point<DistanceType> >
    check_concept_distance_type_is_floating_point;

  public:
    euclidian_neighbor_iterator_pair() { }

    euclidian_neighbor_iterator_pair
    (const euclidian_neighbor_iterator<Container, DistanceType, Diff>& a,
     const euclidian_neighbor_iterator<Container, DistanceType, Diff>& b)
      : neighbor_iterator_pair<Container,
                               euclidian<Container, DistanceType, Diff> >
        (a, b) { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator_pair
    (const neighbor_iterator_pair
     <Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator_pair<Container,
                               euclidian<Container, DistanceType, Diff> >
        (euclidian_neighbor_iterator_pair<Container, DistanceType, Diff>
         (other.first, other.second)) { }
  };

  template <typename Container, typename DistanceType, typename Diff>
  class euclidian_neighbor_iterator_pair<const Container, DistanceType, Diff>
    : public neighbor_iterator_pair
  <const Container, euclidian<Container, DistanceType, Diff> >
  {
    // Some concept checking performed here
    typedef enable_if<import::is_floating_point<DistanceType> >
    check_concept_distance_type_is_floating_point;

  public:
    euclidian_neighbor_iterator_pair() { }

    euclidian_neighbor_iterator_pair
    (const euclidian_neighbor_iterator<const Container, DistanceType, Diff>& a,
     const euclidian_neighbor_iterator<const Container, DistanceType, Diff>& b)
      : neighbor_iterator_pair<const Container,
                               euclidian<Container, DistanceType, Diff> >
        (a, b) { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator_pair
    (const neighbor_iterator_pair
     <const Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator_pair<const Container,
                               euclidian<Container, DistanceType, Diff> >
        (euclidian_neighbor_iterator_pair<const Container, DistanceType, Diff>
         (other.first, other.second)) { }

    template <typename AnyDistanceType>
    euclidian_neighbor_iterator_pair
    (const neighbor_iterator_pair
     <Container, euclidian<Container, AnyDistanceType, Diff> >& other)
      : neighbor_iterator_pair<const Container,
                               euclidian<Container, DistanceType, Diff> >
        (euclidian_neighbor_iterator_pair<const Container, DistanceType, Diff>
         (other.first, other.second)) { }
  };
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing to
   *  the nearest neighbor of \c target.
   *
   *  The search will occur in euclidian space, where distance are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator<Container, float, Diff>
   *    my_float_nearest_iterator
   *    = euclidian_neighbor_begin(container, diff(), target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param diff A model of \difference.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator<Container, double, Diff>
  euclidian_neighbor_begin
  (Container& container, const Diff& diff,
   const typename Container::key_type& target)
  {
    return neighbor_begin
      (container,
       euclidian<typename details::mutate<Container>::type, double, Diff>(diff),
       target);
  }

  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator<const Container, double, Diff>
  euclidian_neighbor_cbegin
  (const Container& container, const Diff& diff,
   const typename Container::key_type& target)
  { return euclidian_neighbor_begin (container, diff, target); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing to
   *  the nearest neighbor of \c target.
   *
   *  The search will occur in euclidian space, where distance are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator<Container, float> my_float_nearest_iterator
   *    = euclidian_neighbor_begin(container, target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator<Container, double> >::type
  euclidian_neighbor_begin
  (Container& container,
   const typename Container::key_type& target)
  {
    return neighbor_begin
      (container,
       euclidian<typename details::mutate<Container>::type, double,
                 typename details::with_builtin_difference<Container>::type>
       (details::with_builtin_difference<Container>()(container)),
       target);
  }

  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator<const Container, double> >::type
  euclidian_neighbor_cbegin
  (const Container& container,
   const typename Container::key_type& target)
  { return euclidian_neighbor_begin (container, target); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  past-the-end.
   *
   *  The search will occur in euclidian space, where distance are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator<Container, float, Diff>
   *    my_float_nearest_iterator
   *    = euclidian_neighbor_end(container, diff(), target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param diff A model of \difference.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator<Container, double, Diff>
  euclidian_neighbor_end
  (Container& container, const Diff& diff,
   const typename Container::key_type& target)
  {
    return neighbor_end
      (container,
       euclidian<typename details::mutate<Container>::type, double, Diff>
       (diff), target);
  }

  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator<const Container, double, Diff>
  euclidian_neighbor_cend
  (const Container& container, const Diff& diff,
   const typename Container::key_type& target)
  { return euclidian_neighbor_end (container, diff, target); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  past-the-end.
   *
   *  The search will occur in euclidian space, where distance are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator<Container, float> my_float_nearest_iterator
   *    = euclidian_neighbor_end(container, target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator<Container, double> >::type
  euclidian_neighbor_end
  (Container& container,
   const typename Container::key_type& target)
  {
    return neighbor_end
      (container,
       euclidian<typename details::mutate<Container>::type, double,
                 typename details::with_builtin_difference<Container>::type>
       (details::with_builtin_difference<Container>()(container)),
       target);
  }

  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator<const Container, double> >::type
  euclidian_neighbor_cend
  (const Container& container,
   const typename Container::key_type& target)
  { return neighbor_end (container, target); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  to the closest element to \c target that is a least as far as \c bound.
   *
   *  \param container The container to iterate.
   *  \param diff A model of \difference.
   *  \param target Search for element in container closest to target.
   *  \param bound The minimum distance at which a neighbor should be found.
   */
  ///@{
  template <typename Container, typename Diff, typename DistanceType>
  inline typename
  enable_if<import::is_floating_point<DistanceType>,
            euclidian_neighbor_iterator<Container, DistanceType, Diff> >::type
  euclidian_neighbor_lower_bound
  (Container& container, const Diff& diff,
   const typename Container::key_type& target,
   DistanceType bound)
  {
    return neighbor_lower_bound
      (container,
       euclidian<typename details::mutate<Container>::type, DistanceType, Diff>
       (diff), target, bound);
  }

  template <typename Container, typename Diff, typename DistanceType>
  inline typename
  enable_if<import::is_floating_point<DistanceType>,
            euclidian_neighbor_iterator<const Container, DistanceType, Diff> >::type
  euclidian_neighbor_clower_bound
  (const Container& container, const Diff& diff,
   const typename Container::key_type& target,
   DistanceType bound)
  { return euclidian_neighbor_lower_bound (container, diff, target, bound); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  to the closest element to \c target that is a least as far as \c bound.
   *
   *  \param container The container to iterate.
   *  \param target Search for element in container closest to target.
   *  \param bound The minimum distance at which an element should be found.
   */
  ///@{
  template <typename Container, typename DistanceType>
  inline typename
  enable_if_c<details::is_compare_builtin<Container>::value
              && import::is_floating_point<DistanceType>::value,
              euclidian_neighbor_iterator<Container, DistanceType> >::type
  euclidian_neighbor_lower_bound
  (Container& container,
   const typename Container::key_type& target,
   DistanceType bound)
  {
    return neighbor_lower_bound
      (container,
       euclidian<typename details::mutate<Container>::type, DistanceType,
                 typename details::with_builtin_difference<Container>::type>
       (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }

  template <typename Container, typename DistanceType>
  inline typename
  enable_if_c<details::is_compare_builtin<Container>::value
              && import::is_floating_point<DistanceType>::value,
              euclidian_neighbor_iterator<const Container, DistanceType> >::type
  euclidian_neighbor_clower_bound
  (const Container& container,
   const typename Container::key_type& target,
   DistanceType bound)
  { return euclidian_neighbor_lower_bound (container, target, bound); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  to the closest element to \c target that further than \c bound.
   *
   *  \param container The container to iterate.
   *  \param diff A model of \difference.
   *  \param target Search for element in container closest to target.
   *  \param bound The minimum distance at which a neighbor should be found.
   */
  ///@{
  template <typename Container, typename Diff, typename DistanceType>
  inline typename
  enable_if<import::is_floating_point<DistanceType>,
            euclidian_neighbor_iterator<Container, DistanceType, Diff> >::type
  euclidian_neighbor_upper_bound
  (Container& container, const Diff& diff,
   const typename Container::key_type& target,
   DistanceType bound)
  {
    return neighbor_upper_bound
      (container,
       euclidian<typename details::mutate<Container>::type,
                 DistanceType, Diff>(diff),
       target, bound);
  }

  template <typename Container, typename Diff, typename DistanceType>
  inline typename
  enable_if<import::is_floating_point<DistanceType>,
            euclidian_neighbor_iterator<const Container, DistanceType, Diff>
            >::type
  euclidian_neighbor_cupper_bound
  (const Container& container, const Diff& diff,
   const typename Container::key_type& target,
   DistanceType bound)
  { return euclidian_neighbor_upper_bound (container, diff, target, bound); }
  ///@}

  /**
   *  Returns a \ref euclidian_neighbor_iterator pointing
   *  to the closest element to \c target that is further than \c bound.
   *
   *  \param container The container to iterate.
   *  \param target Search for element in container closest to target.
   *  \param bound The minimum distance at which an element should be found.
   */
  ///@{
  template <typename Container, typename DistanceType>
  inline typename
  enable_if_c<details::is_compare_builtin<Container>::value
              && import::is_floating_point<DistanceType>::value,
              euclidian_neighbor_iterator<Container, DistanceType> >::type
  euclidian_neighbor_upper_bound
  (Container& container,
   const typename Container::key_type& target,
   DistanceType bound)
  {
    return neighbor_upper_bound
      (container,
       euclidian<typename details::mutate<Container>::type, DistanceType,
                 typename details::with_builtin_difference<Container>::type>
       (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }

  template <typename Container, typename DistanceType>
  inline typename
  enable_if_c<details::is_compare_builtin<Container>::value
              && import::is_floating_point<DistanceType>::value,
              euclidian_neighbor_iterator<const Container, DistanceType> >::type
  euclidian_neighbor_cupper_bound
  (const Container& container,
   const typename Container::key_type& target,
   DistanceType bound)
  { return euclidian_neighbor_upper_bound (container, target, bound); }
  ///@}

  /**
   *  Make a pair of iterators spanning the range of iterable element in \c
   *  container from the closest to the furthest to \c target
   *
   *  The search will occur in euclidian space, where distance are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator_pair<Container, float, Diff>
   *    my_float_iterator_pair
   *    = euclidian_neighbor_range(container, diff(), target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param diff A model of \difference.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator_pair<Container, double, Diff>
  euclidian_neighbor_range
  (Container& container, const Diff& diff,
   const typename Container::key_type& target)
  {
    return neighbor_range
      (container,
       euclidian<typename details::mutate<Container>::type, double, Diff>
       (diff), target);
  }

  template <typename Container, typename Diff>
  inline euclidian_neighbor_iterator_pair<const Container, double, Diff>
  euclidian_neighbor_crange
  (const Container& container, const Diff& diff,
   const typename Container::key_type& target)
  { return euclidian_neighbor_range (container, diff, target); }
  ///@}

  /**
   *  Make a pair of iterators spanning the range of iterable elements in \c
   *  container from the closest to the furthest to \c target.
   *
   *  The search will occur in euclidian space, where distances are computed in
   *  double, by default. However distances can be expressed in any floating
   *  point type by simply assigning the result to an similar iterator using a
   *  different distance type:
   *
   *  \code
   *  euclidian_neighbor_iterator_pair<Container, float> my_float_iterator_pair
   *    = euclidian_neighbor_range(container, target);
   *  \endcode
   *
   *  \param container The container to iterate.
   *  \param target Search for element in container closest to target.
   */
  ///@{
  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator_pair<Container, double> >::type
  euclidian_neighbor_range
  (Container& container,
   const typename Container::key_type& target)
  {
    return neighbor_range
      (container,
       euclidian<typename details::mutate<Container>::type, double,
                 typename details::with_builtin_difference<Container>::type>
       (details::with_builtin_difference<Container>()(container)),
       target);
  }

  template <typename Container>
  inline typename
  enable_if<details::is_compare_builtin<Container>,
            euclidian_neighbor_iterator_pair<const Container, double> >::type
  euclidian_neighbor_crange
  (const Container& container,
   const typename Container::key_type& target)
  { return euclidian_neighbor_range (container, target); }
  ///@}

} // namespace spatial

#endif // SPATIAL_EUCLIDIAN_NEIGHBOR_HPP
