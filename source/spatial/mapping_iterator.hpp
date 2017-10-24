// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   mapping_iterator.hpp
 *  Provides mapping_iterator and all the functions around it.
 */

#ifndef SPATIAL_MAPPING_ITERATOR_HPP
#define SPATIAL_MAPPING_ITERATOR_HPP

#include "spatial.hpp"
#include "bits/spatial_bidirectional.hpp"
#include "bits/spatial_rank.hpp"
#include "bits/spatial_except.hpp"
#include "bits/spatial_mapping.hpp"
#include "bits/spatial_import_tuple.hpp"

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
     *  \tparam Ct The container to which these iterator relate to.
     */
    template <typename Container>
    struct Mapping : Container::key_compare
    {
      //! Build an uninitialized mapping data object.
      Mapping() { }

      /**
       *  Builds required mapping data from the given key comparison functor,
       *  dimension and mapping dimension.
       *
       *  \param c The container being iterated.
       *  \param m The mapping dimension used in the iteration.
       */
      Mapping
      (const typename Container::key_compare& c, dimension_type m)
        : Container::key_compare(c), mapping_dim(m)
      { }

      typename Container::key_compare key_comp() const
      { return static_cast<typename Container::key_compare>(*this); }

      /**
       *  The current dimension of iteration.
       *
       *  You can modify this key if you suddenly want the iterator to change
       *  dimension of iteration. However this field must always satisfy:
       *  \f[
       *  mapping_dim() < Rank()
       *  \f]
       *  Rank being the template rank provider for the iterator.
       *
       *  \attention If you modify this value directly, no safety check will be
       *  performed.
       */
      dimension_type mapping_dim;
    };
  }

  /**
   *  This iterator walks through all items in the container in order from the
   *  lowest to the highest value along a particular dimension. The \c key_comp
   *  comparator of the container is used for comparision.
   *
   *  In effect, that makes any container of the library behave as a \c std::set
   *  or \c std::map. Through this iterator, a spatial container with 3
   *  dimensions can provide the same features as 3 \c std::set(s) or \c
   *  std::map(s) containing the same elements and ordered on each of these
   *  dimensions. Beware that iteration through the tree is very efficient when
   *  the dimension \kdtree is very small by comparison to the number of
   *  objects, but pretty inefficient otherwise, by comparison to a \c std::set.
   */
  template<typename Ct>
  class mapping_iterator
    : public details::Bidirectional_iterator
  <typename Ct::mode_type,
   typename Ct::rank_type>
  {
  private:
    typedef details::Bidirectional_iterator
    <typename Ct::mode_type,
     typename Ct::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    typedef typename Ct::key_compare key_compare;

    //! Uninitialized iterator.
    mapping_iterator() { }

    /**
     *  The standard way to build this iterator: specify a mapping dimension, an
     *  iterator on a container, and that container.
     *
     *  \param container   The container to iterate.
     *  \param mapping_dim The dimension used to order all nodes during the
     *                     iteration.
     *  \param iter        Use the value of \c iter as the start point for the
     *                     iteration.
     */
    mapping_iterator(Ct& container, dimension_type mapping_dim,
                     typename Ct::iterator iter)
      : Base(container.rank(), iter.node, modulo(iter.node, container.rank())),
        _data(container.key_comp(), mapping_dim)
    { except::check_dimension(container.dimension(), mapping_dim); }

    /**
     *  When the information of the dimension for the current node being
     *  pointed to by the iterator is known, this function saves some CPU
     *  cycle, by comparison to the other.
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
     *  \param container The container to iterate.
     *  \param mapping_dim The dimension used to order all nodes during the
     *                     iteration.
     *  \param dim The dimension of the node pointed to by iterator.
     *  \param ptr Use the value of node as the start point for the
     *             iteration.
     */
    mapping_iterator(Ct& container, dimension_type mapping_dim,
                     dimension_type dim,
                     typename Ct::mode_type::node_ptr ptr)
      : Base(container.rank(), ptr, dim),
        _data(container.key_comp(), mapping_dim)
    { except::check_dimension(container.dimension(), mapping_dim); }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    mapping_iterator<Ct>& operator++()
    {
      import::tie(node, node_dim)
        = increment_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    mapping_iterator<Ct> operator++(int)
    {
      mapping_iterator<Ct> x(*this);
      import::tie(node, node_dim)
        = increment_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    mapping_iterator<Ct>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    mapping_iterator<Ct> operator--(int)
    {
      mapping_iterator<Ct> x(*this);
      import::tie(node, node_dim)
        = decrement_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return _data.key_comp(); }

    /**
     *  Accessor to the mapping dimension used by the iterator.
     *
     *  No check is performed on this accessor if a new mapping dimension is
     *  given. If you need to check that the mapping dimension given does not
     *  exceed the rank use the function \ref spatial::mapping_dimension()
     *  instead:
     *
     *  \code
     *  // Create a mapping iterator that works on dimension 0
     *  mapping_iterator<my_container> iter (begin_mapping(container, 0));
     *  // Reset the mapping iterator to work on dimension 2
     *  mapping_dimension(iter, 2);
     *  // This will throw if the container used has a rank lower than 3.
     *  \endcode
     */
    ///@{
    dimension_type
    mapping_dimension() const { return _data.mapping_dim; }
    dimension_type&
    mapping_dimension() { return _data.mapping_dim; }
    ///@}

  private:
    //! The related data for the iterator.
    details::Mapping<Ct> _data;
  };

  /**
   *  This iterator walks through all items in the container in order from the
   *  lowest to the highest value along a particular dimension. The \c key_comp
   *  comparator of the container is used for comparision.
   *
   *  In effect, that makes any container of the library behave as a \c std::set
   *  or \c std::map. Through this iterator, a spatial container with 3
   *  dimensions can provide the same features as 3 \c std::set(s) or \c
   *  std::map(s) containing the same elements and ordered on each of these
   *  dimensions. Beware that iteration through the tree is very efficient when
   *  the dimension \kdtree is very small by comparison to the number of
   *  objects, but pretty inefficient otherwise, by comparison to a \c std::set.
   *
   *  Object deferenced by this iterator are always constant.
   */
  template<typename Ct>
  class mapping_iterator<const Ct>
    : public details::Const_bidirectional_iterator
  <typename Ct::mode_type,
   typename Ct::rank_type>
  {
  private:
    typedef details::Const_bidirectional_iterator
    <typename Ct::mode_type,
     typename Ct::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Alias for the key_compare type used by the iterator.
    typedef typename Ct::key_compare key_compare;

    //! Build an uninitialized iterator.
    mapping_iterator() { }

    /**
     *  The standard way to build this iterator: specify a mapping dimension,
     *  an iterator on a container, and that container.
     *
     *  \param container The container to iterate.
     *  \param mapping_dim The dimension used to order all nodes during the
     *  iteration.
     *  \param iter Use the value of \c iter as the start point for the
     *  iteration.
     */
    mapping_iterator(const Ct& container, dimension_type mapping_dim,
                     typename Ct::const_iterator iter)
      : Base(container.rank(), iter.node, modulo(iter.node, container.rank())),
        _data(container.key_comp(), mapping_dim)
    { except::check_dimension(container.dimension(), mapping_dim); }

    /**
     *  When the information of the dimension for the current node being
     *  pointed to by the iterator is known, this function saves some CPU
     *  cycle, by comparison to the other.
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
     *  \param container The container to iterate.
     *
     *  \param mapping_dim The dimension used to order all nodes during the
     *                     iteration.
     *
     *  \param dim The dimension of the node pointed to by iterator.
     *
     *  \param ptr Use the value of \c node as the start point for the
     *             iteration.
     */
    mapping_iterator
    (const Ct& container, dimension_type mapping_dim, dimension_type dim,
     typename Ct::mode_type::const_node_ptr ptr)
      : Base(container.rank(), ptr, dim),
        _data(container.key_comp(), mapping_dim)
    { except::check_dimension(container.dimension(), mapping_dim); }

    //! Convertion of mutable iterator into a constant iterator is permitted.
    mapping_iterator(const mapping_iterator<Ct>& iter)
      : Base(iter.rank(), iter.node, iter.node_dim),
        _data(iter.key_comp(), iter.mapping_dimension()) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    mapping_iterator<const Ct>& operator++()
    {
      import::tie(node, node_dim)
        = increment_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    mapping_iterator<const Ct> operator++(int)
    {
      mapping_iterator<const Ct> x(*this);
      import::tie(node, node_dim)
        = increment_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    mapping_iterator<const Ct>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    mapping_iterator<const Ct> operator--(int)
    {
      mapping_iterator<const Ct> x(*this);
      import::tie(node, node_dim)
        = decrement_mapping(node, node_dim, rank(),
                            _data.mapping_dim, key_comp());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return _data.key_comp(); }

    /**
     *  Accessor to the mapping dimension used by the iterator.
     *
     *  No check is performed on this accessor if a new mapping dimension is
     *  given. If you need to check that the mapping dimension given does not
     *  exceed the rank use the function \ref spatial::mapping_dimension()
     *  instead:
     *
     *  \code
     *  // Create a mapping iterator that works on dimension 0
     *  mapping_iterator<my_container> iter (begin_mapping(container, 0));
     *  // Reset the mapping iterator to work on dimension 2
     *  mapping_dimension(iter, 2);
     *  // This will throw if the container used has a rank lower than 3.
     *  \endcode
     */
    ///@{
    dimension_type
    mapping_dimension() const { return _data.mapping_dim; }
    dimension_type&
    mapping_dimension() { return _data.mapping_dim; }
    ///@}

  private:
    //! The related data for the iterator.
    details::Mapping<Ct> _data;
  };

  /**
   *  Return the mapping dimension of the iterator.
   *  \param it the iterator where the mapping dimension is retreived.
   */
  template <typename Container>
  inline dimension_type
  mapping_dimension(const mapping_iterator<Container>& it)
  { return it.mapping_dimension(); }

  /**
   *  Sets the mapping dimension of the iterator.
   *  \param it The iterator where the mapping dimension is set.
   *  \param mapping_dim The new mapping dimension to use.
   */
  template <typename Container>
  inline void
  mapping_dimension(mapping_iterator<Container>& it,
                    dimension_type mapping_dim)
  {
    except::check_dimension(it.dimension(), mapping_dim);
    it.mapping_dimension() = mapping_dim;
  }

  /**
   *  Finds the past-the-end position in \c container for this constant
   *  iterator.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \consttime
   */
  ///@{
  template <typename Container>
  inline mapping_iterator<Container>
  mapping_end(Container& container, dimension_type mapping_dim)
  {
    except::check_dimension(container.dimension(), mapping_dim);
    return mapping_iterator<Container>
      (container, mapping_dim, container.dimension() - 1,
       container.end().node); // At header (dim = rank - 1)
  }

  template <typename Container>
  inline mapping_iterator<const Container>
  mapping_cend(const Container& container, dimension_type mapping_dim)
  { return mapping_end(container, mapping_dim); }
  ///@}

  /**
   *  Finds the value in \c container for which its key has the smallest
   *  coordinate over the dimension \c mapping_dim.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *
   *  \fractime
   */
  ///@{
  template <typename Container>
  inline mapping_iterator<Container>
  mapping_begin(Container& container, dimension_type mapping_dim)
  {
    if (container.empty()) return mapping_end(container, mapping_dim);
    except::check_dimension(container.dimension(), mapping_dim);
    typename mapping_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    import::tie(node, dim)
      = minimum_mapping(node, 0,
                        container.rank(), mapping_dim,
                        container.key_comp());
    return mapping_iterator<Container>(container, mapping_dim, dim, node);
  }

  template <typename Container>
  inline mapping_iterator<const Container>
  mapping_cbegin(const Container& container, dimension_type mapping_dim)
  { return mapping_begin(container, mapping_dim); }
  ///@}

  /**
   *  This structure defines a pair of mutable mapping iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see mapping_iterator
   */
  template <typename Ct>
  struct mapping_iterator_pair
    : std::pair<mapping_iterator<Ct>, mapping_iterator<Ct> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<mapping_iterator<Ct>, mapping_iterator<Ct> > Base;

    //! Empty constructor.
    mapping_iterator_pair() { }

    //! Regular constructor that builds a mapping_iterator_pair out of 2
    //! mapping_iterators.
    mapping_iterator_pair(const mapping_iterator<Ct>& a,
                          const mapping_iterator<Ct>& b) : Base(a, b) { }
  };

  /**
   *  This structure defines a pair of constant mapping iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see mapping_iterator
   */
  template <typename Ct>
  struct mapping_iterator_pair<const Ct>
    : std::pair<mapping_iterator<const Ct>, mapping_iterator<const Ct> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<mapping_iterator<const Ct>, mapping_iterator<const Ct> >
    Base;

    //! Empty constructor.
    mapping_iterator_pair() { }

    //! Regular constructor that builds a mapping_iterator_pair out of 2
    //! mapping_iterators.
    mapping_iterator_pair(const mapping_iterator<const Ct>& a,
                          const mapping_iterator<const Ct>& b) : Base(a, b)
    { }

    //! Convert a mutable mapping iterator pair into a const mapping iterator
    //!pair.
    mapping_iterator_pair(const mapping_iterator_pair<Ct>& p)
      : Base(p.first, p.second) { }
  };

  /**
   *  Returns a pair of iterator on the first and the last value in the range
   *  that can be iterated. This function is convenient to use with
   *  \c std::tie, and is equivalent to calling \ref mapping_begin() and \ref
   *  mapping_end() on both iterators.
   *
   *  To combine it with \c std::tie, use it this way:
   *  \code
   *  mapping<pointset<2, int> >::iterator it, end;
   *  std::tie(it, end) = mapping_range(0, my_points);
   *  for(; it != end; ++it)
   *  {
   *    // ...
   *  }
   *  \endcode
   *  Notice that an \c iterator type is declared, and not an \c iterator_pair
   *  type.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \fractime
   */
  template <typename Container>
  inline mapping_iterator_pair<Container>
  mapping_range(Container& container, dimension_type mapping_dim)
  {
    return mapping_iterator_pair<Container>
          (mapping_begin(container, mapping_dim),
           mapping_end(container, mapping_dim));
  }

  ///@{
  /**
   *  Returns a pair of constant iterator on the first and the last value in the
   *  range that can be iterated. This function is convenient to use with
   *  \c std::tie, and is equivalent to calling \ref mapping_begin() and \ref
   *  mapping_end() on both iterators.
   *
   *  To combine it with \c std::tie, use it this way:
   *  \code
   *  mapping<pointset<2, int> >::iterator it, end;
   *  std::tie(it, end) = mapping_range(0, my_points);
   *  for(; it != end; ++it)
   *  {
   *    // ...
   *  }
   *  \endcode
   *  Notice that an \c iterator type is declared, and not an \c iterator_pair
   *  type.
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \fractime
   */
  template <typename Container>
  inline mapping_iterator_pair<const Container>
  mapping_range(const Container& container, dimension_type mapping_dim)
  {
    return mapping_iterator_pair<const Container>
      (mapping_begin(container, mapping_dim),
       mapping_end(container, mapping_dim));
  }

  template <typename Container>
  inline mapping_iterator_pair<const Container>
  mapping_crange(const Container& container, dimension_type mapping_dim)
  {
    return mapping_iterator_pair<const Container>
      (mapping_begin(container, mapping_dim),
       mapping_end(container, mapping_dim));
  }
  ///@}

  /**
   *  Finds the value with the smallest coordinate along the mapping dimension
   *  that is greater or equal to \c bound, and return an iterator pointing to
   *  this value.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c mapping_dim.
   *
   *  \fractime
   */
  ///@{
  template <typename Container>
  inline mapping_iterator<Container>
  mapping_lower_bound(Container& container, dimension_type mapping_dim,
                      const typename Container::key_type&
                      bound)
  {
    if (container.empty()) return mapping_end(container, mapping_dim);
    except::check_dimension(container.dimension(), mapping_dim);
    typename mapping_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    import::tie(node, dim)
      = lower_bound_mapping(node, 0, container.rank(), mapping_dim,
                            container.key_comp(), bound);
    return mapping_iterator<Container>(container, mapping_dim, dim, node);
  }

  template <typename Container>
  inline mapping_iterator<const Container>
  mapping_clower_bound
  (const Container& container, dimension_type mapping_dim,
   const typename Container::key_type& bound)
  { return mapping_lower_bound(container, mapping_dim, bound); }
  ///@}

  /**
   *  Finds the value with the largest coordinate along the mapping dimension
   *  that is stricly less than \c bound, and return an iterator pointing to
   *  this value.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param mapping_dim The dimension that is the reference for the iteration:
   *  all iterated values will be ordered along this dimension, from smallest to
   *  largest.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c mapping_dim.
   *
   *  \fractime
   */
  ///@{
  template <typename Container>
  inline mapping_iterator<Container>
  mapping_upper_bound
  (Container& container, dimension_type mapping_dim,
   const typename Container::key_type& bound)
  {
    if (container.empty()) return mapping_end(container, mapping_dim);
    except::check_dimension(container.dimension(), mapping_dim);
    typename mapping_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    import::tie(node, dim)
      = upper_bound_mapping(node, 0, container.rank(), mapping_dim,
                            container.key_comp(), bound);
    return mapping_iterator<Container>(container, mapping_dim, dim, node);
  }

  template <typename Container>
  inline mapping_iterator<const Container>
  mapping_cupper_bound
  (const Container& container, dimension_type mapping_dim,
   const typename Container::key_type& bound)
  { return mapping_upper_bound(container, mapping_dim, bound); }
  ///@}

  namespace details
  {
    /**
     *  This function provides a common interface for the two \kdtree
     *  invariants.
     */
    ///@{
    template <typename KeyCompare, typename Key>
    inline bool
    left_compare_mapping
    (KeyCompare key_comp, dimension_type map, const Key& x, const Key& y,
     strict_invariant_tag)
    { return key_comp(map, x, y); }

    template <typename KeyCompare, typename Key>
    inline bool
    left_compare_mapping
    (KeyCompare key_comp, dimension_type map, const Key& x, const Key& y,
     relaxed_invariant_tag)
    { return !key_comp(map, y, x); }
    ///@}

    /**
     *  Move the pointer given in parameter to the next element in the ordered
     *  iteration of values along the mapping dimension.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. You should use the
     *  overload \c operator++ on the \mapping_iterator instead. This function
     *  does not perform any sanity checks on the iterator given in parameter.
     *
     *  Since Container is based on \kdtree and \kdtree exhibit good locality of
     *  reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    increment_mapping
    (NodePtr node, dimension_type dim, Rank rank, dimension_type map,
     const KeyCompare& key_comp)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = 0;
      // Look forward to find an equal or greater next best
      // If an equal next best is found, then no need to look further
      for (;;)
        {
          if (node->right != 0
              && (dim != map || best == 0
                  || key_comp(map, const_key(node), const_key(best))))
            {
              node = node->right; dim = incr_dim(rank, dim);
              while (node->left != 0
                     && (dim != map
                         || left_compare_mapping
                            (key_comp, map, const_key(orig), const_key(node),
                             invariant_category(node))))
                { node = node->left; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (key_comp(map, const_key(orig), const_key(node)))
            {
              if (best == 0 || key_comp(map, const_key(node), const_key(best)))
                { best = node; best_dim = dim; }
            }
          else if (!key_comp(map, const_key(node), const_key(orig)))
            {
              SPATIAL_ASSERT_CHECK(dim < rank());
              SPATIAL_ASSERT_CHECK(!header(node));
              return std::make_pair(node, dim);
            }
        }
      SPATIAL_ASSERT_CHECK(dim == rank() - 1);
      SPATIAL_ASSERT_CHECK(header(node));
      // Maybe there is a better best looking backward...
      node = orig;
      dim = orig_dim;
      for (;;)
        {
          if (node->left != 0
              && (dim != map
                  || key_comp(map, const_key(orig), const_key(node))))
            {
              node = node->left; dim = incr_dim(rank, dim);
              while (node->right != 0
                     && (dim != map || best == 0
                         || key_comp(map, const_key(node), const_key(best))))
                { node = node->right; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (key_comp(map, const_key(orig), const_key(node))
              && (best == 0
                  || !key_comp(map, const_key(best), const_key(node))))
            { best = node; best_dim = dim; }
        }
      if (best != 0)
        { node = best; dim = best_dim; }
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK((best == 0 && header(node))
                           || (best != 0 && !header(node)));
      return std::make_pair(node, dim);
    }

    /**
     *  Move the pointer given in parameter to the previous element in the
     *  ordered iteration of values along the mapping dimension.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. You should use the
     *  overload \c operator-- on the \mapping_iterator instead. This function
     *  does not perform any sanity checks on the iterator given in parameter.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *  \return An iterator pointing to the value with the smallest coordinate
     *  along \c iter's \c mapping_dim, and among the children of the node
     *  pointed to by \c iter.
     *
     *  Since Container is based on \kdtree and \kdtree exhibit good locality of
     *  reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    decrement_mapping
    (NodePtr node, dimension_type dim, Rank rank, dimension_type map,
     const KeyCompare& key_comp)
    {
      SPATIAL_ASSERT_CHECK(dim < rank());
      if (header(node))
        return maximum_mapping(node->parent, 0, rank, map, key_comp);
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = 0;
      // Look backward to find an equal or greater next best
      // If an equal next best is found, then no need to look further
      for (;;)
        {
          if (node->left != 0
              && (dim != map || best == 0
                  || key_comp(map, const_key(best), const_key(node))))
            {
              node = node->left; dim = incr_dim(rank, dim);
              while (node->right != 0
                     && (dim != map
                         || !key_comp(map, const_key(orig), const_key(node))))
                { node = node->right; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (key_comp(map, const_key(node), const_key(orig)))
            {
              if (best == 0 || key_comp(map, const_key(best), const_key(node)))
                { best = node; best_dim = dim; }
            }
          else if (!key_comp(map, const_key(orig), const_key(node)))
            {
              SPATIAL_ASSERT_CHECK(dim < rank());
              SPATIAL_ASSERT_CHECK(!header(node));
              return std::make_pair(node, dim);
            }
        }
      SPATIAL_ASSERT_CHECK(dim == rank() - 1);
      SPATIAL_ASSERT_CHECK(header(node));
      // Maybe there is a better best looking forward...
      node = orig;
      dim = orig_dim;
      for (;;)
        {
          if (node->right != 0
              && (dim != map
                  || key_comp(map, const_key(node), const_key(orig))))
            {
              node = node->right; dim = incr_dim(rank, dim);
              while (node->left != 0
                     && (dim != map || best == 0
                         || key_comp(map, const_key(best), const_key(node))))
                { node = node->left; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (key_comp(map, const_key(node), const_key(orig))
              && (best == 0
                  || !key_comp(map, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      if (best != 0)
        { node = best; dim = best_dim; }
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK((best == 0 && header(node))
                           || (best != 0 && !header(node)));
      return std::make_pair(node, dim);
    }

    /**
     *  Move the iterator given in parameter to the value with the smallest
     *  coordinate greater or equal to \c bound along the mapping dimension of
     *  \c iter, but only in the sub-tree composed of the node pointed to by the
     *  iterator and its children. If no such value exists, then move the
     *  iterator to the parent of the value currently pointed to.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. If you feel that you
     *  must use this function, maybe you were actually looking for \ref
     *  mapping_begin(). In any case, use it cautiously, as this function does
     *  not perform any sanity checks on the iterator given in parameter.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *  \param bound    A boundary value to stop the search.
     *  \return An iterator pointing to the value with the smallest coordinate
     *  greater or equal to \c bound along \c iter's \c mapping_dim, or to the
     *  parent of the value pointed to.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename KeyType>
    inline std::pair<NodePtr, dimension_type>
    lower_bound_mapping
    (NodePtr node, dimension_type dim, Rank rank, dimension_type map,
     const KeyCompare& key_comp, const KeyType& bound)
    {
      SPATIAL_ASSERT_CHECK(map < rank());
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      while (node->left != 0
             && (dim != map
                 || left_compare_mapping(key_comp, map, bound, const_key(node),
                                         invariant_category(node))))
        { node = node->left; dim = incr_dim(rank, dim); }
      NodePtr best = 0;
      dimension_type best_dim = 0;
      if (!key_comp(map, const_key(node), bound))
        { best = node; best_dim = dim; }
      for (;;)
        {
          if (node->right != 0 && (dim != map || best == 0))
            {
              node = node->right;
              dim = incr_dim(rank, dim);
              while (node->left != 0
                     && (dim != map
                         || left_compare_mapping(key_comp, map, bound,
                                                 const_key(node),
                                                 invariant_category(node))))
                { node = node->left; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent;
              dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (!key_comp(map, const_key(node), bound)
              && (best == 0 || key_comp(map, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(dim == rank() - 1);
      SPATIAL_ASSERT_CHECK(best != node);
      SPATIAL_ASSERT_CHECK(header(node));
      if (best == 0)
        { best = node; best_dim = dim; }
      return std::make_pair(best, best_dim);
    }

    /**
     *  Move the iterator given in parameter to the value with the largest
     *  coordinate strictly lower than \c bound along the mapping dimension of
     *  \c iter, but only in the sub-tree composed of the node pointed to by the
     *  iterator and its children. If no such value exists, then move the
     *  iterator to the parent of the value currently pointed to.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. If you feel that you
     *  must use this function, maybe you were actually looking for \ref
     *  mapping_begin(). In any case, use it cautiously, as this function does
     *  not perform any sanity checks on the iterator given in parameter.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *  \param bound    A boundary value to stop the search.
     *  \return \c iter moved to the value with the largest coordinate strictly
     *  less than \c bound along \c iter's \c mapping_dim, or to the
     *  parent of the value pointed to.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename KeyType>
    inline std::pair<NodePtr, dimension_type>
    upper_bound_mapping
    (NodePtr node, dimension_type dim, Rank rank, dimension_type map,
     const KeyCompare& key_comp, const KeyType& bound)
    {
      SPATIAL_ASSERT_CHECK(map < rank());
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      while (node->left != 0
             && (dim != map
                 || key_comp(map, bound, const_key(node))))
        { node = node->left; dim = incr_dim(rank, dim); }
      NodePtr best = 0;
      dimension_type best_dim = 0;
      if (key_comp(map, bound, const_key(node)))
        { best = node; best_dim = dim; }
      for (;;)
        {
          if (node->right != 0 && (dim != map || best == 0))
            {
              node = node->right;
              dim = incr_dim(rank, dim);
              while (node->left != 0
                     && (dim != map
                         || key_comp(map, bound, const_key(node))))
                { node = node->left; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent;
              dim = decr_dim(rank, dim);
              while (!header(node)
                     && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (header(node)) break;
            }
          if (key_comp(map, bound, const_key(node))
              && (best == 0 || key_comp(map, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(dim == rank() - 1);
      SPATIAL_ASSERT_CHECK(best != node);
      SPATIAL_ASSERT_CHECK(header(node));
      if (best == 0)
        { best = node; best_dim = dim; }
      return std::make_pair(best, best_dim);
    }

  } // namespace details
}

#endif // SPATIAL_MAPPING_ITERATOR_HPP
