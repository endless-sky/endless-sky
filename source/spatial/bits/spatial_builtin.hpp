// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_builtin.hpp
 *  Defines the set of meta-programing types to find if a comparator is
 *  built-in or not.
 */

#ifndef SPATIAL_COMPARE_BUILTIN_HPP
#define SPATIAL_COMPARE_BUILTIN_HPP

#include "spatial_import_type_traits.hpp"
#include "spatial_check_concept.hpp"

namespace spatial
{
  // Forward declarations for builtin compartors and difference
  template<typename Accessor, typename Tp, typename Unit> struct accessor_minus;
  template<typename Tp, typename Unit> struct bracket_minus;
  template<typename Tp, typename Unit> struct paren_minus;
  template<typename Tp, typename Unit> struct iterator_minus;
  template<typename Accessor, typename Tp> struct accessor_less;
  template<typename Tp> struct bracket_less;
  template<typename Tp> struct paren_less;
  template<typename Tp> struct iterator_less;

  namespace details
  {
    /**
     * Help to resolve whether the type used is a builtin comparator or not. It
     * is used as the base type of \ref is_compare_builtin.
     */
    ///@{
    template <typename>
    struct is_compare_builtin_helper : import::false_type { };
    template <typename Tp>
    struct is_compare_builtin_helper<bracket_less<Tp> >
      : import::true_type { };
    template <typename Tp>
    struct is_compare_builtin_helper<paren_less<Tp> >
      : import::true_type { };
    template <typename Tp>
    struct is_compare_builtin_helper<iterator_less<Tp> >
      : import::true_type { };
    template <typename Accessor, typename Tp>
    struct is_compare_builtin_helper<accessor_less<Accessor, Tp> >
      : import::true_type { };
    ///@}

    /**
     *  Statically resolve if key_compare used in the container \c corresponds
     *  to one of the builtin library comparators or not. Designed to be used
     *  with \ref enable_if.
     */
    template <typename Container>
    struct is_compare_builtin
      : is_compare_builtin_helper<typename Container::key_compare>
    { };

    /**
     *  The generic helper class to determine if a container uses a built-in
     *  compare type. See the specializations of this class.
     */
    template<typename Container, typename Enable = void>
    struct with_builtin_difference { }; // sink for not using built-in compare

    /**
     *  Retrieve the builtin difference functor on the condition that the
     *  compare functor used in Container is a builtin comparator.
     *
     *  If you are not using one of the builtin compare functor for your
     *  Container, then you should also provide your user-defined difference
     *  function when using euclidian_neighbor_iterator<> or other similar
     *  iterators. For builtin comparator, this template helper will always
     *  return a difference operator with 'void' as a distance type. This \c
     *  void distance type is rebound in the neighbor_iterator into the correct
     *  distance type, provided that a builtin difference functor was used.
     *
     *  \tparam Container The container used in the iterator.
     *  \tparam DistanceType The type used to express distances.
     */
    template<typename Container>
    struct with_builtin_difference
    <Container,
     typename enable_if<is_compare_builtin<Container> >::type>
    {
      /**
       *  This internal type casting is used to resolve a built-in compare
       *  functor (provided by the library) into a built-in difference functor.
       *  It will not work for user defined comparators; it means that if you
       *  are using a user-defined comparator in your container, you should also
       *  use a user-defined metric.
       *
       *  \tparam Compare The comparator used in the container.
       *  \tparam DistanceType The type used to express distances.
       */
      ///@{
      template<typename>
      struct builtin_difference { }; // sink type, never used normally

      template<typename Key>
      struct builtin_difference<bracket_less<Key> >
      {
        typedef bracket_minus<Key, void> type;
        type operator() (const bracket_less<Key>&) const
        { return type(); }
      };

      template<typename Key>
      struct builtin_difference<paren_less<Key> >
      {
        typedef paren_minus<Key, void> type;
        type operator() (const paren_less<Key>&) const
        { return type(); }
      };

      template<typename Key>
      struct builtin_difference<iterator_less<Key> >
      {
        typedef iterator_minus<Key, void> type;
        type operator() (const iterator_less<Key>&) const
        { return type(); }
      };

      template<typename Key, typename Accessor>
      struct builtin_difference<accessor_less<Accessor, Key> >
      {
        typedef accessor_minus<Accessor, Key, void> type;
        type operator() (const accessor_less<Accessor, Key>& cmp) const
        { return type(cmp.accessor()); }
      };
      ///@}

      typedef typename builtin_difference
      <typename Container::key_compare>::type type;

      //! Constructs the difference type from the built-in container's key
      //! compare operator.
      type operator()(const Container& container) const
      {
        return builtin_difference
          <typename Container::key_compare>()
          (container.key_comp());
      }
    };

