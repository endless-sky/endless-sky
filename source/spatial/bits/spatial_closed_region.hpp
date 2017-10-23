// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file  spatial_closed_region.hpp
 *
 *  Contains the definition of \closed_region_iterator. These iterators walk
 *  through all items in the container that are contained within an orthogonal
 *  region bounded by a \c low and a \c high value.
 */

#ifndef SPATIAL_CLOSED_REGION_HPP
#define SPATIAL_CLOSED_REGION_HPP

#include "spatial_region.hpp"

namespace spatial
{
  /**
   *  A model of \region_predicate that checks if a value of type \c Key is
   *  contained within the closed boundaries defined by \c lower and \c upper.
   *
   *  To be very specific, for any dimension \f$d\f$ we define that \f$x\f$ is
   *  contained in the closed boundaries \f$(lower, upper)\f$ if:
   *
   *  \f[lower_d <= x_d <= upper_d\f]
   *
   *  Simply stated, \ref closed_bounds used in a region_iterator will match all
   *  keys that are within the region defined by \c lower and \c upper, even if
   *  they "touch" the edge of the region.
   *
   *  \tparam Key The type used during the comparison.
   *  \tparam Compare The comparison functor using to compare 2 objects of type
   *  \c Key along the same dimension.
   *  \concept_region_predicate
   */
  template <typename Key, typename Compare>
  class closed_bounds
    : private Compare // empty member optimization
  {
  public:
    /**
     *  The default constructor leaves everything un-initialized.
     */
    closed_bounds() : Compare(), _lower(), _upper() { }

    /**
     *  Set the lower and upper boundary for the orthogonal region
     *  search.
     */
    closed_bounds(const Compare& compare, const Key& lower,
                        const Key& upper)
      : Compare(compare), _lower(lower), _upper(upper)
    { }

    /**
     *  The operator that tells wheather the point is in region or not.
     */
    relative_order
    operator()(dimension_type dim, dimension_type, const Key& key) const
    {
      return (Compare::operator()(dim, key, _lower)
              ? below
              : (Compare::operator()(dim, _upper, key)
                 ? above
                 : matching));
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
   *  A \ref closed_bounds factory that takes in a \c container, a region defined
   *  by \c lower and \c upper, and returns a constructed \ref closed_bounds
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
   *  \param lower A key defining the lower bound of \ref closed_bounds.
   *  \param upper A key defining the upper bound of \ref closed_bounds.
   *  \return a constructed \ref closed_bounds object.
   *  \throws invalid_bounds
   */
  template <typename Container>
  closed_bounds<typename Container::key_type,
                typename Container::key_compare>
  make_closed_bounds
  (const Container& container,
   const typename Container::key_type& lower,
   const typename Container::key_type& upper)
  {
    except::check_closed_bounds(container, lower, upper);
    return closed_bounds
      <typename Container::key_type,
       typename Container::key_compare>
      (container.key_comp(), lower, upper);
  }

  template<typename Container>
  struct closed_region_iterator
    : region_iterator
      <Container, closed_bounds<typename Container::key_type,
                                typename Container::key_compare> >
  {
    closed_region_iterator() { }

    closed_region_iterator
    (const region_iterator
     <Container, closed_bounds<typename Container::key_type,
                               typename Container::key_compare> >&
     other)
      : region_iterator
        <Container, closed_bounds<typename Container::key_type,
                                  typename Container::key_compare> >
        (other) { }
  };

  template<typename Container>
  struct closed_region_iterator<const Container>
    : region_iterator
      <const Container, closed_bounds<typename Container::key_type,
                                      typename Container::key_compare> >
  {
    closed_region_iterator() { }

    closed_region_iterator
    (const region_iterator
     <const Container, closed_bounds<typename Container::key_type,
                                     typename Container::key_compare> >&
     other)
      : region_iterator
        <const Container, closed_bounds<typename Container::key_type,
                                        typename Container::key_compare> >
        (other) { }

    closed_region_iterator
    (const region_iterator
     <Container, closed_bounds<typename Container::key_type,
                               typename Container::key_compare> >&
     other)
      : region_iterator
        <const Container, closed_bounds<typename Container::key_type,
                                        typename Container::key_compare> >
        (other) { }
  };

  template<typename Container>
  struct closed_region_iterator_pair
    : region_iterator_pair
  <Container, closed_bounds<typename Container::key_type,
                            typename Container::key_compare> >
  {
  closed_region_iterator_pair() { }

  closed_region_iterator_pair
  (const region_iterator
   <Container, closed_bounds<typename Container::key_type,
                             typename Container::key_compare> >& a,
   const region_iterator
    <Container, closed_bounds<typename Container::key_type,
                              typename Container::key_compare> >& b)
    : region_iterator_pair
      <Container, closed_bounds<typename Container::key_type,
                                typename Container::key_compare> >
      (a, b) { }
  };

  template<typename Container>
  struct closed_region_iterator_pair<const Container>
    : region_iterator_pair
      <const Container, closed_bounds<typename Container::key_type,
                                      typename Container::key_compare> >
  {
    closed_region_iterator_pair() { }

    closed_region_iterator_pair
    (const region_iterator
     <const Container, closed_bounds<typename Container::key_type,
                                     typename Container::key_compare> >& a,
     const region_iterator
     <const Container, closed_bounds<typename Container::key_type,
                                     typename Container::key_compare> >& b)
      : region_iterator_pair
        <const Container, closed_bounds<typename Container::key_type,
                                        typename Container::key_compare> >
        (a, b) { }

    closed_region_iterator_pair(const closed_region_iterator_pair<Container>& other)
      : region_iterator_pair
        <const Container, closed_bounds<typename Container::key_type,
                                        typename Container::key_compare> >
        (other) { }
  };

  template <typename Container>
  inline closed_region_iterator<Container>
  closed_region_end(Container& container,
                    const typename Container::key_type& lower,
                    const typename Container::key_type& upper)
  { return region_end(container, make_closed_bounds(container, lower, upper)); }

  template <typename Container>
  inline closed_region_iterator<const Container>
  closed_region_cend(const Container& container,
                     const typename Container::key_type& lower,
                     const typename Container::key_type& upper)
  {
    return region_cend(container, make_closed_bounds(container, lower, upper));
  }

  template <typename Container>
  inline closed_region_iterator<Container>
  closed_region_begin(Container& container,
                      const typename Container::key_type& lower,
                      const typename Container::key_type& upper)
  {
    return region_begin(container, make_closed_bounds(container, lower, upper));
  }

  template <typename Container>
  inline closed_region_iterator<const Container>
  closed_region_cbegin(const Container& container,
                       const typename Container::key_type& lower,
                       const typename Container::key_type& upper)
  {
    return region_cbegin(container,
                         make_closed_bounds(container, lower, upper));
  }

  template <typename Container>
  inline closed_region_iterator_pair<Container>
  closed_region_range(Container& container,
                      const typename Container::key_type& lower,
                      const typename Container::key_type& upper)
  {
    return region_range(container, make_closed_bounds(container, lower, upper));
  }

  template <typename Container>
  inline closed_region_iterator_pair<const Container>
  closed_region_crange(const Container& container,
                       const typename Container::key_type& lower,
                       const typename Container::key_type& upper)
  {
    return region_crange(container,
                         make_closed_bounds(container, lower, upper));
  }

} // namespace spatial

#endif // SPATIAL_CLOSED_REGION_HPP
