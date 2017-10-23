// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_open_region.hpp
 *
 *  Contains the definition of \open_region_iterator. These iterators walk
 *  through all items in the container that are contained within an orthogonal
 *  region bounded by a \c low and a \c high value.
 */

#ifndef SPATIAL_OPEN_REGION_HPP
#define SPATIAL_OPEN_REGION_HPP

#include "spatial_region.hpp"

namespace spatial
{
  /**
   *  A model of \region_predicate that checks if a value of type \c Key is
   *  contained within the open boundaries defined by \c lower and \c upper.
   *
   *  To be very specific, given a dimension \f$d\f$ we define that \f$x\f$ is
   *  contained in the open boundaries \f$(lower, upper)\f$ if:
   *
   *  \f[lower_d < x_d < upper_d\f]
   *
   *  Simply stated, open_bounds used in a region_iterator will match all keys
   *  that are strictly within the region defined by \c lower and \c upper.
   *
   *  \tparam Key The type used during the comparison.
   *  \tparam Compare The comparison functor using to compare 2 objects of type
   *  \c Key along the same dimension.
   *  \concept_region_predicate
   */
  template <typename Key, typename Compare>
  class open_bounds
    : private Compare // empty member optimization
  {
  public:
    /**
     *  The default constructor leaves everything un-initialized
     */
    open_bounds()
      : Compare(), _lower(), _upper() { }

    /**
     *  Set the lower and upper boundary for the orthogonal range
     *  search in the region.
     *  \see make_open_bounds
     *
     *  The constructor does not check that elements of lower are lesser
     *  than elements of \c upper along any dimension. Therefore you must be
     *  careful of the order in which these values are inserted.
     *
     *  \param compare The comparison functor, for initialization.
     *  \param lower The value of the lower bound.
     *  \param upper The value of the upper bound.
     */
    open_bounds(const Compare& compare, const Key& lower,
                const Key& upper)
      : Compare(compare), _lower(lower), _upper(upper)
    { }

    /**
     *  The operator that returns wheather the point is in region or not.
     *
     *  \param dim The dimension of comparison, that should always be less than
     *  the rank of the type \c Key.
     *  \param key The value to compare to the interval defined by \c _lower and
     *  \c _upper.
     *  \returns spatial::below to indicate that \c key is lesser or equal to \c
     *  _lower; spatial::above to indicate that \c key is great or equal to \c
     *  _upper; spatial::matching to indicate that \c key is strictly within \c
     *  _lower and _upper.
     */
    relative_order
    operator()(dimension_type dim, dimension_type, const Key& key) const
    {
      return (!Compare::operator()(dim, _lower, key)
              ? below
              : (Compare::operator()(dim, key, _upper)
                 ? matching
                 : above));
    }

  private:
    /**
     *  The lower bound for the orthogonal region iterator.
     */
    Key _lower;

    /**
     *  The upper bound for the orthogonal region iterator.
     */
    Key _upper;
  };

  /**
   *  A \ref open_bounds factory that takes in a \c container, a region defined
   *  by \c lower and \c upper, and returns a constructed \ref open_bounds
   *  object.
   *
   *  This factory also checks that \c lower is always less than \c upper for
   *  every dimension. If it is not, it raises \ref invalid_bounds.
   *
   *  Because of this extra check, it is safer to invoke the factory rather than
   *  the constructor to build this object, especially if you are expecting user
   *  inputs.
   *
   *  \param container A container to iterate.
   *  \param lower A key defining the lower bound of \ref open_bounds.
   *  \param upper A key defining the upper bound of \ref open_bounds.
   *  \return a constructed \ref open_bounds object.
   *  \throws invalid_bounds
   */
  template <typename Tp>
  open_bounds<typename Tp::key_type,
                    typename Tp::key_compare>
  make_open_bounds
  (const Tp& container,
   const typename Tp::key_type& lower,
   const typename Tp::key_type& upper)
  {
    except::check_open_bounds(container, lower, upper);
    return open_bounds
      <typename Tp::key_type,
      typename Tp::key_compare>
      (container.key_comp(), lower, upper);
  }