    /**
     *  Help to resolve whether the type used is a builtin difference or not.
     *
     *  Inherits \c import::true_type if it is one of the built-in difference
     *  functors, \c import::false_type if it is not. Designed to be used with
     *  \ref spatial::enable_if.
     */
    ///@{
    template <typename>
    struct is_difference_builtin : import::false_type { };
    template <typename Tp, typename Unit>
    struct is_difference_builtin<bracket_minus<Tp, Unit> >
      : import::true_type { };
    template <typename Tp, typename Unit>
    struct is_difference_builtin<paren_minus<Tp, Unit> >
      : import::true_type { };
    template <typename Tp, typename Unit>
    struct is_difference_builtin<iterator_minus<Tp, Unit> >
      : import::true_type { };
    template <typename Accessor, typename Tp, typename Unit>
    struct is_difference_builtin<accessor_minus<Accessor, Tp, Unit> >
      : import::true_type { };
    ///@}

    /**
     *  If \c Diff is a builtin difference type, change the current unit of Diff
     *  to the DistanceType specified in the template parameter. If not, type is
     *  simply equivalent to Diff.
     *
     *  This type is used to rebind the metric from one unit into another when
     *  using built-in difference type. This is necessary because when calling
     *  \ref spatial::euclidian_neighbor_begin(), you do not have the
     *  possibility of specifying a type for the unit to use (the library
     *  assumes `double`). However that type can be defined in the return type,
     *  similarly to:
     *
     *  \code
     *  spatial::euclidian_neighbor_iterator<container_type, float> iter
     *    = spatial::euclidian_neighbor_begin(container, target);
     *  \endcode
     *
     *  \ref spatial::euclidian_neighbor_begin() first creates
     *  a metric of type \euclidian with \c
     *  spatial::euclidian<container_type, double>,
     *  then this metric is rebound into a metric of type \c
     *  spatial::euclidian<container_type, float>.
     *
     *  \tparam Diff Either a built-in difference functor, or one provided by
     *  the user.
     *  \tparam DistanceType The distance to use for `Diff`, if `Diff` is a built-in
     *  difference functor.
     *
     *  \sa spatial::bracket_minus
     *  \sa spatial::paren_minus
     *  \sa spatial::accessor_minus
     *  \sa spatial::iterator_minus
     */
    template <typename Diff, typename DistanceType>
    struct rebind_builtin_difference
    { typedef Diff type; }; // sink type

    /**
     *  Specialization of \ref rebind_builtin_difference for the built-in \ref
     *  spatial::bracket_minus functor. Rebinds the functor to the requested
     *  unit type.
     */
    template <typename Tp, typename Unit, typename DistanceType>
    struct rebind_builtin_difference<bracket_minus<Tp, Unit>, DistanceType>
    { typedef bracket_minus<Tp, DistanceType> type; };

    /**
     *  Specialization of \ref rebind_builtin_difference for the built-in \ref
     *  spatial::paren_minus functor. Rebinds the functor to the requested unit
     *  type.
     */
    template <typename Tp, typename Unit, typename DistanceType>
    struct rebind_builtin_difference<paren_minus<Tp, Unit>, DistanceType>
    { typedef paren_minus<Tp, DistanceType> type; };

    /**
     *  Specialization of \ref rebind_builtin_difference for the built-in \ref
     *  spatial::iterator_minus functor. Rebinds the functor to the requested
     *  unit type.
     */
    template <typename Tp, typename Unit, typename DistanceType>
    struct rebind_builtin_difference<iterator_minus<Tp, Unit>, DistanceType>
    { typedef iterator_minus<Tp, DistanceType> type; };

    /**
     *  Specialization of \ref rebind_builtin_difference for the built-in \ref
     *  spatial::accessor_minus functor. Rebinds the functor to the requested
     *  unit type.
     */
    template <typename Accessor, typename Tp, typename Unit,
              typename DistanceType>
    struct rebind_builtin_difference<accessor_minus<Accessor, Tp, Unit>,
                                     DistanceType>
    { typedef accessor_minus<Accessor, Tp, DistanceType> type; };
  }
}

#endif // SPATIAL_COMPARE_BUILTIN_HPP
