// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_neighbor.hpp
 *  Provides neighbor iterator and all the functions around it.
 */

#ifndef SPATIAL_NEIGHBOR_HPP
#define SPATIAL_NEIGHBOR_HPP

#include <limits> // numeric_limits min() and max()

#include "spatial_import_tuple.hpp"
#include "../metric.hpp"
#include "spatial_bidirectional.hpp"
#include "spatial_compress.hpp"

namespace spatial
{
  namespace details
  {
    /**
     *  Extra information needed by the iterator to perform its work. This
     *  information is copied to each iterator from a given container.
     *
     *  Although it may be possible to modify this information directly from
     *  it's members, it may be unwise to do so, as it could invalidate the
     *  iterator and cause the program to behave unexpectedly. If any of this
     *  information needs to be modified, it is probably recommended to create a
     *  new iterator altogether.
     *
     *  \tparam Container The container to which these iterator relate to.
     *  \tparam Metric The type of \metric applied to the iterator.
     *  \tparam DistanceType The type used to store the distance value.
     */
    template<typename Container, typename Metric>
    struct Neighbor_data : Container::key_compare
    {
      //! Build an unintialized neighbor data object.
      Neighbor_data() { }

      /**
       *  Builds required neighbor data from the given container, metric and
       *  dimension.
       *
       *  \param container The container being iterated.
       *  \param metric The metric to apply during iteration
       *  \param key The key representing the iteration target.
       *  \param distance Cached distance value from the last iterator's
       *  distance computation.
       */
      Neighbor_data
      (const typename Container::key_compare& container,
       const Metric& metric,
       const typename Container::key_type& key,
       typename Metric::distance_type distance)
        : Container::key_compare(container), _target(metric, key),
          _distance(distance) { }

      /**
       *  The target of the iteration; element of the container are iterate
       *  from the closest to the element furthest away from the target.
       */
      Compress<Metric, typename Container::key_type> _target;

      /**
       *  The last valid computed value of the distance. The value stored is
       *  only valid if the iterator is not past-the-end.
       */
      typename Metric::distance_type _distance;
    };
  } // namespace details