  template<typename Container>
  struct open_region_iterator
    : region_iterator<Container,
                      open_bounds<typename Container::key_type,
                                  typename Container::key_compare> >
  {
    open_region_iterator() { }

    open_region_iterator
    (const region_iterator<Container,
     open_bounds <typename Container::key_type,
     typename Container::key_compare> >& other)
      : region_iterator
        <Container, open_bounds<typename Container::key_type,
                                typename Container::key_compare> >
        (other) { }
  };

  template<typename Container>
  struct open_region_iterator<const Container>
    : region_iterator<const Container,
                      open_bounds<typename Container::key_type,
                                  typename Container::key_compare> >
  {
    open_region_iterator() { }

    open_region_iterator
    (const region_iterator<const Container,
                           open_bounds<typename Container::key_type,
                                       typename Container::key_compare> >&
     other)
      : region_iterator<const Container,
                        open_bounds<typename Container::key_type,
                                    typename Container::key_compare> >
      (other) { }

    open_region_iterator
    (const region_iterator<Container,
                           open_bounds<typename Container::key_type,
                                       typename Container::key_compare> >&
     other)
      : region_iterator
        <const Container, open_bounds<typename Container::key_type,
                                      typename Container::key_compare> >
        (other) { }
  };

  template<typename Container>
  struct open_region_iterator_pair
    : region_iterator_pair<Container,
                           open_bounds<typename Container::key_type,
                                       typename Container::key_compare> >
  {
    open_region_iterator_pair() { }

    open_region_iterator_pair
    (const region_iterator<Container,
                           open_bounds<typename Container::key_type,
                                       typename Container::key_compare> >& a,
     const region_iterator<Container,
                           open_bounds<typename Container::key_type,
                                       typename Container::key_compare> >& b)
      : region_iterator_pair<Container,
                             open_bounds<typename Container::key_type,
                                         typename Container::key_compare> >
        (a, b) { }
  };

  template<typename Container>
  struct open_region_iterator_pair<const Container>
    : region_iterator_pair
      <const Container, open_bounds<typename Container::key_type,
                                    typename Container::key_compare> >
  {
    open_region_iterator_pair() { }

    open_region_iterator_pair
    (const region_iterator
     <const Container, open_bounds<typename Container::key_type,
                                   typename Container::key_compare> >& a,
     const region_iterator
     <const Container, open_bounds<typename Container::key_type,
                                   typename Container::key_compare> >& b)
      : region_iterator_pair
        <const Container, open_bounds<typename Container::key_type,
                                      typename Container::key_compare> >
        (a, b) { }

    open_region_iterator_pair
    (const region_iterator_pair
     <Container, open_bounds<typename Container::key_type,
                             typename Container::key_compare> >& other)
      : region_iterator_pair
        <const Container, open_bounds<typename Container::key_type,
                                      typename Container::key_compare> >
        (other) { }
  };

  template <typename Container>
  inline open_region_iterator<Container>
  open_region_end(Container& container,
                  const typename Container::key_type& lower,
                  const typename Container::key_type& upper)
  { return region_end(container, make_open_bounds(container, lower, upper)); }

  template <typename Container>
  inline open_region_iterator<const Container>
  open_region_cend(const Container& container,
                   const typename Container::key_type& lower,
                   const typename Container::key_type& upper)
  { return region_cend(container, make_open_bounds(container, lower, upper)); }

  template <typename Container>
  inline open_region_iterator<Container>
  open_region_begin(Container& container,
                    const typename Container::key_type& lower,
                    const typename Container::key_type& upper)
  { return region_begin(container, make_open_bounds(container, lower, upper)); }

  template <typename Container>
  inline open_region_iterator<const Container>
  open_region_cbegin(const Container& container,
                     const typename Container::key_type& lower,
                     const typename Container::key_type& upper)
  {
    return region_cbegin(container, make_open_bounds(container, lower, upper));
  }

  template <typename Container>
  inline open_region_iterator_pair<Container>
  open_region_range(Container& container,
                    const typename Container::key_type& lower,
                    const typename Container::key_type& upper)
  { return region_range(container, make_open_bounds(container, lower, upper)); }

  template <typename Container>
  inline open_region_iterator_pair<const Container>
  open_region_crange(const Container& container,
                     const typename Container::key_type& lower,
                     const typename Container::key_type& upper)
  {
    return region_crange(container, make_open_bounds(container, lower, upper));
  }

} // namespace spatial

#endif // SPATIAL_OPEN_REGION_HPP