  /**
   *  A spatial iterator for a container \c Container that goes through the nearest
   *  to the furthest element from a target key, with distances applied
   *  according to a user-defined geometric space that is a model of \metric.
   *
   *  \tparam Container The container type bound to the iterator.
   *  \tparam DistanceType The type used to represent distances.
   *  \tparam Metric An type that is a model of \metric.
   *
   *  The Metric type is a complex type that must be a model of \metric:
   *
   *  \code
   *  struct Metric
   *  {
   *    typedef DistanceType distance_type;
   *
   *    DistanceType
   *    distance_to_key(dimension_type rank,
   *                    const Key& origin, const Key& key) const;
   *
   *    DistanceType
   *    distance_to_plane(dimension_type rank, dimension_type dim,
   *                      const Key& origin, const Key& key) const;
   *  };
   *  \endcode
   *
   *  The details of the \c Metric type are explained in \metric. The library
   *  provides ready-made models of \c Metric such as \euclidian and \manhattan
   *  that are designed to work only with C++'s built-in arithmetic types. If
   *  more metrics needs to be defined, see the explanation in the \metric
   *  concept.
   */
  template <typename Container, typename Metric =
            euclidian<typename details::mutate<Container>::type, double,
                      typename details::with_builtin_difference<Container>
                      ::type> >
  class neighbor_iterator
    : public details::Bidirectional_iterator
  <typename Container::mode_type,
   typename Container::rank_type>
  {
  private:
    typedef typename details::Bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Key comparator type transferred from the container
    typedef typename Container::key_compare key_compare;

    //! The metric type used by the iterator
    typedef Metric metric_type;

    //! The distance type that is read from metric_type
    typedef typename Metric::distance_type distance_type;

    //! The key type that is used as a target for the nearest neighbor search
    typedef typename Container::key_type key_type;

    //! Uninitialized iterator.
    neighbor_iterator() { }

    /**
     *  The standard way to build this iterator: specify a metric to apply, an
     *  iterator on a container, and that container.
     *
     *  \param container_ The container to iterate.
     *  \param metric_ The \metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param iter_ An iterator on container.
     *  \param distance_ The distance at which the node pointed by iterator is
     *  from target.
     */
    neighbor_iterator
    (Container& container_, const Metric& metric_,
     const typename Container::key_type& target_,
     const typename Container::iterator& iter_,
     typename Metric::distance_type distance_)
      : Base(container_.rank(), iter_.node,
             modulo(iter_.node, container_.rank())),
        _data(container_.key_comp(), metric_, target_, distance_) { }

    /**
     *  When the information of the dimension for the current node being
     *  pointed to by the iterator is known, this constructor saves some CPU
     *  cycle, by comparison to the other constructor.
     *
     *  \param container_ The container to iterate.
     *  \param metric_ The metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param node_dim_ The dimension of the node pointed to by iterator.
     *  \param node_ Use the value of node as the start point for the
     *  iteration.
     *  \param distance_ The distance between \c node_ and \c target_ according
     *  to \c metric_.
     *
     *  In order to iterate through nodes in the \kdtree built in the
     *  container, the algorithm must know at each node which dimension is
     *  used to partition the space. Some algorithms will provide this
     *  dimension, such as the function \ref spatial::details::modulo().
     *
     *  \attention Specifying the incorrect dimension value for the node will
     *  result in unknown behavior. It is recommended that you do not use this
     *  constructor if you are not sure about this dimension, and use the
     *  other constructors instead.
     */
    neighbor_iterator
    (Container& container_, const Metric& metric_,
     const typename Container::key_type& target_,
     dimension_type node_dim_,
     typename Container::mode_type::node_ptr node_,
     typename Metric::distance_type distance_)
      : Base(container_.rank(), node_, node_dim_),
        _data(container_.key_comp(), metric_, target_, distance_) { }

    /**
     *  Build the iterator with a given rank and key compare functor, if the
     *  container is not available.
     *
     *  In order to iterate through nodes in the \kdtree built in the
     *  container, the algorithm must know at each node which dimension is
     *  used to partition the space. Some algorithms will provide this
     *  dimension, such as the function \ref spatial::details::modulo().
     *
     *  \attention Specifying the incorrect dimension value for the node will
     *  result in unknown behavior. It is recommended that you do not use this
     *  constructor if you are not sure about this dimension, and use the
     *  other constructors instead.
     *
     *  \param rank_ The rank of the container being iterated.
     *  \param key_comp_ The key compare functor associated with the iterator.
     *  \param metric_ The metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param node_dim_ The dimension of the node pointed to by iterator.
     *  \param node_ Use the value of node as the start point for the
     *  iteration.
     *  \param distance_ The distance between \c node_ and \c target_ according
     *  to \c metric_.
     */
    neighbor_iterator
    (const typename Container::rank_type& rank_,
     const typename Container::key_compare& key_comp_,
     const Metric& metric_,
     const typename Container::key_type& target_,
     dimension_type node_dim_,
     typename Container::mode_type::node_ptr node_,
     typename Metric::distance_type distance_)
      : Base(rank_, node_, node_dim_),
        _data(key_comp_, metric_, target_, distance_) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    neighbor_iterator<Container, Metric>& operator++()
    {
      import::tie(node, node_dim, distance())
        = increment_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    neighbor_iterator<Container, Metric> operator++(int)
    {
      neighbor_iterator<Container, Metric> x(*this);
      import::tie(node, node_dim, distance())
        = increment_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    neighbor_iterator<Container, Metric>& operator--()
    {
      import::tie(node, node_dim, distance())
        = decrement_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    neighbor_iterator<Container, Metric> operator--(int)
    {
      neighbor_iterator<Container, Metric> x(*this);
      import::tie(node, node_dim, distance())
        = decrement_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return static_cast<const key_compare&>(_data); }

    //! Return the metric used by the iterator
    metric_type
    metric() const { return _data._target.base(); }

    /**
     *  Read-only accessor to the last valid distance of the iterator.
     *
     *  If the iterator is past-the-end (in other words, equal to an iterator
     *  returned by \ref neighbor_end()), then the value returned by distance()
     *  is undefined.
     */
    const distance_type&
    distance() const { return _data._distance; }

    /**
     *  Read/write accessor to the last valid distance of the iterator.
     *
     *  If the iterator is past-the-end (in other words, equal to an iterator
     *  returned by \ref neighbor_end()), then the value returned by distance()
     *  is undefined.
     */
    distance_type&
    distance() { return _data._distance; }

    //! Read-only accessor to the target of the iterator
    const key_type&
    target_key() const { return _data._target(); }

    //! Read/write accessor to the target of the iterator
    key_type&
    target_key() { return _data._target(); }

  private:
    //! The related data for the iterator.
    details::Neighbor_data<Container, Metric> _data;
  };

  /**
   *  A spatial iterator for a container \c Container that goes through the nearest
   *  to the furthest element from a target key, with distances applied
   *  according to a user-defined geometric space of type \c Metric.
   *
   *  The Metric type is a complex type that must be a model of
   *  \metric:
   *
   *  \code
   *  struct Metric
   *  {
   *    typedef my_distance_type distance_type;
   *
   *    distance_type
   *    distance_to_key(dimension_type rank,
   *                    const Key& origin, const Key& key) const;
   *
   *    distance_type
   *    distance_to_plane(dimension_type rank, dimension_type dim,
   *                      const Key& origin, const Key& key) const;
   *  };
   *  \endcode
   *
   *  The details of the \c Metric type are explained in \metric.  Metrics are
   *  generally not defined by the user of the library, given their
   *  complexity. Rather, the user of the library uses ready-made models of
   *  \metric such as \euclidian and \manhattan. If more metrics needs to be
   *  defined, see the explanation in for the \metric concept.
   *
   *  This iterator only returns constant objects.
   *
   *  \tparam Container The container type bound to the iterator.
   *  \tparam DistanceType The type used to represent distances.
   *  \tparam Metric An type that follow the \metric concept.
   */
  template<typename Container, typename Metric>
  class neighbor_iterator<const Container, Metric>
    : public details::Const_bidirectional_iterator
      <typename Container::mode_type,
       typename Container::rank_type>
  {
  private:
    typedef typename details::Const_bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Key comparator type transferred from the container
    typedef typename Container::key_compare key_compare;

    //! The metric type used by the iterator
    typedef Metric metric_type;

    //! The distance type that is read from metric_type
    typedef typename Metric::distance_type distance_type;

    //! The key type that is used as a target for the nearest neighbor search
    typedef typename Container::key_type key_type;

    //! \empty
    neighbor_iterator() { }

    /**
     *  The standard way to build this iterator: specify a metric to apply,
     *  an iterator on a container, and that container.
     *
     *  \param container_ The container to iterate.
     *  \param metric_ The metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param iter_ An iterator on \c container.
     *  \param distance_ The distance between the node pointed to by \c iter_
     *  and \c target_ according to \c metric_.
     */
    neighbor_iterator
    (const Container& container_, const Metric& metric_,
     const typename Container::key_type& target_,
     typename Container::const_iterator iter_,
     typename Metric::distance_type distance_)
      : Base(container_.rank(), iter_.node,
             modulo(iter_.node, container_.rank())),
        _data(container_.key_comp(), metric_, target_, distance_) { }

    /**
     *  When the information of the dimension for the current node being
     *  pointed to by the iterator is known, this constructor saves some CPU
     *  cycle, by comparison to the other constructor.
     *
     *  \param container_ The container to iterate.
     *  \param metric_ The metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param node_dim_ The dimension of the node pointed to by iterator.
     *  \param node_ Use the value of node as the start point for the
     *  iteration.
     *  \param distance_ The distance between \c node_ and \c target_ according
     *  to \c metric_.
     *
     *  In order to iterate through nodes in the \kdtree built in the
     *  container, the algorithm must know at each node which dimension is
     *  used to partition the space. Some algorithms will provide this
     *  dimension, such as the function \ref spatial::details::modulo().
     *
     *  \attention Specifying the incorrect dimension value for the node will
     *  result in unknown behavior. It is recommended that you do not use this
     *  constructor if you are not sure about this dimension, and use the
     *  other constructors instead.
     */
    neighbor_iterator
    (const Container& container_, const Metric& metric_,
     const typename Container::key_type& target_,
     dimension_type node_dim_,
     typename Container::mode_type::const_node_ptr node_,
     typename Metric::distance_type distance_)
      : Base(container_.rank(), node_, node_dim_),
        _data(container_.key_comp(), metric_, target_, distance_) { }

    /**
     *  Build the iterator with a given rank and key compare functor, if the
     *  container is not available. It requires the node information to be
     *  known but is a fast constructor.
     *
     *  \param rank_ The rank of the container being iterated.
     *  \param key_comp_ The key compare functor associated with the iterator.
     *  \param metric_ The metric applied during the iteration.
     *  \param target_ The target of the neighbor iteration.
     *  \param node_dim_ The dimension of the node pointed to by iterator.
     *  \param node_ Use the value of node as the start point for the
     *  iteration.
     *  \param distance_ The distance between \c node_ and \c target_ according
     *  to \c metric_.
     *
     *  \attention Specifying the incorrect dimension value for the node will
     *  result in unknown behavior. It is recommended that you do not use this
     *  constructor if you are not sure about this dimension, and use the
     *  other constructors instead.
     */
    neighbor_iterator
    (const typename Container::rank_type& rank_,
     const typename Container::key_compare& key_comp_,
     const Metric& metric_,
     const typename Container::key_type& target_,
     dimension_type node_dim_,
     typename Container::mode_type::const_node_ptr node_,
     typename Metric::distance_type distance_)
      : Base(rank_, node_, node_dim_),
        _data(key_comp_, metric_, target_, distance_) { }

    //! Convertion of mutable iterator into a constant iterator.
    neighbor_iterator(const neighbor_iterator<Container, Metric>& iter)
      : Base(iter.rank(), iter.node, iter.node_dim),
        _data(iter.key_comp(), iter.metric(),
              iter.target_key(), iter.distance()) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    neighbor_iterator<const Container, Metric>& operator++()
    {
      import::tie(node, node_dim, distance())
        = increment_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    neighbor_iterator<const Container, Metric> operator++(int)
    {
      neighbor_iterator<const Container, Metric> x(*this);
      import::tie(node, node_dim, distance())
        = increment_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    neighbor_iterator<const Container, Metric>& operator--()
    {
      import::tie(node, node_dim, distance())
        = decrement_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    neighbor_iterator<const Container, Metric> operator--(int)
    {
      neighbor_iterator<const Container, Metric> x(*this);
      import::tie(node, node_dim, distance())
        = decrement_neighbor(node, node_dim, rank(), key_comp(),
                             metric(), target_key(), distance());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return static_cast<const key_compare&>(_data); }

    //! Return the metric used by the iterator
    metric_type
    metric() const { return _data._target.base(); }

    /**
     *  Read-only accessor to the last valid distance of the iterator.
     *
     *  If the iterator is past-the-end (in other words, equal to an iterator
     *  returned by \ref neighbor_end()), then the value returned by distance()
     *  is undefined.
     */
    distance_type
    distance() const { return _data._distance; }

    /**
     *  Read/write accessor to the last valid distance of the iterator.
     *
     *  If the iterator is past-the-end (in other words, equal to an iterator
     *  returned by \ref neighbor_end()), then the value returned by distance()
     *  is undefined.
     */
    distance_type&
    distance() { return _data._distance; }

    //! Read-only accessor to the target of the iterator
    const key_type&
    target_key() const { return _data._target(); }

    //! Read/write accessor to the target of the iterator
    key_type&
    target_key() { return _data._target(); }

  private:
    //! The related data for the iterator.
    details::Neighbor_data<Container, Metric> _data;
  };

  /**
   *  Read/write accessor for neighbor iterators that retrieve the valid
   *  calculated distance from the target. The distance read is only relevant if
   *  the iterator does not point past-the-end.
   */
  ///@{
  template <typename Container, typename Metric>
  inline typename Metric::distance_type
  distance(const neighbor_iterator<Container, Metric>& iter)
  { return iter.distance(); }

  template <typename Container, typename Metric>
  inline void
  distance(neighbor_iterator<Container, Metric>& iter,
           const typename Metric::distance_type& distance)
  { iter.distance() = distance; }
  ///@}

  /**
   *  A quick accessor for neighbor iterators that retrive the key that is the
   *  target for the nearest neighbor iteration.
   */
  template <typename Container, typename Metric>
  inline const typename Container::key_type&
  target_key(const neighbor_iterator<Container, Metric>& iter)
  { return iter.target_key(); }

  /**
   *  This structure defines a pair of neighbor iterator.
   *
   *  \tparam Container The container to which these iterator relate to.
   *  \tparam Metric The metric used by the iterator.
   *  \see neighbor_iterator
   */
  template <typename Container, typename Metric
            = euclidian<typename details::mutate<Container>::type, double,
                        typename details::with_builtin_difference<Container>
                        ::type> >
  struct neighbor_iterator_pair
    : std::pair<neighbor_iterator<Container, Metric>,
                neighbor_iterator<Container, Metric> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<neighbor_iterator<Container, Metric>,
                      neighbor_iterator<Container, Metric> > Base;

    //! Empty constructor.
    neighbor_iterator_pair() { }

    //! Regular constructor that builds a neighbor_iterator_pair out of 2
    //! neighbor_iterators.
    neighbor_iterator_pair(const neighbor_iterator<Container, Metric>& a,
                           const neighbor_iterator<Container, Metric>& b)
      : Base(a, b) { }
  };

  /**
   *  This structure defines a pair of constant neighbor iterator.
   *
   *  \tparam Container The container to which these iterator relate to.
   *  \tparam Metric The metric used by the iterator.
   *  \see neighbor_iterator
   */
  template <typename Container, typename Metric>
  struct neighbor_iterator_pair<const Container, Metric>
    : std::pair<neighbor_iterator<const Container, Metric>,
                neighbor_iterator<const Container, Metric> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<neighbor_iterator<const Container, Metric>,
                      neighbor_iterator<const Container, Metric> > Base;

    //! Empty constructor.
    neighbor_iterator_pair() { }

    //! Regular constructor that builds a neighbor_iterator_pair out of 2
    //! neighbor_iterators.
    neighbor_iterator_pair(const neighbor_iterator<const Container, Metric>& a,
                           const neighbor_iterator<const Container, Metric>& b)
      : Base(a, b)
    { }

    //! Convert a mutable neighbor iterator pair into a const neighbor iterator
    //!pair.
    neighbor_iterator_pair(const neighbor_iterator_pair<Container, Metric>& p)
      : Base(p.first, p.second) { }
  };

  /**
   *  Build a past-the-end neighbor iterator with a user-defined \metric.
   *  \param container The container in which a neighbor must be found.
   *  \param metric The metric to use in search of the neighbor.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container, typename Metric>
  inline neighbor_iterator<Container, Metric>
  neighbor_end(Container& container, const Metric& metric,
               const typename Container::key_type& target)
  {
    return neighbor_iterator<Container, Metric>
      (container, metric, target, container.dimension() - 1,
       container.end().node, typename Metric::distance_type());
  }

  template <typename Container, typename Metric>
  inline neighbor_iterator<const Container, Metric>
  neighbor_cend(const Container& container, const Metric& metric,
                const typename Container::key_type& target)
  {
    return neighbor_end(container, metric, target,
                        typename Metric::distance_type());
  }
  ///@}

  /**
   *  Build a past-the-end neighbor iterator, assuming an euclidian metric with
   *  distances expressed in double. It requires that the container used was
   *  defined with a built-in key compare functor.
   *  \param container The container in which a neighbor must be found.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<Container> >::type
  neighbor_end(Container& container,
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
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<const Container> >::type
  neighbor_cend(const Container& container,
                const typename Container::key_type& target)
  {
    return neighbor_end
      (container,
       euclidian<Container, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target);
  }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the nearest neighbor of \c
   *  target using a user-defined \metric.
   *  \param container The container in which a neighbor must be found.
   *  \param metric The metric to use in search of the neighbor.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container, typename Metric>
  inline neighbor_iterator<Container, Metric>
  neighbor_begin(Container& container, const Metric& metric,
                 const typename Container::key_type& target)
  {
    if (container.empty()) return neighbor_end(container, metric, target);
    typename Container::mode_type::node_ptr node = container.end().node->parent;
    dimension_type dim = 0;
    typename Metric::distance_type dist;
    import::tie(node, dim, dist)
      = first_neighbor(node, dim, container.rank(), container.key_comp(),
                       metric, target);
    return neighbor_iterator<Container, Metric>
      (container, metric, target, dim, node, dist);
  }

  template <typename Container, typename Metric>
  inline neighbor_iterator<const Container, Metric>
  neighbor_cbegin(const Container& container, const Metric& metric,
                  const typename Container::key_type& target)
  { return neighbor_begin(container, metric, target); }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the nearest neighbor of \c
   *  target assuming an euclidian metric with distances expressed in double. It
   *  requires that the container used was defined with a built-in key compare
   *  functor.
   *  \param container The container in which a neighbor must be found.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<Container> >::type
  neighbor_begin(Container& container,
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
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<const Container> >::type
  neighbor_cbegin(const Container& container,
                  const typename Container::key_type& target)
  {
    return neighbor_begin
      (container,
       euclidian<Container, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target);
  }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the neighbor closest to
   *  target but for which distance to target is greater or equal to the value
   *  given in \c bound. Uses a user-defined \metric.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param metric The metric to use in search of the neighbor.
   *  \param target The target key used in the neighbor search.
   *  \param bound The minimum distance from the target.
   */
  ///@{
  template <typename Container, typename Metric>
  inline neighbor_iterator<Container, Metric>
  neighbor_lower_bound(Container& container, const Metric& metric,
                       const typename Container::key_type& target,
                       typename Metric::distance_type bound)
  {
    if (container.empty()) return neighbor_end(container, metric, target);
    typename Container::mode_type::node_ptr node = container.end().node->parent;
    dimension_type dim = 0;
    typename Metric::distance_type dist;
    import::tie(node, dim, dist)
      = lower_bound_neighbor(node, dim, container.rank(), container.key_comp(),
                             metric, target, bound);
    return neighbor_iterator<Container, Metric>
      (container, metric, target, dim, node, dist);
  }

  template <typename Container, typename Metric>
  inline neighbor_iterator<const Container, Metric>
  neighbor_clower_bound(const Container& container, const Metric& metric,
                        const typename Container::key_type& target,
                        typename Metric::distance_type bound)
  { return neighbor_lower_bound(container, metric, target, bound); }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the neighbor closest to
   *  target but for which distance to target is greater or equal to the value
   *  given in \c bound. It assumes an euclidian metric with distances expressed
   *  in double. It also requires that the container used was defined with one
   *  of the built-in key compare functor.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param target The target key used in the neighbor search.
   *  \param bound The minimum distance from the target.
   */
  ///@{
  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<Container> >::type
  neighbor_lower_bound(Container& container,
                       const typename Container::key_type& target,
                       double bound)
  {
    return neighbor_lower_bound
      (container,
       euclidian<typename details::mutate<Container>::type, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }

  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<const Container> >::type
  neighbor_clower_bound(const Container& container,
                        const typename Container::key_type& target,
                        double bound)
  {
    return neighbor_lower_bound
      (container,
       euclidian<Container, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the neighbor closest to
   *  target but for which distance to target is strictly greater than the value
   *  given in \c bound. Uses a user-defined \metric.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param metric The metric to use in search of the neighbor.
   *  \param target The target key used in the neighbor search.
   *  \param bound The minimum distance from the target.
   */
  ///@{
  template <typename Container, typename Metric>
  inline neighbor_iterator<Container, Metric>
  neighbor_upper_bound(Container& container, const Metric& metric,
                       const typename Container::key_type& target,
                       typename Metric::distance_type bound)
  {
    if (container.empty()) return neighbor_end(container, metric, target);
    typename Container::mode_type::node_ptr node = container.end().node->parent;
    dimension_type dim = 0;
    typename Metric::distance_type dist;
    import::tie(node, dim, dist)
      = upper_bound_neighbor(node, dim, container.rank(), container.key_comp(),
                             metric, target, bound);
    return neighbor_iterator<Container, Metric>
      (container, metric, target, dim, node, dist);
  }

  template <typename Container, typename Metric>
  inline neighbor_iterator<const Container, Metric>
  neighbor_cupper_bound(const Container& container, const Metric& metric,
                        const typename Container::key_type& target,
                        typename Metric::distance_type bound)
  { return neighbor_upper_bound(container, metric, target, bound); }
  ///@}

  /**
   *  Build a \ref neighbor_iterator pointing to the neighbor closest to
   *  target but for which distance to target is greater than the value given in
   *  \c bound. It assumes an euclidian metric with distances expressed in
   *  double. It also requires that the container used was defined with one of
   *  the built-in key compare functor.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param target The target key used in the neighbor search.
   *  \param bound The minimum distance to the target.
   */
  ///@{
  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<Container> >::type
  neighbor_upper_bound(Container& container,
                       const typename Container::key_type& target,
                       double bound)
  {
    return neighbor_upper_bound
      (container,
       euclidian<typename details::mutate<Container>::type, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }

  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator<const Container> >::type
  neighbor_cupper_bound(const Container& container,
                        const typename Container::key_type& target,
                        double bound)
  {
    return neighbor_upper_bound
      (container,
       euclidian<Container, double,
                 typename details::with_builtin_difference<Container>::type>
         (details::with_builtin_difference<Container>()(container)),
       target, bound);
  }
  ///@}

  /**
   *  Returns a \ref neighbor_iterator_pair representing the range of values
   *  from the closest to the furthest in the container iterated. Uses a
   *  user-defined \metric.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param metric The metric to use in search of the neighbor.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container, typename Metric>
  inline neighbor_iterator_pair<Container, Metric>
  neighbor_range(Container& container, const Metric& metric,
                 const typename Container::key_type& target)
  {
    return neighbor_iterator_pair<Container, Metric>
      (neighbor_begin(container, metric, target),
       neighbor_end(container, metric, target));
  }

  template <typename Container, typename Metric>
  inline neighbor_iterator_pair<const Container, Metric>
  neighbor_crange(const Container& container, const Metric& metric,
                  const typename Container::key_type& target)
  {
    return neighbor_iterator_pair<const Container, Metric>
      (neighbor_begin(container, metric, target),
       neighbor_end(container, metric, target));
  }
  ///@}

  /**
   *  Returns a \ref neighbor_iterator_pair representing the range of values
   *  from the closest to the furthest in the container iterated. It assumes an
   *  euclidian metric with distances expressed in double. It also requires that
   *  the container used was defined with one of the built-in key compare
   *  functor.
   *
   *  \param container The container in which a neighbor must be found.
   *  \param target The target key used in the neighbor search.
   */
  ///@{
  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator_pair<Container> >::type
  neighbor_range(Container& container,
                 const typename Container::key_type& target)
  {
    return neighbor_iterator_pair<Container>
      (neighbor_begin(container, target), neighbor_end(container, target));
  }

  template <typename Container>
  inline typename enable_if<details::is_compare_builtin<Container>,
                            neighbor_iterator_pair<const Container> >::type
  neighbor_crange(const Container& container,
                  const typename Container::key_type& target)
  {
    return neighbor_iterator_pair<const Container>
      (neighbor_begin(container, target), neighbor_end(container, target));
  }
  ///@}

  namespace details
  {
    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    last_neighbor_sub(NodePtr node, dimension_type dim, Rank rank,
                      KeyCompare key_comp, const Metric& met, const Key& target,
                      typename Metric::distance_type best_dist)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      // Finding the maximum is, for lack of a better algorithm, equivalent to a
      // O(n) search. An alternative has been explored: being able to find if a
      // node is in a cell that is smaller than the current 'far_node' node
      // found.  However, with the data at hand, computing the cell turned out
      // to be more expensive than doing a simple iteration over all nodes in
      // the tree.  Maybe, one day we'll find a better algorithm that also has
      // no impact on the memory footprint of the tree (although I doubt these 2
      // conditions will ever be met. Probably there will be a tradeoff.)
      //
      // Seeks the last node in near-pre-order.
      NodePtr best = node->parent;
      dimension_type best_dim = 0;
      for (;;)
        {
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist >= best_dist)
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (far != 0)
            {
              dimension_type child_dim = incr_dim(rank, dim);
              if (near != 0)
                {
                  import::tuple<NodePtr, dimension_type,
                                typename Metric::distance_type>
                    triplet = last_neighbor_sub(near, child_dim, rank,
                                                key_comp, met, target,
                                                best_dist);
                  if (import::get<0>(triplet) != node)
                    { import::tie(best, best_dim, best_dist) = triplet; }
                }
              node = far; dim = child_dim;
            }
          else if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else
            { return import::make_tuple(best, best_dim, best_dist); }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    last_neighbor(NodePtr node, dimension_type dim, Rank rank,
                  KeyCompare key_comp, const Metric& met, const Key& target)
    {
      return last_neighbor_sub(node, dim, rank, key_comp, met, target,
                               typename Metric::distance_type());
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    first_neighbor_sub(NodePtr node, dimension_type dim, Rank rank,
                       const KeyCompare& key_comp, const Metric& met,
                       const Key& target,
                       typename Metric::distance_type best_dist)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      // Finds the nearest in near-pre-order fashion. Uses semi-recursiveness.
      NodePtr best = node->parent;
      dimension_type best_dim = 0;
      for (;;)
        {
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist < best_dist)
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (far != 0 && (met.distance_to_plane
                           (rank(), dim, target, const_key(node)) < best_dist))
            {
              dimension_type child_dim = incr_dim(rank, dim);
              if (near != 0)
                {
                  import::tuple<NodePtr, dimension_type,
                                typename Metric::distance_type>
                    triplet = first_neighbor_sub(near, child_dim, rank,
                                                 key_comp, met, target,
                                                 best_dist);
                  if (import::get<0>(triplet) != node)
                    {
                      // If I can't go right after exploring left, I'm done
                      if (!(met.distance_to_plane
                            (rank(), dim, target, const_key(node))
                            < import::get<2>(triplet)))
                        { return triplet; }
                      import::tie(best, best_dim, best_dist) = triplet;
                    }
                }
              node = far; dim = child_dim;
            }
          else if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else
            { return import::make_tuple(best, best_dim, best_dist); }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    first_neighbor(NodePtr node, dimension_type dim, Rank rank,
                   const KeyCompare& key_comp, const Metric& met, const Key& target)
    {
      return first_neighbor_sub
        (node, dim, rank, key_comp, met, target,
         (std::numeric_limits<typename Metric::distance_type>::max)());
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    lower_bound_neighbor_sub(NodePtr node, dimension_type dim, Rank rank,
                             KeyCompare key_comp, const Metric& met,
                             const Key& target,
                             typename Metric::distance_type bound,
                             typename Metric::distance_type best_dist)
    {
      // Finds lower bound in left-pre-order fashion. Uses semi-recursiveness.
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      NodePtr best = node->parent;
      dimension_type best_dim = decr_dim(rank, dim);
      for (;;)
        {
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist > bound)
            {
              if (test_dist < best_dist)
                {
                  best = node;
                  best_dim = dim;
                  best_dist = test_dist;
                }
            }
          else if (test_dist == bound)
            { return import::make_tuple(node, dim, test_dist); }
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (far != 0 && (met.distance_to_plane
                           (rank(), dim, target, const_key(node)) < best_dist))
            {
              dimension_type child_dim = incr_dim(rank, dim);
              if (near != 0)
                {
                  import::tuple<NodePtr, dimension_type,
                                typename Metric::distance_type>
                    triplet = lower_bound_neighbor_sub(near, child_dim, rank,
                                                       key_comp, met, target,
                                                       bound, best_dist);
                  if (import::get<0>(triplet) != node)
                    {
                      if (import::get<2>(triplet) == bound
                          // If I can't go right after exploring left, I'm done
                          || !(met.distance_to_plane
                               (rank(), dim, target, const_key(node))
                               < import::get<2>(triplet)))
                        { return triplet; }
                      import::tie(best, best_dim, best_dist) = triplet;
                    }
                }
              node = far; dim = child_dim;
            }
          else if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else
            { return import::make_tuple(best, best_dim, best_dist); }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    lower_bound_neighbor(NodePtr node, dimension_type dim, Rank rank,
                         KeyCompare key_comp, const Metric& met,
                         const Key& target,
                         typename Metric::distance_type bound)
    {
      return lower_bound_neighbor_sub
        (node, dim, rank, key_comp, met, target, bound,
         (std::numeric_limits<typename Metric::distance_type>::max)());
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    upper_bound_neighbor_sub(NodePtr node, dimension_type dim, Rank rank,
                             KeyCompare key_comp, Metric met, const Key& target,
                             typename Metric::distance_type bound,
                             typename Metric::distance_type best_dist)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      // Finds upper bound in left-pre-order fashion. Uses semi-recursiveness.
      NodePtr best = node->parent;
      dimension_type best_dim = decr_dim(rank, dim);
      for (;;)
        {
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist > bound && test_dist < best_dist)
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (far != 0 && (met.distance_to_plane
                           (rank(), dim, target, const_key(node)) < best_dist))
            {
              dimension_type child_dim = incr_dim(rank, dim);
              if (near != 0)
                {
                  import::tuple<NodePtr, dimension_type,
                                typename Metric::distance_type>
                    triplet = upper_bound_neighbor_sub(near, child_dim, rank,
                                                       key_comp, met, target,
                                                       bound, best_dist);
                  if (import::get<0>(triplet) != node)
                    {
                      // If I can't go right after exploring left, I'm done
                      if (!(met.distance_to_plane
                            (rank(), dim, target, const_key(node))
                            < import::get<2>(triplet)))
                        { return triplet; }
                      import::tie(best, best_dim, best_dist) = triplet;
                    }
                }
              node = far; dim = child_dim;
            }
          else if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else
            { return import::make_tuple(best, best_dim, best_dist); }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    upper_bound_neighbor(NodePtr node, dimension_type dim, Rank rank,
                         KeyCompare key_comp, const Metric& met,
                         const Key& target,
                         typename Metric::distance_type bound)
    {
      return upper_bound_neighbor_sub
        (node, dim, rank, key_comp, met, target, bound,
         (std::numeric_limits<typename Metric::distance_type>::max)());
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    increment_neighbor(NodePtr node, dimension_type dim, Rank rank,
                       KeyCompare key_comp, const Metric& met,
                       const Key& target,
                       typename Metric::distance_type node_dist)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = dim;
      typename Metric::distance_type best_dist = node_dist;
      // Looks forward to find an equal or greater next best. If an equal next
      // best is found, then no need to look further. 'Forward' and 'backward'
      // refer to tree walking in near-pre-order.
      for(;;)
        {
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else if (far != 0
                   && (best == 0
                       || met.distance_to_plane
                       (rank(), dim, target, const_key(node)) < best_dist))
            { node = far; dim = incr_dim(rank, dim); }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && (prev_node
                         == (far = key_comp(dim, const_key(node), target)
                             ? node->left : node->right)
                         || far == 0
                         || !(best == 0
                              || (met.distance_to_plane
                                  (rank(), dim, target, const_key(node))
                                  < best_dist))))
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (!header(node))
                { node = far; dim = incr_dim(rank, dim); }
              else break;
            }
          // Test node here and stops as soon as it finds an equal
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist == node_dist)
            {
              SPATIAL_ASSERT_CHECK(dim < rank());
              SPATIAL_ASSERT_CHECK(best == 0 || test_dist < best_dist);
              return import::make_tuple(node, dim, test_dist);
            }
          else if (test_dist > node_dist
                   && (best == 0 || test_dist < best_dist))
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
        }
      // Here, current best_dist > node_dist or best == 0, maybe there is a
      // better best at the back (iterate backwards to header)
      NodePtr prev_node = orig;
      dimension_type prev_dim = orig_dim;
      node = orig->parent;
      dim = decr_dim(rank, orig_dim);
      while(!header(node))
        {
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (far == prev_node && near != 0)
            {
              node = near;
              dim = prev_dim;
              for (;;)
                {
                  import::tie(near, far) = key_comp(dim, const_key(node), target)
                    ? import::make_tuple(node->right, node->left)
                    : import::make_tuple(node->left, node->right);
                  if (far != 0
                      && (best == 0
                          || (met.distance_to_plane(rank(), dim, target,
                                                    const_key(node))
                              <= best_dist))
                      )
                    { node = far; dim = incr_dim(rank, dim); }
                  else if (near != 0)
                    { node = near; dim = incr_dim(rank, dim); }
                  else break;
                }
            }
          // Test node here for new best
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist > node_dist && (best == 0 || test_dist <= best_dist))
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
          prev_node = node;
          prev_dim = dim;
          node = node->parent;
          dim = decr_dim(rank, dim);
        }
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      if (best != 0) { node = best; dim = best_dim; }
      return import::make_tuple(node, dim, best_dist);
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key, typename Metric>
    inline import::tuple<NodePtr, dimension_type,
                         typename Metric::distance_type>
    decrement_neighbor(NodePtr node, dimension_type dim, Rank rank,
                       KeyCompare key_comp, const Metric& met,
                       const Key& target,
                       typename Metric::distance_type node_dist)
    {
      if (header(node))
        { return last_neighbor(node->parent, 0, rank, key_comp, met, target); }
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = dim;
      typename Metric::distance_type best_dist = node_dist;
      // Looks backward to find an equal or lower next best. If an equal next
      // best is found, then no need to look further. 'Forward' and 'backward'
      // refer to tree walking in near-pre-order.
      NodePtr prev_node = node;
      dimension_type prev_dim = dim;
      node = node->parent;
      dim = decr_dim(rank, dim);
      while (!header(node))
        {
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (prev_node == far && near != 0)
            {
              node = near;
              dim = prev_dim;
              for (;;)
                {
                  import::tie(near, far) = key_comp(dim, const_key(node), target)
                    ? import::make_tuple(node->right, node->left)
                    : import::make_tuple(node->left, node->right);
                  if (far != 0
                      && (met.distance_to_plane(rank(), dim, target,
                                                const_key(node))
                          <= node_dist))
                    { node = far; dim = incr_dim(rank, dim); }
                  else if (near != 0)
                    { node = near; dim = incr_dim(rank, dim); }
                  else break;
                }
            }
          // Test node here and stops as soon as it finds an equal
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist == node_dist)
            {
              SPATIAL_ASSERT_CHECK(dim < rank());
              SPATIAL_ASSERT_CHECK(best == 0 || test_dist > best_dist);
              return import::make_tuple(node, dim, test_dist);
            }
          else if (test_dist < node_dist
                   && (best == 0 || test_dist > best_dist))
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
          prev_node = node;
          prev_dim = dim;
          node = node->parent;
          dim = decr_dim(rank, dim);
        }
      // Here, current best_dist < node_dist or best == 0, maybe there is a
      // better best at the front (iterate forward to header)
      node = orig;
      dim = orig_dim;
      for(;;)
        {
          NodePtr near, far;
          import::tie(near, far) = key_comp(dim, const_key(node), target)
            ? import::make_tuple(node->right, node->left)
            : import::make_tuple(node->left, node->right);
          if (near != 0)
            { node = near; dim = incr_dim(rank, dim); }
          else if (far != 0
                   && (met.distance_to_plane(rank(), dim, target,
                                             const_key(node))
                       < node_dist))
            { node = far; dim = incr_dim(rank, dim); }
          else
            {
              prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && (prev_node
                         == (far = key_comp(dim, const_key(node), target)
                             ? node->left : node->right)
                         || far == 0
                         || !(met.distance_to_plane(rank(), dim, target,
                                                    const_key(node))
                                  < node_dist)))
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (!header(node))
                { node = far; dim = incr_dim(rank, dim); }
              else break;
            }
          typename Metric::distance_type test_dist
            = met.distance_to_key(rank(), target, const_key(node));
          if (test_dist < node_dist && (best == 0 || test_dist >= best_dist))
            {
              best = node;
              best_dim = dim;
              best_dist = test_dist;
            }
        }
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(node != 0);
      if (best != 0) { node = best; dim = best_dim; }
      return import::make_tuple(node, dim, best_dist);
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_NEIGHBOR_HPP
